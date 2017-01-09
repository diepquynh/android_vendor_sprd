
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include "audio_hw.h"
#include "audio_control.h"
#include "ring_buffer.h"
#include "Tone_generate.h"
#include "HW_ul_Test.h"
#include "audio_dev_laohua_test.h"
#define LOG_TAG "audio_dev_laohua_test"


#define TEST_AUDIO_INVALID_DATA_MS 500
#define TONE_VOLUME 5000
#define TONE_LEN    16000

#define AUDIO_LOOP_TEST_RESULT_PROPERITY  "audioloop.test.result"

static int get_audio_dev_laohua_test_info(struct str_parms *parms,struct dev_laohua_test_info_t *test_info,int mode){
    int ret=0;
    char value[256]={0};

    if((OUT_MODE == mode)){
        ret = str_parms_get_str(parms,"test_stream_route", value, sizeof(value));
        if(ret >= 0){
            test_info->devices = strtoul(value,NULL,0);
        }else{
            test_info->devices = 0;
        }
    }

    if((IN_MODE == mode)){
        ret = str_parms_get_str(parms,"test_in_stream_route", value, sizeof(value));
        if(ret >= 0){
            test_info->devices=strtoul(value,NULL,16);
        }else{
            test_info->devices=0;
        }
    }

    ret = str_parms_get_str(parms,"samplerate", value, sizeof(value));
    if(ret >= 0){
        test_info->config.rate=strtoul(value,NULL,10);
    }else{
        test_info->config.rate=48000;
    }

    ret = str_parms_get_str(parms,"channels", value, sizeof(value));
    if(ret >= 0){
        test_info->config.channels=strtoul(value,NULL,10);
    }else{
        test_info->config.channels=2;
    }

    ret = str_parms_get_str(parms,"filepath_r", value, sizeof(value));
    if(IN_MODE == mode){
        if(ret >= 0){
            {
                test_info->fd  = open(value, O_RDWR, 0666);
                if(test_info->fd < 0) {
                    LOG_W("open:%s failed",value);
                }
        }
        }else{
           test_info->fd  = -1;
        }
    }
    return 0;
}




void *dev_laohua_test_out_thread(void *args){

    struct dev_laohua_test_info_t *out_test_info=(struct dev_laohua_test_info_t *)args;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)out_test_info->dev;
	struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
	struct dev_laohua_test_info_t *in_test_info = (struct dev_laohua_test_info_t *) dev_test->dev_laohua_in_test_info;


    int notify_wait_count = 3 ;
    int is_notify_to_capture = 0;
    AUDIO_TONE_GENERATE_STRUCT_T   *tone_test_struct;
    int ret = 0;

    /* prepare data */
    out_test_info->bufffer_info.total_size = TONE_LEN*sizeof(int16) ;
    out_test_info->bufffer_info.base =  (int16 *)malloc(out_test_info->bufffer_info.total_size);
    if(out_test_info->bufffer_info.base == NULL){
        ALOGE("neo: buffer malloc error " );
        goto ERR;
    }
    tone_test_struct = (AUDIO_TONE_GENERATE_STRUCT_T*)malloc(sizeof(AUDIO_TONE_GENERATE_STRUCT_T));
    if(tone_test_struct == NULL){
        ALOGE("neo: tone_test_struct malloc error " );
        goto ERR;
    }
    tone_test_struct->freq       = out_test_info->frequency; // frequency
    tone_test_struct->volume     = TONE_VOLUME;                     // volume
    tone_test_struct->len        = TONE_LEN;                        // len
    tone_test_struct->output_ptr = out_test_info->bufffer_info.base;
    audio_tone_generator(tone_test_struct);

    ALOGD("playback_thread wait");
    sem_wait(&out_test_info->sem);
    ALOGD("playback_thread running");


    /* config mixer  */
    dsp_sleep_ctrl(adev->dev_ctl->agdsp_ctl,true);
    set_usecase(adev->dev_ctl, UC_NORMAL_PLAYBACK, true);
    select_devices_new(adev->dev_ctl,UC_NORMAL_PLAYBACK, out_test_info->devices,false);

    /* play */
    out_test_info->config.format   = PCM_FORMAT_S16_LE;
    out_test_info->config.period_size = 1024 ;
    out_test_info->config.period_count = 4 ;
    out_test_info->pcm = pcm_open(0, 0, PCM_OUT | PCM_MMAP | PCM_NOIRQ |PCM_MONOTONIC, &out_test_info->config);
    if (!pcm_is_ready(out_test_info->pcm)) {
        LOG_E("%s:cannot open pcm : %s", __func__,
        pcm_get_error(out_test_info->pcm));
        goto ERR;
    }

    while((!out_test_info->is_exit) ){
        ret = pcm_mmap_write(out_test_info->pcm, (void*)(out_test_info->bufffer_info.base),
            out_test_info->bufffer_info.total_size);
        if(adev->dev_ctl->config.dump.playback.normal.hex_fd>0){
             write(adev->dev_ctl->config.dump.playback.normal.hex_fd,(void *)(out_test_info->bufffer_info.base ),
                 out_test_info->bufffer_info.total_size);
        }
        ALOGI("neo: pcm_mmap_write  base_ptr %p  total size %d  " ,
             out_test_info->bufffer_info.base ,out_test_info->bufffer_info.total_size);
        if(ret<0){
            usleep(20000);
        }else{
        if( (is_notify_to_capture == 0) && !(notify_wait_count--)) {
             is_notify_to_capture= 1;
                 sem_post(&in_test_info->sem);
            }
        }
    }
