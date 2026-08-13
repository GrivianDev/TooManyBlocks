#ifndef TOOMANYBLOCKS_AUDIOENGINE_H
#define TOOMANYBLOCKS_AUDIOENGINE_H

#include <atomic>
#include <glm/vec3.hpp>
#include <string>
#include <thread>
#include <vector>

struct AudioInstance {
    unsigned int id;
    unsigned int version;
};

struct AudioDevice {
    unsigned int id; // TODO replace by stable identifier instead of index (can change when unplugging)
    std::string name;
    bool isDefault;
};

enum class AttenuationModel {
    None,  // 2D sound
    Linear,
    Inverse,
    Exponential
};

struct Attenuation {
    AttenuationModel model = AttenuationModel::None;
    float minDistance = 1.0f;
    float maxDistance = 50.0f;
    float curveStrength = 1.0f;
    bool invert = false;
};

struct ReverbDesign {
    float baseRoomSizeMeters;
    std::vector<float> combMsDelays;

    std::vector<float> allPassMsDelays;
    std::vector<float> allPassBaseFeedbacks;
};

struct ReverbParams {
    float wet;
    float decaySeconds;
    float damping;
};

class AudioEngine {
public:
    static ReverbDesign getReverbDesignPreset();

private:
    struct AudioEngineData* m_data;
    std::thread m_streamingWorker;
    std::thread m_audioLoaderWorker;
    std::atomic<bool> m_stopThreads;

    void streamWorkerLoop();
    void audioLoadWorkerLoop();

    void processCmdsFromAudioThread();
    void resolveAndSendAssetData();
    void sendCommandsToAudioThread();
    bool _isValid(AudioInstance instance) const;

public:
    AudioEngine();
    AudioEngine(AudioEngine&) = delete;

    ~AudioEngine();

    // ---- Device management ----

    /**
     * Set the current output device by id. Allows hot swapping the device
     * even when audio is currently playing.
     * @param deviceId The id of the device.
     * @return true if succeeded to set output device.
     */
    bool setOutputDevice(unsigned int deviceId);
    /**
     * NOT IMPLEMENTED
     */
    bool setInputDevice(unsigned int deviceId);
    /**
     * Loads the list of current output (playback) and input (capture) devices available.
     * @return true if enumerating devices was a success.
     */
    bool loadDevices();
    /**
     * Returns a list of available ouput devices, derived from the last AudioEngine::loadDevices() call.
     * @return List of output devices.
     * @note Output does not change until calling AudioEngine::loadDevices() again.
     */
    std::vector<AudioDevice> getOutputDevices() const;
    /**
     * Returns a list of available intput devices, derived from the last AudioEngine::loadDevices() call.
     * @return List of input devices.
     * @note Output does not change until calling AudioEngine::loadDevices() again.
     */
    std::vector<AudioDevice> getInputDevices() const;

    // ---- Payback control ----

    /**
     * Request play of a sound. Fully loads sound into memory, thus very performant but not viable for longer sound
     * files. This can be frequently called for things like small sound cues or effects.
     * Handles become invalid if the sound finished playing and is not set to loop.
     * @return Audio handle, may be invalid if audio creation failed. This can happen if too many sounds play at once.
     */
    AudioInstance playBuffered(const std::string& file);
    /**
     * Request play of a sound. Sets up a continuous stream of audio data.
     * Best for long sound files that would be too large to fully load.
     * Handles become invalid if the sound finished playing and is not set to loop.
     * @return Audio handle, may be invalid if audio creation failed. This can happen if too many sounds play at once.
     * @note Due to limitations there can only be a very limited number of streamed sounds at a time.
     */
    AudioInstance playStreamed(const std::string& file);
    /**
     * @return true if the handle is valid (The sound is active).
     */
    bool isValid(AudioInstance instance) const;
    /**
     * @return true if the sound is currently playing and not being paused.
     */
    bool isPlaying(AudioInstance instance) const;
    /**
     * Stops the sound if it is active currently. The handle becomes invalid.
     */
    void stop(AudioInstance instance);
    /**
     * Pauses the sound. Playback does not continue until explicitly resuming.
     */
    void pause(AudioInstance instance);
    /**
     * Resumes a paused sound.
     */
    void resume(AudioInstance instance);
    /**
     * Skips to a specific position in playback.
     * @param pos Normalized playback position [0, 1]
     */
    void skipTo(AudioInstance instance, float pos);
    /**
     * Relatively skips by the specified amount of seconds.
     * @param seconds The amount of seconds to skip by.
     */
    void skipBy(AudioInstance instance, float seconds);
    /**
     * Stops all active sounds and invalidates their handles.
     */
    void stopAll();
    /**
     * Pauses all active sounds.
     */
    void pauseAll();
    /**
     * Resumes all paused sounds.
     */
    void resumeAll();

