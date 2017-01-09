#include <utils/Log.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <tinyalsa/asoundlib.h>

#include "aud_enha.h"
#include <cutils/sockets.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>


//#ifdef __cplusplus
//extern "c"
//{
//#endif


//#define MY_DEBUG

#ifdef MY_DEBUG
#define MY_TRACE    ALOGW
#else
#define MY_TRACE(...)
#endif

#define VBC_CMD_TAG   "VBC"
/*
#define READ_PARAS(type, exp)    if (s_vbpipe_fd > 0 && paras_ptr != NULL) { \
exp = read(s_vbpipe_fd, paras_ptr, sizeof(type)); \
}
*/

#define KCTL_MAIN_MIC_ADCL  "ADCL Mixer MainMICADCL Switch"
#define KCTL_AUX_MIC_ADCL   "ADCL Mixer AuxMICADCL Switch"
#define KCTL_HP_MIC_ADCL    "ADCL Mixer HPMICADCL Switch"
#define CALL_END_TIMEOUT_SECONDS	3

/* vbc control parameters struct here.*/
typedef struct Paras_Mode_Gain
{
    unsigned short  is_mode;
    unsigned short  is_volume;

    unsigned short  mode_index;
    unsigned short  volume_index;

    unsigned short  dac_set;
    unsigned short  adc_set;

    unsigned short  dac_gain;
    unsigned short  adc_gain;

    unsigned short  path_set;
    unsigned short  pa_setting;

    unsigned short  reserved[16];
}paras_mode_gain_t;

typedef struct Switch_ctrl
{
    unsigned int  is_switch; /* switch vbc contrl to dsp.*/
}switch_ctrl_t;

typedef struct Samplerate_ctrl
{
    unsigned int samplerate; /* change samplerate.*/
}set_samplerate_t;

typedef struct Set_Mute
{
    unsigned int  is_mute;
}set_mute_t;

typedef struct Device_ctrl
{
    unsigned short  	is_open; /* if is_open is true, open device; else close device.*/
    unsigned short  	is_headphone;
    unsigned int 		is_downlink_mute;
    unsigned int 		is_uplink_mute;
    paras_mode_gain_t 	paras_mode;
}device_ctrl_t;

typedef struct Open_hal
{
    unsigned int  sim_card;   /*sim card number*/
}open_hal_t;


#ifdef AUDIO_SPIPE_TD
#define VBC_ARM_CHANNELID 2
#else
#define VBC_ARM_CHANNELID 1
#endif

typedef struct
{
    char        tag[4];   /* "VBC" */
    unsigned int    cmd_type;
    unsigned int    paras_size; /* the size of Parameters Data, unit: bytes*/
}parameters_head_t;

static unsigned short s_vbc_pipe_count = 0;

typedef struct
{
    char vbpipe[VBC_PIPE_NAME_MAX_LEN];//vbpipe5/vbpipe6
    int vbchannel_id;
    cp_type_t cp_type;
    int vbpipe_fd;
    int vbpipe_pre_fd;
    struct tiny_audio_device *adev;
}vbc_ctrl_thread_para_t;

static pthread_t *s_vbc_ctrl_thread_id =NULL;
static vbc_ctrl_thread_para_t *st_vbc_ctrl_thread_para = NULL;

static uint32_t s_call_dl_devices = 0;   // devices value the same as audiosystem.h
static uint32_t s_call_ul_devices = 0;
static int s_is_active = 0;
static int android_sim_num = 0;
static vbc_ctrl_pipe_para_t s_default_vbc_ctrl_pipe_info =
{
    "/dev/vbpipe6",0,CP_TG
};

/* Transfer packet by vbpipe, packet format as follows.*/
/************************************************
  ----------------------------
  |Paras Head |  Paras Data  |
  ----------------------------
 ************************************************/

/*
 * local functions declaration.
 */
static int  ReadParas_Head(int fd_pipe,  parameters_head_t *head_ptr);

static int  WriteParas_Head(int fd_pipe,  parameters_head_t *head_ptr);

static int  ReadParas_DeviceCtrl(int fd_pipe, device_ctrl_t *paras_ptr);

static int  ReadParas_ModeGain(int fd_pipe,  paras_mode_gain_t *paras_ptr);

static int  ReadParas_SwitchCtrl(int fd_pipe,  switch_ctrl_t *paras_ptr);

static int  ReadParas_Mute(int fd_pipe,  set_mute_t *paras_ptr);

void *vbc_ctrl_thread_routine(void *args);
void *vbc_ctrl_voip_thread_routine(void *arg);

extern int headset_no_mic();

/*
 * local functions definition.
 */

static int read_nonblock(int fd,int8_t *buf,int bytes)
{
    int ret = 0;
    int bytes_to_read = bytes;

    if((fd > 0) && (buf != NULL)) {
        do {
            ret = read(fd, buf, bytes);
            if( ret > 0) {
                if(ret <= bytes) {
                    bytes -= ret;
                }
            }
            else if((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
                ALOGE("pipe read error %d,bytes read is %d",errno,bytes_to_read - bytes);
                break;
            } else {
		ALOGW("pipe_read_warning: %d,ret is %d",errno,ret);
	    }
        }while(bytes);
    }

    if(bytes == bytes_to_read)
        return ret ;
    else
        return (bytes_to_read - bytes);

}
static int write_nonblock(int fd,int8_t *buf,int bytes)
{
    int ret = -1;
    int bytes_to_read = bytes;

    if((fd > 0) && (buf != NULL)) {
        do {
            ret = write(fd, buf, bytes);
            if( ret > 0) {
                if(ret <= bytes) {
                    bytes -= ret;
                }
            }
            else if((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
                ALOGE("pipe write error %d,bytes read is %d",errno,bytes_to_read - bytes);
                break;
            }
	    else {
		ALOGW("pipe_write_warning: %d,ret is %d",errno,ret);
	    }
        }while(bytes);
    }

    if(bytes == bytes_to_read)
        return ret ;
    else
        return (bytes_to_read - bytes);

}

static void vbc_empty_pipe(int fd) {
    char buff[16];
    int ret;

    do {
        ret = read(fd, &buff, sizeof(buff));
    } while (ret > 0 || (ret < 0 && errno == EINTR));
}
static int  ReadParas_Head(int fd_pipe,  parameters_head_t *head_ptr)
{
    int ret = 0;
    ret = read_nonblock(fd_pipe,head_ptr,sizeof(parameters_head_t));
    if(ret != sizeof(parameters_head_t))
	ret = -1;
    return ret;
}

static int  WriteParas_Head(int fd_pipe,  parameters_head_t *head_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && head_ptr != NULL) {
        ret = write_nonblock(fd_pipe, head_ptr, sizeof(parameters_head_t));
        if(ret != sizeof(parameters_head_t))
            ret = -1;
    }
    return ret;
}

static int  ReadParas_OpenHal(int fd_pipe, open_hal_t *hal_open_param)
{
    int ret = 0;
    if (fd_pipe > 0 && hal_open_param != NULL) {
        ret = read_nonblock(fd_pipe, hal_open_param, sizeof(open_hal_t));
        if(ret != sizeof(sizeof(open_hal_t)))
            ret = -1;
    }
    return ret;
}


static int  ReadParas_DeviceCtrl(int fd_pipe, device_ctrl_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read_nonblock(fd_pipe, paras_ptr, sizeof(device_ctrl_t));
        if(ret != sizeof(device_ctrl_t))
            ret = -1;
    }
    return ret;
}

static int  ReadParas_ModeGain(int fd_pipe,  paras_mode_gain_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read_nonblock(fd_pipe, paras_ptr, sizeof(paras_mode_gain_t));
        if(ret != sizeof(paras_mode_gain_t))
            ret = -1;
    }
    return ret;
}

static int  ReadParas_SwitchCtrl(int fd_pipe,  switch_ctrl_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read_nonblock(fd_pipe, paras_ptr, sizeof(switch_ctrl_t));
        if(ret != sizeof(switch_ctrl_t))
            ret = -1;
    }
    return ret;
}

static int  ReadParas_SetSamplerate(int fd_pipe,  set_samplerate_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read_nonblock(fd_pipe, paras_ptr, sizeof(set_samplerate_t));
        if(ret != sizeof(set_samplerate_t))
            ret = -1;
    }
    return ret;
}

static int  ReadParas_Mute(int fd_pipe,  set_mute_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read_nonblock(fd_pipe, paras_ptr, sizeof(set_mute_t));
        if(ret != sizeof(set_mute_t))
            ret = -1;
    }
    return ret;
}

int Write_Rsp2cp(int fd_pipe, unsigned int cmd)
{
    int ret = 0;
    int count = 0;
    parameters_head_t write_common_head;
    memset(&write_common_head, 0, sizeof(parameters_head_t));
    memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = cmd+1;
    write_common_head.paras_size = 0;
    if(fd_pipe < 0){
        ALOGE("%s vbpipe has not open...",__func__);
        return -1;
    }
    ret = WriteParas_Head(fd_pipe, &write_common_head);
    if(ret < 0){
        ALOGE("%s:send cmd(%d) to cp ret(%d) error(%s)",__func__,write_common_head.cmd_type,ret,strerror(errno));
    }else{
        MY_TRACE("%s: send  cmd(%d) to cp ret(%d)",__func__,write_common_head.cmd_type,ret);
    }
    return ret;
}

unsigned short GetAudio_vbcpipe_count(void)
{
    return s_vbc_pipe_count;
}

/* Headset is 0, Handsfree is 3 */
static int32_t GetAudio_InMode_number_from_device(int in_dev)
{
    int ret = 3;

    if (((in_dev & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BUILTIN_MIC)
        ||((in_dev & ~ AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BACK_MIC))
        ret = 3;//handsfree
    else if (((in_dev & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_WIRED_HEADSET)
        ||((in_dev & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET))
        ret = 0;//headset

    return ret;
}

static int32_t GetAudio_mode_number_from_device(int outdev,int indev)
{
    int32_t lmode;
    if(((outdev & AUDIO_DEVICE_OUT_WIRED_HEADSET) && (outdev & AUDIO_DEVICE_OUT_SPEAKER))
            || ((outdev & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) && (outdev & AUDIO_DEVICE_OUT_SPEAKER))){
#if 0
		if(adev->input_source == AUDIO_SOURCE_CAMCORDER)
		{
			lmode = 0 ;  //under camera record , change mode to headset
		}
		else
#endif
		    lmode = 1;  //headfree

    }else if(outdev & AUDIO_DEVICE_OUT_EARPIECE){
        lmode = 2;  //handset
    }else if((outdev & AUDIO_DEVICE_OUT_SPEAKER) || (outdev & AUDIO_DEVICE_OUT_FM_SPEAKER)){
        lmode = 3;  //handsfree
    }else if((outdev & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (outdev & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)
            || (outdev & AUDIO_DEVICE_OUT_FM_HEADSET) || (indev & AUDIO_DEVICE_IN_WIRED_HEADSET)){
        lmode = 0;  //headset
    }else{
        ALOGW("%s device(0x%x) is not support, set default:handsfree \n",__func__,outdev);
        lmode = 3;
    }
    return lmode;
}

static int GetAudio_fd_from_nv()
{
    int fd = -1;
    off_t offset = 0;

    fd = open(ENG_AUDIO_PARA_DEBUG, O_RDONLY);
    if (-1 == fd) {
        ALOGW("%s, file %s open failed:%s\n",__func__,ENG_AUDIO_PARA_DEBUG,strerror(errno));
        fd = open(ENG_AUDIO_PARA,O_RDONLY);
        if(-1 == fd){
            ALOGE("%s, file %s open error:%s\n",__func__,ENG_AUDIO_PARA,strerror(errno));
            return -1;
        }
    }else{
        //check the size of /data/local/tmp/audio_para
        offset = lseek(fd,-1,SEEK_END);
        if((offset+1) != 4*sizeof(AUDIO_TOTAL_T)){
            ALOGE("%s, file %s size (%ld) error \n",__func__,ENG_AUDIO_PARA_DEBUG,offset+1);
            close(fd);
            fd = open(ENG_AUDIO_PARA,O_RDONLY);
            if(-1 == fd){
                ALOGE("%s, file %s open error:%s\n",__func__,ENG_AUDIO_PARA,strerror(errno));
                return -1;
            }
        }
    }
    return fd;
}

static int  GetAudio_PaConfig_nv(struct tiny_audio_device *adev, AUDIO_TOTAL_T *aud_params_ptr, pga_gain_nv_t *pga_gain_nv)
{
    AUDIO_NV_ARM_MODE_STRUCT_T *ptArmModeStruct = NULL;
    if((NULL == aud_params_ptr) || (NULL == pga_gain_nv)){
        ALOGE("%s aud_params_ptr or pga_gain_nv is NULL",__func__);
        return -1;
    }

    ptArmModeStruct = (AUDIO_NV_ARM_MODE_STRUCT_T *)(&(aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct));

    pga_gain_nv->pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN_INDEX] | //45
        ((ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN2_INDEX]<<16) & 0xffff0000));    //53
    pga_gain_nv->fm_pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN_INDEX] |    //47
        ((ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN2_INDEX]<<16) & 0xffff0000));    //48

    pga_gain_nv->hp_pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG1_INDEX]
        |  ((ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG2_INDEX]<<16) & 0xffff0000));         //49-50
    pga_gain_nv->fm_hp_pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG1_INDEX]
        |  ((ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG2_INDEX]<<16) & 0xffff0000));   //51-52
	pga_gain_nv->hp_pa_delay_config = ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG_DELAY_INDEX];         //63

    ALOGW("vb_control_parameters.c hp_pa_delay_config 0x%x pa_config:0x%x(0x%x, %x), 0x%x(fm)(0x%x, %x),  hpPaConfig:0x%x(0x%x, %x), 0x%x(fm)(0x%x, %x)",
			pga_gain_nv->hp_pa_delay_config,
			pga_gain_nv->pa_config,ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN2_INDEX],
            pga_gain_nv->fm_pa_config,ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN2_INDEX],
            pga_gain_nv->hp_pa_config,ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG1_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG2_INDEX],
            pga_gain_nv->fm_hp_pa_config,
            ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG1_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG2_INDEX]);
    return 0;
}