ERR:

    if(out_test_info->bufffer_info.base)  {
        free(out_test_info->bufffer_info.base);
        out_test_info->bufffer_info.base = NULL;
    }
    if(out_test_info->pcm) {
        pcm_close(out_test_info->pcm);
        out_test_info->pcm = NULL;
    }
    if(tone_test_struct){
        free(tone_test_struct);
    }
    set_usecase(adev->dev_ctl, UC_NORMAL_PLAYBACK, false);
    sem_destroy(&out_test_info->sem);
    out_test_info->is_exit=true;
    dev_test->is_in_test = false ;
    if(out_test_info)
        free(out_test_info);

    LOG_E("audiotest_playback_thread exit");
    return NULL;
}



void *dev_laohua_test_in_thread(void *args){
    struct dev_laohua_test_info_t *in_test_info=(struct dev_laohua_test_info_t *)args;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)in_test_info->dev;
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct dev_laohua_test_info_t *out_test_info = (struct dev_laohua_test_info_t *) dev_test->dev_laohua_out_test_info;

    unsigned int bytes_read = 0;
    int size = 0 ;
    char* buffer = NULL ;
    int invalid_data_size=0;
    int16 pass_ratio_thd = 29490;
    int16 Energy_ratio_thd = 2;
    int ret = 0 ;
    int count = 0;
    int is_notify_to_playback_exit = 0 ;
    AUDIO_LOOP_TEST_INFO_STRUCT_T *audio_test_verify_info;
    audio_test_verify_info = malloc(sizeof(AUDIO_LOOP_TEST_INFO_STRUCT_T));
    if(!audio_test_verify_info) {
       ALOGE("audio_test_verify_info creating failed !!!!");
       goto ERR;
    }
    memset(audio_test_verify_info , 0 , sizeof(AUDIO_LOOP_TEST_INFO_STRUCT_T));
    ALOGD("capture_thread wait");
    sem_wait(&in_test_info->sem);
    ALOGD("capture_thread running");


    in_test_info->bufffer_info.size= sizeof(audio_test_verify_info->input);
    in_test_info->bufffer_info.base = malloc(in_test_info->bufffer_info.size);
    if (!in_test_info->bufffer_info.base) {
        ALOGW("Unable to allocate %d bytes\n", in_test_info->bufffer_info.size);
        goto ERR;
    }
    memset(in_test_info->bufffer_info.base,0,in_test_info->bufffer_info.size);

    ALOGD("begin capture!!!!!!!!!!!!!!!!");
    // init
    audio_hw_test_handle  audio_test_verify_handle;
    audio_test_verify_handle = audio_HW_ul_Test_init(Energy_ratio_thd ,pass_ratio_thd );
    invalid_data_size=(TEST_AUDIO_INVALID_DATA_MS * in_test_info->config.rate
        *in_test_info->config.channels*2/1000);
    audio_test_verify_info->frequency = in_test_info->frequency ;

    /* config mixer  */
    set_usecase(adev->dev_ctl, UC_MM_RECORD, true);
    select_devices_new(adev->dev_ctl,UC_MM_RECORD,in_test_info->devices,true);

    //  record and process 
    in_test_info->config.format   = PCM_FORMAT_S16_LE;
    in_test_info->config.period_size = 320 ;
    in_test_info->config.period_count = 4 ;
    in_test_info->pcm = pcm_open(0, 0, PCM_IN, &in_test_info->config);
    if (!pcm_is_ready(in_test_info->pcm)) {
        LOG_E("%s:cannot open pcm : %s", __func__,
        pcm_get_error(in_test_info->pcm));
        goto ERR;
    }


    while((!in_test_info->is_exit) &&  !audio_test_verify_info->isReady ){
        ret = pcm_read(in_test_info->pcm, in_test_info->bufffer_info.base,in_test_info->bufffer_info.size);
        if (ret<0){
            LOG_W("in_test_thread: no data read ret:%d",ret);
            usleep(20000);
        }else{
            if(adev->dev_ctl->config.dump.record.vbc.hex_fd>0){
                write(adev->dev_ctl->config.dump.record.vbc.hex_fd,in_test_info->bufffer_info.base,
                               in_test_info->bufffer_info.size);
        }
        ALOGI("neo: capture %d bytes" , in_test_info->bufffer_info.size ) ;
        bytes_read += in_test_info->bufffer_info.size ;
            if(bytes_read >(invalid_data_size + 10240) ){
	            memcpy(audio_test_verify_info->input, in_test_info->bufffer_info.base , sizeof(audio_test_verify_info->input));
	            audio_HW_ul_Test(audio_test_verify_info, audio_test_verify_handle);
                ALOGE("neo***:audio_loop_devices_test %d %d  count = %d",
                audio_test_verify_info->hwStestResult,audio_test_verify_info->isReady , count++);
            }
        }
    }

   // 3. deinit
    audio_HW_ul_Test_deinit(audio_test_verify_handle);

    // 4. set test result 
    if(audio_test_verify_info->hwStestResult) {
        ALOGE("sprd_devicelaohua_audioloop_test the result is %d" ,audio_test_verify_info->hwStestResult );
        property_set(AUDIO_LOOP_TEST_RESULT_PROPERITY,"1");
    }
    else {
        ALOGE("sprd_devicelaohua_audioloop_test the result is %d" ,audio_test_verify_info->hwStestResult );
        property_set(AUDIO_LOOP_TEST_RESULT_PROPERITY,"0"); 
   }


