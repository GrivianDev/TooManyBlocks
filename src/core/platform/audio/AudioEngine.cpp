#include "AudioEngine.h"

#include <condition_variable>
#include <cstdint>
#include <queue>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

#include "Logger.h"
#include "internal/AudioEngineDefs.h"
#include "internal/AudioPlayback.h"

struct AudioAsset {
    enum class State {
        Unloaded = 0,
        MetadataLoading,
        MetadataReady,
        FullLoading,
        FullyLoaded,
        Failed
    };

    std::string assetPath;

    std::mutex mtx;
    State state;
    bool isFullLoadRequested;

    size_t totalFrames;
    std::vector<float> pcmData;
};

struct EngineState {
    ma_context context;

    ma_decoder_config decoderConfig;  // Constant

    ma_device currentOutputDevice;

    ma_device_info* outputDeviceInfos;
    ma_device_info* inputDeviceInfos;
    ma_uint32 outputDeviceCount;
    ma_uint32 inputDeviceCount;

    std::unordered_map<std::string, std::unique_ptr<AudioAsset>> loadedAssets;

    // Commands
    std::vector<AudioCmdKey> cmdOrdering;                                     // This preserves pushed order
    std::unordered_map<AudioCmdKey, AudioCmd, AudioCmdKeyHash> pushableCmds;  // This erases duplicates

    // Audio instance management
    AudioInstanceSlot audioInstSlots[AUDIO_INSTANCE_LIMIT];
    std::vector<unsigned int> freeSlotIndices;
    AudioAsset* usedAssets[AUDIO_INSTANCE_LIMIT];
    unsigned int slotVersions[AUDIO_INSTANCE_LIMIT];

    // Listener
    ListenerState listenerState;

    // Streaming
    struct StreamData {
        StreamBufferSlot streamSlots[STREAM_CAPACITY];
        std::mutex wakeMtx;
        std::atomic<bool> streamWakeRequested{false};
        std::condition_variable wakeCV;
        float* tmpStreamBuffer;
    } streamData;

    struct LoaderData {
        std::mutex loadMtx;
        std::condition_variable loaderCV;
        std::queue<AudioAsset*> pendingLoads;
    } loaderData;

    // Mixing buses
    MixingBus mixingBuses[MIXING_BUS_LIMIT];
    float* resolvedBusVolumes;
    float* pendingResolvedBusVolumes;

    // Reverb
    ReverbInstanceSlot reverbInstSlots[REVERB_INSTANCE_LIMIT];
    ReverbDesign reverbSlotDesigns[REVERB_INSTANCE_LIMIT];
    ReverbBuffers* pendingReverbBuffers[REVERB_INSTANCE_LIMIT];
    bool pendingReverbBufferPresent[REVERB_INSTANCE_LIMIT];

    // Access Management
    std::shared_mutex engineMutex;
};

struct AudioEngineData {
    SharedState shared;
    EngineState engine;
    AudioThreadState audio;

    AudioThreadContext callbackContext;
};

void ensureMetadata(AudioAsset* asset, EngineState& engineState) {
    std::lock_guard<std::mutex> assetLock(asset->mtx);

    if (asset->state == AudioAsset::State::Unloaded) {
        asset->state = AudioAsset::State::MetadataLoading;

        std::lock_guard<std::mutex> lock(engineState.loaderData.loadMtx);
        engineState.loaderData.pendingLoads.push(asset);
        engineState.loaderData.loaderCV.notify_one();
    }
}

void ensureFullyLoaded(AudioAsset* asset, EngineState& engineState) {
    std::lock_guard<std::mutex> assetLock(asset->mtx);

    asset->isFullLoadRequested = true;
    switch (asset->state) {
        case AudioAsset::State::Unloaded: {
            asset->state = AudioAsset::State::MetadataLoading;

            std::lock_guard<std::mutex> lock(engineState.loaderData.loadMtx);
            engineState.loaderData.pendingLoads.push(asset);
            engineState.loaderData.loaderCV.notify_one();
            break;
        }

        case AudioAsset::State::MetadataReady: {
            asset->state = AudioAsset::State::FullLoading;

            std::lock_guard<std::mutex> lock(engineState.loaderData.loadMtx);
            engineState.loaderData.pendingLoads.push(asset);
            engineState.loaderData.loaderCV.notify_one();
            break;
        }

        default: break;
    }
}

static bool createsCyclicBusDependency(const MixingBus* buses, unsigned int busId, int newParentId) {
    if (newParentId < 0) return false;

    if (static_cast<int>(busId) == newParentId) return true;  // Self parenting

    int p = newParentId;

    while (p != -1) {
        if (p == static_cast<int>(busId)) return true;  // bus would become its own ancestor

        if (p < 0) break;

        p = buses[p].parent;
    }

    return false;
}

static void computeResolvedBusVolumes(const MixingBus* buses, float* out) {
    for (unsigned int i = 0; i < MIXING_BUS_LIMIT; i++) {
        float v = buses[i].volume;
        int p = buses[i].parent;

        while (p != -1) {
            v *= buses[p].volume;
            p = buses[p].parent;
        }

        out[i] = v;
    }
}

static ReverbDesign sanitizeReverbDesign(const ReverbDesign& in) {
    ReverbDesign out{};

    size_t count = in.combMsDelays.size();
    out.combMsDelays.reserve(count);

    for (size_t i = 0; i < count; i++) {
        float delayMs = in.combMsDelays[i];

        // Delay must produce at least 1 frame
        if (delayMs < FRAME_DURATION) continue;

        out.combMsDelays.push_back(delayMs);
    }

    count = std::min(in.allPassMsDelays.size(), in.allPassBaseFeedbacks.size());
    out.allPassMsDelays.reserve(count);
    out.allPassBaseFeedbacks.reserve(count);

    for (size_t i = 0; i < count; i++) {
        float delayMs = in.allPassMsDelays[i];
        float fb = in.allPassBaseFeedbacks[i];

        // Delay must produce at least 1 frame
        if (delayMs < FRAME_DURATION) continue;

        fb = std::clamp(fb, -MAX_REVERB_FEEDBACK, MAX_REVERB_FEEDBACK);
        if (std::fabs(fb) < NEAR_ZERO_EPSILON) continue;

        out.allPassMsDelays.push_back(delayMs);
        out.allPassBaseFeedbacks.push_back(fb);
    }

    return out;
}