static int  GetAudio_outpga_nv(struct tiny_audio_device *adev, AUDIO_TOTAL_T *aud_params_ptr, pga_gain_nv_t *pga_gain_nv, uint32_t vol_level)
{
    AUDIO_NV_ARM_MODE_STRUCT_T *ptArmModeStruct = NULL;
    if((NULL == aud_params_ptr) || (NULL == pga_gain_nv)){
        ALOGE("%s aud_params_ptr or pga_gain_nv is NULL",__func__);
        return -1;
    }

    ptArmModeStruct = (AUDIO_NV_ARM_MODE_STRUCT_T *)(&(aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct));

    pga_gain_nv->dac_pga_gain_l = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[0].arm_volume[vol_level];
    pga_gain_nv->dac_pga_gain_r = pga_gain_nv->dac_pga_gain_l;

    pga_gain_nv->fm_pga_gain_l  = (aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_GAINL_INDEX]
        | ((aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_FM_DGAIN_INDEX]<<16) & 0xffff0000));  //18,19
    pga_gain_nv->fm_pga_gain_r  = pga_gain_nv->fm_pga_gain_l;

    pga_gain_nv->pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN_INDEX] | //45
        ((ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN2_INDEX]<<16) & 0xffff0000));    //53
    pga_gain_nv->fm_pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN_INDEX] |    //47
        ((ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN2_INDEX]<<16) & 0xffff0000));    //48

    pga_gain_nv->hp_pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG1_INDEX]
        |  ((ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG2_INDEX]<<16) & 0xffff0000));         //49-50


    pga_gain_nv->fm_hp_pa_config =
        (ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG1_INDEX]
        |  ((ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG2_INDEX]<<16) & 0xffff0000));   //51-52

    pga_gain_nv->cg_pga_gain_l = ptArmModeStruct->reserve[AUDIO_NV_CG_PGA_GAIN_L_INDEX];
    pga_gain_nv->cg_pga_gain_r = ptArmModeStruct->reserve[AUDIO_NV_CG_PGA_GAIN_R_INDEX];
    pga_gain_nv->fm_cg_pga_gain_l = ptArmModeStruct->reserve[AUDIO_NV_FM_CG_PGA_GAIN_L_INDEX];
    pga_gain_nv->fm_cg_pga_gain_r = ptArmModeStruct->reserve[AUDIO_NV_FM_CG_PGA_GAIN_R_INDEX];

    ALOGW("vb_control_parameters.c pa_config:0x%x(0x%x, %x), 0x%x(fm)(0x%x, %x),  hpPaConfig:0x%x(0x%x, %x), 0x%x(fm)(0x%x, %x)",
            pga_gain_nv->pa_config,ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_INTPA_GAIN2_INDEX],
            pga_gain_nv->fm_pa_config,ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_FM_INTPA_GAIN2_INDEX],
            pga_gain_nv->hp_pa_config,ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG1_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_INTHPPA_CONFIG2_INDEX],
            pga_gain_nv->fm_hp_pa_config,
            ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG1_INDEX],
            ptArmModeStruct->reserve[AUDIO_NV_FM_INTHPPA_CONFIG2_INDEX]);

    ALOGW("vb_control_parameters.c %s, dac_pga_gain_l:0x%x fm_pga_gain_l:0x%x fm_pga_gain_r:0x%x vol_level:0x%x ",
            __func__,pga_gain_nv->dac_pga_gain_l,pga_gain_nv->fm_pga_gain_l,pga_gain_nv->fm_pga_gain_r,
            vol_level);
    return 0;
}

static int  GetAudio_inpga_nv(struct tiny_audio_device *adev, AUDIO_TOTAL_T *aud_params_ptr, pga_gain_nv_t *pga_gain_nv, uint32_t vol_level)
{
    if((NULL == aud_params_ptr) || (NULL == pga_gain_nv)){
        ALOGE("%s aud_params_ptr or pga_gain_nv is NULL",__func__);
        return -1;
    }
    pga_gain_nv->adc_pga_gain_l = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[AUDIO_NV_CAPTURE_GAIN_INDEX];    //43
    if (adev->fm_open && adev->fm_record) {
        uint16_t fm_record_dg = aud_params_ptr->audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[15].arm_volume[16]; //16 is used for fm record dg gain
        //set fm record dg gain in bit 8~bit 14
        pga_gain_nv->adc_pga_gain_l = ((pga_gain_nv->adc_pga_gain_l & 0x80FF) | ((fm_record_dg & 0x7F) << 8));
        ALOGD("set fm adc_pga_gain_l=0x%x", pga_gain_nv->adc_pga_gain_l);
    }
    pga_gain_nv->adc_pga_gain_r = pga_gain_nv->adc_pga_gain_l;

    ALOGW("%s, adc_pga_gain_l:0x%x device:0x%x vol_level:0x%x",
        __func__,pga_gain_nv->adc_pga_gain_l, pga_gain_nv->in_devices,vol_level);
    return 0;
}

static int GetAudio_PaConfig_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv,int out_dev,int in_dev)
{
    int ret = 0;
    int32_t lmode = 0;
    AUDIO_TOTAL_T * aud_params_ptr = NULL;
    char * dev_name = NULL;
    lmode = GetAudio_mode_number_from_device(out_dev,in_dev);

    aud_params_ptr = adev->audio_para;//(AUDIO_TOTAL_T *)mmap(0, 4*sizeof(AUDIO_TOTAL_T),PROT_READ,MAP_SHARED,fd,0);
    if ( NULL == aud_params_ptr ) {
        ALOGE("%s, mmap failed %s",__func__,strerror(errno));
        return -1;
    }
    pga_gain_nv->out_devices = out_dev;
    pga_gain_nv->in_devices = in_dev;

    //get music gain from nv
    ret = GetAudio_PaConfig_nv(adev, &aud_params_ptr[lmode], pga_gain_nv);
    if(ret < 0){
        return -1;
    }
    return 0;
}

static int GetAudio_gain_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv, uint32_t vol_level,int out_dev,int in_dev)
{
    int ret = 0;
    int32_t outmode = 0, inmode=0;
    AUDIO_TOTAL_T * aud_params_ptr = NULL;
    char * dev_name = NULL;
    outmode = GetAudio_mode_number_from_device(out_dev,in_dev);
    inmode = GetAudio_InMode_number_from_device(in_dev);

    ALOGW("%s, outmode:%d inmode:%d,indev:0x%x, outdev:0x%x",
        __func__,outmode, inmode, adev->in_devices, adev->out_devices);

    aud_params_ptr = adev->audio_para;//(AUDIO_TOTAL_T *)mmap(0, 4*sizeof(AUDIO_TOTAL_T),PROT_READ,MAP_SHARED,fd,0);
    if ( NULL == aud_params_ptr ) {
        ALOGE("%s, mmap failed %s",__func__,strerror(errno));
        return -1;
    }
    pga_gain_nv->out_devices = out_dev;
    pga_gain_nv->in_devices = in_dev;

    //get music gain from nv
    ret = GetAudio_outpga_nv(adev, &aud_params_ptr[outmode], pga_gain_nv, vol_level);
    if(ret < 0){
        ALOGE("%s, GetAudio_outpga_nv fail",__func__);
        return -1;
    }

    if (adev->fm_open) {
        //if outmode is handset or headfree, use handsfree mode incase.
        if (outmode == 1 || outmode == 2) {
            outmode = 3;
        }
        ret = GetAudio_inpga_nv(adev, &aud_params_ptr[outmode], pga_gain_nv, vol_level);
    } else {
        ret = GetAudio_inpga_nv(adev, &aud_params_ptr[inmode], pga_gain_nv, vol_level);
    }
    if(ret < 0){
        ALOGE("%s, GetAudio_inpga_nv fail",__func__);
        return -1;
    }
    return 0;
}

static int SetVoice_PaConfig_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv)
{
    if(NULL == pga_gain_nv){
        ALOGE("%s pga_gain_nv NULL",__func__);
        return -1;
    }

    if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER) && ((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE))){
        mixer_ctl_set_value(adev->private_ctl.internal_pa, 0, pga_gain_nv->voice_pa_config);
        mixer_ctl_set_value(adev->private_ctl.internal_hp_pa, 0, pga_gain_nv->voice_hp_pa_config);
    }else{
        if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER){
            mixer_ctl_set_value(adev->private_ctl.internal_pa, 0, pga_gain_nv->voice_pa_config);
        }
        if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
            mixer_ctl_set_value(adev->private_ctl.internal_hp_pa, 0, pga_gain_nv->voice_hp_pa_config);
        }
    }
    ALOGW("%s out, out_devices:0x%x, in_devices:0x%x", __func__, pga_gain_nv->out_devices, pga_gain_nv->in_devices);
    return 0;
}

static int SetVoice_gain_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv)
{
    if(NULL == pga_gain_nv){
        ALOGE("%s pga_gain_nv NULL",__func__);
        return -1;
    }
    if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_EARPIECE){
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"voice-earpiece");
    }
    if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER) && ((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE))){
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"voice-headphone-spk-l");
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"voice-headphone-spk-r");
        audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_l,"cg-pga-gain-l");
        audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_r,"cg-pga-gain-r");
    }else{
        if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER){
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"voice-speaker-l");
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"voice-speaker-r");
        }
        if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"voice-headphone-l");
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"voice-headphone-r");
            audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_l,"cg-pga-gain-l");
            audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_r,"cg-pga-gain-r");
        }
    }
    if((pga_gain_nv->in_devices & AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN) || (pga_gain_nv->in_devices & AUDIO_DEVICE_IN_BACK_MIC& ~AUDIO_DEVICE_BIT_IN) || (pga_gain_nv->in_devices & AUDIO_DEVICE_IN_WIRED_HEADSET& ~AUDIO_DEVICE_BIT_IN)){
        audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_l,"voice-capture-l");
        audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_r,"voice-capture-r");
    }
    ALOGW("%s out, out_devices:0x%x, in_devices:0x%x", __func__, pga_gain_nv->out_devices, pga_gain_nv->in_devices);
    return 0;
}

static int SetAudio_gain_fmradio(struct tiny_audio_device *adev, uint32_t gain)
{
    audio_pga_apply(adev->pga,gain,"digital-fm");
    return 0;
}

