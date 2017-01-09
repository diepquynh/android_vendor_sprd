#define LOG_TAG "AudioPolicyManagerSPRD new"
#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <hardware/audio.h>
#include "AudioPolicyManagerSPRD.h"
#include <media/mediarecorder.h>
#include <media/AudioParameter.h>
#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define APM_AUDIO_DEVICE_MATCH_ADDRESS_ALL (AUDIO_DEVICE_IN_REMOTE_SUBMIX | \
                                            AUDIO_DEVICE_OUT_REMOTE_SUBMIX)


namespace android {

extern "C" AudioPolicyInterface* createAudioPolicyManager(
        AudioPolicyClientInterface *clientInterface)
{
    return new AudioPolicyManagerSPRD(clientInterface);
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface)
{
    delete interface;
}

AudioPolicyManagerSPRD::AudioPolicyManagerSPRD(
        AudioPolicyClientInterface *clientInterface)
    :AudioPolicyManager(clientInterface), isVoipSet(false)
{
    char bootvalue[PROPERTY_VALUE_MAX];
    char status[PROPERTY_VALUE_MAX];
    // prop sys.boot_completed will set 1 when system ready (ActivityManagerService.java)...
    property_get("sys.boot_completed", bootvalue, "");
    if (strncmp("1", bootvalue, 1) != 0) {
      property_get("init.svc.autotest", status, NULL); // property from engpc when running BBAT mode ...
      if (strncmp("running", status, 7) == 0) {
        ALOGD("AudioPolicyManagerSPRD() autotest BBAT Mode is Running");
        startReadingThread();
      }
    }
}


status_t AudioPolicyManagerSPRD::startOutput(audio_io_handle_t output,
                                             audio_stream_type_t stream,
                                             audio_session_t session)
{
    ALOGV("startOutput() output %d, stream %d, session %d",
          output, stream, session);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("startOutput() unknown output %d", output);
        return BAD_VALUE;
    }

    sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);


    // Routing?
    mOutputRoutes.incRouteActivity(session);

    audio_devices_t newDevice;
    if (outputDesc->mPolicyMix != NULL) {
        newDevice = AUDIO_DEVICE_OUT_REMOTE_SUBMIX;
    } else if (mOutputRoutes.hasRouteChanged(session)) {
        newDevice = getNewOutputDevice(outputDesc, false /*fromCache*/);
        checkStrategyRoute(getStrategy(stream), output);
    } else {
        newDevice = AUDIO_DEVICE_NONE;
    }

    uint32_t delayMs = 0;

    status_t status = startSource(outputDesc, stream, newDevice, &delayMs);

    if (status != NO_ERROR) {
        mOutputRoutes.decRouteActivity(session);
        return status;
    }

    ALOGD("startOutput() isVoipSet %d,stream %d,", isVoipSet, stream);
    if((!isVoipSet)&&(stream == AUDIO_STREAM_VOICE_CALL)&&(mPrimaryOutput->mIoHandle == output)) {
        ALOGD("startOutput() outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] %d",outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL]);
        if(outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] == 1) {
            AudioParameter param;
            param.add(String8("sprd_voip_start"), String8("true"));
            mpClientInterface->setParameters(0, param.toString());
            isVoipSet = true;
        }
    }

    // Automatically enable the remote submix input when output is started on a re routing mix
    // of type MIX_TYPE_RECORDERS
    if (audio_is_remote_submix_device(newDevice) && outputDesc->mPolicyMix != NULL &&
            outputDesc->mPolicyMix->mMixType == MIX_TYPE_RECORDERS) {
            setDeviceConnectionState(AUDIO_DEVICE_IN_REMOTE_SUBMIX,
                    AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                    outputDesc->mPolicyMix->mRegistrationId,
                    "remote-submix");
    }

    if (delayMs != 0) {
        usleep(delayMs * 1000);
    }

    return status;
}

