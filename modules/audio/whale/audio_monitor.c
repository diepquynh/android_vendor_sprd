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

#include "audio_hw.h"
#include "audio_control.h"
#include <system/audio.h>
#include <hardware/audio.h>
#include "audio_debug.h"
#include <cutils/sockets.h>

#define LOG_TAG "audio_hw_monitor"
extern int audio_agdsp_reset(void * dev,struct str_parms *parms,bool is_start);
static char *mystrstr(char *s1 , char *s2)
{
  if(*s1==0)
  {
    if(*s2) return(char*)NULL;
    return (char*)s1;
  }
  while(*s1)
  {
    int i=0;
    while(1)
   {
      if(s2[i]==0) return s1;
      if(s2[i]!=s1[i]) break;
      i++;
    }
    s1++;
  }
  return (char*)NULL;
}

static void *modem_monitor_routine(void *arg)
{
    int fd = -1;
    int numRead = 0;
    int buf[128] = {0};
    int try_count=10;

    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    LOG_D("modem_monitor_routine in");
    if( !adev) {
        LOG_E("modem_monitor_routine:error adev is null");
        return NULL;
    }

reconnect:
    do {
        try_count--;
        fd = socket_local_client("modemd",
                                 ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
        if(fd < 0 ) {
            LOG_E("modem_monitor_routine:socket_local_client failed %d", errno);
            usleep(2000 * 1000);
        }
    } while((fd < 0) && (try_count > 0));

    if(fd<0){
        return NULL;
    }

    while(1) {
        memset (buf, 0 , sizeof(buf));
        do {
            numRead = read(fd, buf, sizeof(buf));
        } while(numRead < 0 && errno == EINTR);

        if(numRead <= 0) {
            LOG_E("modem_monitor: error: got to reconnect");
            goto reconnect;
        }
        LOG_I("modem_monitor: %s", buf);
        if(mystrstr(buf, "Assert") || mystrstr(buf, "Reset")
           || mystrstr(buf, "Blocked")) {
            LOG_E("modem_monitor assert:stop voice call start");
            adev->hw_device.set_mode(adev,AUDIO_MODE_NORMAL);
            //audio_agdsp_reset(adev,NULL,true);
            LOG_E("modem_monitor assert:stop voice call end");
        }
    }
    return NULL;
}

int modem_monitor_open(void *arg)
{
    int rc;
    pthread_t thread_id;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    if( !adev) {
        LOG_E("modem_monitor_open:error adev is null");
        return -1;
    }

    rc = pthread_create((pthread_t *) & (thread_id), NULL,
                        modem_monitor_routine, (void *)adev);
    if (rc) {
        LOG_E("modem_monitor_open,pthread_create failed, rc=0x%x", rc);
        return -2;
    }
    LOG_I("modem_monitor_open,pthread_create ok, rc=0x%x", rc);
    return 0;
}