static int SetAudio_PaConfig_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv)
{
    if(NULL == pga_gain_nv){
        ALOGE("%s pga_gain_nv NULL",__func__);
        return -1;
    }
    if(adev->fm_open){
        ALOGI("%s fm device ",__func__);
        if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET ||
                        pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE){
            mixer_ctl_set_value(adev->private_ctl.internal_hp_pa, 0, pga_gain_nv->fm_hp_pa_config);
            mixer_ctl_set_value(adev->private_ctl.internal_hp_pa_delay, 0, pga_gain_nv->hp_pa_delay_config);
        }else if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER){
            mixer_ctl_set_value(adev->private_ctl.internal_pa, 0, pga_gain_nv->fm_pa_config);
        }
        ALOGW("%s out, out_devices:0x%x, in_devices:0x%x", __func__,pga_gain_nv->out_devices, pga_gain_nv->in_devices);
        return 0;
    }

    if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER) && ((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE))){
        mixer_ctl_set_value(adev->private_ctl.internal_pa, 0, pga_gain_nv->pa_config);
        mixer_ctl_set_value(adev->private_ctl.internal_hp_pa, 0, pga_gain_nv->hp_pa_config);
        mixer_ctl_set_value(adev->private_ctl.internal_hp_pa_delay, 0, pga_gain_nv->hp_pa_delay_config);
    }else{
        if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER){
            mixer_ctl_set_value(adev->private_ctl.internal_pa, 0, pga_gain_nv->pa_config);
        }
        if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
            mixer_ctl_set_value(adev->private_ctl.internal_hp_pa, 0, pga_gain_nv->hp_pa_config);
            mixer_ctl_set_value(adev->private_ctl.internal_hp_pa_delay, 0, pga_gain_nv->hp_pa_delay_config);
        }
    }
    ALOGW("%s out, out_devices:0x%x, in_devices:0x%x", __func__,pga_gain_nv->out_devices, pga_gain_nv->in_devices);
    return 0;
}

static int SetAudio_gain_by_devices(struct tiny_audio_device *adev, pga_gain_nv_t *pga_gain_nv)
{
    if(NULL == pga_gain_nv){
        ALOGE("%s pga_gain_nv NULL",__func__);
        return -1;
    }
    if(adev->fm_open){
        ALOGI("%s,fm device",__func__);
        if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET ||
                        pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE){
            audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_l,"linein-hp-l");
            audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_r,"linein-hp-r");
            audio_pga_apply(adev->pga,pga_gain_nv->fm_cg_pga_gain_l,"cg-pga-gain-l");
            audio_pga_apply(adev->pga,pga_gain_nv->fm_cg_pga_gain_r,"cg-pga-gain-r");
        }else if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER){
            audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_l,"linein-spk-l");
            audio_pga_apply(adev->pga,pga_gain_nv->fm_pga_gain_r,"linein-spk-r");
        }
        if(((pga_gain_nv->in_devices & AUDIO_DEVICE_IN_BUILTIN_MIC) || (pga_gain_nv->in_devices & AUDIO_DEVICE_IN_BACK_MIC) || (pga_gain_nv->in_devices & AUDIO_DEVICE_IN_WIRED_HEADSET)) && adev->fm_record){
            audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_l,"fm-capture-l");
            audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_r,"fm-capture-r");
        }
        ALOGW("%s out, out_devices:0x%x, in_devices:0x%x", __func__,pga_gain_nv->out_devices, pga_gain_nv->in_devices);
        return 0;
    }
    if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_EARPIECE){
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"earpiece");
    }
    if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER) && ((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE))){
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"headphone-spk-l");
        audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"headphone-spk-r");
        audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_l,"cg-pga-gain-l");
        audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_r,"cg-pga-gain-r");
    }else{
        if(pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_SPEAKER){
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"speaker-l");
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"speaker-r");
        }
        if((pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) || (pga_gain_nv->out_devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE)){
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_l,"headphone-l");
            audio_pga_apply(adev->pga,pga_gain_nv->dac_pga_gain_r,"headphone-r");
            audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_l,"cg-pga-gain-l");
            audio_pga_apply(adev->pga,pga_gain_nv->cg_pga_gain_r,"cg-pga-gain-r");
        }
    }
    if((pga_gain_nv->in_devices & AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN) || (pga_gain_nv->in_devices & AUDIO_DEVICE_IN_BACK_MIC & ~AUDIO_DEVICE_BIT_IN) || (pga_gain_nv->in_devices & AUDIO_DEVICE_IN_WIRED_HEADSET & ~AUDIO_DEVICE_BIT_IN)){
        audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_l,"capture-l");
        audio_pga_apply(adev->pga,pga_gain_nv->adc_pga_gain_r,"capture-r");
    }
    ALOGW("%s out, out_devices:0x%x, in_devices:0x%x", __func__,pga_gain_nv->out_devices, pga_gain_nv->in_devices);
    return 0;
}

static void SetAudio_gain_route(struct tiny_audio_device *adev, uint32_t vol_level,int out_dev,int in_dev)
{
    int ret = 0;
    memset(adev->pga_gain_nv,0,sizeof(pga_gain_nv_t));
    ret = GetAudio_gain_by_devices(adev,adev->pga_gain_nv,vol_level,out_dev,in_dev);
    if(ret < 0){
        return;
    }
    ret = SetAudio_gain_by_devices(adev,adev->pga_gain_nv);
    if(ret < 0){
        return;
    }
}

/* DSP read only one adc channel, all ADCL should be closed */
static void close_adc_channel(struct mixer *amixer, bool main, bool aux, bool hp)
{
    struct mixer_ctl *mic_adcl = NULL;
    if (amixer) {
        if (main) {
            mic_adcl = mixer_get_ctl_by_name(amixer, KCTL_MAIN_MIC_ADCL);
            mixer_ctl_set_value(mic_adcl, 0, 0);
        }
        if (aux) {
            mic_adcl = mixer_get_ctl_by_name(amixer, KCTL_AUX_MIC_ADCL);
            mixer_ctl_set_value(mic_adcl, 0, 0);
        }
        if (hp) {
            mic_adcl = mixer_get_ctl_by_name(amixer, KCTL_HP_MIC_ADCL);
            mixer_ctl_set_value(mic_adcl, 0, 0);
        }
    }
}

static void GetCall_VolumePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
    int ret = 0;
    pga_gain_nv_t *pga_gain_nv = NULL;
    if(NULL == mode_gain_paras){
        ret = -1;
        ALOGE("%s mode paras is NULL!!",__func__);
        return;
    }
    if(NULL==adev){
        ret = -1;
        ALOGE("%s adev paras is NULL!!",__func__);
        return;
    }
    pga_gain_nv = adev->pga_gain_nv;
    if(NULL == pga_gain_nv){
        ret = -1;
        ALOGE("%s pga_gain_nv paras is NULL!!",__func__);
        return;
    }

    pga_gain_nv->out_devices = s_call_dl_devices;
    pga_gain_nv->in_devices = s_call_ul_devices;
    pga_gain_nv->mode = adev->mode;
    pga_gain_nv->adc_pga_gain_l= mode_gain_paras->adc_gain & 0x00ff;
    pga_gain_nv->adc_pga_gain_r= (mode_gain_paras->adc_gain & 0xff00) >> 8;
    pga_gain_nv->dac_pga_gain_l= mode_gain_paras->dac_gain & 0x000000ff;
    pga_gain_nv->dac_pga_gain_r= (mode_gain_paras->dac_gain & 0x0000ff00) >> 8;
    pga_gain_nv->voice_pa_config   =
        (mode_gain_paras->pa_setting | (mode_gain_paras->reserved[0]<<16));
    pga_gain_nv->voice_hp_pa_config  =
        (mode_gain_paras->reserved[1] | (mode_gain_paras->reserved[2]<<16));
    pga_gain_nv->cg_pga_gain_l = mode_gain_paras->reserved[3];
    pga_gain_nv->cg_pga_gain_r = mode_gain_paras->reserved[4];
    
    ret = SetVoice_gain_by_devices(adev,pga_gain_nv);
    if(ret < 0){
        return;
    }

    ALOGW("SetCall_VolumePara hp_pa_config: 0x%x, 0x%x, 0x%x,pa_config: 0x%x, 0x%x, 0x%x.",
        pga_gain_nv->voice_hp_pa_config, mode_gain_paras->reserved[1], mode_gain_paras->reserved[2],
        pga_gain_nv->voice_pa_config, mode_gain_paras->pa_setting, mode_gain_paras->reserved[0]);

    ALOGW("%s successfully ,dac_pga_gain_l:0x%x ,dac_pga_gain_r:0x%x ,adc_pga_gain_l:0x%x ,adc_pga_gain_r:0x%x, devices:0x%x ,mode:%d ",
        __func__,pga_gain_nv->dac_pga_gain_l,pga_gain_nv->dac_pga_gain_r,pga_gain_nv->adc_pga_gain_l,
        pga_gain_nv->adc_pga_gain_r,pga_gain_nv->out_devices,adev->mode);
}

static void SetCall_ModePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
    unsigned short i = 0;
    unsigned short switch_earpice = 0;
    unsigned short switch_headset = 0;
    unsigned short switch_speaker = 0;
    unsigned short switch_mic0 = 0;
    unsigned short switch_mic1 = 0;
    unsigned short switch_hp_mic = 0;
    unsigned short switch_table[7] = {0};
    uint32_t android_pre_device = 0x0;
    uint32_t switch_device[] = {AUDIO_DEVICE_OUT_EARPIECE,AUDIO_DEVICE_OUT_SPEAKER,AUDIO_DEVICE_IN_BUILTIN_MIC,AUDIO_DEVICE_IN_BACK_MIC,AUDIO_DEVICE_IN_WIRED_HEADSET,AUDIO_DEVICE_OUT_WIRED_HEADSET,SPRD_AUDIO_IN_DUALMIC_VOICE};

    MY_TRACE("%s path_set:0x%x ,dl_device:0x%x ", __func__,mode_gain_paras->path_set,s_call_dl_devices);
    switch_earpice = (mode_gain_paras->path_set & 0x0040)>>6;
    switch_headset = mode_gain_paras->path_set & 0x0001;
    switch_speaker = (mode_gain_paras->path_set & 0x0008)>>3;
    switch_mic0 = (mode_gain_paras->path_set & 0x0400)>>10;     //AUDIO_DEVICE_IN_BUILTIN_MIC
    switch_mic1 = (mode_gain_paras->path_set & 0x0800)>>11;     //AUDIO_DEVICE_IN_BACK_MIC
    switch_hp_mic = (mode_gain_paras->path_set & 0x1000)>>12;

    /*need to check the type of headset(3-pole or 4-pole) when wants to open hp-mic*/
    if(switch_hp_mic == 1){
        if(headset_no_mic()){
            MY_TRACE("%s 3-pole headset, need to open main-mic instead of hp-mic ",__func__);
            switch_mic0 = 1;    /*open main-mic*/
            switch_hp_mic = 0;
        }
    }

    switch_table[0] = switch_earpice;
    switch_table[1] = switch_speaker;
    switch_table[2] = switch_mic0;
    switch_table[3] = switch_mic1;
    switch_table[4] = switch_hp_mic;
    switch_table[5] = switch_headset;
    switch_table[6] = 0;

    //At present, switch of pa cannot handle mulit-device
    android_pre_device = s_call_dl_devices|s_call_ul_devices;
    s_call_dl_devices = 0;
    s_call_ul_devices = 0;
    if(switch_earpice){
        s_call_dl_devices |= AUDIO_DEVICE_OUT_EARPIECE;
    }
    if(switch_speaker){
        s_call_dl_devices |= AUDIO_DEVICE_OUT_SPEAKER;
    }
    if(switch_headset){
        s_call_dl_devices |= AUDIO_DEVICE_OUT_WIRED_HEADSET;
    }
    if(switch_mic0){
        s_call_ul_devices |= AUDIO_DEVICE_IN_BUILTIN_MIC;
    }
    if(switch_mic1){
        s_call_ul_devices |= AUDIO_DEVICE_IN_BACK_MIC;
    }
    if(switch_mic0 && switch_mic1) {
        s_call_ul_devices |= SPRD_AUDIO_IN_DUALMIC_VOICE;
        switch_table[6] = 1;
    }
    if(switch_hp_mic){
        s_call_ul_devices |= AUDIO_DEVICE_IN_WIRED_HEADSET;
    }

    GetCall_VolumePara(adev, mode_gain_paras);

    SetVoice_PaConfig_by_devices(adev,adev->pga_gain_nv);

    for(i=0; i<(sizeof(switch_table)/sizeof(unsigned short));i++)
    {
        if(switch_table[i]){
            set_call_route(adev,switch_device[i],1);
        }
    }
    for(i=0; i<(sizeof(switch_table)/sizeof(unsigned short));i++)
    {
        if(!switch_table[i]){
#ifdef _DSP_CTRL_CODEC      //if dsp control codec, we cann't close headset.
            if(i == 5 || i == 4){
                continue;
            }
#endif
            set_call_route(adev,switch_device[i],0);
        }
    }

    if (!(switch_mic0 && switch_mic1)) {
        close_adc_channel(adev->mixer, switch_mic0, switch_mic1, switch_hp_mic);
    }
    /*
       we need to wait for codec here before call connected, maybe driver needs to fix this problem.
       if android_pre_device = 0, means   I2S --> CODEC
       */
    if(!adev->call_connected || (android_pre_device == 0)){
        usleep(1000*100);
    }
    ALOGW("%s successfully, device: earpice(%s), headphone(%s), speaker(%s), Main_Mic(%s), Back_Mic(%s), hp_mic(%s)"
            ,__func__,switch_earpice ? "Open":"Close",switch_headset ? "Open":"Close",switch_speaker ? "Open":"Close",
            switch_mic0 ? "Open":"Close",switch_mic1 ? "Open":"Close",switch_hp_mic ? "Open":"Close");
}