status_t AudioPolicyManagerSPRD::stopOutput(audio_io_handle_t output,
                                            audio_stream_type_t stream,
                                            audio_session_t session)
{
    ALOGV("stopOutput() output %d, stream %d, session %d", output, stream, session);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("stopOutput() unknown output %d", output);
        return BAD_VALUE;
    }

    sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);

    ALOGD("stopOutput() isVoipSet %d,stream %d,output size %d", isVoipSet, stream,mOutputs.size());
    if(isVoipSet &&(stream == AUDIO_STREAM_VOICE_CALL)&&(mPrimaryOutput->mIoHandle==output)) {
            ALOGD("stopOutput() outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] %d",outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL]);
            if(outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] == 1) {
                AudioParameter param;
                param.add(String8("sprd_voip_start"), String8("false"));
                mpClientInterface->setParameters(0, param.toString());
                isVoipSet = false;
            }
    }

    if (outputDesc->mRefCount[stream] == 1) {
        // Automatically disable the remote submix input when output is stopped on a
        // re routing mix of type MIX_TYPE_RECORDERS
        if (audio_is_remote_submix_device(outputDesc->mDevice) &&
                outputDesc->mPolicyMix != NULL &&
                outputDesc->mPolicyMix->mMixType == MIX_TYPE_RECORDERS) {
            setDeviceConnectionState(AUDIO_DEVICE_IN_REMOTE_SUBMIX,
                    AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
                    outputDesc->mPolicyMix->mRegistrationId,
                    "remote-submix");
        }
    }

    // Routing?
    bool forceDeviceUpdate = false;
    if (outputDesc->mRefCount[stream] > 0) {
        int activityCount = mOutputRoutes.decRouteActivity(session);
        forceDeviceUpdate = (mOutputRoutes.hasRoute(session) && (activityCount == 0));

        if (forceDeviceUpdate) {
            checkStrategyRoute(getStrategy(stream), AUDIO_IO_HANDLE_NONE);
        }
    }

    return stopSource(outputDesc, stream, forceDeviceUpdate);
}

void AudioPolicyManagerSPRD::releaseOutput(audio_io_handle_t output,
                                       audio_stream_type_t stream __unused,
                                       audio_session_t session __unused)
{
    ALOGV("releaseOutput() %d", output);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("releaseOutput() releasing unknown output %d", output);
        return;
    }

    if(isVoipSet && (mPrimaryOutput->mIoHandle == output)) {
        sp<SwAudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);
        if(outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] == 0) {
            AudioParameter param;
            param.add(String8("sprd_voip_start"), String8("false"));
            mpClientInterface->setParameters(0, param.toString());
            isVoipSet = false;
        }
    }

#ifdef AUDIO_POLICY_TEST
    int testIndex = testOutputIndex(output);
    if (testIndex != 0) {
        sp<AudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);
        if (outputDesc->isActive()) {
            mpClientInterface->closeOutput(output);
            removeOutput(output);
            mTestOutputs[testIndex] = 0;
        }
        return;
    }
#endif //AUDIO_POLICY_TEST

    // Routing
    mOutputRoutes.removeRoute(session);

    sp<SwAudioOutputDescriptor> desc = mOutputs.valueAt(index);
    if (desc->mFlags & AUDIO_OUTPUT_FLAG_DIRECT) {
        if (desc->mDirectOpenCount <= 0) {
            ALOGW("releaseOutput() invalid open count %d for output %d",
                                                              desc->mDirectOpenCount, output);
            return;
        }
        if (--desc->mDirectOpenCount == 0) {
            closeOutput(output);
            // If effects where present on the output, audioflinger moved them to the primary
            // output by default: move them back to the appropriate output.
            audio_io_handle_t dstOutput = getOutputForEffect();
            if (hasPrimaryOutput() && dstOutput != mPrimaryOutput->mIoHandle) {
                mpClientInterface->moveEffects(AUDIO_SESSION_OUTPUT_MIX,
                                               mPrimaryOutput->mIoHandle, dstOutput);
            }
            mpClientInterface->onAudioPortListUpdate();
        }
    }
}

status_t AudioPolicyManagerSPRD::setDeviceConnectionState(audio_devices_t device,
                                                          audio_policy_dev_state_t state,
                                                          const char *device_address,
                                                          const char *device_name)
{
    int ret;
    if (!audio_is_output_device(device) && !audio_is_input_device(device)) return BAD_VALUE;
    if (audio_is_output_device(device)) {
        //handle fm device connect state
        if(device == AUDIO_DEVICE_OUT_FM_HEADSET){
            AudioParameter param = AudioParameter();
            if(state == AUDIO_POLICY_DEVICE_STATE_AVAILABLE){
                if(false == isStreamActive(AUDIO_STREAM_FM, 0)){
                    mOutputs.getPrimaryOutput()->changeRefCount(AUDIO_STREAM_FM, 1);
                }
                param.addInt(String8("handleFm"), 1);
                ALOGV("FM device connected");
            }else {
                sp<SwAudioOutputDescriptor> outputDescriptor = mOutputs.getPrimaryOutput();
                outputDescriptor->changeRefCount(AUDIO_STREAM_FM, -1);
                if(outputDescriptor->mRefCount[AUDIO_STREAM_FM] == 0) {
                   outputDescriptor->mStopTime[AUDIO_STREAM_FM] = systemTime();
                }
                param.addInt(String8("handleFm"), 0);
                ALOGV("FM device un connected");
            }
            ALOGV("setDeviceConnectionState() setParameters handle_fm");
            mpClientInterface->setParameters(0, param.toString());
        }
    }
    ret = AudioPolicyManager::setDeviceConnectionState(
                    device, state, device_address, device_name);

    return ret;
}