// Does design. ReverbDesign needs to be validated beforehand to be sure -> invalid combs / filters should be discarded
// before
static ReverbBuffers* allocateReverbBuffers(const ReverbDesign& design) {
    ReverbBuffers* buffers = new ReverbBuffers{};
    buffers->combCount = static_cast<unsigned int>(design.combMsDelays.size());
    buffers->allPassCount = static_cast<unsigned int>(design.allPassMsDelays.size());
    buffers->combs = nullptr;
    buffers->allPassFilters = nullptr;

    // Allocate scratch buffer
    buffers->input = new float[FRAMES_PER_PERIOD]{};

    // Allocate comb filters
    if (buffers->combCount > 0) {
        buffers->combs = new CombFilter[buffers->combCount]{};

        for (unsigned int i = 0; i < buffers->combCount; i++) {
            CombFilter& c = buffers->combs[i];

            c.delayMs = design.combMsDelays[i];
            c.bufferSize = static_cast<unsigned int>((c.delayMs / 1000.0f) * FORMAT_FRAME_RATE);
            c.buffer = new float[c.bufferSize]{};
        }
    }

    // Allocate all pass filters
    if (buffers->allPassCount > 0) {
        buffers->allPassFilters = new AllPassFilter[buffers->allPassCount]{};

        for (unsigned int i = 0; i < buffers->allPassCount; i++) {
            AllPassFilter& ap = buffers->allPassFilters[i];

            ap.bufferSize = static_cast<unsigned int>((design.allPassMsDelays[i] / 1000.0f) * FORMAT_FRAME_RATE);
            ap.feedback = design.allPassBaseFeedbacks[i];
            ap.buffer = new float[ap.bufferSize]{};
        }
    }

    return buffers;
}

static void freeReverbBuffers(ReverbBuffers* buffers) {
    // Free scratch buffer
    delete[] buffers->input;

    // Free comb buffers
    if (buffers->combs) {
        for (unsigned int i = 0; i < buffers->combCount; i++) {
            delete[] buffers->combs[i].buffer;
        }
        delete[] buffers->combs;
    }

    // Free all pass buffers
    if (buffers->allPassFilters) {
        for (unsigned int i = 0; i < buffers->allPassCount; i++) {
            delete[] buffers->allPassFilters[i].buffer;
        }
        delete[] buffers->allPassFilters;
    }

    delete buffers;
}

static void pushCommand(const AudioCmd& cmd, EngineState& state) {
    AudioCmdKey key = cmd.mappingKey();
    auto it = state.pushableCmds.find(key);
    if (it != state.pushableCmds.end()) {
        it->second = cmd;
    } else {
        state.cmdOrdering.push_back(key);
        state.pushableCmds[key] = cmd;
    }
}

static bool isCmdAllowed(const AudioCmd& cmd, const EngineState& state) {
    switch (cmd.target) {
        case AudioCmdTarget::NoTarget: {
            switch (cmd.type) {
                case AudioCmdType::SetResolvedBusVolumes: return state.pendingResolvedBusVolumes == nullptr;
                default: return true;
            }
        }
        case AudioCmdTarget::AudioInstanceTarget: {
            const AudioInstanceSlot& slot = state.audioInstSlots[cmd.instanceId];
            switch (cmd.type) {
                case AudioCmdType::Play: return true;
                case AudioCmdType::SkipToRequest: return !slot.isSkipRequested;
                case AudioCmdType::SkipByRequest: return !slot.isSkipRequested;
                default: return slot.isActive;
            }
        }
        case AudioCmdTarget::ReverbInstanceTarget: {
            const ReverbInstanceSlot& slot = state.reverbInstSlots[cmd.instanceId];
            switch (cmd.type) {
                case AudioCmdType::SetReverbInstanceBuffers: return !state.pendingReverbBufferPresent[cmd.instanceId];
                default: return slot.buffers != nullptr;
            }
        }
        default: return false;
    }
}