static void SetCall_VolumePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
    int ret = 0;
    if(NULL == mode_gain_paras){
        ret = -1;
        ALOGE("%s mode paras is NULL!!",__func__);
        return;
    }
    GetCall_VolumePara(adev, mode_gain_paras);
    ret = SetVoice_gain_by_devices(adev,adev->pga_gain_nv);
    if(ret < 0){
        return;
    }
}

int SetParas_OpenHal_Incall(struct tiny_audio_device *adev, int fd_pipe)	//Get open hal cmd and sim card
{
    int ret = 0;
    open_hal_t hal_open_param;
    parameters_head_t read_common_head;
    memset(&hal_open_param,0,sizeof(open_hal_t));
    memset(&read_common_head, 0, sizeof(parameters_head_t));

    ret = ReadParas_OpenHal(fd_pipe,&hal_open_param);
    if (ret <= 0) {
        ALOGE("Error, read %s ret(%d) failed(%s).",__func__,ret,strerror(errno));
    }
    android_sim_num = hal_open_param.sim_card;
    MY_TRACE("%s successfully,sim card number(%d)",__func__,android_sim_num);
    return ret;
}

int GetParas_DeviceCtrl_Incall(int fd_pipe,device_ctrl_t *device_ctrl_param)	//open,close
{
    int ret = 0;
    MY_TRACE("%s in... ",__func__);
    ret = ReadParas_DeviceCtrl(fd_pipe,device_ctrl_param);
    if (ret <= 0) {
        ALOGE("Error, read %s ret(%d) failed(%s).",__func__,ret,strerror(errno));
    }
    if((!device_ctrl_param->paras_mode.is_mode) || (!device_ctrl_param->paras_mode.is_volume)){	//check whether is setDevMode
        ret =-1;
        ALOGE("Error: %s,ReadParas_DeviceCtrl wrong cmd_type.",__func__);
        return ret;
    }
    MY_TRACE("%s successfully ,is_open(%d) is_headphone(%d) is_downlink_mute(%d) is_uplink_mute(%d) volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x) ",__func__,device_ctrl_param->is_open,device_ctrl_param->is_headphone, \
        device_ctrl_param->is_downlink_mute,device_ctrl_param->is_uplink_mute,device_ctrl_param->paras_mode.volume_index,device_ctrl_param->paras_mode.adc_gain,device_ctrl_param->paras_mode.path_set,device_ctrl_param->paras_mode.dac_gain,device_ctrl_param->paras_mode.pa_setting);
    return ret;
}

int GetParas_Route_Incall(int fd_pipe,paras_mode_gain_t *mode_gain_paras)	//set_volume & set_route
{
    int ret = 0;
    parameters_head_t read_common_head;
    memset(&read_common_head, 0, sizeof(parameters_head_t));
    MY_TRACE("%s in...",__func__);
    ret = ReadParas_ModeGain(fd_pipe,mode_gain_paras);
    if (ret <= 0) {
        ALOGE("Error, read %s ret(%d) failed(%s).",__func__,ret,strerror(errno));
        return ret;
    }
    if((!mode_gain_paras->is_mode)){	//check whether is setDevMode
        ret =-1;
        ALOGE("Error: %s ReadParas_ModeGain wrong cmd_type.",__func__);
        return ret;
    }
    MY_TRACE("%s successfully,volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x)",__func__,mode_gain_paras->volume_index,mode_gain_paras->adc_gain, \
        mode_gain_paras->path_set, mode_gain_paras->dac_gain,mode_gain_paras->pa_setting);
    return ret;
}

int GetParas_Volume_Incall(int fd_pipe,paras_mode_gain_t *mode_gain_paras)	//set_volume & set_route
{
    int ret = 0;
    parameters_head_t read_common_head;
    memset(&read_common_head, 0, sizeof(parameters_head_t));
    MY_TRACE("%s in...",__func__);

    ret = ReadParas_ModeGain(fd_pipe,mode_gain_paras);
    if (ret <= 0) {
        ALOGE("Error, read %s ret(%d) failed(%s).",__func__,ret,strerror(errno));
    }
    if((!mode_gain_paras->is_volume)){	//check whether is setDevMode
        ret =-1;
        ALOGE("Error: %s ReadParas_ModeGain wrong cmd_type.",__func__);
        return ret;
    }
    MY_TRACE("%s successfully,volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x)",__func__,mode_gain_paras->volume_index,mode_gain_paras->adc_gain, \
        mode_gain_paras->path_set, mode_gain_paras->dac_gain, mode_gain_paras->pa_setting);
    return ret;
}

int GetParas_Switch_Incall(int fd_pipe,switch_ctrl_t *swtich_ctrl_paras)	/* switch vbc contrl to dsp.*/
{
    int ret = 0;
    parameters_head_t read_common_head;
    memset(&read_common_head, 0, sizeof(parameters_head_t));
    MY_TRACE("%s in...",__func__);

    ret = ReadParas_SwitchCtrl(fd_pipe,swtich_ctrl_paras);
    if (ret <= 0) {
        ALOGE("Error, read ReadParas_SwitchCtrl ret(%d) failed(%s)",ret,strerror(errno));
    }
    MY_TRACE("%s successfully ,is_switch(%d) ",__func__,swtich_ctrl_paras->is_switch);
    return ret;
}

int GetParas_Samplerate_Incall(int fd_pipe,set_samplerate_t *set_samplerate_paras)	/* set samplerate*/
{
    int ret = 0;
    parameters_head_t read_common_head;
    memset(&read_common_head, 0, sizeof(parameters_head_t));
    MY_TRACE("%s in...",__func__);

    ret = ReadParas_SetSamplerate(fd_pipe,set_samplerate_paras);
    if (ret <= 0) {
        ALOGE("Error, read ReadParas_SetSamplerate ret(%d) failed(%s)",ret,strerror(errno));
    }
    if(set_samplerate_paras->samplerate <= 0){
        ALOGW("Error, get wrong samplerate(%d),set default ",set_samplerate_paras->samplerate);
        set_samplerate_paras->samplerate = VX_NB_SAMPLING_RATE; //8k
    }
    MY_TRACE("%s successfully ,samplerate(%d) ",__func__,set_samplerate_paras->samplerate);
    return ret;
}


int SetParas_Route_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
    int ret = 0;
    unsigned short switch_earpice = 0;
    unsigned short switch_headset = 0;
    unsigned short switch_speaker = 0;
    unsigned short switch_mic0 = 0;
    unsigned short switch_mic1 = 0;
    unsigned short switch_hp_mic = 0;
    paras_mode_gain_t mode_gain_paras;
    memset(&mode_gain_paras,0,sizeof(paras_mode_gain_t));
    MY_TRACE("%s in.....",__func__);
    ret = GetParas_Route_Incall(fd_pipe,&mode_gain_paras);
    if(ret < 0){
        return ret;
    }
    SetCall_ModePara(adev,&mode_gain_paras);
    SetCall_VolumePara(adev,&mode_gain_paras);
    return ret;
}

int SetParas_Volume_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
    int ret = 0;
    paras_mode_gain_t mode_gain_paras;
    memset(&mode_gain_paras,0,sizeof(paras_mode_gain_t));
    MY_TRACE("%s in.....",__func__);
    ret = GetParas_Volume_Incall(fd_pipe,&mode_gain_paras);
    if(ret < 0){
        return ret;
    }
    SetCall_VolumePara(adev,&mode_gain_paras);
    return ret;
}

int SetParas_Switch_Incall(int fd_pipe,int vbchannel_id,struct tiny_audio_device *adev)
{
    int ret = 0;
    parameters_head_t write_common_head;
    switch_ctrl_t swtich_ctrl_paras;
    memset(&swtich_ctrl_paras,0,sizeof(swtich_ctrl_paras));
    MY_TRACE("%s in...",__func__);
    ret = GetParas_Switch_Incall(fd_pipe,&swtich_ctrl_paras);
    if(ret < 0){
        return ret;
    }

    mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, vbchannel_id);
    ALOGW("%s, vbchannel_id : %d , VBC %s dsp...",__func__,vbchannel_id,(swtich_ctrl_paras.is_switch)?"Switch control to":"Get control back from");

    return ret;
}

int SetParas_DeviceCtrl_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
    int ret = 0;
    device_ctrl_t device_ctrl_paras;
    memset(&device_ctrl_paras,0,sizeof(device_ctrl_t));
    MY_TRACE("%s in.....",__func__);

    //because of codec,we should set headphone on first if codec is controlled by dsp
#ifdef _DSP_CTRL_CODEC
    set_call_route(adev, AUDIO_DEVICE_OUT_WIRED_HEADSET, 1);
#endif


    ret =GetParas_DeviceCtrl_Incall(fd_pipe,&device_ctrl_paras);
    if(ret < 0){
        return ret;
    }

    //set arm mode paras
    if(device_ctrl_paras.is_open){
        SetCall_ModePara(adev,&device_ctrl_paras.paras_mode);
        SetCall_VolumePara(adev,&device_ctrl_paras.paras_mode);
    }else{
        ALOGW("%s close device...",__func__);
    }
    return ret;
}

int SetParas_Samplerate_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
    int ret = 0;
    set_samplerate_t device_set_samplerate;
    memset(&device_set_samplerate,0,sizeof(set_samplerate_t));
    MY_TRACE("%s in.....",__func__);

    ret = GetParas_Samplerate_Incall(fd_pipe,&device_set_samplerate);
	    LOG_I("device_set_samplerate_>samplerate is  %d ",device_set_samplerate.samplerate);
    if(ret < 0){
        return ret;
    }

    pthread_mutex_lock(&adev->vbc_dlulock);
    if(adev->pcm_modem_dl){
        ret = pcm_set_samplerate(adev->pcm_modem_dl, PCM_OUT, &pcm_config_vx, device_set_samplerate.samplerate);
        if(ret < 0){
            pthread_mutex_unlock(&adev->vbc_dlulock);
            return ret;
        }
        ret = pcm_set_samplerate(adev->pcm_modem_ul, PCM_IN, &pcm_config_vrec_vx, device_set_samplerate.samplerate);
        if(ret < 0){
            pthread_mutex_unlock(&adev->vbc_dlulock);
            return ret;
        }
     }
     pthread_mutex_unlock(&adev->vbc_dlulock);
    return ret;
}

