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

#define LOG_TAG "sound_trigger_hw_primary"
/*#define LOG_NDEBUG 0*/

#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <system/sound_trigger.h>
#include <hardware/sound_trigger.h>
#include <tinyalsa/asoundlib.h>



static const struct sound_trigger_properties hw_properties = {
        "The Android Open Source Project", // implementor
        "Sound Trigger Sprd HAL", // description
        1, // version
        { 0xed7a7d60, 0xc65e, 0x11e3, 0x9be4, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // uuid
        1, // max_sound_models
        1, // max_key_phrases
        1, // max_users
        RECOGNITION_MODE_VOICE_TRIGGER, // recognition_modes
        false, // capture_transition
        0, // max_buffer_ms
        false, // concurrent_capture
        false, // trigger_in_event
        0 // power_consumption_mw
};

struct stub_sound_trigger_device {
    struct sound_trigger_hw_device device;
    sound_model_handle_t model_handle;
    recognition_callback_t recognition_callback;
    void *recognition_cookie;
    sound_model_callback_t sound_model_callback;
    void *sound_model_cookie;
    pthread_t callback_thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    int opened;
    struct mixer *mixer;
    struct mixer_ctl *ctl_mic;
    int st_fd  ;
    int dsp_state  ;
    unsigned char *firmware_data;
    unsigned int  firmware_size;
    unsigned char *firmware_config_data;
    unsigned int  firmware_config_size;
};

#define  SOUND_TRIGGER_DEV  "/dev/rt5512"
#define  STDEV_CTRL_MICBIAS2  "micbias2 power"
#define  STDEV_MIXER_DEV    0

typedef enum {
    SOUND_TRIGGER_NORMAL,
    SOUND_TRIGGER_EVENT_HAPPENED,
    SOUND_TRIGGER_STOPPING_CAPTURING,
    SOUND_TRIGGER_UPDATE_FIRMWARE,
}  sound_trigger_event_state;


typedef enum {
    STDEV_DSP_STOP,
    STDEV_DSP_START,
    STDEV_DSP_ONESHOT,
    STDEV_DSP_DMIC,
    STDEV_DSP_RELOAD,
    STDEV_USE_DMIC,
    STDEV_DSP_FAST_RELOAD,
    STDEV_DSP_SET_SENSITIVITY,
} sound_trigger_dsp_action ;

static int stdev_set_dsp(struct stub_sound_trigger_device *stdev,
                                sound_trigger_dsp_action val)
{
    int ret;
    char  val_to_string[2] =  "0" ;
    stdev->dsp_state = val;
    if(val == STDEV_DSP_FAST_RELOAD){
        ret = write(stdev->st_fd, (void*)stdev->firmware_data, stdev->firmware_size);
        if(ret == stdev->firmware_size){
            ALOGI("load dsp firmware success \n");
            ret = 0 ;
        }
    }
    else if (val == STDEV_DSP_SET_SENSITIVITY) {
        ret = write(stdev->st_fd, (void*)stdev->firmware_config_data, stdev->firmware_config_size);
        if(ret == stdev->firmware_config_size){
            ALOGI("set dsp sensibility  success \n");
            ret = 0 ;
        }

    }
    else {
            sprintf(val_to_string,"%d",val);
            ret = write(stdev->st_fd, val_to_string , 1);
            if (ret == 1) {
                ret =0 ;
            }
    }
    return ret ;
}


static void *callback_thread_loop(void *context)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)context;
    ALOGI("%s", __func__);
    unsigned char capture_val;
    prctl(PR_SET_NAME, (unsigned long)"sound trigger callback", 0, 0, 0);

    if (stdev->recognition_callback == NULL) {
        goto exit;
    }

    // read kernel voice trigger event
    do {
        ALOGI("%s neo1: soundtrigger thread sleep %d", __func__, capture_val);
        int rc = read(stdev->st_fd, &capture_val, 1);
        ALOGI("%s neo2: soundtrigger thread wake up %d", __func__, capture_val);
        pthread_mutex_lock(&stdev->lock);
        if (capture_val == SOUND_TRIGGER_EVENT_HAPPENED && stdev->recognition_callback != NULL) {
            char *data = (char *)calloc(1, sizeof(struct sound_trigger_phrase_recognition_event) + 1);
            struct sound_trigger_phrase_recognition_event *event =
                    (struct sound_trigger_phrase_recognition_event *)data;
            event->common.status = RECOGNITION_STATUS_SUCCESS;
            event->common.type = SOUND_MODEL_TYPE_KEYPHRASE;
            event->common.model = stdev->model_handle;
            event->num_phrases = 1;
            event->phrase_extras[0].recognition_modes = RECOGNITION_MODE_VOICE_TRIGGER;
            event->phrase_extras[0].confidence_level = 100;
            event->phrase_extras[0].num_levels = 1;
            event->phrase_extras[0].levels[0].level = 100;
            event->phrase_extras[0].levels[0].user_id = 0;
            event->common.data_offset = sizeof(struct sound_trigger_phrase_recognition_event);
            event->common.data_size = 1;
            data[event->common.data_offset] = 8;
            ALOGI("%s neo:send recognition_callback model %d", __func__, stdev->model_handle);
            stdev->recognition_callback(&event->common, stdev->recognition_cookie);
            free(data);
        }
        else if (capture_val == SOUND_TRIGGER_STOPPING_CAPTURING) {
            ALOGI("%s neo:abort recognition model %d", __func__, stdev->model_handle);
            char *data = (char *)calloc(1, sizeof(struct sound_trigger_phrase_recognition_event) + 1);
            struct sound_trigger_phrase_recognition_event *event =
                    (struct sound_trigger_phrase_recognition_event *)data;
            event->common.status = RECOGNITION_STATUS_ABORT;
            event->common.type = SOUND_MODEL_TYPE_KEYPHRASE;
            event->common.model = stdev->model_handle;
            free(data);
            pthread_mutex_unlock(&stdev->lock);
            break;
        }
        else {
            ALOGI("%s neo: recognition err  model %d", __func__, stdev->model_handle);
            char *data = (char *)calloc(1, sizeof(struct sound_trigger_phrase_recognition_event) + 1);
            struct sound_trigger_phrase_recognition_event *event =
                    (struct sound_trigger_phrase_recognition_event *)data;
            event->common.status = RECOGNITION_STATUS_FAILURE;
            event->common.type = SOUND_MODEL_TYPE_KEYPHRASE;
            event->common.model = stdev->model_handle;
            free(data);
        }
        pthread_mutex_unlock(&stdev->lock);
    }
    while(true);
    stdev->recognition_callback = NULL;