void AudioPolicyManagerSPRD::setSystemProperty(const char* property, const char* value)
{
    ALOGV("setSystemProperty() property %s, value %s", property, value);
    if (strcmp(property, "ro.camera.sound.forced") == 0) {
        const StreamDescriptor& streamDesc = mStreams.valueFor(AUDIO_STREAM_ENFORCED_AUDIBLE);
        if (atoi(value)) {
            ALOGI("ENFORCED_AUDIBLE cannot be muted");
            mStreams.setCanBeMuted(AUDIO_STREAM_ENFORCED_AUDIBLE, false);
        } else {
            ALOGI("ENFORCED_AUDIBLE can be muted");
            mStreams.setCanBeMuted(AUDIO_STREAM_ENFORCED_AUDIBLE, true);
        }
    }
}

status_t AudioPolicyManagerSPRD::startReadingThread()
{
    ALOGV("startReadingThread");
    mDone = false;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);
    return OK;
}

// static
void *AudioPolicyManagerSPRD::ThreadWrapper(void *me) {
    ALOGV("ThreadWrapper %p", me);
    AudioPolicyManagerSPRD *mBase = static_cast<AudioPolicyManagerSPRD *>(me);
    mBase->threadFunc();
    return NULL;
}

void AudioPolicyManagerSPRD::threadFunc() {
    ALOGV("threadFunc in");
    int preValue = 0;
    audio_devices_t connectedDevice = AUDIO_DEVICE_NONE;
    // add for bug158794 start
    char bootvalue[PROPERTY_VALUE_MAX];
    while (!mDone) {
        property_get("sys.boot_completed", bootvalue, "");
        if (strncmp("1", bootvalue, 1) == 0) {
            mDone = true;
            break;
        }
    // add for bug158749 end
        char buf[12] = {'\0'};
        const char* headsetStatePath = "/sys/class/switch/h2w/state";
        int fd = open(headsetStatePath,O_RDONLY);
        if(fd < 0) {
            ALOGE("open failed %s ",strerror(errno));
        } else {
            ssize_t mBytesRead = read(fd,(char*)buf,12);
            close(fd);
            if(mBytesRead>0) {
                int value = atoi((char*)buf);
                if (value != preValue) {
                    preValue = value;
                    ALOGD("headsets type = %s",(char*)buf);
                    //
                    if (value == 1 || value == 2) {
                        audio_devices_t tmpdevice = (value == 1) ?
                            AUDIO_DEVICE_OUT_WIRED_HEADSET : AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
                        connectedDevice |= tmpdevice;
                        setDeviceConnectionState(tmpdevice, AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
                    } else if (value == 0) {
                        if (connectedDevice & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
                            connectedDevice &= ~AUDIO_DEVICE_OUT_WIRED_HEADSET;
                            setDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "");
                        }
                        if (connectedDevice & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
                            connectedDevice &= ~AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
                            setDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "");
                        }

                    } else {
                        usleep(100*1000);
                        continue;
                    }

                    audio_devices_t device = getNewOutputDevice(mPrimaryOutput, false);
                    checkAndSetVolume(AUDIO_STREAM_SYSTEM, mStreams[AUDIO_STREAM_SYSTEM].getVolumeIndex(device),
                            mPrimaryOutput,   device, 0,  false);
                    checkAndSetVolume(AUDIO_STREAM_ALARM, mStreams[AUDIO_STREAM_ALARM].getVolumeIndex(device),
                            mPrimaryOutput,   device, 0,  false);
                }
            }
        }
        usleep(100*1000);
    }
    ALOGV("threadFunc exit");
    // add for bug158794 start
    void *temp = NULL;
    pthread_exit(temp);
    // add for bug 158749 end
    return;
}

}
