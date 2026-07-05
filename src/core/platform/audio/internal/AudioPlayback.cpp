#include "AudioPlayback.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#include "AudioEngineDefs.h"
#include "Logger.h"

enum FetchResult {
    Success,
    Starved,
    EndReached
};

// Reads from the index at specific count of frames. Does NOT handle wrap around, thus may read less frames then
// requested.
static FetchResult fetchFrames(
    AudioInstanceSlot& slot,
    float* dst,
    size_t frameIndex,
    size_t frameCount,
    size_t* framesRead
) {
    if (frameIndex >= slot.totalPcmFrames) return FetchResult::EndReached;

    size_t framesUntilEnd = slot.totalPcmFrames - frameIndex;
    size_t samplesToRead = std::min<size_t>(frameCount, framesUntilEnd) * FORMAT_CHANNELS;
    size_t framesToRead = samplesToRead / FORMAT_CHANNELS;

    if (slot.isStreamed) {
        // read from ringbuffer
        *framesRead = rbReadElements<float>((ma_rb*)slot.audioData, dst, samplesToRead) / FORMAT_CHANNELS;

        if (*framesRead != framesToRead) {
            return FetchResult::Starved;
        }
    } else {
        const float* pcm = (float*)slot.audioData;
        std::memcpy(dst, pcm + (frameIndex * FORMAT_CHANNELS), samplesToRead * SAMPLE_BYTE_SIZE);
        *framesRead = framesToRead;
    }
    return framesToRead < frameCount ? FetchResult::EndReached : FetchResult::Success;
}

// Fills the prebuffer to its ability. May fail to fill partially or fully if starved of frames or end reached (Second
// case only if non looping).
static FetchResult bulkFetchIntoPrebuffer(AudioInstanceSlot& slot) {
    FetchResult res = FetchResult::Success;
    while (slot.state->validPrebufferFrames < PREBUFFER_FRAME_COUNT) {
        size_t frameFetchPos = slot.state->currentFrameIndex + slot.state->validPrebufferFrames;

        // Wrap around if looping
        if (slot.isLooping && frameFetchPos >= slot.totalPcmFrames) {
            frameFetchPos %= slot.totalPcmFrames;
        }

        size_t framesRead = 0;
        res = fetchFrames(
            slot,
            slot.state->prebuffer + (slot.state->validPrebufferFrames * FORMAT_CHANNELS),
            frameFetchPos,
            PREBUFFER_FRAME_COUNT - slot.state->validPrebufferFrames,
            &framesRead
        );

        slot.state->validPrebufferFrames += framesRead;

        if (res == FetchResult::Starved || framesRead == 0) {
            break;  // Break if frames missing
        }
        if (res == FetchResult::EndReached && !slot.isLooping) {
            break;  // Break if end reached and not looping
        }
    }
    return res;
}

// Shifts the prebuffer by the offset and adjust the valid frame count
static void shiftPrebufferWindow(AudioInstanceSlot& slot, unsigned int offset) {
    if (offset == 0 || slot.state->validPrebufferFrames == 0) return;

    if (offset >= slot.state->validPrebufferFrames) {
        slot.state->validPrebufferFrames = 0;
        return;
    }

    unsigned int framesRemaining = slot.state->validPrebufferFrames - offset;
    unsigned int samplesToMove = framesRemaining * FORMAT_CHANNELS;

    const float* src = slot.state->prebuffer + (offset * FORMAT_CHANNELS);
    std::memmove(slot.state->prebuffer, src, samplesToMove * SAMPLE_BYTE_SIZE);

    slot.state->validPrebufferFrames = framesRemaining;
}

// Given the prebuffer and the factional frame index, produces an interpolated output frame
static void produceInterpolatedFrame(const float* prebuffer, size_t validFrames, float frac, float* outFrame) {
    frac = std::clamp<float>(frac, 0.0f, 1.0f);

    // No data -> write silence
    if (validFrames == 0) {
        for (unsigned c = 0; c < FORMAT_CHANNELS; c++) {
            outFrame[c] = 0.0f;
        }
        return;
    }

    // One frame -> ouput this one only
    if (validFrames == 1) {
        for (unsigned c = 0; c < FORMAT_CHANNELS; c++) {
            outFrame[c] = prebuffer[c];
        }
        return;
    }

    // Linear interpolation
    const float* a = prebuffer;
    const float* b = prebuffer + FORMAT_CHANNELS;

    for (unsigned c = 0; c < FORMAT_CHANNELS; c++) {
        outFrame[c] = a[c] * (1.0f - frac) + b[c] * frac;
    }
}