static void beforeCmdSend(AudioCmd& cmd, EngineState& state) {
    switch (cmd.target) {
        case AudioCmdTarget::NoTarget: {
            switch (cmd.type) {
                case AudioCmdType::SetResolvedBusVolumes: {
                    float* resolvedVolumes = new float[MIXING_BUS_LIMIT];
                    computeResolvedBusVolumes(state.mixingBuses, resolvedVolumes);
                    cmd.data.singlePtr = resolvedVolumes;
                    break;
                }
                default: break;
            }
            break;
        }
        case AudioCmdTarget::ReverbInstanceTarget: {
            ReverbInstanceSlot& slot = state.reverbInstSlots[cmd.instanceId];

            switch (cmd.type) {
                case AudioCmdType::SetReverbInstanceBuffers: {
                    const ReverbDesign& sDesign = state.reverbSlotDesigns[cmd.instanceId];
                    if (sDesign.allPassMsDelays.size() != 0 || sDesign.combMsDelays.size() != 0) {
                        // Allocate new buffer
                        cmd.data.singlePtr = allocateReverbBuffers(sDesign);
                    }
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }
}

static void afterCmdSend(const AudioCmd& cmd, EngineState& state) {
    switch (cmd.target) {
        case AudioCmdTarget::NoTarget: {
            switch (cmd.type) {
                case AudioCmdType::SetResolvedBusVolumes:
                    state.pendingResolvedBusVolumes = static_cast<float*>(cmd.data.singlePtr);
                    break;
                default: break;
            }
            break;
        }
        case AudioCmdTarget::AudioInstanceTarget: {
            AudioInstanceSlot& slot = state.audioInstSlots[cmd.instanceId];

            switch (cmd.type) {
                case AudioCmdType::SkipToRequest: slot.isSkipRequested = true; break;
                case AudioCmdType::SkipByRequest: slot.isSkipRequested = true; break;
                default: break;
            }
            break;
        }
        case AudioCmdTarget::ReverbInstanceTarget: {
            ReverbInstanceSlot& slot = state.reverbInstSlots[cmd.instanceId];

            switch (cmd.type) {
                case AudioCmdType::SetReverbInstanceBuffers:
                    state.pendingReverbBuffers[cmd.instanceId] = static_cast<ReverbBuffers*>(cmd.data.singlePtr);
                    state.pendingReverbBufferPresent[cmd.instanceId] = true;
                    break;
                default: break;
            }
            break;
        }
        default: break;
    }
}

ReverbDesign AudioEngine::getReverbDesignPreset() {
    // Magic numbers
    ReverbDesign d;
    d.baseRoomSizeMeters = 18.74f;
    d.combMsDelays = {36.7f, 48.6f, 44.0f, 40.6f};
    d.allPassMsDelays = {6.12, 2.12};
    d.allPassBaseFeedbacks = {0.65f, 0.60f};
    return d;
}

void AudioEngine::streamWorkerLoop() {
    EngineState::StreamData* streamData = &m_data->engine.streamData;

    while (true) {
        {
            // Sleep until notified
            std::unique_lock<std::mutex> lock(streamData->wakeMtx);
            streamData->wakeCV.wait(lock, [this, streamData]() {
                return m_stopThreads.load() || streamData->streamWakeRequested.exchange(false);
            });
            if (m_stopThreads.load()) return;
        }

        for (StreamBufferSlot& slot : streamData->streamSlots) {
            if (m_stopThreads.load()) return;
            std::lock_guard<std::mutex> lock(slot.streamingMtx);
            if (!slot.isActive) continue;

            if (slot.seekTargetFrameIdx != SIZE_MAX) {
                if (ma_decoder_seek_to_pcm_frame(&slot.decoder, slot.seekTargetFrameIdx) != MA_SUCCESS) {
                    lgr::lout.error("Could not skip to frame index  with streamed audio.");
                }
                slot.seekTargetFrameIdx = SIZE_MAX;
            }

            ma_uint32 writableSamples = ma_rb_available_write(&slot.streamBuffer) / SAMPLE_BYTE_SIZE;
            if (writableSamples < FORMAT_CHANNELS) continue;

            uint32_t framesToDecode = writableSamples / FORMAT_CHANNELS;

            ma_uint64 framesRead = 0;
            ma_result res = ma_decoder_read_pcm_frames(
                &slot.decoder, streamData->tmpStreamBuffer, framesToDecode, &framesRead
            );

            if (res != MA_SUCCESS || framesRead == 0) {
                if (slot.isLooping) {
                    ma_result couldSkip = ma_decoder_seek_to_pcm_frame(&slot.decoder, 0);
                    if (couldSkip != MA_SUCCESS) {
                        lgr::lout.error("Could not return to frame index 0 with looping streamed audio.");
                    }
                }
                continue;
            }

            size_t samplesToWrite = framesRead * FORMAT_CHANNELS;
            size_t writtenSamples = rbWriteElements<float>(
                &slot.streamBuffer, streamData->tmpStreamBuffer, samplesToWrite
            );
            if (writtenSamples != samplesToWrite) {
                lgr::lout.error("Written less samples on streaming audio. This should not happen!");
            }
        }
    }
}

void AudioEngine::audioLoadWorkerLoop() {
    EngineState::LoaderData* loadState = &m_data->engine.loaderData;

    while (true) {
        AudioAsset* asset = nullptr;

        // ---- Wait for work ----
        {
            std::unique_lock lock(loadState->loadMtx);

            loadState->loaderCV.wait(lock, [this, loadState] {
                return !loadState->pendingLoads.empty() || m_stopThreads.load();
            });

            if (m_stopThreads.load()) return;

            asset = loadState->pendingLoads.front();
            loadState->pendingLoads.pop();
        }

        // ---- Init decoder ----
        ma_decoder decoder;
        if (ma_decoder_init_file(asset->assetPath.c_str(), &m_data->engine.decoderConfig, &decoder) != MA_SUCCESS) {
            lgr::lout.error("Failed to init decoder for audio file: " + asset->assetPath);

            std::lock_guard<std::mutex> lock(asset->mtx);
            asset->state = AudioAsset::State::Failed;
            continue;
        }

        bool doMetadata;

        // ---- Metadata Stage ----
        {
            std::lock_guard<std::mutex> lock(asset->mtx);
            doMetadata = asset->state == AudioAsset::State::MetadataLoading;
        }

        if (doMetadata) {
            ma_uint64 frameCount = 0;
            if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS) {
                lgr::lout.error("Failed to read pcm length: " + asset->assetPath);

                std::lock_guard<std::mutex> lock(asset->mtx);
                asset->state = AudioAsset::State::Failed;
            } else {
                std::lock_guard<std::mutex> lock(asset->mtx);

                asset->totalFrames = frameCount;
                asset->state = asset->isFullLoadRequested ? AudioAsset::State::FullLoading
                                                          : AudioAsset::State::MetadataReady;
            }
        }

        bool doFullLoad = false;
        ma_uint64 frameCount = 0;

        {
            std::lock_guard<std::mutex> lock(asset->mtx);

            // If the asset requested full load and it's not yet fully loaded
            if ((asset->state == AudioAsset::State::FullLoading) ||
                (asset->state == AudioAsset::State::MetadataReady && asset->isFullLoadRequested)) {
                doFullLoad = true;
                asset->state = AudioAsset::State::FullLoading;  // mark as loading
                frameCount = asset->totalFrames;
            }
        }
        // ---- Full Load Stage ----
        if (doFullLoad) {
            std::vector<float> pcm(frameCount * decoder.outputChannels);

            ma_uint64 framesRead = 0;
            ma_result result = ma_decoder_read_pcm_frames(&decoder, pcm.data(), frameCount, &framesRead);

            std::lock_guard<std::mutex> lock(asset->mtx);
            if (result != MA_SUCCESS || framesRead != frameCount) {
                lgr::lout.error("Failed to read audio file: " + asset->assetPath);
                asset->state = AudioAsset::State::Failed;
            } else {
                asset->pcmData = std::move(pcm);
                asset->state = AudioAsset::State::FullyLoaded;
            }
        }

        ma_decoder_uninit(&decoder);
    }
}

void AudioEngine::processCmdsFromAudioThread() {
    AudioCmd cmd;
    while (rbReadElements<AudioCmd>(&m_data->shared.audioToMainQueue, &cmd, 1) == 1) {
        switch (cmd.target) {
            case AudioCmdTarget::NoTarget: {
                switch (cmd.type) {
                    case AudioCmdType::SetResolvedBusVolumes: {
                        delete[] m_data->engine.resolvedBusVolumes;
                        m_data->engine.resolvedBusVolumes = m_data->engine.pendingResolvedBusVolumes;
                        m_data->engine.pendingResolvedBusVolumes = nullptr;
                    }
                    default: break;
                }
                break;
            }
            case AudioCmdTarget::AudioInstanceTarget: {
                AudioInstanceSlot& slot = m_data->engine.audioInstSlots[cmd.instanceId];
                switch (cmd.type) {
                    case AudioCmdType::Stop: {
                        if (slot.isStreamed) {
                            std::lock_guard<std::mutex> lock(slot.streamSlot->streamingMtx);
                            slot.streamSlot->isActive = false;
                            slot.streamSlot->isLooping = false;
                            slot.streamSlot->seekTargetFrameIdx = SIZE_MAX;
                            ma_rb_reset(&slot.streamSlot->streamBuffer);
                            ma_decoder_uninit(&slot.streamSlot->decoder);
                        }
                        slot.isActive = false;
                        if (!cmd.needsAck) {
                            // Ack was not set: stop cmd was generated by audio thread -> handles need to be
                            // invalidated.
                            m_data->engine.slotVersions[cmd.instanceId]++;
                        }

                        m_data->engine.freeSlotIndices.push_back(cmd.instanceId);
                        break;
                    }

                    case AudioCmdType::SkipToRequest:

                    case AudioCmdType::SkipByRequest: {
                        if (slot.isActive && slot.isStreamed) {
                            std::lock_guard<std::mutex> lock(slot.streamSlot->streamingMtx);
                            // Set the new frame index for streamed audio
                            slot.streamSlot->seekTargetFrameIdx = cmd.type == AudioCmdType::SkipToRequest
                                                                      ? cmd.data.skipTo.newFrameIndex
                                                                      : cmd.data.skipBy.newFrameIndex;
                            ma_rb_reset(&slot.streamSlot->streamBuffer);  // Reset buffer

                            AudioCmd seekResolvedNote{};
                            seekResolvedNote.type = AudioCmdType::SkipResolved;
                            seekResolvedNote.target = AudioCmdTarget::AudioInstanceTarget;
                            seekResolvedNote.instanceId = cmd.instanceId;
                            seekResolvedNote.needsAck = true;
                            pushCommand(seekResolvedNote, m_data->engine);

                            // Notify stream thread
                            m_data->engine.streamData.streamWakeRequested.store(true);
                            m_data->engine.streamData.wakeCV.notify_one();
                        }
                        break;
                    }

                    case AudioCmdType::SkipResolved: slot.isSkipRequested = false; break;

                    default: break;
                }
                break;
            }

            case AudioCmdTarget::ReverbInstanceTarget: {
                ReverbInstanceSlot& slot = m_data->engine.reverbInstSlots[cmd.instanceId];

                switch (cmd.type) {
                    case AudioCmdType::SetReverbInstanceBuffers: {
                        if (slot.buffers != nullptr) {
                            freeReverbBuffers(slot.buffers);
                        }
                        ReverbBuffers*& buffers = m_data->engine.pendingReverbBuffers[cmd.instanceId];
                        slot.buffers = buffers;
                        buffers = nullptr;
                        m_data->engine.pendingReverbBufferPresent[cmd.instanceId] = false;
                    }

                    default: break;
                }
                break;
            }

            default: break;
        }
    }
}

void AudioEngine::resolveAndSendAssetData() {
    for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
        AudioInstanceSlot& slot = m_data->engine.audioInstSlots[i];
        if (!slot.isActive || slot.audioData != nullptr) continue;

        AudioAsset* asset = m_data->engine.usedAssets[i];
        std::lock_guard<std::mutex> lock(asset->mtx);
        if (slot.isStreamed) {
            if (asset->state == AudioAsset::State::MetadataReady || asset->state == AudioAsset::State::FullLoading ||
                asset->state == AudioAsset::State::FullyLoaded) {
                slot.audioData = &slot.streamSlot->streamBuffer;
                slot.totalPcmFrames = asset->totalFrames;

                AudioCmd cmd{};
                cmd.type = AudioCmdType::AssetLoaded;
                cmd.target = AudioCmdTarget::AudioInstanceTarget;
                cmd.instanceId = i;
                cmd.data.assetData.audioData = &slot.streamSlot->streamBuffer;
                cmd.data.assetData.totalPcmFrames = asset->totalFrames;
                pushCommand(cmd, m_data->engine);
            }
        } else {
            if (asset->state == AudioAsset::State::FullyLoaded) {
                slot.audioData = asset->pcmData.data();
                slot.totalPcmFrames = asset->totalFrames;

                AudioCmd cmd{};
                cmd.type = AudioCmdType::AssetLoaded;
                cmd.target = AudioCmdTarget::AudioInstanceTarget;
                cmd.instanceId = i;
                cmd.data.assetData.audioData = asset->pcmData.data();
                cmd.data.assetData.totalPcmFrames = asset->totalFrames;
                pushCommand(cmd, m_data->engine);
            }
        }
    }
}

void AudioEngine::sendCommandsToAudioThread() {
    std::vector<unsigned int> sentIndices;

    for (unsigned int i = 0; i < m_data->engine.cmdOrdering.size(); i++) {
        const AudioCmdKey& key = m_data->engine.cmdOrdering[i];
        AudioCmd& cmd = m_data->engine.pushableCmds[key];

        if (!isCmdAllowed(cmd, m_data->engine)) continue;  // dependency based ignore

        beforeCmdSend(cmd, m_data->engine);
        if (rbWriteElements<AudioCmd>(&m_data->shared.mainToAudioQueue, &cmd, 1) != 1) {
            lgr::lout.warn("Failed to send cmd to audio thread because ring buffer is full");
            break;
        }
        afterCmdSend(cmd, m_data->engine);

        sentIndices.push_back(i);
        m_data->engine.pushableCmds.erase(key);
    }

    // Remove sent commands from ordering vec
    for (int i = sentIndices.size() - 1; i >= 0; i--) {
        unsigned int idx = sentIndices[i];
        m_data->engine.cmdOrdering.erase(m_data->engine.cmdOrdering.begin() + idx);
    }
}

bool AudioEngine::_isValid(AudioInstance instance) const {
    if (instance.id >= AUDIO_INSTANCE_LIMIT) return false;

    const AudioInstanceSlot& slot = m_data->engine.audioInstSlots[instance.id];
    return slot.isActive && m_data->engine.slotVersions[instance.id] == instance.version;
}

AudioEngine::AudioEngine() {
    m_data = new AudioEngineData{};
    std::fill_n(m_data->engine.slotVersions, AUDIO_INSTANCE_LIMIT, 1U);
    m_data->engine.streamData
        .tmpStreamBuffer = new float[STREAM_BUFFER_DURATION_SEC * FORMAT_FRAME_RATE * FORMAT_CHANNELS];

    m_data->engine.freeSlotIndices.reserve(AUDIO_INSTANCE_LIMIT);
    for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
        m_data->engine.freeSlotIndices.push_back(i);
    }

    if (ma_context_init(nullptr, 0, nullptr, &m_data->engine.context) != MA_SUCCESS) {
        lgr::lout.error("Could not create audio context...");
    }

    if (ma_rb_init(sizeof(AudioCmd) * COMMAND_QUEUE_CAPACITY, nullptr, nullptr, &m_data->shared.mainToAudioQueue) !=
        MA_SUCCESS) {
        lgr::lout.error("Could not create command buffer...");
    }
    if (ma_rb_init(sizeof(AudioCmd) * COMMAND_QUEUE_CAPACITY, nullptr, nullptr, &m_data->shared.audioToMainQueue) !=
        MA_SUCCESS) {
        lgr::lout.error("Could not create command buffer...");
    }

    // Define constant internal format for all decoders
    m_data->engine.decoderConfig = ma_decoder_config_init(ma_format_f32, FORMAT_CHANNELS, FORMAT_FRAME_RATE);

    // Configure streaming slots
    m_stopThreads.store(false);
    for (StreamBufferSlot& streamSlot : m_data->engine.streamData.streamSlots) {
        if (ma_rb_init(STREAM_BUFFER_BYTE_SIZE, nullptr, nullptr, &streamSlot.streamBuffer) != MA_SUCCESS) {
            lgr::lout.error("Could not create stream buffer...");
        }
    }

    // Point to stream wake up cvar and flag
    m_data->audio.streamWakeRequested = &m_data->engine.streamData.streamWakeRequested;
    m_data->audio.streamWakeCV = &m_data->engine.streamData.wakeCV;

    float* resolvedBusVolumes = new float[MIXING_BUS_LIMIT];
    std::fill_n(resolvedBusVolumes, MIXING_BUS_LIMIT, 1.0f);
    m_data->engine.resolvedBusVolumes = resolvedBusVolumes;
    m_data->audio.resolvedBusVolumes = resolvedBusVolumes;

    m_data->callbackContext.shared = &m_data->shared;
    m_data->callbackContext.audio = &m_data->audio;

    m_streamingWorker = std::thread(&AudioEngine::streamWorkerLoop, this);
    m_audioLoaderWorker = std::thread(&AudioEngine::audioLoadWorkerLoop, this);
}