void vbc_ctrl_init(struct tiny_audio_device *adev)
{
    char prop_t[PROPERTY_VALUE_MAX] = {0};
    char prop_w[PROPERTY_VALUE_MAX] = {0};
    bool csfb_enable = false;
    bool t_enable = false;
    bool w_enalbe = false;
    int i=0;
    bool result=false;
    vbc_ctrl_thread_para_t* vbc_ctrl_index = NULL;
    if(property_get(MODEM_T_ENABLE_PROPERTY, prop_t, "") && 0 == strcmp(prop_t, "1") )
    {
        MY_TRACE("%s:%s", __func__, MODEM_T_ENABLE_PROPERTY);
        t_enable = true;
    }
    if(property_get(MODEM_W_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
    {
        MY_TRACE("%s:%s", __func__, MODEM_W_ENABLE_PROPERTY);
        w_enalbe = true;
    }
    if(property_get(MODEM_TDDCSFB_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
    {
        MY_TRACE("%s:%s", __func__, MODEM_TDDCSFB_ENABLE_PROPERTY);
        csfb_enable = true;
    }
    if(property_get(MODEM_FDDCSFB_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
    {
        MY_TRACE("%s:%s", __func__, MODEM_FDDCSFB_ENABLE_PROPERTY);
        csfb_enable = true;
    }
    if(property_get(MODEM_CSFB_ENABLE_PROPERTY, prop_w, "") && 0 == strcmp(prop_w, "1"))
    {
        MY_TRACE("%s:%s", __func__, MODEM_FDDCSFB_ENABLE_PROPERTY);
        csfb_enable = true;
    }


	for(i=0;i<adev->cp->num;i++)
	{

		if(csfb_enable && (adev->cp->vbc_ctrl_pipe_info+i)->cp_type == CP_CSFB){
		       MY_TRACE("%s:found CP_CSFB", __func__);
			s_vbc_pipe_count++;
			continue;
		}
		if(w_enalbe && (adev->cp->vbc_ctrl_pipe_info+i)->cp_type == CP_W){
		       MY_TRACE("%s:found W", __func__);
			s_vbc_pipe_count++;
			continue;
		}
		if(t_enable && (adev->cp->vbc_ctrl_pipe_info+i)->cp_type == CP_TG){
			 MY_TRACE("%s:found TG", __func__);
			s_vbc_pipe_count++;
			continue;
			}

	}



    if(adev->cp && s_vbc_pipe_count)
    {
        st_vbc_ctrl_thread_para = malloc(s_vbc_pipe_count *
            sizeof(vbc_ctrl_thread_para_t));
        if(!st_vbc_ctrl_thread_para)
        {
            MY_TRACE("error, vbc_ctrl_init malloc para failed,%d",s_vbc_pipe_count);
        }
        s_vbc_ctrl_thread_id = malloc(s_vbc_pipe_count *
            sizeof(pthread_t));
        if(!s_vbc_ctrl_thread_id)
        {
            MY_TRACE("error, vbc_ctrl_init malloc id failed,%d",s_vbc_pipe_count);
        }
        //initialize vbc pipe information
        vbc_ctrl_index = st_vbc_ctrl_thread_para;
        for(i=0;i<adev->cp->num;i++)
        {
            if((t_enable && (adev->cp->vbc_ctrl_pipe_info+i)->cp_type == CP_TG) ||
                (w_enalbe && (adev->cp->vbc_ctrl_pipe_info+i)->cp_type == CP_W) ||
                    (csfb_enable && (adev->cp->vbc_ctrl_pipe_info+i)->cp_type == CP_CSFB))
            {
                if(vbc_ctrl_index)
                {
                    memcpy(vbc_ctrl_index->vbpipe, (adev->cp->vbc_ctrl_pipe_info+i)->s_vbc_ctrl_pipe_name, VBC_PIPE_NAME_MAX_LEN);
                    vbc_ctrl_index->vbchannel_id = (adev->cp->vbc_ctrl_pipe_info+i)->channel_id;
                    vbc_ctrl_index->cp_type = (adev->cp->vbc_ctrl_pipe_info+i)->cp_type;
                    vbc_ctrl_index->adev = adev;
                    vbc_ctrl_index->vbpipe_fd = -1;
                    vbc_ctrl_index->vbpipe_pre_fd = -1;
                    vbc_ctrl_index++;
                    result = true;
                }
            }
        }
    }

    if(!result)
    {
        MY_TRACE("warning: no ro.modem.x.enable,apply default modem profile");
        if(st_vbc_ctrl_thread_para)
        {
            free(st_vbc_ctrl_thread_para);
        }
        if(s_vbc_ctrl_thread_id)
        {
            free(s_vbc_ctrl_thread_id);
        }
        s_vbc_pipe_count = 1;
        st_vbc_ctrl_thread_para = malloc(sizeof(vbc_ctrl_thread_para_t));
        if(!st_vbc_ctrl_thread_para)
        {
            MY_TRACE("error, vbc_ctrl_init malloc para failed,%d",s_vbc_pipe_count);
        }
        s_vbc_ctrl_thread_id = malloc(sizeof(pthread_t));
        if(!s_vbc_ctrl_thread_id)
        {
            MY_TRACE("error, vbc_ctrl_init malloc id failed,%d",s_vbc_pipe_count);
        }
        //initialize vbc pipe information
        if(st_vbc_ctrl_thread_para)
        {
            memcpy(st_vbc_ctrl_thread_para->vbpipe, s_default_vbc_ctrl_pipe_info.s_vbc_ctrl_pipe_name, VBC_PIPE_NAME_MAX_LEN);
            st_vbc_ctrl_thread_para->vbchannel_id = s_default_vbc_ctrl_pipe_info.channel_id;
            st_vbc_ctrl_thread_para->cp_type = s_default_vbc_ctrl_pipe_info.cp_type;
            st_vbc_ctrl_thread_para->adev = adev;
            st_vbc_ctrl_thread_para->vbpipe_fd = -1;
            st_vbc_ctrl_thread_para->vbpipe_pre_fd = -1;
        }
    }

    MY_TRACE("%s:enable modem :%d",__func__,s_vbc_pipe_count);
}


int vbc_ctrl_open(struct tiny_audio_device *adev)
{
    if (s_is_active) return (-1);
    int rc,i=0,j=0;

    MY_TRACE("%s IN.",__func__);
    s_is_active = 1;
    vbc_ctrl_init(adev);
    while(i<s_vbc_pipe_count)
    {
        rc = pthread_create((pthread_t *)(s_vbc_ctrl_thread_id+i), NULL,
            vbc_ctrl_thread_routine, (void *)(st_vbc_ctrl_thread_para+i));
        if (rc) {
            ALOGE("error, pthread_create failed, rc=%d, i:%d", rc, i);
            while(j<i)
            {
                //need to delete the threads.
                //pthread_cancel (s_vbc_ctrl_thread_id[j]);
                j++;
            }
            s_is_active = 0;
            return (-1);
        }
        i++;
    }
    return (0);
}

int vbc_ctrl_close()
{
    if (!s_is_active) return (-1);
    MY_TRACE("%s IN.",__func__);

    int i=0;
    s_is_active = 0;
    /* close vbpipe.*/
    while(i<s_vbc_pipe_count)
    {
        close((st_vbc_ctrl_thread_para+i)->vbpipe_fd);
        (st_vbc_ctrl_thread_para+i)->vbpipe_fd = -1;
        (st_vbc_ctrl_thread_para+i)->vbpipe_pre_fd = -1;
        i++;
    }

    free(s_vbc_ctrl_thread_id);
    free(st_vbc_ctrl_thread_para);

    /* terminate thread.*/
    //pthread_cancel (s_vbc_ctrl_thread);    
    return (0);
}

int vbc_ctrl_voip_open(struct voip_res *res)
{
    int rc;

    if(!res) {
        return -1;
    }
   if(!res->enable){
        ALOGD("voip is not enable");
    }

    rc = pthread_create((pthread_t *)&(res->thread_id), NULL,
            vbc_ctrl_voip_thread_routine, (void *)res);
    if (rc) {
        ALOGE("error,voip pthread_create failed, rc=%d", rc);
        return -1;
    }

    return 0;
} 

static int vbc_call_end_process(struct tiny_audio_device *adev,int is_timeout)
{
    ALOGW("voice:vbc_call_end_process in");
    pthread_mutex_lock(&adev->lock);
    ALOGW("voice:vbc_call_end_process, got lock");
    if(adev->call_start){
        force_all_standby(adev);
        mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, VBC_ARM_CHANNELID);  //switch vbc to arm

        pthread_mutex_lock(&adev->vbc_dlulock);
        if(adev->pcm_modem_ul) {
            pcm_close(adev->pcm_modem_ul);
            adev->pcm_modem_ul = NULL;
        }
        if(adev->pcm_modem_dl) {
            pcm_close(adev->pcm_modem_dl);
            adev->pcm_modem_dl = NULL;
        }
        pthread_mutex_unlock(&adev->vbc_dlulock);

        adev->call_start = 0;
        adev->call_connected = 0;
        i2s_pin_mux_sel(adev,AP_TYPE);
    }
    if(is_timeout) {
		mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, VBC_ARM_CHANNELID);  //switch vbc to arm
		adev->call_prestop = 0;
		adev->call_start = 0;
                voip_forbid(adev,false);
    }else{
        voip_forbid_cancel(adev,CALL_END_TIMEOUT_SECONDS);
    }
    pthread_mutex_unlock(&adev->lock);
    ALOGW("voice:vbc_call_end_process out");
    return 0;
}

void *vbc_ctrl_voip_thread_routine(void *arg)
{
    int ret = 0;
    vbc_ctrl_thread_para_t     para_res = {0};
    vbc_ctrl_thread_para_t  *para = &para_res;
    struct voip_res * res = (struct voip_res *)arg;
    struct tiny_audio_device *adev;
    fd_set fds_read;
    struct timeval timeout = {5,0};
    struct timeval *cur_timeout = NULL;
    int maxfd;
    parameters_head_t read_common_head;
    parameters_head_t write_common_head;

    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio=39;
    
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_getschedparam(&attr,&m_param);
    m_param.sched_priority=newprio;
    pthread_attr_setschedparam(&attr,&m_param);

    para->cp_type = res->cp_type;
    para->vbchannel_id = res->channel_id;
    para->adev = res->adev;
    para->vbpipe_fd = -1;

    memcpy(para->vbpipe, res->pipe_name,strlen(res->pipe_name)+1); 

    adev = (struct tiny_audio_device *)(para->adev);

    memset(&read_common_head, 0, sizeof(parameters_head_t));
    memset(&write_common_head, 0, sizeof(parameters_head_t));
    
    memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = VBC_CMD_NONE;
    write_common_head.paras_size = 0;
    ALOGI("voip1:vbc_ctrl_thread_routine in pipe_name:%s.", para->vbpipe);
    
RESTART:
    /* open vbpipe to build connection.*/
    if (para->vbpipe_fd == -1) {
        para->vbpipe_fd = open(para->vbpipe, O_RDWR);//open("/dev/vbpipe6", O_RDWR);
        if (para->vbpipe_fd < 0) {
            ALOGE("vbc_ctrl_voip_thread_routine open fail, pipe_name:%s, %d.", para->vbpipe, errno);
            sleep(1);
            goto RESTART;
        } else {
            ALOGD("voip1:vbpipe_name(%s) vbpipe_fd(%d) open successfully.", para->vbpipe, para->vbpipe_fd);
        }
    }

    maxfd = para->vbpipe_fd + 1;
    if((fcntl(para->vbpipe_fd,F_SETFL,O_NONBLOCK))<0)
    {
        ALOGD("voip1:vbpipe_name(%s) vbpipe_fd(%d) fcntl error.", para->vbpipe, para->vbpipe_fd);
    }

    /* loop to read parameters from vbpipe.*/
    while(1)
    {
        int result = 0; 
        timeout.tv_sec = 5;;
        timeout.tv_usec = 0;;
        ALOGW("voip1:%s, looping now...cur_timeout %x,timeout %d", para->vbpipe,cur_timeout,cur_timeout?cur_timeout->tv_sec:0);

        FD_ZERO(&fds_read);
        FD_SET(para->vbpipe_fd,&fds_read);

        result = select(maxfd,&fds_read,NULL,NULL,cur_timeout);
        if(result < 0) {
            ALOGE("voip: select error %d",errno);
        vbc_call_end_process(adev,true);
        vbc_empty_pipe(para->vbpipe_fd);
        cur_timeout = NULL;
            continue;
        }
        else if(!result) {
            ALOGE("voip1: select timeout");
        vbc_call_end_process(adev,true);
        vbc_empty_pipe(para->vbpipe_fd);
        cur_timeout = NULL;
        continue;
        }
        if(FD_ISSET(para->vbpipe_fd,&fds_read) <= 0) {
            ALOGE("voip1: select ok but no fd is set");
            continue;
        }
        /* read parameters common head of the packet.*/
        ret = ReadParas_Head(para->vbpipe_fd, &read_common_head);
        if(ret < 0) {
            ALOGE("voip1:Error, %s read head failed(%s), need to read again ",__func__,strerror(errno));
            cur_timeout = NULL;
               continue;
            }

        ALOGD("voip1: get cmd ok");

        if (memcmp(&read_common_head.tag[0], VBC_CMD_TAG, 3)) {
            ALOGE("voip1:error read_common_head.tag is %d",read_common_head.tag[0]);
            vbc_call_end_process(adev,true);
            vbc_empty_pipe(para->vbpipe_fd);
            cur_timeout = NULL;
            continue;
        }

    ALOGD("voip1: Get CMD(%d) from cp(pipe:%s, pipe_fd:%d,paras_size:%d devices:0x%x mode:%d",
            read_common_head.cmd_type, para->vbpipe, para->vbpipe_fd, read_common_head.paras_size,adev->out_devices,adev->mode);

        switch (read_common_head.cmd_type)
        {
            case VBC_CMD_HAL_OPEN:
            {
                cur_timeout = &timeout;
#ifndef AUDIO_OLD_MODEM
                parameters_head_t write_common_head = {0};
#ifdef NXP_SMART_PA
		i2s_ctl_t * i2s_ctl_info = adev->i2s_btcall_info->i2s_ctl_info + adev->cp->vbc_ctrl_pipe_info->cpu_index;
		i2s_ctl_t * i2s_extspk_info =adev->cp->i2s_extspk + adev->cp->vbc_ctrl_pipe_info->cpu_index;
		ALOGE("peter_voip: index= %d,i2s_speaker_info->is_switch= %d,i2s_speaker_info->i2s_index=%d",adev->cp->vbc_ctrl_pipe_info->cpu_index,i2s_extspk_info->is_switch,i2s_extspk_info->i2s_index);
		uint32_t i2s_ctl = ((i2s_ctl_info->is_switch << 8) | (i2s_ctl_info->i2s_index << 0)
        |(i2s_extspk_info->is_switch << 9) | (i2s_extspk_info->i2s_index<< 4));
#else
        i2s_ctl_t * i2s_ctl_info = adev->i2s_btcall_info->i2s_ctl_info + adev->cp->vbc_ctrl_pipe_info->cpu_index;;
        uint32_t i2s_ctl = ((i2s_ctl_info->is_switch << 8) | (i2s_ctl_info->i2s_index << 0) );
        //  |(adev->cp->i2s_extspk.is_switch << 9) | (adev->cp->i2s_extspk.index << 4));
#endif
#endif
                MY_TRACE("voip1:VBC_CMD_HAL_OPEN IN.");
                SetParas_OpenHal_Incall(adev,para->vbpipe_fd); 
                adev->cp_type = para->cp_type;
                if(adev->out_devices & (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_ALL_SCO)) {
                    if(adev->cp_type == CP_TG)
                        i2s_pin_mux_sel(adev,1);
                    else if(adev->cp_type == CP_W)
                        i2s_pin_mux_sel(adev,0);
                    else if( adev->cp_type == CP_CSFB)
                        i2s_pin_mux_sel(adev,CP_CSFB);
                }
#ifndef AUDIO_OLD_MODEM
                write_common_head.cmd_type = VBC_CMD_RSP_OPEN;
                write_common_head.paras_size = i2s_ctl ;
                ret = WriteParas_Head(para->vbpipe_fd, &write_common_head);
#else
                ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_HAL_OPEN);
#endif
                if(ret < 0){
                    ALOGE("voip1:VBC_CMD_HAL_OPEN: write1 cmd VBC_CMD_RSP_CLOSE ret(%d) error(%s).",ret,strerror(errno));
                }
                MY_TRACE("voip1:VBC_CMD_HAL_OPEN OUT,vbpipe_name:%s.",para->vbpipe);
            }
            break;
            case VBC_CMD_HAL_CLOSE:
            {
               cur_timeout = &timeout;
                
                MY_TRACE("voip1:VBC_CMD_HAL_CLOSE IN.");
                write_common_head.cmd_type = VBC_CMD_RSP_CLOSE;
                ret = WriteParas_Head(para->vbpipe_fd, &write_common_head);
                if(ret < 0){
                    ALOGE("voip1:VBC_CMD_HAL_CLOSE: write1 cmd VBC_CMD_RSP_CLOSE ret(%d) error(%s).",ret,strerror(errno));
                }
                mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, VBC_ARM_CHANNELID);  //switch vbc to arm
                MY_TRACE("voip1:VBC_CMD_HAL_CLOSE OUT.");
            }
            break;
            case VBC_CMD_RSP_CLOSE:
            {
                MY_TRACE("voip1:VBC_CMD_RSP_CLOSE IN.");
                cur_timeout = NULL;
                ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_HAL_CLOSE);
                if(ret < 0){
                    ALOGE("voip1:VBC_CMD_RSP_CLOSE: write2 cmd VBC_CMD_RSP_CLOSE error(%d).",ret);
                }
#ifdef NXP_SMART_PA
                if(adev->callHandle)
                {
                    i2s_pin_mux_sel_new(adev, EXTSPK, AP_TYPE);
                    ALOGE("zhanpo_ 1 NxpTfa_Close in");
                    NxpTfa_Close(adev->callHandle);
                    adev->callHandle = NULL;
                }
#endif

                MY_TRACE("voip1:VBC_CMD_RSP_CLOSE OUT.");
            }
            break;
            case VBC_CMD_SET_MODE:
            {
                MY_TRACE("voip1:VBC_CMD_SET_MODE IN.");
#ifdef NXP_SMART_PA
                ALOGE("peter_voice: in");
                if(adev->routeDev & AUDIO_DEVICE_OUT_SPEAKER ) {
                ALOGE("peter_voice: 2");
                if(!adev->callHandle)
                    {
                        i2s_pin_mux_sel_new(adev, EXTSPK, AP_TYPE);
                        ALOGE("peter_voice: 4");
                        adev->callHandle = NxpTfa_Open(&NxpTfa9890_Call_Info);
                        ALOGE("peter_voice: 5");
                        i2s_pin_mux_sel_new(adev, EXTSPK, CP_CSFB);
                    }
                    ALOGE("peter_voice: 3");
                }
                else {
                    ALOGE("peter_voice: 22");
                    if(adev->callHandle)
                    {
                        i2s_pin_mux_sel_new(adev, EXTSPK, AP_TYPE);
                        ALOGE("peter_voice: 24");
                        NxpTfa_Close(adev->callHandle);
                        adev->callHandle = NULL;
                        ALOGE("peter_voice: 25");
                    }
                }

#endif

                ret = SetParas_Route_Incall(para->vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("voip1:VBC_CMD_SET_MODE SetParas_Route_Incall error. pipe:%s",
                    para->vbpipe);
                }
                ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SET_MODE);
                if(ret < 0){
                    ALOGE("voip1:Error,SET_MODE Write_Rsp2cp1 failed(%d).",ret);
                }
                MY_TRACE("voip1:VBC_CMD_SET_MODE OUT.");
            }
            break;
            case VBC_CMD_SET_GAIN:
            {
                MY_TRACE("voip1:VBC_CMD_SET_GAIN IN.");
                ret = SetParas_Volume_Incall(para->vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SET_GAIN SetParas_Route_Incall error. pipe:%s",
                    para->vbpipe);
                }
                ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SET_GAIN);
                if(ret < 0){
                    ALOGE("Error, %s Write_Rsp2cp1 failed(%d).",__func__,ret);
                }
                MY_TRACE("voip1:VBC_CMD_SET_GAIN OUT.");
            }
           break;
           case VBC_CMD_SWITCH_CTRL:
            {
                cur_timeout = NULL;
                MY_TRACE("voip1:VBC_CMD_SWITCH_CTRL IN.");
                ret = SetParas_Switch_Incall(para->vbpipe_fd,para->vbchannel_id,adev);
                if(ret < 0){
                    MY_TRACE("voip:VBC_CMD_SWITCH_CTRL SetParas_Switch_Incall error. ");
                }
                ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SWITCH_CTRL);
                if(ret < 0){
                    ALOGE("voip1:Error, DEVICE_CTRL Write_Rsp2cp1 failed(%d).",ret);
                }
                MY_TRACE("voip1:VBC_CMD_SWITCH_CTRL OUT.");
            }
            break;
            case VBC_CMD_SET_MUTE:
            {
                MY_TRACE("voip1:VBC_CMD_SET_MUTE IN.");

                MY_TRACE("voip1:VBC_CMD_SET_MUTE OUT.");
            }
            break;
            case VBC_CMD_DEVICE_CTRL:
            {
                MY_TRACE("voip1:VBC_CMD_DEVICE_CTRL IN.");
                ret = SetParas_DeviceCtrl_Incall(para->vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("voip1:VBC_CMD_DEVICE_CTRL SetParas_DeviceCtrl_Incall error.");
                }
                ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_DEVICE_CTRL);
        		if(ret < 0){
        		    ALOGE("voip1:Error, DEVICE_CTRL Write_Rsp2cp1 failed(%d).",ret);
        		}
                MY_TRACE("voip1:VBC_CMD_DEVICE_CTRL OUT.");
            }
            break;
