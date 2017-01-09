
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>


#include <pthread.h>
#include <stdlib.h>
#include "audio_server.h"


using namespace android;

int setDeviceConnectionState(int device, int state) {

   //return  AudioSystem::setDeviceConnectionState(static_cast <audio_devices_t>(device),static_cast <audio_policy_dev_state_t>(state),"");
    return 0;
}

int setForceUse(int usage, int config) {
   return  AudioSystem::setForceUse(static_cast <audio_policy_force_use_t>(usage), static_cast <audio_policy_forced_cfg_t>(config));
}
int setParameters(){
    String8 fm_volume("FM_Volume=11");
    return AudioSystem::setParameters(audio_io_handle_t(0),fm_volume);
}
int setVolume(int volume){
    char *buf=NULL;
    asprintf(&buf,"FM_Volume=%d",volume);
    String8 fm_volume(buf);
    free(buf);
    return AudioSystem::setParameters(audio_io_handle_t(0),fm_volume);

}