static float computeDopplerPitch(const AudioInstanceSlot& slot, const ListenerState& listener) {
    glm::vec3 delta = listener.position - slot.spatial.position;

    // Check if distance is not near zero
    float dist2 = glm::dot(delta, delta);
    if (dist2 < NEAR_ZERO_EPSILON * NEAR_ZERO_EPSILON) return 1.0f;

    glm::vec3 dirToListener = delta * glm::inversesqrt(dist2);
    float relativeVelocity = glm::dot(slot.spatial.velocity - listener.velocity, dirToListener);
    relativeVelocity = glm::clamp(relativeVelocity, -0.9f * SPEED_OF_SOUND, 0.9f * SPEED_OF_SOUND);  // Prevent extremes

    float doppler = SPEED_OF_SOUND / (SPEED_OF_SOUND - slot.spatial.dopplerFactor * relativeVelocity);

    return std::clamp<float>(doppler, 0.5f, 2.0f);
}

static float computeAttenuationVolumeFactor(const AudioInstanceSlot& slot, const ListenerState& listener) {
    const Attenuation& att = slot.spatial.attenuation;

    if (att.model == AttenuationModel::None) {
        return 1.0f;
    }

    float dist = glm::length(slot.spatial.position - listener.position);
    // Handle near zero distance and malformed boundaries
    if (att.maxDistance <= att.minDistance + NEAR_ZERO_EPSILON) {
        float volume = dist <= att.minDistance ? 1.0f : 0.0f;
        if (att.invert) {
            volume = 1.0f - volume;
        }
        return volume;
    }

    float t = glm::clamp((dist - att.minDistance) / (att.maxDistance - att.minDistance), 0.0f, 1.0f);
    if (att.invert) {
        t = 1.0f - t;
    }

    switch (att.model) {
        case AttenuationModel::Linear: return 1.0f - t;

        case AttenuationModel::Inverse: {
            // Smooth inverse falloff, normalized to [0,1]
            float base = 1.0f / (1.0f + att.curveStrength * t * t);
            float maxBase = 1.0f / (1.0f + att.curveStrength);
            return (base - maxBase) / (1.0f - maxBase);
        }

        case AttenuationModel::Exponential: return std::pow(1.0f - t, att.curveStrength);

        default: return 1.0f;
    }
}

static float computeSpatialPan(const AudioInstanceSlot& slot, const ListenerState& listener) {
    glm::vec3 rightVec = glm::normalize(glm::cross(listener.forwardVec, listener.upVec));

    glm::vec3 delta = slot.spatial.position - listener.position;
    float azimuth = std::atan2(glm::dot(delta, rightVec), glm::dot(delta, listener.forwardVec));
    return std::sin(azimuth);  // simple left/right pan
}

static float computeAlphaLowpass(const LowpassConfig& lowpassConf) {
    return 1.0f - expf(-2.0f * M_PI * lowpassConf.cutoffHz / FORMAT_FRAME_RATE);
}

static float computeAlphaHighpass(const HighpassConfig& highpassConf) {
    return expf(-2.0f * M_PI * highpassConf.cutoffHz / FORMAT_FRAME_RATE);
}

static float processLowpass(LowpassState& state, float x) {
    state.yPrev = state.yPrev + state.alpha * (x - state.yPrev);
    return state.yPrev;
}

static float processHighpass(HighpassState& state, float x) {
    state.yPrev = state.alpha * (state.yPrev + x - state.xPrev);
    state.xPrev = x;
    return state.yPrev;
}

// Returns true if read cursor has reached end and slot has been deactivated
static bool advanceReadCursor(AudioInstanceSlot& slot, float amount) {
    slot.state->currentFrameIndexFractional += amount;

    unsigned int shiftAmount = static_cast<unsigned int>(slot.state->currentFrameIndexFractional);
    shiftPrebufferWindow(slot, shiftAmount);
    slot.state->currentFrameIndexFractional -= shiftAmount;
    slot.state->currentFrameIndex += shiftAmount;

    // Mark as expired if has ended, return true in this case
    if (slot.state->currentFrameIndex >= slot.totalPcmFrames) {
        if (!slot.isLooping) {
            slot.isActive = false;
            return true;
        }
        // Wrap around
        slot.state->currentFrameIndex %= slot.totalPcmFrames;
    }
    return false;
}