#ifndef AUDIO_OLD_MODEM
        case VBC_CMD_SET_SAMPLERATE:
            {
            MY_TRACE("voip1:VBC_CMD_SET_SAMPLERATE IN.");
            pthread_mutex_lock(&adev->vbc_dlulock);
            if((adev->pcm_modem_dl!=NULL) && (0 != pcm_stop(adev->pcm_modem_dl)))
            {
                ALOGE("pcm dl start unsucessfully");
            }
            if((adev->pcm_modem_ul!=NULL) && (0 != pcm_stop(adev->pcm_modem_ul)))
            {
                ALOGE("pcm ul start unsucessfully");
            }
            pthread_mutex_unlock(&adev->vbc_dlulock);

            ret = SetParas_Samplerate_Incall(para->vbpipe_fd,adev);
            if(ret < 0){
                MY_TRACE("voip1:VBC_CMD_SET_SAMPLERATE SetParas_Samplerate_Incall error.s ");
            }

	         pthread_mutex_lock(&adev->vbc_dlulock);
            if((adev->pcm_modem_dl!=NULL) && (0 != pcm_start(adev->pcm_modem_dl)))
            {
                ALOGE("pcm dl start unsucessfully");
            }
            if((adev->pcm_modem_ul!=NULL) && (0 != pcm_start(adev->pcm_modem_ul)))
            {
                ALOGE("pcm ul start unsucessfully");
            }
            pthread_mutex_unlock(&adev->vbc_dlulock);

            ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SET_SAMPLERATE);
            if(ret < 0){
                ALOGE("Error, SET_SAMPLERATE Write_Rsp2cp1 failed(%d).",ret);
            }
            MY_TRACE("voip1:VBC_CMD_SET_SAMPLERATE OUT.");
            }
            break;
#endif
            default:
                ALOGE("voip1:Error: %s wrong cmd_type(%d)",__func__,read_common_head.cmd_type);
            break;
        }
        MY_TRACE("voip1:VBC_CMD_HAL_get_cmd release vbc_lock.");
   }

VOIP_EXIT:
    ALOGE("voip1:vbc_ctrl_thread exit, pipe:%s!!!", para->vbpipe);
    return 0;
}

//the timeout function to do that set adev->realCall to false for voip
void timer_handler(union sigval arg){
   ALOGV("%s in",__func__);
   struct tiny_audio_device *adev = (struct tiny_audio_device *)arg.sival_ptr;
   pthread_mutex_lock(&adev->lock);
   voip_forbid(adev,false);
   pthread_mutex_unlock(&adev->lock);
   ALOGV("%s out",__func__);
}