    // ---- Instance properties ----

    /**
     * Sets volume of sound.
     * @param volume New base volume.
     */
    void setVolume(AudioInstance instance, float volume);
    /**
     * Sets base pitch of sound.
     * @param pitch Pitch in range of [0.25, 4.0].
     */
    void setPitch(AudioInstance instance, float pitch);
    /**
     * Sets the stereo panning.
     * @param pan panning value in range of [-1.0, 1.0].
     * Where -1.0 is fully left and 1.0 is fully right.
     * @note This parameter does not apply when spatialization enabled.
     */
    void setPan(AudioInstance instance, float pan);
    /**
     * Set wether the sound should seamlessly loop. Sound will not naturally finish if looping.
     * @param looping If it should loop.
     */
    void setLooping(AudioInstance instance, bool looping);
    /**
     * Set wether sound should be treated as a 3D audio placed in a world. By enabling sounds will
     * be effected by their position, velocity, doppler settings, attenuation settings and the listener state.
     * @param enabled If sound should be affected by spatial attributes.
     */
    void setSpatialization(AudioInstance instance, bool enabled);
    /**
     * Sets the 3D position, aka where the sound origin should be in space.
     * @param position Position in 3D space.
     */
    void set3DPosition(AudioInstance instance, const glm::vec3& position);
    /**
     * Sets the velocity of the sound source. This is used in doppler effect calculation for moving sound sources.
     * @param velocity Velocity vector of the sound.
     */
    void setVelocity(AudioInstance instance, const glm::vec3& velocity);
    /**
     * Sets the doppler factor. Is 1.0 by default and can be set to anything in the range of [0.0, 2.0].
     * 0.0 completely deactivates doppler effects and 2.0 doubles the effect.
     * @param dopplerFactor The strength of the doppler effect.
     */
    void setDoppler(AudioInstance instance, float dopplerFactor);
    /**
     * Sets the distance-based attenuation parameters for an audio instance.
     * @param instance The audio instance to configure.
     * @param attenuation Attenuation settings (model, min/max distance, curve strength, invert).
     * @note When setting invert to true, sounds will start to fade in at min distance and fully audible at max distance.
     * Basically sound gets quieter the closer the listener gets to the source.
     */
    void setAttenuation(AudioInstance instance, Attenuation attenuation);
    /**
     * Sets the mixing bus. Each sound per default is assign do bus 0.
     * Each a bus can have a parent from which it will inherit volume and multiplies it with its own.
     * @param busId Mixing bus id 0-31.
     */
    void setBus(AudioInstance instance, unsigned int busId);
    /**
     * Sets wether lowpass filtering is applied.
     * @param enabled If lowpass should be applied.
     */
    void setLowpassEnabled(AudioInstance instance, bool enabled);
    /**
     * Cutoff frequency for lowpass filtering. Frequencies above will be filtered out.
     * @param cutoffHz Cutoff frequency for filtering.
     */
    void setLowpassCutoffHz(AudioInstance instance, float cutoffHz);
    /**
     * Sets wether highpass filtering is applied.
     * @param enabled If highpass should be applied.
     */
    void setHighpassEnabled(AudioInstance instance, bool enabled);
    /**
     * Cutoff frequency for highpass filtering. Frequencies below will be filtered out.
     * @param cutoffHz Cutoff frequency for filtering.
     */
    void setHighpassCuroffHz(AudioInstance instance, float cutoffHz);
    /**
     * The amount of the output of the sound after mixing is send to reverb as input.
     * @param amount Amount in range [0.0, 1.0].
     */
    void setReverbSend(AudioInstance instance, float amount);
    /**
     * Define what reverb instance to use. Multiple sounds can use the same reverb instance if desired.
     * @param reverbId Reverb instance id 0-31.
     */
    void setReverb(AudioInstance instance, unsigned int reverbId);

