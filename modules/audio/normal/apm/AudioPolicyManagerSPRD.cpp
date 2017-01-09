/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AudioPolicyManagerSPRD"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include "AudioPolicyManagerSPRD.h"
#include <media/mediarecorder.h>


namespace android_audio_legacy {

using namespace android;

// ----------------------------------------------------------------------------
// AudioPolicyManagerSPRD
// ----------------------------------------------------------------------------

// ---  class factory

extern "C" AudioPolicyInterface* createAudioPolicyManager(AudioPolicyClientInterface *clientInterface)
{
    ALOGI("SPRD policy manager created.");
    return new AudioPolicyManagerSPRD(clientInterface);
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface)
{
    delete interface;
}

// Nothing currently different between the Base implementation.

AudioPolicyManagerSPRD::AudioPolicyManagerSPRD(AudioPolicyClientInterface *clientInterface)
    : AudioPolicyManagerBase(clientInterface),is_voip_set(false)
{
    //loadVolumeProfilesInternal();
}

AudioPolicyManagerSPRD::~AudioPolicyManagerSPRD()
{
    //freeVolumeProfiles();
}

status_t AudioPolicyManagerSPRD::startOutput(audio_io_handle_t output,
                                             AudioSystem::stream_type stream,
                                             int session)
{
    ALOGD("startOutput() output %d, stream %d, session %d", output, stream, session);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("startOutput() unknow output %d", output);
        return BAD_VALUE;
    }

    AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);

    // increment usage count for this stream on the requested output:
    // NOTE that the usage count is the same for duplicated output and hardware output which is
    // necessary for a correct control of hardware output routing by startOutput() and stopOutput()
    outputDesc->changeRefCount(stream, 1);
	ALOGD("startOutput() is_voip_set %d,stream %d,",is_voip_set,stream);
	if((!is_voip_set)&&(stream == AudioSystem::VOICE_CALL)&&(mPrimaryOutput == output)) {
			ALOGD("startOutput() outputDesc->mRefCount[AudioSystem::VOICE_CALL] %d",outputDesc->mRefCount[AudioSystem::VOICE_CALL]);
			if(outputDesc->mRefCount[AudioSystem::VOICE_CALL] == 1) {
                AudioParameter param;
                param.add(String8("sprd_voip_start"), String8("true"));
				mpClientInterface->setParameters(0, param.toString());
				is_voip_set = true;
			}
	}

    if (outputDesc->mRefCount[stream] == 1) {
        audio_devices_t newDevice = getNewDevice(output, false /*fromCache*/);
        routing_strategy strategy = getStrategy(stream);
        bool shouldWait = (strategy == STRATEGY_SONIFICATION) ||
                            (strategy == STRATEGY_SONIFICATION_RESPECTFUL);
        uint32_t waitMs = 0;
        bool force = false;
        for (size_t i = 0; i < mOutputs.size(); i++) {
            AudioOutputDescriptor *desc = mOutputs.valueAt(i);
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
        // filter devices according to output selected
        if(!outputDesc->isDuplicated())
        newDevice = (audio_devices_t)(newDevice & outputDesc->mProfile->mSupportedDevices);

        ALOGW("startOutput() select newDevice %d", newDevice);

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
                                            AudioSystem::stream_type stream,
                                            int session)
{
    ALOGD("stopOutput() output %d, stream %d, session %d", output, stream, session);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("stopOutput() unknow output %d", output);
        return BAD_VALUE;
    }

    AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);

    // handle special case for sonification while in call
    if (isInCall()) {
        handleIncallSonification(stream, false, false);
    }

	ALOGD("stopOutput() is_voip_set %d,stream %d,output size %d",is_voip_set,stream,mOutputs.size());
		if(is_voip_set &&(stream == AudioSystem::VOICE_CALL)&&(mPrimaryOutput == output)) {
				ALOGD("stopOutput() outputDesc->mRefCount[AudioSystem::VOICE_CALL] %d",outputDesc->mRefCount[AudioSystem::VOICE_CALL]);
				if(outputDesc->mRefCount[AudioSystem::VOICE_CALL] == 1) {
					AudioParameter param;
					param.add(String8("sprd_voip_start"), String8("false"));
					mpClientInterface->setParameters(0, param.toString());
					is_voip_set = false;
				}
		}

    if (outputDesc->mRefCount[stream] > 0) {
        // decrement usage count of this stream on the output
        outputDesc->changeRefCount(stream, -1);
        // store time at which the stream was stopped - see isStreamActive()
        if (outputDesc->mRefCount[stream] == 0) {
            outputDesc->mStopTime[stream] = systemTime();
            audio_devices_t newDevice = getNewDevice(output, false /*fromCache*/);
            // delay the device switch by twice the latency because stopOutput() is executed when
            // the track stop() command is received and at that time the audio track buffer can
            // still contain data that needs to be drained. The latency only covers the audio HAL
            // and kernel buffers. Also the latency does not always include additional delay in the
            // audio path (audio DSP, CODEC ...)
            setOutputDevice(output, newDevice, false, outputDesc->mLatency*2);

            // force restoring the device selection on other active outputs if it differs from the
            // one being selected for this output
            for (size_t i = 0; i < mOutputs.size(); i++) {
                audio_io_handle_t curOutput = mOutputs.keyAt(i);
                AudioOutputDescriptor *desc = mOutputs.valueAt(i);
                if (curOutput != output &&
                        desc->isActive() &&
                        outputDesc->sharesHwModuleWith(desc) &&
                        (newDevice != desc->device())) {
                    setOutputDevice(curOutput,
                                    getNewDevice(curOutput, false /*fromCache*/),
                                    true,
                                    outputDesc->mLatency*2);
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

void AudioPolicyManagerSPRD::releaseOutput(audio_io_handle_t output)
{
    ALOGD("releaseOutput() %d", output);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("releaseOutput() releasing unknown output %d", output);
        return;
    }

	if(is_voip_set&&(mPrimaryOutput == output)) {
	    AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);
		if(outputDesc->mRefCount[AudioSystem::VOICE_CALL] == 0) {
			AudioParameter param;
			param.add(String8("sprd_voip_start"), String8("false"));
			mpClientInterface->setParameters(0, param.toString());
			is_voip_set = false;
		}
	}

#ifdef AUDIO_POLICY_TEST
    int testIndex = testOutputIndex(output);
    if (testIndex != 0) {
        AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);
        if (outputDesc->isActive()) {
            mpClientInterface->closeOutput(output);
            delete mOutputs.valueAt(index);
            mOutputs.removeItem(output);
            mTestOutputs[testIndex] = 0;
        }
        return;
    }
#endif //AUDIO_POLICY_TEST

    AudioOutputDescriptor *desc = mOutputs.valueAt(index);
    if (desc->mFlags & AudioSystem::OUTPUT_FLAG_DIRECT) {
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
        }
    }
}

void AudioPolicyManagerSPRD::handleNotificationRoutingForStream(AudioSystem::stream_type stream) {
    switch(stream) {
    case AudioSystem::MUSIC:
        checkOutputForStrategy(STRATEGY_SONIFICATION_RESPECTFUL);
        updateDevicesAndOutputs();
        break;
    default:
        break;
    }
}

}; // namespace android_audio_legacy