AudioEngine::~AudioEngine() {
    {
        std::unique_lock lock(m_data->engine.engineMutex);

        m_stopThreads.store(true);
        m_data->engine.streamData.wakeCV.notify_one();
        m_streamingWorker.join();
        m_data->engine.loaderData.loaderCV.notify_one();
        m_audioLoaderWorker.join();
        delete[] m_data->engine.streamData.tmpStreamBuffer;

        ma_device_uninit(&m_data->engine.currentOutputDevice);

        ma_rb_uninit(&m_data->shared.mainToAudioQueue);
        ma_rb_uninit(&m_data->shared.audioToMainQueue);
        for (StreamBufferSlot& streamSlot : m_data->engine.streamData.streamSlots) {
            ma_rb_uninit(&streamSlot.streamBuffer);
            if (streamSlot.isActive) {
                ma_decoder_uninit(&streamSlot.decoder);
            }
        }

        delete[] m_data->engine.resolvedBusVolumes;
        if (m_data->engine.pendingResolvedBusVolumes != nullptr) {
            delete[] m_data->engine.pendingResolvedBusVolumes;
        }

        for (unsigned int r = 0; r < REVERB_INSTANCE_LIMIT; r++) {
            ReverbInstanceSlot& reverbSlot = m_data->engine.reverbInstSlots[r];
            ReverbBuffers* pendingReverbBuff = m_data->engine.pendingReverbBuffers[r];
            if (reverbSlot.buffers != nullptr) {
                freeReverbBuffers(reverbSlot.buffers);
            }
            if (pendingReverbBuff != nullptr) {
                freeReverbBuffers(pendingReverbBuff);
            }
        }
        ma_context_uninit(&m_data->engine.context);
    }
    delete m_data;
}

