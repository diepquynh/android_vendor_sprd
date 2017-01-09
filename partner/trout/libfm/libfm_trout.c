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

#include <sys/ioctl.h>
#include <sys/types.h>
#include "fm.h"


// ioctl command
#define FM_IOCTL_BASE     'R'
#define FM_IOCTL_ENABLE      _IOW(FM_IOCTL_BASE, 0, int)
#define FM_IOCTL_GET_ENABLE  _IOW(FM_IOCTL_BASE, 1, int)
#define FM_IOCTL_SET_TUNE    _IOW(FM_IOCTL_BASE, 2, int)
#define FM_IOCTL_GET_FREQ    _IOW(FM_IOCTL_BASE, 3, int)
#define FM_IOCTL_SEARCH      _IOW(FM_IOCTL_BASE, 4, int[4])
#define FM_IOCTL_STOP_SEARCH _IOW(FM_IOCTL_BASE, 5, int)
#define FM_IOCTL_SET_VOLUME  _IOW(FM_IOCTL_BASE, 7, int)
#define FM_IOCTL_GET_VOLUME  _IOW(FM_IOCTL_BASE, 8, int)
#define FM_IOCTL_CONFIG      _IOW(FM_IOCTL_BASE, 9, int)
#define FM_IOCTL_GET_RSSI    _IOW(FM_IOCTL_BASE, 10,int)

// operation result
#define FM_SUCCESS 0
#define FM_FAILURE -1
#define FM_TIMEOUT 1

// search direction
#define SEARCH_DOWN 0
#define SEARCH_UP   1

// operation command
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)
#define V4L2_CID_FM_CONFIG              (V4L2_CID_BASE + 10)

struct sr2315_device_t
{
	struct fm_device_t dev;
	int fd;
};

static int
getFreq(struct fm_device_t* dev,int* freq)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
    return ioctl(device->fd,FM_IOCTL_GET_FREQ,freq);
}

static int 
getRssi(struct fm_device_t* dev,int* rssi)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
   
    if((NULL == device) || (device->fd < 0))
    {
         return FM_FAILURE;
    }
    return ioctl(device->fd,FM_IOCTL_GET_RSSI,rssi);
}

/*native interface */
static int
setFreq(struct fm_device_t* dev,int freq)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
    return ioctl(device->fd, FM_IOCTL_SET_TUNE, &freq);
}

static int gVolume = 0;

static int
setControl(struct fm_device_t* dev,int id, int value)
{
    int err = -1;
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
    int fd = device->fd;
	
    switch(id) {
        case V4L2_CID_PRIVATE_TAVARUA_STATE:
        {
            err = ioctl(fd, FM_IOCTL_ENABLE, &value);
        }
        break;

        case V4L2_CID_AUDIO_VOLUME:
        {
            gVolume = value;
            err = ioctl(fd, FM_IOCTL_SET_VOLUME, &value);
        }
        break;

        case V4L2_CID_AUDIO_MUTE:
        {
            if (value == 0) {
                err = ioctl(fd, FM_IOCTL_SET_VOLUME, &gVolume);
            } else {
                int volume = -1;
                err = ioctl(fd, FM_IOCTL_GET_VOLUME, &volume);
                if (err >= 0) {
                    gVolume = volume;
                    volume = 0;
                    err = ioctl(fd, FM_IOCTL_SET_VOLUME, &volume);
                }
            }
        }
        break;

        case V4L2_CID_FM_CONFIG:
        {
            err = ioctl(fd, FM_IOCTL_CONFIG, &value);
        }
        break;
	default:
	    ALOGE("peter: error default");
    }

    ALOGE("(setControl)operation=%x value=%d result=%d errno=%d", id, value, err, errno);
    if (err < 0) {
        return FM_FAILURE;
    }

    return FM_SUCCESS;
}

/* native interface */
static int
startSearch(struct fm_device_t* dev,int freq, int dir, int timeout, int reserve)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
    int buffer[4] = {0};
    buffer[0] = freq;    //start frequency
    buffer[1] = dir;     //search direction
    buffer[2] = timeout; //timeout
    buffer[3] = 0;       //reserve
    int err = -1;
    int count = 0;
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
    int fd = device->fd;

    err = ioctl(fd, FM_IOCTL_SEARCH, buffer);    
    ALOGE("err=%d\n", err);
    ALOGE("(seek)freq=%d direction=%d timeout=%d reserve=%d result=%d errno=%d\n", freq, dir, timeout, reserve, err, errno);
    if(err < 0){
        return FM_FAILURE;
    }
    if (err == FM_TIMEOUT) {
        return FM_TIMEOUT;
    }
    return FM_SUCCESS;
}

static int
cancelSearch(struct fm_device_t* dev)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if((NULL == device) || (device->fd < 0))
    {
         return FM_FAILURE;
    }
	
    return ioctl(device->fd, FM_IOCTL_STOP_SEARCH);
}

static int
close_fm(struct hw_device_t *dev)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if(NULL != device)
    {
	if (device->fd >= 0)
        {
            close(device->fd);
	}
        
        free(device);    
        device = NULL;    
    }
    return 0;
}



static int open_fm(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int fd;

    struct sr2315_device_t *sr2315_dev =(struct sr2315_device_t *) malloc(sizeof(struct sr2315_device_t));
    
    memset(sr2315_dev, 0, sizeof(*sr2315_dev));

    sr2315_dev->dev.common.tag = HARDWARE_DEVICE_TAG;
    sr2315_dev->dev.common.version = 0;
    sr2315_dev->dev.common.module = (struct hw_module_t*)module;
    sr2315_dev->dev.common.close = close_fm;
    sr2315_dev->dev.setFreq = setFreq;
    sr2315_dev->dev.getFreq = getFreq;
    sr2315_dev->dev.setControl = setControl;
    sr2315_dev->dev.startSearch = startSearch;
    sr2315_dev->dev.cancelSearch = cancelSearch;
    sr2315_dev->dev.getRssi = getRssi;
    fd = open("/dev/Trout_FM", O_RDONLY);

    if(fd <0)
    {
        free(sr2315_dev);
        sr2315_dev = NULL; 
        return FM_FAILURE;
    }
    sr2315_dev->fd = fd;
    *device = &(sr2315_dev->dev.common);
    return 0;
}


static struct hw_module_methods_t fm_module_methods = {
    .open =  open_fm,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = HARDWARE_MODULE_API_VERSION(0, 1),
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = FM_HARDWARE_MODULE_ID,
    .name = "troutfm",
    .author = "Spreadtrum, Inc.",
    .methods = &fm_module_methods,
};

