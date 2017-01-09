/*
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of Code Aurora nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "fmradio"

#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cutils/properties.h>
#include <sys/time.h>

#include <btl_if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/fm.h>
#include <CMutex.h>
#include <com_broadcom_bt_service_fm_FmReceiverService_int.h>
#define     DEBUG_VALUE 0
#define FM_SUCCESS 0
#define FM_FAILURE -1

// operation command
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)

struct bcm4330_fm_device_t
{
	struct fm_device_t dev;
};

bool enableNative(int functionalityMask);
bool disableNative(boolean bForcing);
bool searchAbortNative();
bool searchNative(int scanMode, int rssiThreshold, int condVal, int condType);
bool muteNative(boolean toggle);
bool setFMVolumeNative(int volume);

static int FUNC_REGION_NA = 0x00;
static int FUNC_REGION_EUR = 0x01;
static int FUNC_REGION_JP = 0x02;
static int FUNC_REGION_JP_II = 0x03;

static int gVolume = 0;
static int volumeTbl[16] = {0, 1, 2, 4, 8, 16, 32, 48, 64, 80, 108, 128, 150, 180, 210, 255};

/* native interface */
const int APP_SCAN_MODE_DOWN = 0x00;
const int APP_SCAN_MODE_UP = 0x01;

const int SCAN_MODE_DOWN = 0x00;
const int SCAN_MODE_UP = 0x80;

extern tBTA_FM gfm_params;
extern CMutex gMutex;
extern tBTLIF_CTRL_MSG_ID gCurrentEventID;
extern tBTA_FM_CTX gfm_context;

bool tuneNative(int freq) ;

static int
getFreq(struct fm_device_t* dev,int* freq)
{
    struct bcm4330_fm_device_t *device = (struct bcm4330_fm_device_t *)dev;
    LOGD("%s, %d\n", __FUNCTION__, gfm_params.chnl_info.freq );

    //*freq = gfm_params.chnl_info.freq/10;
    *freq = gfm_context.cur_freq/10;

    return FM_SUCCESS;
}

/*native interface */
static int
setFreq(struct fm_device_t* dev,int freq)
{
    struct bcm4330_fm_device_t *device = (struct bcm4330_fm_device_t *)dev;

    LOGD("%s, %d\n", __FUNCTION__, freq);

    bool ret =tuneNative(freq*10) ;

    if(!ret) return FM_FAILURE;

    if(gMutex.wait(0, 3))
	    return FM_FAILURE;

    if(gfm_params.chnl_info.status == BTA_FM_OK)
    {
	    return FM_SUCCESS;
    }else
    {
	    return FM_FAILURE;
    }
}



static int
setControl(struct fm_device_t* dev,int id, int value)
{
    int err = -1;
    struct bcm4330_fm_device_t *device = (struct bcm4330_fm_device_t *)dev;

    LOGD("%s, %d\n", __FUNCTION__, id );

    switch(id) {
        case V4L2_CID_PRIVATE_TAVARUA_STATE:
        {
	          value ? enableNative(FUNC_REGION_NA): disableNative(false);

	          if(gMutex.wait(0, 3))
	              return FM_FAILURE;

           if(gfm_params.chnl_info.status == BTA_FM_OK)
           {
	             return FM_SUCCESS;
           }
           else
           {
	             return FM_FAILURE;
           }
        }
        break;

        case V4L2_CID_AUDIO_VOLUME:
        {
                  if( value < 0 )
                  {
                      gVolume = 0;
                  }
                  else if( value > 15 )
                  {
                      gVolume = 255;
                  }
                  else
                      gVolume = volumeTbl[value];

                  LOGD("Set Volume : %d, %d\n", value, gVolume);

	          setFMVolumeNative( gVolume );

	          if(gMutex.wait(0, 3))
	              return FM_FAILURE;

                 if(gfm_params.chnl_info.status == BTA_FM_OK)
                {
	                return FM_SUCCESS;
                }
                else
               {
	               return FM_FAILURE;
               }
        }
        break;

        case V4L2_CID_AUDIO_MUTE:
        {
	           muteNative(value);

                   if(gMutex.wait(0, 3))
	               return FM_FAILURE;

                if(gfm_params.chnl_info.status == BTA_FM_OK)
               {
	              return FM_SUCCESS;
                }
               else
               {
	             return FM_FAILURE;
               }
        }
        break;
    }
/*
    LOGE("(setControl)operation=%x value=%d result=%d errno=%d", id, value, err, errno);
    if (err < 0) {
	    return DEBUG_VALUE;
    }
*/
    return FM_SUCCESS;
}