static float processComb(CombFilter& c, float x, float decaySeconds, float damping) {
    float y = c.buffer[c.currPosition];

    // One-pole lowpass inside feedback loop
    c.lastSample = (1.0f - damping) * c.lastSample + damping * y;

    c.buffer[c.currPosition] = x + (c.lastSample * c.feedback);

    if (++c.currPosition >= c.bufferSize) {
        c.currPosition = 0;
    }
    return y;
}

static float processAllPass(AllPassFilter& a, float x) {
    float current = a.buffer[a.currPosition];

    float y = -x + current;
    a.buffer[a.currPosition] = x + current * a.feedback;

    if (++a.currPosition >= a.bufferSize) {
        a.currPosition = 0;
    }
    return y;
}

static float processReverbSample(ReverbInstanceSlot& slot, float x) {
    ReverbBuffers* b = slot.buffers;

    // Parallel combs
    float combSum = 0.0f;
    for (unsigned int i = 0; i < b->combCount; i++) {
        combSum += processComb(b->combs[i], x, slot.params.decaySeconds, slot.params.damping);
    }

    float y = combSum * (1.0f / static_cast<float>(b->combCount));

    // Serial all passes
    for (unsigned int i = 0; i < b->allPassCount; i++) {
        y = processAllPass(b->allPassFilters[i], y);
    }

    return y;
}

static void processReverbInstance(ReverbInstanceSlot& slot, float* out, size_t frameCount) {
    for (size_t f = 0; f < frameCount; f++) {
        float wetSample = slot.params.wet * processReverbSample(slot, slot.buffers->input[f]);
        for (unsigned int c = 0; c < FORMAT_CHANNELS; c++) {
            out[f * FORMAT_CHANNELS + c] += wetSample;
        }
    }
}

static void calculateChannelGains(const AudioInstanceSlot& slot, const ListenerState& listener, float* channelGains) {
    float totalVolume = slot.volume;
    float totalPan = slot.pan;
    if (slot.spatial.enabled) {
        totalVolume *= computeAttenuationVolumeFactor(slot, listener);
        totalPan = computeSpatialPan(slot, listener);  // Overwrite base panning on 3d audio
    }
    if constexpr (FORMAT_CHANNELS == 2) {
        // TODO Expand pan to multichannel
        channelGains[0] = std::sqrt(0.5f * (1.0f - totalPan)) * totalVolume;
        channelGains[1] = std::sqrt(0.5f * (1.0f + totalPan)) * totalVolume;
        return;
    } else {
        for (unsigned int c = 0; c < FORMAT_CHANNELS; c++) {
            channelGains[c] = totalVolume;
        }
    }
}

static void calculateCombFeedbacks(ReverbInstanceSlot& slot) {
    for (unsigned int c = 0; c < slot.buffers->combCount; c++) {
        CombFilter& comb = slot.buffers->combs[c];
        // Compute feedback from RT60
        float feedback = std::pow(10.0f, -(comb.delayMs / 1000.0f) / slot.params.decaySeconds);
        comb.feedback = std::clamp<float>(feedback, -MAX_REVERB_FEEDBACK, MAX_REVERB_FEEDBACK);
    }
}

static float caclulatePitch(const AudioInstanceSlot& slot, const ListenerState& listener) {
    float resultPitch = slot.pitch;
    if (slot.spatial.enabled) {
        resultPitch *= computeDopplerPitch(slot, listener);
    }
    return std::clamp<float>(resultPitch, 0.25f, 4.0f);
}