bool AudioEngine::setOutputDevice(unsigned int deviceId) {
    std::unique_lock lock(m_data->engine.engineMutex);

    if (deviceId >= m_data->engine.outputDeviceCount) {
        return false;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = &m_data->engine.outputDeviceInfos[deviceId].id;
    config.playback.format = m_data->engine.decoderConfig.format;
    config.playback.channels = m_data->engine.decoderConfig.channels;
    config.sampleRate = m_data->engine.decoderConfig.sampleRate;
    config.dataCallback = audioCallback;
    config.pUserData = &m_data->callbackContext;

    // Fixate frame count on callbacks
    config.periodSizeInFrames = FRAMES_PER_PERIOD;
    config.noFixedSizedCallback = MA_FALSE;

    ma_device* device = &m_data->engine.currentOutputDevice;
    ma_device_uninit(device);
    if (ma_device_init(&m_data->engine.context, &config, device) != MA_SUCCESS) {
        lgr::lout.error("Could not create audio output device!");
        return false;
    }

    if (ma_device_start(device) != MA_SUCCESS) {
        ma_device_uninit(device);
        lgr::lout.error("Could not start audio output device!");
        return false;
    }
    return true;
}

bool AudioEngine::setInputDevice(unsigned int deviceId) {
    // TODO Implement maybe, doesnt have a usecase tho currently
    throw std::runtime_error("AudioEngine::setInputDevice method not implemented");
}

bool AudioEngine::loadDevices() {
    std::unique_lock lock(m_data->engine.engineMutex);

    m_data->engine.outputDeviceInfos = nullptr;
    m_data->engine.outputDeviceCount = 0;
    m_data->engine.inputDeviceInfos = nullptr;
    m_data->engine.inputDeviceCount = 0;

    ma_result result = ma_context_get_devices(
        &m_data->engine.context,
        &m_data->engine.outputDeviceInfos,
        &m_data->engine.outputDeviceCount,
        &m_data->engine.inputDeviceInfos,
        &m_data->engine.inputDeviceCount
    );
    return result == MA_SUCCESS;
}

std::vector<AudioDevice> AudioEngine::getOutputDevices() const {
    std::shared_lock lock(m_data->engine.engineMutex);

    std::vector<AudioDevice> outputDevices;
    outputDevices.reserve(m_data->engine.outputDeviceCount);
    for (unsigned int i = 0; i < m_data->engine.outputDeviceCount; i++) {
        outputDevices.push_back(
            {i, m_data->engine.outputDeviceInfos[i].name, m_data->engine.outputDeviceInfos[i].isDefault != MA_FALSE}
        );
    }
    return outputDevices;
}

std::vector<AudioDevice> AudioEngine::getInputDevices() const {
    std::shared_lock lock(m_data->engine.engineMutex);

    std::vector<AudioDevice> inputDevices;
    inputDevices.reserve(m_data->engine.inputDeviceCount);
    for (unsigned int i = 0; i < m_data->engine.inputDeviceCount; i++) {
        inputDevices.push_back(
            {i, m_data->engine.inputDeviceInfos[i].name, m_data->engine.inputDeviceInfos[i].isDefault != MA_FALSE}
        );
    }
    return inputDevices;
}

AudioInstance AudioEngine::playBuffered(const std::string& file) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (m_data->engine.freeSlotIndices.empty()) {
        lgr::lout.error("Could not reserve audio instance slot!");
        return {};
    }

    AudioAsset* asset = nullptr;
    auto it = m_data->engine.loadedAssets.find(file);
    if (it != m_data->engine.loadedAssets.end()) {
        asset = it->second.get();
    } else {
        std::unique_ptr<AudioAsset> newAssetPtr = std::make_unique<AudioAsset>();
        newAssetPtr->assetPath = file;
        asset = newAssetPtr.get();
        m_data->engine.loadedAssets.emplace(file, std::move(newAssetPtr));
    }

    unsigned int nextIndex = m_data->engine.freeSlotIndices.back();
    m_data->engine.freeSlotIndices.pop_back();

    m_data->engine.usedAssets[nextIndex] = asset;

    ensureFullyLoaded(asset, m_data->engine);

    AudioCmd cmd{};
    cmd.type = AudioCmdType::Play;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = nextIndex;
    cmd.data.singleBool = false;  // Not streamed
    pushCommand(cmd, m_data->engine);

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[nextIndex];
    slot = {};  // Reset
    slot.isActive = true;
    slot.isPlaying = true;

    return {nextIndex, m_data->engine.slotVersions[nextIndex]};
}