//this function is the interface to set adev->realCall value  adev-> mutex must get
void voip_forbid (struct tiny_audio_device * adev  ,bool value){
    ALOGV("%s, in",__func__);
	if(adev->voip_timer.created){
	    ALOGV("%s ,have create timer,so we delete it",__func__);
	    timer_delete(adev->voip_timer.timer_id);
	    adev->voip_timer.created = false;
	}
	adev->realCall = value;
	ALOGV("%s, out",__func__);
}

// set a timer for voip if the real call is end  adev-> mutex must get
bool voip_is_forbid(struct tiny_audio_device * adev)
{
	return adev->realCall;

}

//adev-> mutex must get
void voip_forbid_cancel(struct tiny_audio_device * adev,int delay){
    ALOGV("%s ,in",__func__);
    int status;
    struct sigevent se;
    struct itimerspec ts;

    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = adev;
    se.sigev_notify_function = timer_handler;
    se.sigev_notify_attributes = NULL;

    ts.it_value.tv_sec = delay;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    status = timer_create(CLOCK_MONOTONIC, &se,&((adev->voip_timer).timer_id));
    if(status == 0){
        adev->voip_timer.created = true;
        timer_settime((adev->voip_timer).timer_id, 0, &ts, 0);
        ALOGV("%s :timer for voip when call end is created",__func__);
    }else{
        adev->voip_timer.created = false;
        ALOGE("create timer err !");
    }
    ALOGV("%s ,out",__func__);
}

void *vbc_ctrl_thread_routine(void *arg)
{
    int ret = 0;
    vbc_ctrl_thread_para_t	*para = NULL;
    struct tiny_audio_device *adev;
    fd_set fds_read;
    struct timeval timeout = {5,0};
    struct timeval *cur_timeout = NULL;
    int maxfd;
    parameters_head_t read_common_head;
    parameters_head_t write_common_head;

    pthread_attr_t attr;
    struct sched_param m_param;
    int newprio=39;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_getschedparam(&attr,&m_param);
    m_param.sched_priority=newprio;
    pthread_attr_setschedparam(&attr,&m_param);

    para = (vbc_ctrl_thread_para_t *)arg;
    adev = (struct tiny_audio_device *)(para->adev);

    memset(&read_common_head, 0, sizeof(parameters_head_t));
    memset(&write_common_head, 0, sizeof(parameters_head_t));

    memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = VBC_CMD_NONE;
    write_common_head.paras_size = 0;
    MY_TRACE("voice:vbc_ctrl_thread_routine in pipe_name:%s.", para->vbpipe);
    if( para->vbpipe == NULL)
		goto EXIT;

RESTART:
    /* open vbpipe to build connection.*/
    if (para->vbpipe_fd <= 0) {
        para->vbpipe_fd = open(para->vbpipe, O_RDWR);//open("/dev/vbpipe6", O_RDWR);
        if (para->vbpipe_fd <= 0) {
            MY_TRACE("vbc_ctrl_thread_routine open fail, pipe_name:%s, %d.", para->vbpipe, errno);
            sleep(1);
            goto RESTART;
        } else {
            para->vbpipe_pre_fd = para->vbpipe_fd;
            ALOGW("voice:vbpipe_name(%s) vbpipe_fd(%d) open successfully.", para->vbpipe, para->vbpipe_fd);
        }
    } else {
        ALOGW("voice:vbpipe_name(%s) warning: vbpipe_fd(%d) NOT closed.", para->vbpipe, para->vbpipe_fd);
    }

    maxfd = para->vbpipe_fd + 1;
    if((fcntl(para->vbpipe_fd,F_SETFL,O_NONBLOCK))<0)
    {
        ALOGD("voice:vbpipe_name(%s) vbpipe_fd(%d) fcntl error.", para->vbpipe, para->vbpipe_fd);
    }

    /* loop to read parameters from vbpipe.*/
    while(1)
    {
        int result;
	timeout.tv_sec = 5;;
	timeout.tv_usec = 0;
        ALOGW("voice:%s, looping now...cur_timeout %x,timeout %d", para->vbpipe,cur_timeout,cur_timeout?cur_timeout->tv_sec:0);

        FD_ZERO(&fds_read);
        FD_SET(para->vbpipe_fd,&fds_read);
        result = select(maxfd,&fds_read,NULL,NULL,cur_timeout);
        if(result < 0) {
            ALOGE("voice:select error %d",errno);
            continue;
        }
        else if(!result) {
            ALOGE("voice:select timeout");
	    vbc_call_end_process(adev,true);
	    vbc_empty_pipe(para->vbpipe_fd);
	    cur_timeout = NULL;
	    continue;
        }
        if(FD_ISSET(para->vbpipe_fd,&fds_read) <= 0) {
            ALOGE("voice:select ok but no fd is set");
            continue;
        }
        /* read parameters common head of the packet.*/
        ret = ReadParas_Head(para->vbpipe_fd, &read_common_head);
        MY_TRACE("voice:VBC_CMD_HAL_get_cmd try vbc_lock, pipe_name:%s, ret:%d.", para->vbpipe, ret);
        if(ret < 0) {
            ALOGE("voice:Error, %s read head failed(%s), pipe_name:%s, need to read again ",__func__,strerror(errno),para->vbpipe);
	    vbc_call_end_process(adev,true);
	    vbc_empty_pipe(para->vbpipe_fd);
	    cur_timeout = NULL;
            continue;
	}
        ALOGW("voice:%s In Call, Get CMD(%d) from cp(pipe:%s, pipe_fd:%d), paras_size:%d devices:0x%x mode:%d",
            adev->call_start ? "":"NOT", read_common_head.cmd_type,
            para->vbpipe, para->vbpipe_fd,
            read_common_head.paras_size,adev->out_devices,adev->mode);

        if (memcmp(&read_common_head.tag[0], VBC_CMD_TAG, 3)) {
            ALOGE("voice:Error, (0x%x)NOT match VBC_CMD_TAG, wrong packet.", *((int*)read_common_head.tag));
	    vbc_call_end_process(adev,true);
	    vbc_empty_pipe(para->vbpipe_fd);
	    cur_timeout = NULL;
	    continue;
	}
        pthread_mutex_lock(&adev->vbc_lock);

	switch (read_common_head.cmd_type)
	{
	case VBC_CMD_HAL_OPEN:
	    {
#ifdef NXP_SMART_PA
		i2s_ctl_t * i2s_ctl_info = adev->i2s_btcall_info->i2s_ctl_info + adev->cp->vbc_ctrl_pipe_info->cpu_index;
		i2s_ctl_t * i2s_extspk_info =adev->cp->i2s_extspk + adev->cp->vbc_ctrl_pipe_info->cpu_index;
		ALOGE("peter_voice4: index= %d,i2s_speaker_info->is_switch= %d,i2s_speaker_info->i2s_index=%d",adev->cp->vbc_ctrl_pipe_info->cpu_index,i2s_extspk_info->is_switch,i2s_extspk_info->i2s_index);
		uint32_t i2s_ctl = ((i2s_ctl_info->is_switch << 8) | (i2s_ctl_info->i2s_index << 0)
           |(i2s_extspk_info->is_switch << 9) | (i2s_extspk_info->i2s_index<< 4));
#else
        i2s_ctl_t * i2s_ctl_info = adev->i2s_btcall_info->i2s_ctl_info + adev->cp->vbc_ctrl_pipe_info->cpu_index;;
        uint32_t i2s_ctl = ((i2s_ctl_info->is_switch << 8) | (i2s_ctl_info->i2s_index << 0) );
        //  |(adev->cp->i2s_extspk.is_switch << 9) | (adev->cp->i2s_extspk.index << 4));
#endif
		cur_timeout = &timeout;
		MY_TRACE("vocie:VBC_CMD_HAL_OPEN IN.");
		ALOGW("vocie:VBC_CMD_HAL_OPEN, try lock");
        pthread_mutex_lock(&adev->device_lock);
		pthread_mutex_lock(&adev->lock);
		ALOGW("voice:VBC_CMD_HAL_OPEN, got lock");
		if(adev->pcm_fm_dl) {
			out_device_disable(adev,AUDIO_DEVICE_OUT_FM);
		}
		force_all_standby(adev);    /*should standby because MODE_IN_CALL is later than call_start*/

		pthread_mutex_lock(&adev->vbc_dlulock);
		adev->pcm_modem_dl= pcm_open(s_tinycard, PORT_MODEM, PCM_OUT, &pcm_config_vx);
		if (!pcm_is_ready(adev->pcm_modem_dl)) {
		    ALOGE("voice:cannot open pcm_modem_dl : %s", pcm_get_error(adev->pcm_modem_dl));
		    pcm_close(adev->pcm_modem_dl);
		    adev->pcm_modem_dl = NULL;
		}
		adev->pcm_modem_ul= pcm_open(s_tinycard, PORT_MODEM, PCM_IN, &pcm_config_vrec_vx);
		if (!pcm_is_ready(adev->pcm_modem_ul)) {
		    ALOGE("voice:cannot open pcm_modem_ul : %s", pcm_get_error(adev->pcm_modem_ul));
		    pcm_close(adev->pcm_modem_ul);
		    pcm_close(adev->pcm_modem_dl);
		    adev->pcm_modem_ul = NULL;
		    adev->pcm_modem_dl = NULL;
		}
		if((adev->pcm_modem_dl!=NULL) && (0 != pcm_start(adev->pcm_modem_dl)))
		{
		    ALOGE("pcm dl start unsucessfully");
		}
		if((adev->pcm_modem_ul!=NULL) && (0 != pcm_start(adev->pcm_modem_ul)))
		{
		    ALOGE("pcm ul start unsucessfully");
		}
		pthread_mutex_unlock(&adev->vbc_dlulock);

		ALOGW("voice:START CALL,open pcm device...");

		SetParas_OpenHal_Incall(adev,para->vbpipe_fd);   //get sim card number

		adev->cp_type = para->cp_type;
		if(adev->out_devices & (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_ALL_SCO)) {
		    if(adev->cp_type == CP_TG)
			i2s_pin_mux_sel(adev,1);
		    else if(adev->cp_type == CP_W)
			i2s_pin_mux_sel(adev,0);
		    else if( adev->cp_type == CP_CSFB)
			i2s_pin_mux_sel(adev,CP_CSFB);
		}
		voip_forbid(adev, true);
		adev->call_start = 1;
		if (adev->cache_mute = 1) {
		    adev->cache_mute = 0;
		    adev->master_mute = 0;
		    codec_lowpower_open(adev, FALSE);
		}
		pthread_mutex_unlock(&adev->lock);
		pthread_mutex_unlock(&adev->device_lock);

		ALOGD("i2s_ctl is %x",i2s_ctl);
		parameters_head_t write_common_head = {0};
		write_common_head.cmd_type = VBC_CMD_RSP_OPEN;
		write_common_head.paras_size = i2s_ctl ;
		ret = WriteParas_Head(para->vbpipe_fd, &write_common_head);
		if(ret < 0) {
		    MY_TRACE("VBC_CMD_HAL_OPEN writeparas_head error %d",ret);
		}

		MY_TRACE("voice:VBC_CMD_HAL_OPEN OUT, vbpipe_name:%s.", para->vbpipe);
	    }
	    break;
	case VBC_CMD_HAL_CLOSE:
	    {
		MY_TRACE("voice:VBC_CMD_HAL_CLOSE IN.");
		cur_timeout = &timeout;

		pthread_mutex_lock(&adev->lock);
		adev->call_prestop = 1;
		force_all_standby(adev);
		pthread_mutex_unlock(&adev->lock);

		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_HAL_CLOSE);
		if(ret < 0){
		    ALOGE("voice:VBC_CMD_HAL_CLOSE: write1 cmd VBC_CMD_RSP_CLOSE ret(%d) error(%s).",ret,strerror(errno));
		}
		MY_TRACE("voice:VBC_CMD_HAL_CLOSE OUT.");
	    }
	    break;
	case VBC_CMD_RSP_CLOSE:
	    {
		cur_timeout = NULL;

		vbc_call_end_process(adev,false);
		MY_TRACE("voice:VBC_CMD_RSP_CLOSE IN.");
		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_HAL_CLOSE);
		if(ret < 0){
		    ALOGE("voice:VBC_CMD_RSP_CLOSE: write2 cmd VBC_CMD_RSP_CLOSE error(%d).",ret);
		}
		pthread_mutex_lock(&adev->lock);
#ifdef  NXP_SMART_PA
        if(adev->callHandle)
        {
            i2s_pin_mux_sel_new(adev, EXTSPK, AP_TYPE);
            ALOGE("zhanpo_ 1 NxpTfa_Close in");
            NxpTfa_Close(adev->callHandle);
            adev->callHandle = NULL;
        }
#endif

		adev->call_prestop = 0;

		adev->device_force_set = 1;
		select_devices_signal_asyn(adev);

		pthread_mutex_unlock(&adev->lock);
		MY_TRACE("voice:VBC_CMD_RSP_CLOSE OUT.");
	    }
	    break;
	case VBC_CMD_SET_MODE:
	    {
		MY_TRACE("voice:VBC_CMD_SET_MODE IN.");
#ifdef  NXP_SMART_PA
        ALOGE("peter_voice: in");
        pthread_mutex_lock(&adev->lock);
        if(adev->routeDev & AUDIO_DEVICE_OUT_SPEAKER ) {
            ALOGE("peter_voice: 2");
            if(!adev->callHandle)
            {
                i2s_pin_mux_sel_new(adev, EXTSPK, AP_TYPE);
                ALOGE("peter_voice: 4");
                adev->callHandle = NxpTfa_Open(&NxpTfa9890_Call_Info);
                ALOGE("peter_voice: 5");
                i2s_pin_mux_sel_new(adev, EXTSPK, CP_CSFB);
            }
            ALOGE("peter_voice: 3");
        }
        else {
            ALOGE("peter_voice: 22");
            if(adev->callHandle)
            {
                i2s_pin_mux_sel_new(adev, EXTSPK, AP_TYPE);
                ALOGE("peter_voice: 24");
                NxpTfa_Close(adev->callHandle);
                adev->callHandle = NULL;
                ALOGE("peter_voice: 25");
            }
            ALOGE("peter_voice: 23");
            if(adev->routeDev & (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_ALL_SCO)) {
                if(adev->cp_type == CP_TG)
                    i2s_pin_mux_sel(adev,1);
                else if(adev->cp_type == CP_W)
                    i2s_pin_mux_sel(adev,0);
                else if( adev->cp_type == CP_CSFB)
                    i2s_pin_mux_sel(adev,CP_CSFB);
            }
        }
        ALOGE("peter_voice: out");
        pthread_mutex_unlock(&adev->lock);
#else
		if(adev->routeDev & (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_ALL_SCO)) {
		    if(adev->cp_type == CP_TG)
			    i2s_pin_mux_sel(adev,1);
            else if(adev->cp_type == CP_W)
			    i2s_pin_mux_sel(adev,0);
            else if( adev->cp_type == CP_CSFB)
			    i2s_pin_mux_sel(adev,CP_CSFB);
		}
#endif
		ret = SetParas_Route_Incall(para->vbpipe_fd,adev);
		if(ret < 0){
		    MY_TRACE("voice:VBC_CMD_SET_MODE SetParas_Route_Incall error. pipe:%s",
			para->vbpipe);
		}
		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SET_MODE);
		if(ret < 0){
		    ALOGE("Error,SET_MODE Write_Rsp2cp1 failed(%d).",ret);
		}
		MY_TRACE("voice:VBC_CMD_SET_MODE OUT.");
	    }
	    break;
	case VBC_CMD_SET_GAIN:
	    {
		MY_TRACE("voice:VBC_CMD_SET_GAIN IN.");

		ret = SetParas_Volume_Incall(para->vbpipe_fd,adev);
		if(ret < 0){
		    MY_TRACE("voice:VBC_CMD_SET_GAIN SetParas_Route_Incall error. pipe:%s",
			para->vbpipe);
		}
		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SET_GAIN);
		if(ret < 0){
		    ALOGE("Error, %s Write_Rsp2cp1 failed(%d).",__func__,ret);
		}

		MY_TRACE("voice:VBC_CMD_SET_GAIN OUT.");
	    }
	    break;
	case VBC_CMD_SWITCH_CTRL:
	    {
		cur_timeout = NULL;
		MY_TRACE("voice:VBC_CMD_SWITCH_CTRL IN.");
		ret = SetParas_Switch_Incall(para->vbpipe_fd,para->vbchannel_id,adev);
		if(ret < 0){
		    MY_TRACE("voice:VBC_CMD_SWITCH_CTRL SetParas_Switch_Incall error.");
		}
		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SWITCH_CTRL);
		if(ret < 0){
		    ALOGE("voice:Error, VBC_CMD_SWITCH_CTRL: Write_Rsp2cp1 failed(%d).",ret);
		}
		pthread_mutex_lock(&adev->lock);
		adev->call_connected = 1;
		pthread_mutex_unlock(&adev->lock);
		MY_TRACE("voice:VBC_CMD_SWITCH_CTRL OUT.");
	    }
	    break;
	case VBC_CMD_SET_MUTE:
	    {
		MY_TRACE("voice:VBC_CMD_SET_MUTE IN.");

		MY_TRACE("voice:VBC_CMD_SET_MUTE OUT.");
	    }
	    break;
	case VBC_CMD_DEVICE_CTRL:
	    {
		MY_TRACE("voice:VBC_CMD_DEVICE_CTRL IN.");
		ret = SetParas_DeviceCtrl_Incall(para->vbpipe_fd,adev);
		if(ret < 0){
		    MY_TRACE("voice:VBC_CMD_DEVICE_CTRL SetParas_DeviceCtrl_Incall error. ");
		}
		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_DEVICE_CTRL);
		if(ret < 0){
		    ALOGE("voice:Error, DEVICE_CTRL Write_Rsp2cp1 failed(%d).",ret);
		}
		MY_TRACE("voice:VBC_CMD_DEVICE_CTRL OUT.");
	    }
	    break;
	case VBC_CMD_SET_SAMPLERATE:
	    {
		MY_TRACE("voice:VBC_CMD_SET_SAMPLERATE IN.");
		pthread_mutex_lock(&adev->vbc_dlulock);
		if(( NULL != adev->pcm_modem_dl ) && (0 != pcm_stop(adev->pcm_modem_dl))) 
		{
		    ALOGE("pcm dl start unsucessfully");
		}
		if(( NULL != adev->pcm_modem_ul ) && (0 != pcm_stop(adev->pcm_modem_ul)))
		{
		    ALOGE("pcm ul start unsucessfully");
		}
		pthread_mutex_unlock(&adev->vbc_dlulock);

		ret = SetParas_Samplerate_Incall(para->vbpipe_fd,adev);
		if(ret < 0){
		    MY_TRACE("voice:VBC_CMD_SET_SAMPLERATE SetParas_Samplerate_Incall error.s ");
		}

		pthread_mutex_lock(&adev->vbc_dlulock);
		if(( NULL != adev->pcm_modem_dl ) && (0 != pcm_start(adev->pcm_modem_dl)))
		{
		    ALOGE("pcm dl start unsucessfully");
		}
		if(( NULL != adev->pcm_modem_ul ) && (0 != pcm_start(adev->pcm_modem_ul)))
		{
		    ALOGE("pcm ul start unsucessfully");
		}
		pthread_mutex_unlock(&adev->vbc_dlulock);

		ret = Write_Rsp2cp(para->vbpipe_fd,VBC_CMD_SET_SAMPLERATE);
		if(ret < 0){
		    ALOGE("Error, SET_SAMPLERATE Write_Rsp2cp1 failed(%d).",ret);
		}
		MY_TRACE("voice:VBC_CMD_SET_SAMPLERATE OUT.");
	    }
	    break;
	default:
	    ALOGE("voice:Error: %s wrong cmd_type(%d)",__func__,read_common_head.cmd_type);
	    break;
	}
        MY_TRACE("voice:VBC_CMD_HAL_get_cmd release vbc_lock.");
        pthread_mutex_unlock(&adev->vbc_lock);
    }