exit:

    return NULL;
}

static int stdev_get_properties(const struct sound_trigger_hw_device *dev,
                                struct sound_trigger_properties *properties)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;

    ALOGI("%s", __func__);
    if (properties == NULL)
        return -EINVAL;
    memcpy(properties, &hw_properties, sizeof(struct sound_trigger_properties));
    return 0;
}

static int stdev_load_sound_model(const struct sound_trigger_hw_device *dev,
                                  struct sound_trigger_sound_model *sound_model,
                                  sound_model_callback_t callback,
                                  void *cookie,
                                  sound_model_handle_t *handle)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;
    ALOGI("%s stdev %p", __func__, stdev);
    pthread_mutex_lock(&stdev->lock);
    if (handle == NULL || sound_model == NULL) {
        status = -EINVAL;
        goto exit;
    }
    if (sound_model->data_size == 0 ||
            sound_model->data_offset < sizeof(struct sound_trigger_sound_model)) {
        status = -EINVAL;
        goto exit;
    }

    if (stdev->model_handle == 1) {
        status = -ENOSYS;
        goto exit;
    }


    char *data = (char *)sound_model + sound_model->data_offset;
    ALOGI("sound_model->data_offset %d  %p   %p" ,sound_model->data_offset ,sound_model , data );
    ALOGI("%s data size %d data %d - %d - %d - %d", __func__,
          sound_model->data_size, data[0], data[1] , data[2] ,data[sound_model->data_size - 1]);
    stdev->model_handle  = 1;
    stdev->sound_model_callback = callback;
    stdev->sound_model_cookie = cookie;
    stdev->firmware_data = (unsigned char *) data ;
    stdev->firmware_size = sound_model->data_size ;

    status = stdev_set_dsp(stdev, STDEV_DSP_STOP);
    if(status) {
        ALOGE("%s set dsp state err" ,  __func__) ;
        goto exit ;
    }
    status = stdev_set_dsp(stdev, STDEV_DSP_FAST_RELOAD);
    if (!status) {
        char *data = (char *)calloc(1, sizeof(struct sound_trigger_model_event) + 1);
        struct sound_trigger_model_event *event =
                (struct sound_trigger_model_event *)data;
        event->status = SOUND_MODEL_STATUS_UPDATED;
        event->model = stdev->model_handle;
        event->data_offset = sizeof(struct sound_trigger_model_event);
        event->data_size = 1;
        ALOGI("%s neo:send sound_model_callback model %d", __func__, stdev->model_handle);
        stdev->sound_model_callback(event, stdev->sound_model_cookie);
        free(data);
        status = 0;
    }

    *handle = stdev->model_handle;
    stdev->sound_model_callback = NULL;