AudioInstance AudioEngine::playStreamed(const std::string& file) {
    std::unique_lock lock(m_data->engine.engineMutex);

    if (m_data->engine.freeSlotIndices.empty()) {
        lgr::lout.error("Could not reserve audio instance slot!");
        return {};
    }

    StreamBufferSlot* freeSlot = nullptr;
    for (StreamBufferSlot& slot : m_data->engine.streamData.streamSlots) {
        std::lock_guard<std::mutex> lock(slot.streamingMtx);
        if (!slot.isActive) {
            freeSlot = &slot;
            break;
        }
    }
    if (freeSlot == nullptr) {
        lgr::lout.error("Could not reserve streaming slot!");
        return {};
    }

    if (ma_decoder_init_file(file.c_str(), &m_data->engine.decoderConfig, &freeSlot->decoder) != MA_SUCCESS) {
        lgr::lout.error("Failed to load audio file: " + file);
        return {};
    }

    AudioAsset* asset = nullptr;
    auto it = m_data->engine.loadedAssets.find(file);
    if (it != m_data->engine.loadedAssets.end()) {
        asset = it->second.get();
    } else {
        std::unique_ptr<AudioAsset> newAssetPtr = std::make_unique<AudioAsset>();
        newAssetPtr->assetPath = file;
        asset = newAssetPtr.get();
        m_data->engine.loadedAssets.emplace(file, std::move(newAssetPtr));
    }

    unsigned int nextIndex = m_data->engine.freeSlotIndices.back();
    m_data->engine.freeSlotIndices.pop_back();

    m_data->engine.usedAssets[nextIndex] = asset;

    ensureMetadata(asset, m_data->engine);

    AudioCmd cmd{};
    cmd.type = AudioCmdType::Play;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = nextIndex;
    cmd.data.singleBool = true;  // Streamed
    pushCommand(cmd, m_data->engine);

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[nextIndex];
    slot = {};  // Reset
    slot.isActive = true;
    slot.isPlaying = true;
    slot.isStreamed = true;
    slot.streamSlot = freeSlot;

    freeSlot->isActive = true;

    // Notify stream thread
    m_data->engine.streamData.streamWakeRequested.store(true);
    m_data->engine.streamData.wakeCV.notify_one();

    return {nextIndex, m_data->engine.slotVersions[nextIndex]};
}

bool AudioEngine::isValid(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    return _isValid(instance);
}

bool AudioEngine::isPlaying(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].isPlaying;
}

void AudioEngine::stop(AudioInstance instance) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.slotVersions[instance.id]++;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::Stop;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.needsAck = true;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::pause(AudioInstance instance) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[instance.id];
    if (!slot.isPlaying) {
        return;  // Early return
    }
    slot.isPlaying = false;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetPlaying;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleBool = slot.isPlaying;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::resume(AudioInstance instance) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[instance.id];
    if (slot.isPlaying) {
        return;  // Early return
    }
    slot.isPlaying = true;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetPlaying;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleBool = slot.isPlaying;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::skipTo(AudioInstance instance, float pos) {
    pos = std::max<float>(-1.0f, std::min<float>(pos, 1.0f));

    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[instance.id];

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SkipToRequest;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.needsAck = slot.isStreamed;
    cmd.data.skipTo.normalizedPos = pos;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::skipBy(AudioInstance instance, float seconds) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[instance.id];

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SkipByRequest;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.needsAck = slot.isStreamed;
    cmd.data.skipBy.deltaSeconds = seconds;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::stopAll() {
    std::unique_lock lock(m_data->engine.engineMutex);
    for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
        AudioInstanceSlot& slot = m_data->engine.audioInstSlots[i];

        if (slot.isActive) {
            m_data->engine.slotVersions[i]++;

            AudioCmd cmd{};
            cmd.type = AudioCmdType::Stop;
            cmd.target = AudioCmdTarget::AudioInstanceTarget;
            cmd.instanceId = i;
            cmd.needsAck = true;
            pushCommand(cmd, m_data->engine);
        }
    }
}

void AudioEngine::pauseAll() {
    std::unique_lock lock(m_data->engine.engineMutex);
    for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
        AudioInstanceSlot& slot = m_data->engine.audioInstSlots[i];

        if (slot.isActive && slot.isPlaying) {
            slot.isPlaying = false;

            AudioCmd cmd{};
            cmd.type = AudioCmdType::SetPlaying;
            cmd.target = AudioCmdTarget::AudioInstanceTarget;
            cmd.instanceId = i;
            cmd.data.singleBool = slot.isPlaying;
            pushCommand(cmd, m_data->engine);
        }
    }
}

