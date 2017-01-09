/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
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

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include "audio_debug.h"
#include "audio_hw.h"
#include "audio_control.h"
#define LOG_TAG "audio_hw_vbcdump"
#define VBC_MIXER_DUMP_FILE "/data/local/media/vbcmixer.pcm"

struct vbc_dump_mode_t {
    PCM_DUMP_TYPE type;
    const char *name;
};

static const struct vbc_dump_mode_t  vbc_dump_mode_table[] = {
    { VBC_DISABLE_DMUP,                "disable"                         },
    { VBC_DAC0_MIXER_DMUP,             "vbc_dac0_mixer_dump"             },
    { VBC_DAC0_A1_DMUP,                "vbc_dac0_a1_mixer_dump"          },
    { VBC_DAC0_A2_DMUP,                "vbc_dac0_a2_mixer_dump"          },
    { VBC_DAC0_A3_DMUP,                "vbc_dac0_a3_mixer_dump"          },
    { VBC_DAC0_A4_DMUP,                "vbc_dac0_a4_mixer_dump"          },
    { VBC_DAC1_MIXER_DMUP,             "vbc_dac1_mixer_dump"             },
    { VBC_DAC1_V1_DMUP,                "vbc_dac1_v1_mixer_dump"          },
    { VBC_DAC1_V2_DMUP,                "vbc_dac1_v2_mixer_dump"          },
    { VBC_DAC1_V3_DMUP,                "vbc_dac1_v3_mixer_dump"          },
    { VBC_DAC1_V4_DMUP,                "vbc_dac1_v4_mixer_dump"          },
};

static bool check_vbc_dump_enable(struct audio_control *ctl,PCM_DUMP_TYPE type){
    bool ret=false;
    pthread_mutex_lock(&ctl->lock);
    switch(type){
        case VBC_DAC0_MIXER_DMUP:
            if(    (0==(ctl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO)) &&
                    (is_usecase_unlock(ctl,UC_MM_RECORD|UC_FM_RECORD|UC_BT_RECORD|UC_BT_VOIP)==false)&&
                    (is_usecase_unlock(ctl, UC_VOIP|UC_CALL|UC_FM|UC_NORMAL_PLAYBACK|UC_DEEP_BUFFER_PLAYBACK|UC_OFFLOAD_PLAYBACK)==true)){
                ret=true;
             }
            break;

        case VBC_DAC0_A1_DMUP:
        case VBC_DAC0_A2_DMUP:
        case VBC_DAC0_A3_DMUP:
        case VBC_DAC0_A4_DMUP:
            if(    (is_usecase_unlock(ctl,UC_MM_RECORD|UC_FM_RECORD|UC_BT_RECORD|UC_BT_VOIP)==false)&&
                    (is_usecase_unlock(ctl, UC_FM|UC_NORMAL_PLAYBACK|UC_DEEP_BUFFER_PLAYBACK|UC_OFFLOAD_PLAYBACK)==true)){
                ret=true;
             }
            break;

        case VBC_DAC1_MIXER_DMUP:
            if(    (ctl->out_devices&AUDIO_DEVICE_OUT_ALL_SCO)&&
                    (is_usecase_unlock(ctl,UC_MM_RECORD|UC_FM_RECORD|UC_BT_RECORD)==false)&&
                    (is_usecase_unlock(ctl,UC_CALL|UC_BT_VOIP)==true)){
                ret=true;
            }
            break;

        case VBC_DAC1_V1_DMUP:
        case VBC_DAC1_V2_DMUP:
        case VBC_DAC1_V3_DMUP:
        case VBC_DAC1_V4_DMUP:
            if((is_usecase_unlock(ctl,UC_MM_RECORD|UC_FM_RECORD|UC_BT_RECORD)==false)&&
                (is_usecase_unlock(ctl,UC_CALL|UC_BT_VOIP|UC_VOIP)==true)){
                ret=true;
            }
            break;

        default:
            break;
    }
    pthread_mutex_unlock(&ctl->lock);
    LOG_I("check_vbc_dump_enable type:%d ret:%d",type,ret);
    return ret;
}
static void *vbc_pcm_dump_thread(void *args){
    struct audio_control *dev_ctl=(struct vbc_dump_ctl *)args;
    struct vbc_dump_ctl * vbc_dump=&dev_ctl->vbc_dump;
    struct pcm_config config;
    struct pcm *pcm;
    char *buffer;
    int size;
    int num_read;

    LOG_I("vbc_pcm_dump_thread enter");
    pthread_mutex_lock(&dev_ctl->lock);
    vbc_dump->is_exit=false;
    pthread_mutex_unlock(&dev_ctl->lock);
    set_usecase(dev_ctl,UC_VBC_PCM_DUMP, true);
    set_vbc_dump_control(dev_ctl,vbc_dump->dump_name,true);
    pcm = pcm_open(0, vbc_dump->pcm_devices, PCM_IN | PCM_MMAP | PCM_NOIRQ, &vbc_dump->config);
    if (!pcm || !pcm_is_ready(pcm)) {
        LOG_E("Unable to open PCM device 0 (%s)\n", pcm_get_error(pcm));
        goto out;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        LOG_E("Unable to allocate %d bytes\n", size);
        goto out;
    }

    while(vbc_dump->is_exit==false){
        if(!pcm_mmap_read(pcm, buffer, size)){
            if(vbc_dump->dump_fd>0){
                write(vbc_dump->dump_fd,buffer,size);
            }
        }else{
            pcm_close(pcm);
            set_vbc_dump_control(dev_ctl,vbc_dump->dump_name,true);
            pcm = pcm_open(0, vbc_dump->pcm_devices, PCM_IN | PCM_MMAP | PCM_NOIRQ, &vbc_dump->config);
            if (!pcm || !pcm_is_ready(pcm)) {
                LOG_E("Unable to open PCM device 0 (%s)\n", pcm_get_error(pcm));
                goto out;
            }
        }
    }
    LOG_I("vbc_pcm_dump_thread Exit");
out:
    if(buffer)
        free(buffer);

    if(pcm)
        pcm_close(pcm);

    if(vbc_dump->dump_fd)
        close(vbc_dump->dump_fd);

    pthread_mutex_lock(&dev_ctl->lock);
    vbc_dump->is_exit=true;
    pthread_mutex_unlock(&dev_ctl->lock);
    set_vbc_dump_control(dev_ctl,vbc_dump->dump_name,false);
    set_usecase(dev_ctl,UC_VBC_PCM_DUMP, false);
    return NULL;
}

