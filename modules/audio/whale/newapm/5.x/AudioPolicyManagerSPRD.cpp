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
namespace android
{

    extern "C" AudioPolicyInterface *createAudioPolicyManager(
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
        : AudioPolicyManager(clientInterface), isVoipSet(false)
    {
        char bootvalue[PROPERTY_VALUE_MAX];
        // prop sys.boot_completed will set 1 when system ready (ActivityManagerService.java)...
        property_get("sys.boot_completed", bootvalue, "");
        ALOGD("%s %d",__func__,__LINE__);
    }

    uint32_t AudioPolicyManagerSPRD::nextUniqueId()
    {
        return android_atomic_inc(&mNextUniqueId);
    }

    bool AudioPolicyManagerSPRD::deviceDistinguishesOnAddress(
        audio_devices_t device)
    {
        return ((device & APM_AUDIO_DEVICE_MATCH_ADDRESS_ALL) != 0);
    }

    status_t AudioPolicyManagerSPRD::startOutput(audio_io_handle_t output,
            audio_stream_type_t stream,
            audio_session_t session)
    {
        ALOGD("startOutput() output %d, stream %d, session %d", output, stream,
              session);
        ssize_t index = mOutputs.indexOfKey(output);
        if (index < 0) {
            ALOGW("startOutput() unknown output %d", output);
            return BAD_VALUE;
        }

        sp<AudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);

        // increment usage count for this stream on the requested output:
        // NOTE that the usage count is the same for duplicated output and hardware output which is
        // necessary for a correct control of hardware output routing by startOutput() and stopOutput()
        outputDesc->changeRefCount(stream, 1);
        ALOGD("startOutput() isVoipSet %d,stream %d,", isVoipSet, stream);
        if((!isVoipSet) && (stream == AUDIO_STREAM_VOICE_CALL)) {
            for (size_t i = 0; i < mOutputs.size(); i++) {
                sp<AudioOutputDescriptor> outputDesc = mOutputs.valueAt(i);
                ALOGD("startOutput() outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] %d",
                      outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL]);
                if(outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] == 1) {
                    AudioParameter param;
                    param.add(String8("sprd_voip_start"), String8("true"));
                    mpClientInterface->setParameters(0, param.toString());
                    isVoipSet = true;
                    ALOGD("%s %d",__func__,__LINE__);
                }
            }
        }


        if (outputDesc->mRefCount[stream] == 1) {
            audio_devices_t newDevice = getNewOutputDevice(output, false /*fromCache*/);
            routing_strategy strategy = getStrategy(stream);
            bool shouldWait = (strategy == STRATEGY_SONIFICATION) ||
                              (strategy == STRATEGY_SONIFICATION_RESPECTFUL);
            uint32_t waitMs = 0;
            bool force = false;
            for (size_t i = 0; i < mOutputs.size(); i++) {
                sp<AudioOutputDescriptor> desc = mOutputs.valueAt(i);
                if (desc != outputDesc) {
                    // force a device change if any other output is managed by the same hw
                    // module and has a current device selection that differs from selected device.
                    // In this case, the audio HAL must receive the new device selection so that it can
                    // change the device currently selected by the other active output.
                    if (outputDesc->sharesHwModuleWith(desc) &&
                        desc->device() != newDevice) {
                        force = true;
                    }
                    // wait for audio on other active outputs to be presented when starting
                    // a notification so that audio focus effect can propagate.
                    uint32_t latency = desc->latency();
                    if (shouldWait && desc->isActive(latency * 2) && (waitMs < latency)) {
                        waitMs = latency;
                    }
                }
            }
            uint32_t muteWaitMs = setOutputDevice(output, newDevice, force);

            // handle special case for sonification while in call
            if (isInCall()) {
                handleIncallSonification(stream, true, false);
            }

            // apply volume rules for current stream and device if necessary
            checkAndSetVolume(stream,
                              mStreams[stream].getVolumeIndex(newDevice),
                              output,
                              newDevice);

            // update the outputs if starting an output with a stream that can affect notification
            // routing
            handleNotificationRoutingForStream(stream);
            if (waitMs > muteWaitMs) {
                usleep((waitMs - muteWaitMs) * 2 * 1000);
            }
        }
        return NO_ERROR;
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

        sp<AudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);

        // handle special case for sonification while in call
        if (isInCall()) {
            handleIncallSonification(stream, false, false);
        }

        ALOGD("stopOutput() isVoipSet %d,stream %d,output size %d", isVoipSet, stream,
              mOutputs.size());
        if(isVoipSet && (stream == AUDIO_STREAM_VOICE_CALL)) {
            for (size_t i = 0; i < mOutputs.size(); i++) {
                sp<AudioOutputDescriptor> outputDesc = mOutputs.valueAt(i);
                ALOGD("stopOutput() outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] %d",
                      outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL]);
                if(outputDesc->mRefCount[AUDIO_STREAM_VOICE_CALL] == 1) {
                    AudioParameter param;
                    param.add(String8("sprd_voip_start"), String8("false"));
                    mpClientInterface->setParameters(0, param.toString());
                    isVoipSet = false;
                }
            }
        }



        if (outputDesc->mRefCount[stream] > 0) {
            // decrement usage count of this stream on the output
            outputDesc->changeRefCount(stream, -1);
            // store time at which the stream was stopped - see isStreamActive()
            if (outputDesc->mRefCount[stream] == 0) {
                outputDesc->mStopTime[stream] = systemTime();
                audio_devices_t newDevice = getNewOutputDevice(output, false /*fromCache*/);
                // delay the device switch by twice the latency because stopOutput() is executed when
                // the track stop() command is received and at that time the audio track buffer can
                // still contain data that needs to be drained. The latency only covers the audio HAL
                // and kernel buffers. Also the latency does not always include additional delay in the
                // audio path (audio DSP, CODEC ...)
                setOutputDevice(output, newDevice, false, outputDesc->mLatency * 2);

                // force restoring the device selection on other active outputs if it differs from the
                // one being selected for this output
                for (size_t i = 0; i < mOutputs.size(); i++) {
                    audio_io_handle_t curOutput = mOutputs.keyAt(i);
                    sp<AudioOutputDescriptor> desc = mOutputs.valueAt(i);
                    if (curOutput != output &&
                        desc->isActive() &&
                        outputDesc->sharesHwModuleWith(desc) &&
                        (newDevice != desc->device())) {
                        setOutputDevice(curOutput,
                                        getNewOutputDevice(curOutput, false /*fromCache*/),
                                        true,
                                        outputDesc->mLatency * 2);
                    }
                }
                // update the outputs if stopping one with a stream that can affect notification routing
                handleNotificationRoutingForStream(stream);
            }
            return NO_ERROR;
        } else {
            ALOGW("stopOutput() refcount is already 0 for output %d", output);
            return INVALID_OPERATION;
        }
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

    if(isVoipSet&&(mPrimaryOutput == output)) {
        sp<AudioOutputDescriptor> outputDesc = mOutputs.valueAt(index);
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
            mOutputs.removeItem(output);
            mTestOutputs[testIndex] = 0;
        }
        return;
    }