void AudioEngine::resumeAll() {
    std::unique_lock lock(m_data->engine.engineMutex);
    for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
        AudioInstanceSlot& slot = m_data->engine.audioInstSlots[i];

        if (slot.isActive && !slot.isPlaying) {
            slot.isPlaying = true;

            AudioCmd cmd{};
            cmd.type = AudioCmdType::SetPlaying;
            cmd.target = AudioCmdTarget::AudioInstanceTarget;
            cmd.instanceId = i;
            cmd.data.singleBool = slot.isPlaying;
            pushCommand(cmd, m_data->engine);
        }
    }
}

void AudioEngine::setVolume(AudioInstance instance, float volume) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    volume = std::max<float>(0.0f, volume);
    m_data->engine.audioInstSlots[instance.id].volume = volume;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetVolume;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = volume;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setPitch(AudioInstance instance, float pitch) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    pitch = std::clamp<float>(pitch, 0.25f, 4.0f);
    m_data->engine.audioInstSlots[instance.id].pitch = pitch;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetPitch;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = pitch;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setPan(AudioInstance instance, float pan) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    pan = std::clamp<float>(pan, -1.0f, 1.0f);
    m_data->engine.audioInstSlots[instance.id].pan = pan;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetPan;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = pan;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setLooping(AudioInstance instance, bool looping) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    AudioInstanceSlot& slot = m_data->engine.audioInstSlots[instance.id];

    slot.isLooping = looping;
    if (slot.isStreamed) {
        std::lock_guard<std::mutex> lock(slot.streamSlot->streamingMtx);
        slot.streamSlot->isLooping = true;
    }

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetLooping;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleBool = looping;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setSpatialization(AudioInstance instance, bool enabled) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].spatial.enabled = enabled;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetSpatialEnabled;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleBool = enabled;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::set3DPosition(AudioInstance instance, const glm::vec3& position) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].spatial.position = position;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetSpatialPosition;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.vec3Value = position;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setVelocity(AudioInstance instance, const glm::vec3& velocity) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].spatial.velocity = velocity;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetSpatialVelocity;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.vec3Value = velocity;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setDoppler(AudioInstance instance, float dopplerFactor) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    dopplerFactor = std::clamp<float>(dopplerFactor, 0.0f, 2.0f);
    m_data->engine.audioInstSlots[instance.id].spatial.dopplerFactor = dopplerFactor;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetDopplerFactor;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = dopplerFactor;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setAttenuation(AudioInstance instance, Attenuation attenuation) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    attenuation.minDistance = std::max<float>(attenuation.minDistance, 0.0f);
    attenuation.maxDistance = std::max<float>(attenuation.maxDistance, 0.0f);

    m_data->engine.audioInstSlots[instance.id].spatial.attenuation = attenuation;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetAttenuation;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.attenuation = attenuation;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setBus(AudioInstance instance, unsigned int busId) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;
    if (busId >= MIXING_BUS_LIMIT) return;

    m_data->engine.audioInstSlots[instance.id].busId = busId;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetBus;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleUInt = busId;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setLowpassEnabled(AudioInstance instance, bool enabled) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].lowpassConf.enabled = enabled;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetLowpassEnabled;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleBool = enabled;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setLowpassCutoffHz(AudioInstance instance, float cutoffHz) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].lowpassConf.cutoffHz = cutoffHz;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetLowpassCutoffHz;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = cutoffHz;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setHighpassEnabled(AudioInstance instance, bool enabled) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].highpassConf.enabled = enabled;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetHighpassEnabled;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleBool = enabled;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setHighpassCuroffHz(AudioInstance instance, float cutoffHz) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].highpassConf.cutoffHz = cutoffHz;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetHighpassCutoffHz;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = cutoffHz;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setReverbSend(AudioInstance instance, float amount) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;

    m_data->engine.audioInstSlots[instance.id].reverbSend = amount;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetReverbSend;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleFloat = amount;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setReverb(AudioInstance instance, unsigned int reverbId) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return;
    if (reverbId >= REVERB_INSTANCE_LIMIT) return;

    m_data->engine.audioInstSlots[instance.id].reverbId = reverbId;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetReverbId;
    cmd.target = AudioCmdTarget::AudioInstanceTarget;
    cmd.instanceId = instance.id;
    cmd.data.singleUInt = reverbId;
    pushCommand(cmd, m_data->engine);
}

float AudioEngine::getVolume(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return 0.0f;

    return m_data->engine.audioInstSlots[instance.id].volume;
}

float AudioEngine::getPitch(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return 0.0f;

    return m_data->engine.audioInstSlots[instance.id].pitch;
}

float AudioEngine::getPan(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return 0.0f;

    return m_data->engine.audioInstSlots[instance.id].pan;
}

bool AudioEngine::getLooping(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].isLooping;
}

bool AudioEngine::getSpatialization(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].spatial.enabled;
}

glm::vec3 AudioEngine::get3DPosition(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return {};

    return m_data->engine.audioInstSlots[instance.id].spatial.position;
}

glm::vec3 AudioEngine::getVelocity(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return {};

    return m_data->engine.audioInstSlots[instance.id].spatial.velocity;
}

float AudioEngine::getDoppler(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return 0.0f;

    return m_data->engine.audioInstSlots[instance.id].spatial.dopplerFactor;
}

Attenuation AudioEngine::getAttenuation(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return {};

    return m_data->engine.audioInstSlots[instance.id].spatial.attenuation;
}

unsigned int AudioEngine::getBus(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);

    return m_data->engine.audioInstSlots[instance.id].busId;
}

bool AudioEngine::getLowpassEnabled(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].lowpassConf.enabled;
}

float AudioEngine::getLowpassCutoffHz(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].lowpassConf.cutoffHz;
}

bool AudioEngine::getHighpassEnabled(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].highpassConf.enabled;
}

float AudioEngine::getHighpassCutoffHz(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return false;

    return m_data->engine.audioInstSlots[instance.id].highpassConf.cutoffHz;
}

float AudioEngine::getReverbSend(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return 0.0f;

    return m_data->engine.audioInstSlots[instance.id].reverbSend;
}

unsigned int AudioEngine::getReverb(AudioInstance instance) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (!_isValid(instance)) return 0.0f;

    return m_data->engine.audioInstSlots[instance.id].reverbId;
}