static void checkRemainingSampleAvailability(const AudioThreadContext* context, const AudioInstanceSlot& slot) {
    if (!slot.isStreamed) return;

    size_t availableSamples = ma_rb_available_read((ma_rb*)slot.audioData) / SAMPLE_BYTE_SIZE;
    size_t availableFrames = availableSamples / FORMAT_CHANNELS;
    if (availableFrames * slot.pitch <= SAFETY_STREAM_BUFFER_FRAMES) {
        if (slot.state->currentFrameIndex + availableFrames >= slot.totalPcmFrames && !slot.isLooping) {
            return;
        }

        // Notify streaming thread that more data is needed
        context->audio->streamWakeRequested->store(true);
        context->audio->streamWakeCV->notify_one();
    }
}

static void mixInstance(const AudioThreadContext* context, AudioInstanceSlot& slot, float* out, size_t frameCount) {
    ReverbInstanceSlot& revSlot = context->audio->reverbInstSlots[slot.reverbId];

    float totalPitch = slot.state->mix.totalPitch;
    float mixingBusVolume = context->audio->resolvedBusVolumes[slot.busId];
    const float* channelGains = slot.state->mix.channelGains;

    float frame[FORMAT_CHANNELS];
    for (size_t f = 0; f < frameCount; f++) {
        FetchResult lastFetch = bulkFetchIntoPrebuffer(slot);
        // If no frames are available after fetch, exit and dont advance read cursor
        if (slot.state->validPrebufferFrames == 0) {
            return;
        }

        produceInterpolatedFrame(
            slot.state->prebuffer, slot.state->validPrebufferFrames, slot.state->currentFrameIndexFractional, frame
        );

        if (slot.lowpassConf.enabled) {
            for (unsigned int c = 0; c < FORMAT_CHANNELS; c++) {
                frame[c] = processLowpass(slot.state->lowpassStates[c], frame[c]);
            }
        }
        if (slot.highpassConf.enabled) {
            for (unsigned int c = 0; c < FORMAT_CHANNELS; c++) {
                frame[c] = processHighpass(slot.state->highpassStates[c], frame[c]);
            }
        }

        // Apply channel gains
        for (unsigned int c = 0; c < FORMAT_CHANNELS; c++) {
            frame[c] *= channelGains[c] * mixingBusVolume;
        }

        // Output
        float frameSum = 0.0f;
        for (unsigned int c = 0; c < FORMAT_CHANNELS; c++) {
            out[f * FORMAT_CHANNELS + c] += frame[c];
            frameSum += frame[c];
        }

        // Reverb
        if (revSlot.isEnabled && revSlot.buffers != nullptr && slot.reverbSend > NEAR_ZERO_EPSILON) {
            frameSum /= FORMAT_CHANNELS;
            revSlot.buffers->input[f] += (frameSum / FORMAT_CHANNELS) * slot.reverbSend;
        }

        // Advance read cursor and exit if instance finished
        if (advanceReadCursor(slot, totalPitch)) {
            return;
        }
    }

    checkRemainingSampleAvailability(context, slot);
}

