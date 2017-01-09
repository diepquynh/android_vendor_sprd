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
                                              const char *device_address,
                                              const char *device_name);
    virtual void setSystemProperty(const char* property, const char* value);

private:
    bool isVoipSet;
    volatile bool mDone;
    pthread_t mThread;
    status_t startReadingThread();
    static void *ThreadWrapper(void *me);
    void threadFunc();

};

} //namespace android

#endif //ANDROID_AUDIO_POLICY_MANAGER_SPRD_H