static int
startSearch(struct fm_device_t* dev,int freq, int dir, int timeout, int reserve)
{
    struct bcm4330_fm_device_t *device = (struct bcm4330_fm_device_t *)dev;
    unsigned long long last_time, cur_time;
    timeval tv;
    unsigned   elapse_time = 0;
    LOGD("%s\n", __FUNCTION__);

    gettimeofday( &tv, NULL);
    last_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;

    if( tuneNative(freq*10) )
    {
        if( gMutex.wait(0, 2))
            return FM_FAILURE;
    }

    searchNative(dir == APP_SCAN_MODE_DOWN ? SCAN_MODE_DOWN : SCAN_MODE_UP,
		105/*rssi*/, 0/*rds enable*/, 0/* rds type*/);
    gettimeofday( &tv, NULL);
    cur_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;
    elapse_time = cur_time - last_time;

    LOGD("%s, Tag1 elapse time is %d ms\n", __FUNCTION__, elapse_time );

    if(gMutex.wait(0, 5-(elapse_time+500)/1000))
    {
        return FM_FAILURE;
    }

    gettimeofday( &tv, NULL);
    cur_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;
    elapse_time = cur_time - last_time;

    LOGD("%s, Tag2 elapse time is %d ms\n", __FUNCTION__, elapse_time );

    if(gfm_params.chnl_info.status == BTA_FM_OK)
    {
       return FM_SUCCESS;
    }
    else if( elapse_time < 4000 )
    {
        searchNative(dir == APP_SCAN_MODE_DOWN ? SCAN_MODE_DOWN : SCAN_MODE_UP,
		105/*rssi*/, 0/*rds enable*/, 0/* rds type*/);
        gettimeofday( &tv, NULL);
        cur_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;
        elapse_time = cur_time - last_time;

        LOGD("%s, Tag3 elapse time is %d ms\n", __FUNCTION__, elapse_time );

        if(gMutex.wait(0, 5-(elapse_time+500)/1000))
            return FM_FAILURE;

        if(gfm_params.chnl_info.status == BTA_FM_OK)
       {
           return FM_SUCCESS;
        }

        return FM_FAILURE;
    }

    return FM_FAILURE;
}

static int
cancelSearch(struct fm_device_t* dev)
{
    struct bcm4330_fm_device_t *device = (struct bcm4330_fm_device_t *)dev;
    unsigned long long last_time, cur_time;
    timeval tv;
    unsigned   elapse_time = 0;

    gettimeofday( &tv, NULL);
    last_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;
    LOGD("%s  ==>\n", __FUNCTION__);

    searchAbortNative();

    if(gMutex.wait(0, 3))
    {
        return FM_FAILURE;
    }

    if(gfm_params.chnl_info.status == BTA_FM_OK)
    {
       gettimeofday( &tv, NULL);
       cur_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;
       elapse_time = cur_time - last_time;

       LOGD("%s  <== Tag1, elapse time is %d ms\n", __FUNCTION__ , elapse_time);
       return FM_SUCCESS;
    }
    else
    {
       gettimeofday( &tv, NULL);
       cur_time = ( tv.tv_sec*1000*1000 + tv.tv_usec )/1000;
       elapse_time = cur_time - last_time;

       LOGD("%s  <== Tag2, elapse time is %d ms\n", __FUNCTION__ , elapse_time);
       return FM_FAILURE;
    }
    

}