static void processCmdsFromMain(AudioThreadContext* context) {
    AudioCmd cmd;
    bool markAllSpatialsAsDirty = false;
    while (rbReadElements<AudioCmd>(&context->shared->mainToAudioQueue, &cmd, 1) == 1) {
        switch (cmd.target) {
            case AudioCmdTarget::NoTarget: {
                switch (cmd.type) {
                    case AudioCmdType::SetListenerVelocity:
                        context->audio->listenerState.velocity = cmd.data.vec3Value;
                        markAllSpatialsAsDirty = true;
                        break;

                    case AudioCmdType::SetListenerPosition:
                        context->audio->listenerState.position = cmd.data.vec3Value;
                        markAllSpatialsAsDirty = true;
                        break;

                    case AudioCmdType::SetListenerForward:
                        context->audio->listenerState.forwardVec = cmd.data.vec3Value;
                        markAllSpatialsAsDirty = true;
                        break;

                    case AudioCmdType::SetListenerUp:
                        context->audio->listenerState.upVec = cmd.data.vec3Value;
                        markAllSpatialsAsDirty = true;
                        break;

                    case AudioCmdType::SetResolvedBusVolumes:
                        context->audio->resolvedBusVolumes = static_cast<float*>(cmd.data.singlePtr);
                        break;

                    default: lgr::lout.warn("Unhandled command on audio thread!"); break;
                }
                break;
            }

            case AudioCmdTarget::AudioInstanceTarget: {
                AudioInstanceSlot& slot = context->audio->audioInstSlots[cmd.instanceId];

                if (cmd.type != AudioCmdType::Play && !slot.isActive) {
                    continue;
                }

                switch (cmd.type) {
                    case AudioCmdType::Play:
                        slot = {};  // Reset

                        slot.isActive = true;
                        slot.isPlaying = true;
                        slot.isStreamed = cmd.data.singleBool;
                        slot.state = &context->audio->audioInstStates[cmd.instanceId];
                        *slot.state = {};  // Reset state

                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::Stop: slot.isActive = false; break;

                    case AudioCmdType::AssetLoaded:
                        slot.audioData = cmd.data.assetData.audioData;
                        slot.totalPcmFrames = cmd.data.assetData.totalPcmFrames;
                        break;

                    case AudioCmdType::SetPlaying: slot.isPlaying = cmd.data.singleBool; break;

                    case AudioCmdType::SkipToRequest: {
                        size_t newFrameIndex = static_cast<size_t>(slot.totalPcmFrames * cmd.data.skipTo.normalizedPos);
                        cmd.data.skipTo.newFrameIndex = newFrameIndex;

                        slot.isSkipRequested = slot.isStreamed;  // Only suspend streamed audio
                        slot.state->currentFrameIndexFractional = 0;
                        slot.state->currentFrameIndex = newFrameIndex;
                        slot.state->validPrebufferFrames = 0;
                        break;
                    }

                    case AudioCmdType::SkipByRequest: {
                        size_t deltaFrames = static_cast<size_t>(FORMAT_FRAME_RATE * cmd.data.skipBy.deltaSeconds);
                        size_t newFrameIndex = slot.state->currentFrameIndex + deltaFrames;

                        // Wrap around case
                        if (slot.isLooping) {
                            newFrameIndex %= slot.totalPcmFrames;
                        }
                        cmd.data.skipBy.newFrameIndex = newFrameIndex;

                        slot.isSkipRequested = slot.isStreamed;  // Only suspend streamed audio
                        slot.state->currentFrameIndexFractional = 0;
                        slot.state->currentFrameIndex = newFrameIndex;
                        slot.state->validPrebufferFrames = 0;
                        break;
                    }

                    case AudioCmdType::SkipResolved: slot.isSkipRequested = false; break;

                    case AudioCmdType::SetLooping: slot.isLooping = cmd.data.singleBool; break;

                    case AudioCmdType::SetVolume:
                        slot.volume = cmd.data.singleFloat;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetPitch:
                        slot.pitch = cmd.data.singleFloat;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetPan:
                        slot.pan = cmd.data.singleFloat;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetSpatialEnabled: slot.spatial.enabled = cmd.data.singleBool; break;

                    case AudioCmdType::SetSpatialPosition:
                        slot.spatial.position = cmd.data.vec3Value;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetSpatialVelocity:
                        slot.spatial.velocity = cmd.data.vec3Value;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetDopplerFactor:
                        slot.spatial.dopplerFactor = cmd.data.singleFloat;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetAttenuation:
                        slot.spatial.attenuation = cmd.data.attenuation;
                        slot.state->mix.dirty = true;
                        break;

                    case AudioCmdType::SetLowpassEnabled: {
                        slot.lowpassConf.enabled = cmd.data.singleBool;
                        for (LowpassState& lState : slot.state->lowpassStates) {
                            lState.yPrev = 0.0f;
                        }
                        break;
                    }

                    case AudioCmdType::SetLowpassCutoffHz: {
                        slot.lowpassConf.cutoffHz = cmd.data.singleFloat;
                        float alpha = computeAlphaLowpass(slot.lowpassConf);
                        for (LowpassState& lState : slot.state->lowpassStates) {
                            lState.alpha = alpha;
                        }
                        break;
                    }

                    case AudioCmdType::SetHighpassEnabled: {
                        slot.highpassConf.enabled = cmd.data.singleBool;
                        for (HighpassState& hState : slot.state->highpassStates) {
                            hState.xPrev = 0.0f;
                            hState.yPrev = 0.0f;
                        }
                        break;
                    }

                    case AudioCmdType::SetHighpassCutoffHz: {
                        slot.highpassConf.cutoffHz = cmd.data.singleFloat;
                        float alpha = computeAlphaHighpass(slot.highpassConf);
                        for (HighpassState& hState : slot.state->highpassStates) {
                            hState.alpha = alpha;
                        }
                        break;
                    }

                    case AudioCmdType::SetBus: slot.busId = cmd.data.singleUInt; break;

                    case AudioCmdType::SetReverbSend: slot.reverbSend = cmd.data.singleFloat; break;

                    case AudioCmdType::SetReverbId: slot.reverbId = cmd.data.singleUInt; break;

                    default: lgr::lout.warn("Unhandled command on audio thread!"); break;
                }
                break;
            }

            case AudioCmdTarget::ReverbInstanceTarget: {
                ReverbInstanceSlot& slot = context->audio->reverbInstSlots[cmd.instanceId];

                if (cmd.type != AudioCmdType::SetReverbInstanceBuffers && slot.buffers == nullptr) {
                    continue;
                }

                switch (cmd.type) {
                    case AudioCmdType::SetReverbInstanceEnabled: slot.isEnabled = cmd.data.singleBool; break;

                    case AudioCmdType::SetReverbInstanceParams:
                        slot.params = cmd.data.reverbParams;
                        slot.buffers->combFeedbacksDirty = true;
                        break;

                    case AudioCmdType::SetReverbInstanceBuffers:
                        slot.buffers = (ReverbBuffers*)cmd.data.singlePtr;
                        if (slot.buffers != nullptr) {
                            slot.buffers->combFeedbacksDirty = true;
                        }
                        break;

                    default: lgr::lout.warn("Unhandled command on audio thread!"); break;
                }
                break;
            }

            default: lgr::lout.warn("Unhandled command on audio thread!"); break;
        }

        if (cmd.needsAck) {  // Send back ones with flag set
            if (rbWriteElements<AudioCmd>(&context->shared->audioToMainQueue, &cmd, 1) != 1) {
                lgr::lout.error("Failed to send audio cmd ack back to main thread");
            }
        }
    }

    if (markAllSpatialsAsDirty) {
        for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
            AudioInstanceSlot& slot = context->audio->audioInstSlots[i];
            if (slot.isActive && slot.spatial.enabled) {
                slot.state->mix.dirty = true;
            }
        }
    }
}