EXIT:
    ALOGW("voice:vbc_ctrl_thread exit, pipe:%s!!!", para->vbpipe);
    return 0;
}

char *mystrstr(char *s1 , char *s2)
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

static void do_select_devices_l(struct tiny_audio_device *adev,int devices_out_l)
{
    unsigned int i;
    int ret = 0;
    ret = GetAudio_PaConfig_by_devices(adev,adev->pga_gain_nv,devices_out_l,adev->in_devices);
    if(ret < 0){
        return;
    }
    SetAudio_PaConfig_by_devices(adev,adev->pga_gain_nv);
    adev->prev_out_devices = 0;
    if(adev->eq_available)
        vb_effect_sync_devices(devices_out_l, adev->in_devices);

    /* Turn on new devices first so we don't glitch due to powerdown... */
    for (i = 0; i < adev->num_dev_cfgs; i++) {
    /* separate INPUT/OUTPUT case for some common bit used. */
        if ((devices_out_l & adev->dev_cfgs[i].mask)
        && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            set_route_by_array(adev->mixer, adev->dev_cfgs[i].on,
                    adev->dev_cfgs[i].on_len);
        }
    }

    /* ...then disable old ones. */
    for (i = 0; i < adev->num_dev_cfgs; i++) {
        if (!(devices_out_l & adev->dev_cfgs[i].mask)
        && !(adev->dev_cfgs[i].mask & AUDIO_DEVICE_BIT_IN)) {
            set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
                    adev->dev_cfgs[i].off_len);
        }
    }
    /* update EQ profile*/
    if(adev->eq_available)
        vb_effect_profile_apply();
    SetAudio_gain_route(adev,1,devices_out_l,adev->in_devices);
}


static void * vbc_ctl_modem_monitor_routine(void *arg)
{
    int cur_out_devices_l;
    int fd = -1;
    int numRead = 0;
    int buf[128] = {0};
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    ALOGD("vbc_ctl_modem_monitor_routine in");
    if( !adev) {
            ALOGD("vbc_ctl_modem_monitor_routine:error adev is null");
            return -1;
    }
reconnect:
    do {
       fd = socket_local_client("modemd",
           ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
       if(fd <0 ) {
           ALOGE("vbc_ctl_modem_monitor_routine:socket_local_client failed %d", errno);
               usleep(2000*1000);
           }
   } while(fd < 0);

   while(1)
   {
        memset (buf, 0 , sizeof(buf));
        do {
               numRead = read(fd, buf, sizeof(buf));
           } while(numRead < 0 && errno == EINTR);

        if(numRead <= 0) {
            ALOGE("modem_monitor: error: got to reconnect");
            goto reconnect;
        }
        ALOGD("modem_monitor: %s",buf);
        if(mystrstr(buf,"Assert") || mystrstr(buf,"Reset") || mystrstr(buf,"Blocked")) {
            ALOGD("modem asserted1 %s",buf);
            cur_out_devices_l = adev->out_devices;
            cur_out_devices_l &= ~AUDIO_DEVICE_OUT_ALL;
            pthread_mutex_lock(&adev->device_lock);
            pthread_mutex_lock(&adev->lock);
            if(adev->call_start){
                do_select_devices_l(adev,cur_out_devices_l);
            }
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&adev->device_lock);

            ALOGD("modem_monitor assert:vbc_call_end_process");
            vbc_call_end_process(adev,true);
            ALOGD("modem asserted2");
        }
    }

    return 0;
}

int vb_ctl_modem_monitor_open(void * arg)
{
    int rc;
    pthread_t thread_id;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)arg ;
    ALOGD("vb_ctl_modem_monitor_open in");
    if( !adev) {
        ALOGD("vb_ctl_modem_monitor_open:error adev is null");
        return -1;
    }
    rc = pthread_create((pthread_t *)&(thread_id), NULL,
            vbc_ctl_modem_monitor_routine, (void *)adev);
    if (rc) {
        ALOGE("vb_ctl_modem_monitor_open,voip pthread_create failed, rc=%d", rc);
        return -1;
    }
     ALOGE("vb_ctl_modem_monitor_open,voip pthread_create ok, rc=%d", rc);
    return 0;
}