    float getVolume(AudioInstance instance) const;
    float getPitch(AudioInstance instance) const;
    float getPan(AudioInstance instance) const;
    bool getLooping(AudioInstance instance) const;
    bool getSpatialization(AudioInstance instance) const;
    glm::vec3 get3DPosition(AudioInstance instance) const;
    glm::vec3 getVelocity(AudioInstance instance) const;
    float getDoppler(AudioInstance instance) const;
    Attenuation getAttenuation(AudioInstance instance) const;
    unsigned int getBus(AudioInstance instance) const;
    bool getLowpassEnabled(AudioInstance instance) const;
    float getLowpassCutoffHz(AudioInstance instance) const;
    bool getHighpassEnabled(AudioInstance instance) const;
    float getHighpassCutoffHz(AudioInstance instance) const;
    float getReverbSend(AudioInstance instance) const;
    unsigned int getReverb(AudioInstance instance) const;

    // ---- Bus controls ----

    /**
     * Set the volume of a specific bus if this bus is parent of others then this affects the total volume of the children too.
     * @param busId Mixing bus id 0-31.
     * @param volume New volume for mixing.
     */
    void setBusVolume(unsigned int busId, float volume);
    /**
     * Defines the parent of this mixing bus.
     * @param busId Mixing bus id 0-31.
     * @param parentId Index of parent mixing bus. Pass -1 if no parent. 
     */
    void setBusParent(unsigned int busId, int parentId);

    float getBusVolume(unsigned int busId) const;
    int getBusParent(unsigned int busId) const;

    // ---- Reverb ----

    /**
     * Allocates processing buffers for a reverb instance at this index. May overwrite existing ones.
     * @param reverbId Index of new reverb instance. Available ids are 0-31.
     * @param design Design of the comb filters and all passes.
     * @param roomSizeMeters Room size of the "room" this should represents. Will linearly scale the buffer suggestion from the design.
     * @note It is highly encourage to just use the design provided by AudioEngine::getReverbDesignPreset().
     */
    void createReverbInstance(unsigned int reverbId, ReverbDesign design, float roomSizeMeters);
    /**
     * Destroys the reverb buffers for this reverb instance.
     * @param reverbId Reverb instance 0-31.
     */
    void destroyReverbInstance(unsigned int reverbId);
    /**
     * Helper for destroying all reverb buffers at once.
     */
    void destroyAllReverbInstances();
    /**
     * Enables or disables a reverb instance. Disabled once wont ouput anything.
     * @param reverbId Reverb instance 0-31.
     * @param enabled If reverb instance is enabled
     */
    void setReverbInstanceEnabled(unsigned int reverbId, bool enabled);
    /**
     * Specifies the parameters used for reverb on a specific reverb instance.
     * @param reverbId Reverb instance 0-31.
     * @param params Reverb parameters. Consists of wet output factor, decay time in seconds and the amount of damping.
     */
    void setReverbInstanceParams(unsigned int reverbId, ReverbParams params);

    ReverbDesign getReverbInstanceDesign(unsigned int reverbId) const;
    bool getReverbInstanceEnabled(unsigned int reverbId) const;
    ReverbParams getReverbInstanceParams(unsigned int reverbId) const;

    // ---- Listener state ----

    /**
     * Sets the velocity of the listener. This affects doppler calculation if spatialization is enabled.
     * @param velocity Velocity of the listener.
     */
    void setListenerVelocity(const glm::vec3& velocity);
    /**
     * Sets the position of the listener. This influences attenuation effects if spatialization is enabled.
     * @param position Position of the listener.
     */
    void setListenerPosition(const glm::vec3& position);
    /**
     * Sets the forward vector for the listener. Affects panning for 3D sounds if spatialization is enabled.
     * @param forward Forward vector of the listener.
     */
    void setListenerForward(const glm::vec3& forward);
    /**
     * Sets the listener upward vector. Affects panning for 3D sounds if spatialization is enabled.
     * @param up Up vector of the listener.
     */
    void setListenerUp(const glm::vec3& up);

    glm::vec3 getListenerVelocity() const;
    glm::vec3 getListenerPosition() const;
    glm::vec3 getListenerForward() const;
    glm::vec3 getListenerUp() const;

    /**
     * Needs to repeatedly called to allow smooth communication with the audio thread.
     */
    void update(float deltaSeconds);

    AudioEngine& operator=(AudioEngine&) = delete;
};

#endif