// Callback is guaranteed to not be killed mid execution.
void audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioThreadContext* context = (AudioThreadContext*)pDevice->pUserData;
    float* out = (float*)pOutput;

    // Process commands
    processCmdsFromMain(context);

    // Mix audio
    for (unsigned int i = 0; i < AUDIO_INSTANCE_LIMIT; i++) {
        AudioInstanceSlot& slot = context->audio->audioInstSlots[i];
        if (!slot.isActive || !slot.isPlaying || !slot.audioData || slot.isSkipRequested) continue;

        // Recalc channel gains and pitch
        if (slot.state->mix.dirty) {
            slot.state->mix.totalPitch = caclulatePitch(slot, context->audio->listenerState);
            calculateChannelGains(slot, context->audio->listenerState, slot.state->mix.channelGains);
            slot.state->mix.dirty = false;
        }

        mixInstance(context, slot, out, frameCount);

        // Send back stop cmd if instance finished while mixing
        if (!slot.isActive) {
            AudioCmd stopCmd;
            stopCmd.type = AudioCmdType::Stop;
            stopCmd.target = AudioCmdTarget::AudioInstanceTarget;
            stopCmd.instanceId = i;
            if (rbWriteElements<AudioCmd>(&context->shared->audioToMainQueue, &stopCmd, 1) != 1) {
                lgr::lout.error("Failed to send audio stop cmd to main thread");
            }
        }
    }

    for (unsigned int i = 0; i < REVERB_INSTANCE_LIMIT; i++) {
        ReverbInstanceSlot& revSlot = context->audio->reverbInstSlots[i];
        if (!revSlot.isEnabled || revSlot.buffers == nullptr) continue;

        // Recalc comb feedbacks
        if (revSlot.buffers->combFeedbacksDirty) {
            calculateCombFeedbacks(revSlot);
            revSlot.buffers->combFeedbacksDirty = false;
        }

        processReverbInstance(revSlot, out, frameCount);
        std::memset(revSlot.buffers->input, 0, SAMPLE_BYTE_SIZE * FRAMES_PER_PERIOD);
    }
}