ERR:

    // 5. let the playback thread to exit
    if(is_notify_to_playback_exit == 0) {
        is_notify_to_playback_exit = 1 ;
        out_test_info->is_exit = true;
    }

    if(audio_test_verify_info){
        free(audio_test_verify_info);
        audio_test_verify_info = NULL;
    }
    if(in_test_info->bufffer_info.base){
        free(in_test_info->bufffer_info.base);
        in_test_info->bufffer_info.base=NULL;
    }

    set_usecase(adev->dev_ctl, UC_MM_RECORD, false);
    if(in_test_info->pcm) {
        pcm_close(in_test_info->pcm);
        in_test_info->pcm = NULL;
    }

    in_test_info->is_exit=true;
    sem_destroy(&in_test_info->sem);
    dev_test->is_in_test = false ;
    if(in_test_info)
        free(in_test_info) ;
    ALOGE("audiotest_capture_thread exit");
    return NULL;
}

int sprd_audio_dev_laohua_test(void * dev,struct str_parms *parms){
    int ret=0;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct dev_laohua_test_info_t *out_test_info ;
    struct dev_laohua_test_info_t *in_test_info ;

    const uint16 Tone_frequency_table[10] = {875,1000,1125,1250,1375,1500,1625,1750,1875,2000};
    pthread_attr_t capture_attr;
    pthread_attr_t playback_attr;

    LOG_D("%s %d",__func__,__LINE__);

    if(dev_test->is_in_test){
        LOG_W("sprd_audio_dev_laohua_test aready in test ");
	    return -1;
    }

    /* alloc test info */
    in_test_info = (struct dev_laohua_test_info_t *)malloc(sizeof (struct dev_laohua_test_info_t));
    out_test_info = (struct dev_laohua_test_info_t *)malloc(sizeof (struct dev_laohua_test_info_t));
    memset(in_test_info , 0 , sizeof (struct dev_laohua_test_info_t));
    memset(out_test_info , 0 , sizeof (struct dev_laohua_test_info_t));


    /*init test info */
    ret=get_audio_dev_laohua_test_info(parms,out_test_info, 1);  // mode  1 : out  0 : in
    if(ret<0){
        goto error;
    }
    ret=get_audio_dev_laohua_test_info(parms,in_test_info, 0);
    if(ret<0){
        goto error;
    }

    srand(time(0));
    out_test_info->frequency = Tone_frequency_table[rand()%10]	;
    in_test_info->frequency  = out_test_info->frequency;
    out_test_info->dev = adev ;
    in_test_info->dev = adev ;
    LOG_D("%s %d",__func__,__LINE__);
    dev_test->dev_laohua_in_test_info  =  in_test_info ;
    dev_test->dev_laohua_out_test_info = out_test_info;
    LOG_D("%s %d",__func__,__LINE__);

    out_test_info->is_exit=false;
    in_test_info->is_exit=false;

    sem_init(&in_test_info->sem, 0, 0);
    sem_init(&out_test_info->sem, 0, 1);
    ALOGI("filepath_r test_info->fd %d" , in_test_info->fd);
    dev_test->is_in_test = true ;

    // set thread attr DETACHED to auto free thread resource
    pthread_attr_init(&playback_attr);
    pthread_attr_setdetachstate(&playback_attr, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&out_test_info->thread, &playback_attr, dev_laohua_test_out_thread, (void *)out_test_info)) {
        ALOGE("audio_out_devices_test creating failed !!!!");
        goto error;
    }
    // set thread attr DETACHED to auto free thread resource
    pthread_attr_init(&capture_attr);
    pthread_attr_setdetachstate(&capture_attr, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&in_test_info->thread, &capture_attr, dev_laohua_test_in_thread, (void *)in_test_info)) {
        ALOGE("audiotest_capture_thread creating failed !!!!");
        goto error;
    }
    return ret;
error:
    if(in_test_info)
        free(in_test_info) ;
    if(out_test_info)
        free(out_test_info) ;

    dev_test->is_in_test = false ;
    return -1;
}