void AudioEngine::setBusVolume(unsigned int busId, float volume) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (busId >= MIXING_BUS_LIMIT) return;

    m_data->engine.mixingBuses[busId].volume = std::max<float>(volume, 0.0f);

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetResolvedBusVolumes;
    cmd.needsAck = true;
    cmd.data.singlePtr = nullptr;  // Allocated and pushed later
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setBusParent(unsigned int busId, int parentId) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (busId >= MIXING_BUS_LIMIT) return;
    if (parentId >= static_cast<int>(MIXING_BUS_LIMIT)) return;

    if (createsCyclicBusDependency(m_data->engine.mixingBuses, busId, parentId)) return;

    m_data->engine.mixingBuses[busId].parent = parentId;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetResolvedBusVolumes;
    cmd.needsAck = true;
    cmd.data.singlePtr = nullptr;  // Allocated and pushed later
    pushCommand(cmd, m_data->engine);
}

float AudioEngine::getBusVolume(unsigned int busId) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (busId >= MIXING_BUS_LIMIT) return 0.0f;

    return m_data->engine.mixingBuses[busId].volume;
}

int AudioEngine::getBusParent(unsigned int busId) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (busId >= MIXING_BUS_LIMIT) return 0;

    return m_data->engine.mixingBuses[busId].parent;
}

void AudioEngine::createReverbInstance(unsigned int reverbId, ReverbDesign design, float roomSizeMeters) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (reverbId >= REVERB_INSTANCE_LIMIT) return;

    roomSizeMeters = std::clamp<float>(roomSizeMeters, MIN_ROOM_SIZE_METERS, MAX_ROOM_SIZE_METERS);

    float delayFactor = roomSizeMeters / design.baseRoomSizeMeters;
    for (float& msDelay : design.combMsDelays) {
        msDelay *= delayFactor;
    }
    for (float& msDelay : design.allPassMsDelays) {
        msDelay *= delayFactor;
    }
    ReverbDesign sDesign = sanitizeReverbDesign(design);

    m_data->engine.reverbSlotDesigns[reverbId] = std::move(sDesign);

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetReverbInstanceBuffers;
    cmd.target = AudioCmdTarget::ReverbInstanceTarget;
    cmd.instanceId = reverbId;
    cmd.needsAck = true;
    cmd.data.singlePtr = nullptr;  // Constructed later or keep nullptr if free

    pushCommand(cmd, m_data->engine);
}

void AudioEngine::destroyReverbInstance(unsigned int reverbId) {
    createReverbInstance(reverbId, {}, 0.0f);  // Just set to empty design
}

void AudioEngine::destroyAllReverbInstances() {
    for (unsigned int r = 0; r < REVERB_INSTANCE_LIMIT; r++) {
        destroyReverbInstance(r);
    }
}

void AudioEngine::setReverbInstanceEnabled(unsigned int reverbId, bool enabled) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (reverbId >= REVERB_INSTANCE_LIMIT) return;

    m_data->engine.reverbInstSlots[reverbId].isEnabled = enabled;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetReverbInstanceEnabled;
    cmd.target = AudioCmdTarget::ReverbInstanceTarget;
    cmd.instanceId = reverbId;
    cmd.data.singleBool = enabled;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setReverbInstanceParams(unsigned int reverbId, ReverbParams params) {
    std::unique_lock lock(m_data->engine.engineMutex);
    if (reverbId >= REVERB_INSTANCE_LIMIT) return;

    params.wet = std::clamp<float>(params.wet, 0.0f, 1.0f);
    params.decaySeconds = std::max<float>(params.decaySeconds, NEAR_ZERO_EPSILON);
    params.damping = std::clamp<float>(params.damping, 0.0f, 1.0f);

    m_data->engine.reverbInstSlots[reverbId].params = params;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetReverbInstanceParams;
    cmd.target = AudioCmdTarget::ReverbInstanceTarget;
    cmd.instanceId = reverbId;
    cmd.data.reverbParams = params;
    pushCommand(cmd, m_data->engine);
}

ReverbDesign AudioEngine::getReverbInstanceDesign(unsigned int reverbId) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (reverbId >= REVERB_INSTANCE_LIMIT) return {};

    return m_data->engine.reverbSlotDesigns[reverbId];
}

bool AudioEngine::getReverbInstanceEnabled(unsigned int reverbId) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (reverbId >= REVERB_INSTANCE_LIMIT) return false;

    return m_data->engine.reverbInstSlots[reverbId].isEnabled;
}

ReverbParams AudioEngine::getReverbInstanceParams(unsigned int reverbId) const {
    std::shared_lock lock(m_data->engine.engineMutex);
    if (reverbId >= REVERB_INSTANCE_LIMIT) return {};

    return m_data->engine.reverbInstSlots[reverbId].params;
}

void AudioEngine::setListenerVelocity(const glm::vec3& velocity) {
    std::unique_lock lock(m_data->engine.engineMutex);

    m_data->engine.listenerState.velocity = velocity;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetListenerVelocity;
    cmd.data.vec3Value = velocity;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setListenerPosition(const glm::vec3& position) {
    std::unique_lock lock(m_data->engine.engineMutex);

    m_data->engine.listenerState.position = position;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetListenerPosition;
    cmd.data.vec3Value = position;
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setListenerForward(const glm::vec3& forward) {
    if (isNearZero(forward)) return;
    std::unique_lock lock(m_data->engine.engineMutex);

    m_data->engine.listenerState.forwardVec = forward;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetListenerForward;
    cmd.data.vec3Value = glm::normalize(forward);
    pushCommand(cmd, m_data->engine);
}

void AudioEngine::setListenerUp(const glm::vec3& up) {
    if (isNearZero(up)) return;
    std::unique_lock lock(m_data->engine.engineMutex);

    m_data->engine.listenerState.upVec = up;

    AudioCmd cmd{};
    cmd.type = AudioCmdType::SetListenerUp;
    cmd.data.vec3Value = glm::normalize(up);
    pushCommand(cmd, m_data->engine);
}

glm::vec3 AudioEngine::getListenerVelocity() const {
    std::shared_lock lock(m_data->engine.engineMutex);

    return m_data->engine.listenerState.velocity;
}

glm::vec3 AudioEngine::getListenerPosition() const {
    std::shared_lock lock(m_data->engine.engineMutex);

    return m_data->engine.listenerState.position;
}

glm::vec3 AudioEngine::getListenerForward() const {
    std::shared_lock lock(m_data->engine.engineMutex);

    return m_data->engine.listenerState.forwardVec;
}

glm::vec3 AudioEngine::getListenerUp() const {
    std::shared_lock lock(m_data->engine.engineMutex);

    return m_data->engine.listenerState.upVec;
}

void AudioEngine::update(float deltaSeconds) {
    std::unique_lock lock(m_data->engine.engineMutex);
    processCmdsFromAudioThread();
    resolveAndSendAssetData();
    sendCommandsToAudioThread();
}