exit:
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static int stdev_unload_sound_model(const struct sound_trigger_hw_device *dev,
                                    sound_model_handle_t handle)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;

    ALOGI("%s handle %d", __func__, handle);
    pthread_mutex_lock(&stdev->lock);
    if (handle != 1) {
        status = -EINVAL;
        goto exit;
    }
    if (stdev->model_handle == 0) {
        status = -ENOSYS;
        goto exit;
    }
    stdev->model_handle = 0;

    if (stdev->recognition_callback != NULL) {
        stdev->recognition_callback = NULL;
        status = ioctl(stdev->st_fd, SOUND_TRIGGER_STOPPING_CAPTURING, (void *)stdev);
        if (status){
            ALOGE("write st_fd err");
            goto exit;
        }


        pthread_mutex_unlock(&stdev->lock);
        pthread_join(stdev->callback_thread, (void **) NULL);
        pthread_mutex_lock(&stdev->lock);
    }

exit:
    mixer_ctl_set_value(stdev->ctl_mic, 0, 0);
    status = stdev_set_dsp(stdev, STDEV_DSP_STOP);
    if(status) {
        ALOGE("%s set dsp state err" ,  __func__) ;
    }
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static int stdev_start_recognition(const struct sound_trigger_hw_device *dev,
                                   sound_model_handle_t sound_model_handle,
                                   const struct sound_trigger_recognition_config *config,
                                   recognition_callback_t callback,
                                   void *cookie)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;

    pthread_mutex_lock(&stdev->lock);
    if (stdev->model_handle != sound_model_handle) {
        status = -ENOSYS;
        goto exit;
    }

    if (stdev->st_fd < 0) {
        status = -ENODEV;
        goto exit;
    }


    if (config->data_size != 0) {
        char *data = (char *)config + config->data_offset;
        ALOGI("%s data size %d data %d - %d", __func__,
              config->data_size, data[0], data[config->data_size - 1]);
        stdev->firmware_config_data = (unsigned char *) data ;
        stdev->firmware_config_size =  config->data_size ;
        status = stdev_set_dsp(stdev, STDEV_DSP_SET_SENSITIVITY);
        if(status) {
            ALOGE("%s set dsp state err" ,  __func__) ;
            goto exit ;
        }

    }



    mixer_ctl_set_value(stdev->ctl_mic, 0, 1);  // open micbias2

    status = stdev_set_dsp(stdev, STDEV_DSP_START); // start dsp
    if(status) {
        ALOGE("%s set dsp state err" ,  __func__) ;
        goto exit ;
    }

    if(stdev->recognition_callback == NULL) {
        stdev->recognition_callback = callback;
        stdev->recognition_cookie = cookie;
        pthread_create(&stdev->callback_thread, (const pthread_attr_t *) NULL,
                        callback_thread_loop, stdev);
    }

exit:
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static int stdev_stop_recognition(const struct sound_trigger_hw_device *dev,
                                 sound_model_handle_t sound_model_handle)
{
    struct stub_sound_trigger_device *stdev = (struct stub_sound_trigger_device *)dev;
    int status = 0;
    ALOGI("%s sound model %d ", __func__, sound_model_handle );
    pthread_mutex_lock(&stdev->lock);
    if (stdev->model_handle != sound_model_handle) {
        status = -ENOSYS;
        goto exit;
    }
    if (stdev->recognition_callback == NULL) {
        status = -ENOSYS;
        goto exit;
    }

    status = ioctl(stdev->st_fd, SOUND_TRIGGER_STOPPING_CAPTURING, (void *)NULL); // exit thread
    if (status){
        status = EIO ;
        goto exit;
    }

    pthread_mutex_unlock(&stdev->lock);
    pthread_join(stdev->callback_thread, (void **) NULL);
    pthread_mutex_lock(&stdev->lock);
    stdev->recognition_callback = NULL;

exit:
    mixer_ctl_set_value(stdev->ctl_mic, 0, 0);    // close micbias2
    status = stdev_set_dsp(stdev, STDEV_DSP_STOP);
    if(status) {
        ALOGE("%s set dsp state err" ,  __func__) ;
    }
    pthread_mutex_unlock(&stdev->lock);
    return status;
}