static int init_vbcmixer_playback_dump(struct vbc_dump_ctl *vbc_dump,char *dump_file_name){
    char file_name[256]={0};
    struct   tm     *timenow;
    time_t current_time;
    time(&current_time);
    timenow = localtime(&current_time);

    snprintf(file_name,sizeof(file_name),"/data/local/media/vbc_dump_%s_%04d_%02d_%02d_%02d_%02d_%02d.pcm",
        dump_file_name,timenow->tm_year+1900,timenow->tm_mon+1,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
    vbc_dump->dump_fd=open(file_name, O_WRONLY | O_CREAT |O_TRUNC ,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(vbc_dump->dump_fd<0){
        vbc_dump->is_exit=true;
        LOG_E("init_vbcmixer_playback_dump create:%s failed",file_name);
        return -1;
    }

    memset(&vbc_dump->config,0,sizeof(struct pcm_config));
    vbc_dump->config.channels=2;
    vbc_dump->config.period_count=2;
    vbc_dump->config.period_size=960;
    vbc_dump->config.rate=48000;

    vbc_dump->pcm_devices=0;
    return 0;
}

int disable_vbc_playback_dump(struct audio_control *dev_ctl,struct vbc_dump_ctl * vbc_dump){

    if((vbc_dump->is_exit==false)
        && (is_usecase(dev_ctl,UC_VBC_PCM_DUMP))){
        vbc_dump->is_exit=true;
        vbc_dump->dump_fd=-1;
        vbc_dump->dump_name=NULL;
        pthread_join(vbc_dump->thread, NULL);
    }
    return 0;
}

int vbc_playback_dump(void *dev,struct str_parms *parms,int opt, char * val){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int ret=-1;
    int i=0;
    struct vbc_dump_ctl * vbc_dump=&adev->dev_ctl->vbc_dump;
    PCM_DUMP_TYPE type=VBC_INVALID_DMUP;
    const char *dump_name=NULL;
    pthread_mutex_lock(&adev->lock);

    for(i=0;i<sizeof(vbc_dump_mode_table)/sizeof(struct vbc_dump_mode_t);i++){
        if(strncmp(val,vbc_dump_mode_table[i].name,strlen(vbc_dump_mode_table[i].name))==0){
            type=vbc_dump_mode_table[i].type;
            dump_name=vbc_dump_mode_table[i].name;
            break;
        }
    }

    if(VBC_INVALID_DMUP!=type){
        if(VBC_DISABLE_DMUP==type){
            disable_vbc_playback_dump(adev->dev_ctl,vbc_dump);
            ret=0;
        }else{
           LOG_I("is_exit:%d dump_name:%s usecase:0x%x",vbc_dump->is_exit,dump_name,adev->dev_ctl->usecase);
            if((vbc_dump->is_exit==true) && (false==is_usecase(adev->dev_ctl,UC_VBC_PCM_DUMP))
                &&(check_vbc_dump_enable(adev->dev_ctl,type))
                &&(dump_name!=NULL)){
                vbc_dump->dump_name=dump_name;
                ret=init_vbcmixer_playback_dump(vbc_dump,dump_name);
                if((ret==0) && (vbc_dump->dump_name!=NULL)){
                    if(pthread_create(&vbc_dump->thread, NULL, vbc_pcm_dump_thread,adev->dev_ctl)) {
                        LOG_E("vbc_playback_dump creating thread failed !!!!");
                    }
                }
            }
        }
    }else{
        LOG_W("vbc_playback_dump:%s",val);
    }
    pthread_mutex_unlock(&adev->lock);
    return ret;
}