static void enable_fm()
{
	  int sock;
	  char value = '1';
	  //sendto FmDaemon
	  struct sockaddr_in toAddr;
	  //recvfrom
	  struct sockaddr_in fromAddr;

	  char result[PROPERTY_VALUE_MAX];
	  int retry_count = 500;

	  sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	  if(sock < 0)
	  {
		  LOGE("Create socket failed.\r\n");
      return;
	  }

	  memset(&toAddr,0,sizeof(toAddr));
	  toAddr.sin_family=AF_INET;
	  toAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	  toAddr.sin_port = htons(4000);

	  if(sendto(sock,&value, 1, 0,(struct sockaddr*)&toAddr,sizeof(toAddr)) != 1)
	  {
		   LOGE("sendto() failed.\r\n");
		   close(sock);
		   return;
	  }

	  close(sock);

	  while( --retry_count )
	  {
	      property_get( "service.brcm.bt.fm.active",  result, "0");

	      LOGI("enable_fm : =%s\n", result);
        if (strcmp( result, "1") == 0) {
            break;
        }
        usleep( 200000 );
    }


	  return;
}

static void disable_fm()
{
	   int sock;
	   char value = '0';
	  //sendto FmDaemon
	  struct sockaddr_in toAddr;
	  //recvfrom
	  struct sockaddr_in fromAddr;

	  char result[PROPERTY_VALUE_MAX];
	  int retry_count = 500;

	  sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	  if(sock < 0)
	  {
		  LOGE("Create socket failed.\r\n");
      return;
	  }

	  memset(&toAddr,0,sizeof(toAddr));
	  toAddr.sin_family=AF_INET;
	  toAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	  toAddr.sin_port = htons(4000);

	  if(sendto(sock, &value, 1, 0,(struct sockaddr*)&toAddr,sizeof(toAddr)) != 1)
	  {
		   LOGE("sendto() failed.\r\n");
		   close(sock);
		   return;
	  }

	  close(sock);

	  while( --retry_count )
	  {
	      property_get( "service.brcm.bt.fm.active",  result, "0");

	      LOGI("disable_fm : =%s\n",  result );
        if (strcmp( result, "0") == 0) {
            break;
        }
        usleep( 200000 );
    }


	  return;
}


static int
close_fm(struct hw_device_t *dev)
{
    struct bcm4330_fm_device_t *device = (struct bcm4330_fm_device_t *)dev;
   // close(device->fd);
    LOGD("%s\n", __FUNCTION__);


    if (dev) {
        free(dev);
    }

    disable_fm();

    return 0;
}



static int open_fm(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{

    struct bcm4330_fm_device_t *brcm4330_fm_dev =(struct bcm4330_fm_device_t *) malloc(sizeof(struct bcm4330_fm_device_t));

    LOGD("%s\n", __FUNCTION__);

    enable_fm();


    memset(brcm4330_fm_dev, 0, sizeof(*brcm4330_fm_dev));

    brcm4330_fm_dev->dev.common.tag = HARDWARE_DEVICE_TAG;
    brcm4330_fm_dev->dev.common.version = 0;
    brcm4330_fm_dev->dev.common.module = (struct hw_module_t*)module;
    brcm4330_fm_dev->dev.common.close = close_fm;
    brcm4330_fm_dev->dev.setFreq = setFreq;
    brcm4330_fm_dev->dev.getFreq = getFreq;
    brcm4330_fm_dev->dev.setControl = setControl;
    brcm4330_fm_dev->dev.startSearch = startSearch;
    brcm4330_fm_dev->dev.cancelSearch = cancelSearch;
    *device = &(brcm4330_fm_dev->dev.common);
    return 0;
}

static struct hw_module_methods_t fm_module_methods ;

struct hw_module_t HAL_MODULE_INFO_SYM;


struct hw_module_wrapper_t {
     hw_module_t * module;

     hw_module_wrapper_t()
     {
         module = &HAL_MODULE_INFO_SYM;

         module->tag = HARDWARE_MODULE_TAG;
         module->version_major = 1;
         module->version_minor = 0;
         module->id = FM_HARDWARE_MODULE_ID;
         module->name = "fm";
         module->author = "Broadcom, Inc.";
         module->methods = &fm_module_methods;
         fm_module_methods.open = open_fm;
     }
}module_instance;