static void stdev_uninit(struct stub_sound_trigger_device *stdev)
{
    if (stdev) {
        mixer_close(stdev->mixer);
        close(stdev->st_fd);
    }
}

static int stdev_close(hw_device_t *device)
{
    struct stub_sound_trigger_device *stdev =
                                (struct stub_sound_trigger_device *)device;
    int ret = 0;

    pthread_mutex_lock(&stdev->lock);
    if (!stdev->opened) {
        ALOGE("%s: device already closed", __func__);
        ret = -EFAULT;
        goto exit;
    }
    stdev_uninit(stdev);

    ret = stdev_set_dsp(stdev, STDEV_DSP_STOP);
    if(ret) {
        ALOGE("%s set dsp state err" ,  __func__) ;
        goto exit;
    }

    stdev->model_handle = 0;
    stdev->opened = false;

exit:
    pthread_mutex_unlock(&stdev->lock);
    free(stdev);
    return ret;
}

static int stdev_init(struct stub_sound_trigger_device *stdev)
{
    int ret = -1;

    stdev->st_fd = open(SOUND_TRIGGER_DEV, O_RDWR);
    if (stdev->st_fd < 0) {
        ALOGE("Error opening st device path  stdev->st_fd : %d  %s" , stdev->st_fd ,strerror(errno));
        ret = -ENODEV;
        goto exit;
    }

    stdev->mixer = mixer_open(STDEV_MIXER_DEV);
    if (!stdev->mixer)
        goto exit;

    stdev->ctl_mic = mixer_get_ctl_by_name(stdev->mixer, STDEV_CTRL_MICBIAS2);
    if (!stdev->ctl_mic)
        goto exit;

    /* Commented this line by peng.lee tmply.
    mixer_ctl_set_value(stdev->ctl_mic, 0, 0);
    */

    ret = stdev_set_dsp(stdev, STDEV_DSP_STOP);
    if(ret) {
        ALOGE("%s set dsp state err" ,  __func__) ;
        goto exit;
    }
    return 0;

exit:
    if (stdev->mixer)
        mixer_close(stdev->mixer);
    return ret;
}

static int stdev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct stub_sound_trigger_device *stdev;
    int ret;

    ALOGI("%s stdev_open ", __func__);

    if (strcmp(name, SOUND_TRIGGER_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    stdev = calloc(1, sizeof(struct stub_sound_trigger_device));
    if (!stdev)
        return -ENOMEM;

    pthread_mutex_lock(&stdev->lock);
    if (stdev->opened) {
        ALOGE("%s: Only one sountrigger can be opened at a time", __func__);
        ret = -EBUSY;
        goto exit;
    }

    ret = stdev_init(stdev);
    if (ret) {
        ALOGE("Error mixer init");
        goto exit;
    }

    stdev->device.common.tag = HARDWARE_DEVICE_TAG;
    stdev->device.common.version = SOUND_TRIGGER_DEVICE_API_VERSION_1_0;
    stdev->device.common.module = (struct hw_module_t *) module;
    stdev->device.common.close = stdev_close;
    stdev->device.get_properties = stdev_get_properties;
    stdev->device.load_sound_model = stdev_load_sound_model;
    stdev->device.unload_sound_model = stdev_unload_sound_model;
    stdev->device.start_recognition = stdev_start_recognition;
    stdev->device.stop_recognition = stdev_stop_recognition;

    pthread_mutex_init(&stdev->lock, (const pthread_mutexattr_t *) NULL);

    *device = &stdev->device.common;

exit:
    pthread_mutex_unlock(&stdev->lock);
    return ret;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = stdev_open,
};

struct sound_trigger_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SOUND_TRIGGER_MODULE_API_VERSION_1_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SOUND_TRIGGER_HARDWARE_MODULE_ID,
        .name = "Sprd sound trigger HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