#endif //AUDIO_POLICY_TEST

    sp<AudioOutputDescriptor> desc = mOutputs.valueAt(index);
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
            if (dstOutput != mPrimaryOutput) {
                mpClientInterface->moveEffects(AUDIO_SESSION_OUTPUT_MIX, mPrimaryOutput, dstOutput);
            }
            mpClientInterface->onAudioPortListUpdate();
        }
    }
}




    status_t AudioPolicyManagerSPRD::setDeviceConnectionState(
        audio_devices_t device,
        audio_policy_dev_state_t state,
        const char *device_address)
    {
        int ret;
        if (!audio_is_output_device(device) && !audio_is_input_device(device)) { return BAD_VALUE; }
        if (audio_is_output_device(device)) {
            //handle fm device connect state
            if(device == AUDIO_DEVICE_OUT_FM_HEADSET) {
                AudioParameter param = AudioParameter();
                if(state == AUDIO_POLICY_DEVICE_STATE_AVAILABLE) {
                    if(false == isStreamActive(AUDIO_STREAM_FM,
                                               SONIFICATION_RESPECTFUL_AFTER_MUSIC_DELAY)) {
                        mOutputs.valueFor(mPrimaryOutput)->changeRefCount(AUDIO_STREAM_FM, 1);
                    }
                    param.addInt(String8("handleFm"), 1);
                    ALOGV("FM device connected");
                } else {
                    mOutputs.valueFor(mPrimaryOutput)->changeRefCount(AUDIO_STREAM_FM, -1);
                    param.addInt(String8("handleFm"), 0);
                    ALOGV("FM device un connected");
                }
                ALOGV("setDeviceConnectionState() setParameters handle_fm");
                mpClientInterface->setParameters(0, param.toString());
            }
        }
        ret = AudioPolicyManager::setDeviceConnectionState(
                  device, state, device_address);

        return ret;
    }

    void AudioPolicyManagerSPRD::setSystemProperty(const char *property,
            const char *value)
    {
        ALOGV("setSystemProperty() property %s, value %s", property, value);
        if (strcmp(property, "ro.camera.sound.forced") == 0) {
            if (atoi(value)) {
                ALOGI("ENFORCED_AUDIBLE cannot be muted");
                mStreams[AUDIO_STREAM_ENFORCED_AUDIBLE].mCanBeMuted = false;
            } else {
                ALOGI("ENFORCED_AUDIBLE can be muted");
                mStreams[AUDIO_STREAM_ENFORCED_AUDIBLE].mCanBeMuted = true;
            }
        }

    }

    void AudioPolicyManagerSPRD::handleNotificationRoutingForStream(
        audio_stream_type_t stream)
    {
        switch(stream) {
        case AUDIO_STREAM_MUSIC:
            checkOutputForStrategy(STRATEGY_SONIFICATION_RESPECTFUL);
            updateDevicesAndOutputs();
            break;
        default:
            break;
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
    void *AudioPolicyManagerSPRD::ThreadWrapper(void *me)
    {
        ALOGV("ThreadWrapper %p", me);
        AudioPolicyManagerSPRD *mBase = static_cast<AudioPolicyManagerSPRD *>(me);
        mBase->threadFunc();
        return NULL;
    }

    void AudioPolicyManagerSPRD::threadFunc()
    {
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
            const char *headsetStatePath = "/sys/class/switch/h2w/state";
            int fd = open(headsetStatePath, O_RDONLY);
            if(fd < 0) {
                ALOGE("open failed %s ", strerror(errno));
            } else {
                ssize_t mBytesRead = read(fd, (char *)buf, 12);
                close(fd);
                if(mBytesRead > 0) {
                    int value = atoi((char *)buf);
                    if (value != preValue) {
                        preValue = value;
                        ALOGD("headsets type = %s", (char *)buf);
                        //
                        if (value == 1 || value == 2) {
                            audio_devices_t tmpdevice = (value == 1) ?
                                                        AUDIO_DEVICE_OUT_WIRED_HEADSET : AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
                            connectedDevice |= tmpdevice;
                            setDeviceConnectionState(tmpdevice, AUDIO_POLICY_DEVICE_STATE_AVAILABLE, NULL);
                        } else if (value == 0) {
                            if (connectedDevice & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
                                connectedDevice &= ~AUDIO_DEVICE_OUT_WIRED_HEADSET;
                                setDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADSET,
                                                         AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, NULL);
                            }
                            if (connectedDevice & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
                                connectedDevice &= ~AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
                                setDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                                         AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, NULL);
                            }

                        } else {
                            usleep(100 * 1000);
                            continue;
                        }

                        audio_devices_t device = getNewOutputDevice(mPrimaryOutput, false);
                        checkAndSetVolume(AUDIO_STREAM_SYSTEM,
                                          mStreams[AUDIO_STREAM_SYSTEM].getVolumeIndex(device),
                                          mPrimaryOutput,   device, 0,  false);
                        checkAndSetVolume(AUDIO_STREAM_ALARM,
                                          mStreams[AUDIO_STREAM_ALARM].getVolumeIndex(device),
                                          mPrimaryOutput,   device, 0,  false);
                    }
                }
            }
            usleep(100 * 1000);
        }
        ALOGV("threadFunc exit");
        // add for bug158794 start
        void *temp = NULL;
        pthread_exit(temp);
        // add for bug 158749 end
        return;
    }





} //end namespace android
