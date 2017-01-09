#include <stdint.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <AudioPolicyManager.h>

#ifndef ANDROID_AUDIO_POLICY_MANAGER_SPRD_H
#define ANDROID_AUDIO_POLICY_MANAGER_SPRD_H

namespace android {
class AudioPolicyManagerSPRD: public AudioPolicyManager
{
public:
    AudioPolicyManagerSPRD(AudioPolicyClientInterface *clientInterface);
    virtual ~AudioPolicyManagerSPRD() {}
    virtual status_t startOutput(audio_io_handle_t output,
                                 audio_stream_type_t stream,
                                 audio_session_t session);
    virtual status_t stopOutput(audio_io_handle_t output,
                                audio_stream_type_t stream,
                                audio_session_t session);
    virtual void releaseOutput(audio_io_handle_t output,
                               audio_stream_type_t stream,
                               audio_session_t session);

    virtual status_t setDeviceConnectionState(audio_devices_t device,
                                              audio_policy_dev_state_t state,
                                              const char *device_address);
    virtual void setSystemProperty(const char* property, const char* value);

private:
    bool isVoipSet;
    volatile bool mDone;
    pthread_t mThread;
    status_t startReadingThread();
    static void *ThreadWrapper(void *me);
    void threadFunc();

    void handleNotificationRoutingForStream(audio_stream_type_t stream);
    uint32_t nextUniqueId();
    static bool deviceDistinguishesOnAddress(audio_devices_t device);
    // return true if any output is playing anything besides the stream to ignore
    bool isAnyOutputActive(audio_stream_type_t streamToIgnore);
    // event is one of STARTING_OUTPUT, STARTING_BEACON, STOPPING_OUTPUT, STOPPING_BEACON
    // returns 0 if no mute/unmute event happened, the largest latency of the device where
    //   the mute/unmute happened
    uint32_t handleEventForBeacon(int event);
    uint32_t setBeaconMute(bool mute);
};

} //namespace android

#endif //ANDROID_AUDIO_POLICY_MANAGER_SPRD_H
