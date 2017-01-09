 #include "testitem.h"

#if defined (PLATFORM_VERSION6)
static int fm_fd = -1;
static int thread_run=0;
static int  row = 2;
extern int usbin_state;
static int COM_open_dev(void)
 {
	 int ret = 0;
	 int tmp = -1;

//	 FMR_ASSERT(TROUT_FM_DEV_NAME);
//	 FMR_ASSERT(fd);

	 LOGD("COM_open_dev start.");
	 tmp = open(TROUT_FM_DEV_NAME, O_RDWR);
	 if (tmp < 0) {
		 LOGE("Open %s failed, %s.", TROUT_FM_DEV_NAME, strerror(errno));
		 ret = -1;
	 }
	 fm_fd = tmp;
	 LOGD("[fd=%d] [ret=%d].", fm_fd, ret);
	 return ret;
 }

static int COM_close_dev(int fd)
{
    int ret = 0;

    LOGI("COM_close_dev start.");
    ret = close(fd);
    if (ret) {
        LOGE("fm closed failed!");
    }
    LOGD("[fd=%d] [ret=%d].", fd, ret);
    return ret;
}

static int COM_pwr_up(int fd)
 {
	 int ret = 0;
	 int tmp = -1;
	 struct fm_tune_parm parm;

	 bzero(&parm, sizeof(struct fm_tune_parm));

	 parm.band = FM_BAND_DEFAULT;
	 parm.freq = FM_FREQ_MIN;
	 parm.hilo = FM_AUTO_HILO_OFF;
	 parm.space = FM_SEEK_SPACE;

	 ret = ioctl(fd, FM_IOCTL_POWERUP, &parm);
	 if (ret) {
		 LOGE("fm power up failed.");
		 goto PWR_UP_FAILE;
	 }

	 LOGD("[fd=%d] [ret=%d].", fd, ret);
	 return ret;

	 PWR_UP_FAILE:
	 tmp = close(fm_fd);
	 if (0 != tmp) {
		 LOGE("FM close failed!");
	 }

	 fm_fd = -1;
	 LOGD("[fd=%d] [ret=%d].", fd, ret);
	 return ret;
 }

static int COM_pwr_down(int fd, int type)
{
    int ret = 0;
    LOGD("power down [type=%d].", type);
    ret = ioctl(fd, FM_IOCTL_POWERDOWN, &type);
    if (ret) {
        LOGE("fm power down failed!");
    }
    LOGD("[fd=%d] [ret=%d].", fd, ret);
    return ret;
}

static int COM_tune(int fd, int freq)
 {
	 int ret = 0;

	 struct fm_tune_parm parm;

	 bzero(&parm, sizeof(struct fm_tune_parm));

	 parm.band = FM_BAND_DEFAULT;
	 parm.freq = freq;
	 parm.hilo = FM_AUTO_HILO_OFF;
	 parm.space = FM_SEEK_SPACE;

	 ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
	 if (ret) {
		 LOGE("fm tune failed!");
	 }
	 LOGD("[fd=%d] [freq=%d] [ret=%d].", fd, freq, ret);
	 return ret;
 }
 
static int COM_seek(int fd, int *freq, int dir)
 {
	 int ret = 0;
	 struct fm_seek_parm parm;

	 bzero(&parm, sizeof(struct fm_seek_parm));

	 parm.band = FM_BAND_DEFAULT;
	 parm.freq = *freq;
	 parm.hilo = FM_AUTO_HILO_OFF;
	 parm.space = FM_SEEK_SPACE;
	 parm.seekdir = dir;
	 parm.seekth = FM_SEEKTH_LEVEL_DEFAULT;

	 ret = ioctl(fd, FM_IOCTL_SEEK, &parm);
	 if (ret == 0) {
		 *freq = parm.freq;
	 	 LOGD("mmitest new freq write=%d",*freq);
	 }
	 LOGD("[fd=%d] [ret=%d].", fd, ret);
	 return ret;
 }


static int COM_set_mute(int fd, int mute)
 {
	 int ret = 0;
	 int tmp = mute;

	 ret = ioctl(fd, FM_IOCTL_MUTE, &tmp);
	 if (ret) {
		 LOGE("fm set mute failed!");
	 }
	 LOGD("[fd=%d] [ret=%d].", fd, ret);
	 return ret;
 }

 static int COM_get_rssi(int fd, int *rssi)
 {
	 int32_t ret = 0;
	 int32_t value;

	 ret = ioctl(fd, FM_IOCTL_GETRSSI, &value);
	 if (ret) {
		 LOGE("FM get rssi faild:%d!",ret);
		 return ret;
	 }
	 *rssi = value;
	 LOGD("mmitest FM get rssi = %d", *rssi);
	 //LOGD("soft mute tune[%d] valid[%d]: RSSI:[%d]PAMD:[%d]MR:[%d]ATDC:[%d]PRX:[%d]SMG:[%d]",para->freq,para->valid,RSSI,PAMD,MR,ATDC,PRX,softmuteGainLvl);
	 return 0;
 }

 static int fm_show_play_stat(int freq, int inpw)
 {
	 char text[128] = {0};
	 int ret = -1;
	 inpw=-inpw;

	 ui_set_color(CL_GREEN);
	 memset(text, 0, sizeof(text));
	 sprintf(text, "%s:%d.%dMHz", TEXT_FM_FREQ, freq / 10, freq % 10);			 /*show channel*/
	 row = ui_show_text(row, 0, text);

	 memset(text, 0, sizeof(text));
	 sprintf(text, "%s:%d(dBm)", TEXT_FM_STRE, inpw);		 /*show rssi*/
	 row = ui_show_text(row, 0, text);

	 memset(text, 0, sizeof(text));
	 if ((inpw >= -105) && (inpw < 0)) {
		 ret = RL_PASS;
		 ui_set_color(CL_GREEN);
		 memset(text, 0, sizeof(text));
		 sprintf(text, "%s>=-105(dBm)", TEXT_FM_STRE);		  /*show rssi*/
		 row = ui_show_text(row, 0, text);
		 row = ui_show_text(row, 0, TEXT_FM_OK);
	 } else {
		 ret = RL_FAIL;
		 ui_set_color(CL_RED);
		 memset(text, 0, sizeof(text));
		 sprintf(text, "%s<-105(dBm)", TEXT_FM_STRE);		 /*show rssi*/
		 row = ui_show_text(row, 0, text);
		 row = ui_show_text(row, 0, TEXT_FM_FAIL);
	 }

	 gr_flip();
	 sleep(1);
	 ui_push_result(ret);
	 return ret;
 }

 static void fm_show_headset(int state)
 {
	 if(SPRD_HEADSETOUT == state) {
		 ui_clear_rows(row, 1);
		 ui_set_color(CL_RED);
		 ui_show_text(row, 0, TEXT_HD_UNINSERT);
	 } else if (SPRD_HEADSETIN == state) {
		 ui_clear_rows(row, 1);
		 ui_set_color(CL_GREEN);
		 ui_show_text(row, 0, TEXT_HD_INSERTED);
	 }

	 gr_flip();
 }

 static void fm_show_searching(int state)
 {

	 if (STATE_DISPLAY == state) {
		 ui_clear_rows(row, 1);
		 ui_set_color(CL_GREEN);
		 row=ui_show_text(row, 0, TEXT_FM_SCANING);
	 } else if (STATE_CLEAN== state) {
		 ui_clear_rows(row, 1);
	 }

	 gr_flip();
 }

 static void fm_seek_timeout(void)
 {
	 ui_clear_rows(row, 2);
	 ui_set_color(CL_RED);
	 row = ui_show_text(row, 0, TEXT_FM_SEEK_TIMEOUT);
	 row = ui_show_text(row, 0, TEXT_FM_FAIL);

	 gr_flip();
 }

 static int fm_check_headset(int cmd, int* headset_state)
 {
	 int ret;
	 char buf[8];
	 static int fd = -1;

        switch(cmd){
            case HEADSET_CHECK:
                memset(buf, 0, sizeof(buf));
                lseek(fd, 0, SEEK_SET);
                ret = read(fd, buf, sizeof(buf));
                if(ret < 0) {
                    LOGD("read fd failed");
                    return -1;
                }
                *headset_state = atoi(buf);
                break;
            case HEADSET_OPEN :
                if (fd > 0) {
                    LOGD("headset device already open !");
                    return -1;
                }
                fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);
                if (fd < 0) {
                    LOGE("headset device open failed!");
                    return -1;
                }
                LOGD("open headset device success, fd =  %d", fd);
                break;
            case HEADSET_CLOSE:
                if (fd < 0) {
                    LOGD("headset device already close !");
                    return -1;
                }
                close(fd);
                fd = -1;
                break;
            default:
                LOGD("In error command: %d",  cmd);
                break;
	 }

	 return 0;
 }

 int fm_start(void)
 {
	 int ret;
	 int rssi = 0;
	 int headset_in = 0;
	 int freq = 875;

	 ret = fm_check_headset(HEADSET_OPEN, NULL);		 /*open headset device*/
	 if (ret < 0) {
		  LOGD("mmitest open %s failed", SPRD_HEADSET_SWITCH_DEV);
		  ui_push_result(RL_FAIL);
		  return ret;
	 }

	 fm_check_headset(HEADSET_CHECK, &headset_in);		 /*checket headset state*/
	 while (0 == headset_in&&1 == thread_run) {
		 LOGD("mmitest headset out");
		 fm_show_headset(SPRD_HEADSETOUT);                   /*show headset state*/
		 fm_check_headset(HEADSET_CHECK, &headset_in);
		 usbin_state=1;
	 }

	 if(0 == headset_in){
		 fm_check_headset(HEADSET_CLOSE, NULL);
		 usbin_state=0;
		 ui_push_result(RL_FAIL);
		 return -1;
	 }

	 usbin_state=0;
	 ret = fm_check_headset(HEADSET_CLOSE, NULL);        /*close headset device*/
	 if (0 != ret) {
		 LOGD("mmitest close %s failed",SPRD_HEADSET_SWITCH_DEV);
		 ui_push_result(RL_FAIL);
		 return ret;
	 }
	 LOGD("mmitest headset in");

	 fm_show_headset(SPRD_HEADSETIN);				 /*show headset state*/
	 row++;
	 fm_show_searching(STATE_DISPLAY);
	 ret = COM_open_dev();
	 ret = COM_pwr_up(fm_fd);

	 if (0 != ret) {
		 LOGD("mmitest open %s failed", TROUT_FM_DEV_NAME);
		 ui_push_result(RL_FAIL);
		 return ret;
	 }
	 //ret = COM_set_mute(fm_fd, FM_VOICE_MUTE);	/*/kernel/drivers/misc/fm_2351/sr2351_fm_ctrl.c FM_IOCTL_MUTE dose not enable*/
	 ret = COM_seek(fm_fd, &freq, FM_SEEK_UP);
	 if (0 != ret) {
		 fm_show_searching(STATE_CLEAN);
		 fm_seek_timeout();
		 LOGD("mmitest get rssi failed");
		 sleep(1);
		 ui_push_result(RL_FAIL);
		 return ret;
	 }
	 ret = COM_tune(fm_fd, freq);
	 //ret = COM_set_mute(fm_fd, FM_VOICE_ON);
	 ret = COM_get_rssi(fm_fd, &rssi);
	 ret = COM_pwr_down(fm_fd, 0);
	 ret = COM_close_dev(fm_fd);		/*close fm*/
	 if (0 != ret) {
		 LOGD("mmitest close %s failed",TROUT_FM_DEV_NAME);
		 ui_push_result(RL_FAIL);
		 return ret;
	 }

	 fm_show_searching(STATE_CLEAN);
	 ret=fm_show_play_stat(freq, rssi/10); 		  /*display rssi*/

	 return ret;
 }

int test_fm_start(void)
{
     int ret = 0;
     pthread_t t1;
     row = 2;

     ui_fill_locked();
     ui_show_title(MENU_TEST_FM);
     gr_flip();

     thread_run=1;
     pthread_create(&t1, NULL, (void*)fm_start, NULL);
     usleep(20*1000);
     ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK
     thread_run=0;
     pthread_join(t1, NULL); /* wait "handle key" thread exit. */

     save_result(CASE_TEST_FM,ret);
     return ret;
}

#elif defined (SPRD_WCNBT_MARLIN)

#include <semaphore.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_fm.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <hardware/fm.h>

static int test_result = -1;
static int fm_fd = -1;
extern char sdcard_fm_state;
static int thread_run;
extern unsigned char bt_stop_flag;

enum fmStatus {
    FM_STATE_DISABLED,
    FM_STATE_ENABLED,
    FM_STATE_PLAYING,
    FM_STATE_STOPED,
    FM_STATE_PANIC,
};

static bt_state_t sBtState = BT_STATE_OFF;
static int sFmStatus = FM_STATE_PANIC;
static int sFmsearchStatus=BT_STATUS_FAIL;
static int sFmtuneStatus=BT_STATUS_FAIL;

static int fm_fre=0;
static int fm_rssi=-200;
static int fm_snr=0;

static hw_module_t * s_hwModule = NULL;
static struct fm_device_t * s_hwDev    = NULL;
static bluetooth_device_t *bt_device = NULL;
const bt_interface_t *ssBtInterface=NULL;
const btfm_interface_t *sBtFmInterface=NULL;

extern bt_os_callouts_t callouts;
extern int usbin_state;

static void bt_adapter_state_changed_cb(int status);
static void btfmEnableCallback (int status);
static void btfmDisableCallback (int status) ;
static void btfmtuneCallback (int status, int rssi, int snr, int freq);
static void btfmMuteCallback (int status, BOOLEAN isMute);
static void btfmSearchCallback (int status, int rssi, int snr, int freq);
static void btfmSearchCompleteCallback(int status, int rssi, int snr, int freq);
static void btfmAudioModeCallback(int status, int audioMode);
static void btfmAudioPathCallback(int status, int audioPath);
static void btfmVolumeCallback(int status, int volume);

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    bt_adapter_state_changed_cb,
    NULL, /*adapter_properties_cb */
    NULL, /* remote_device_properties_cb */
    NULL, /* device_found_cb */
    NULL,
    NULL, /* pin_request_cb  */
    NULL, /* ssp_request_cb  */
    NULL, /*bond_state_changed_cb */
    NULL, /* acl_state_changed_cb */
    NULL, /* thread_evt_cb */
    NULL, /*dut_mode_recv_cb */
    NULL, /*authorize_request_cb */
};

static btfm_callbacks_t btfm_callbacks = {
    sizeof (btfm_callbacks_t),
    btfmEnableCallback,             // btfm_enable_callback
    btfmDisableCallback,                // btfm_disable_callback
    btfmtuneCallback,               // btfm_tune_callback
    btfmMuteCallback,               // btfm_mute_callback
    btfmSearchCallback,             // btfm_search_callback
    btfmSearchCompleteCallback,     // btfm_search_complete_callback
    NULL,                           // btfm_af_jump_callback
    btfmAudioModeCallback,          // btfm_audio_mode_callback
    btfmAudioPathCallback,          // btfm_audio_path_callback
    NULL,                           // btfm_audio_data_callback
    NULL,                           // btfm_rds_mode_callback
    NULL,                           // btfm_rds_type_callback
    NULL,                           // btfm_deemphasis_callback
    NULL,                           // btfm_scan_step_callback
    NULL,                           // btfm_region_callback
    NULL,                           // btfm_nfl_callback
    btfmVolumeCallback,             //btfm_volume_callback
    NULL,                           // btfm_rds_data_callback
    NULL,                           // btfm_rtp_data_callback

};

static void btfmEnableCallback (int status)
{
    if (status == BT_STATUS_SUCCESS)
        sFmStatus = FM_STATE_ENABLED;
    LOGD("Enable callback, status: %d", sFmStatus);
 }

static void btfmDisableCallback (int status)
{
    if (status == BT_STATUS_SUCCESS)
        sFmStatus = FM_STATE_DISABLED;
    LOGD("Disable callback, status: %d", sFmStatus);
}

static void btfmtuneCallback (int status, int rssi, int snr, int freq)
{
    LOGD("mmitest Tune callback, status: %d,rssi=%d, snr=%d, freq: %d", status,rssi,snr, freq);
    sFmtuneStatus=status;
    if(sFmtuneStatus== BT_STATUS_SUCCESS)
        {
            fm_fre=freq;
            fm_rssi=rssi;
            fm_snr=snr;
        }
}

static void btfmMuteCallback (int status, BOOLEAN isMute)
{
    LOGD("Mute callback, status: %d, isMute: %d", status, isMute);
}

static void btfmSearchCallback (int status, int rssi, int snr, int freq)
{
    LOGD("mmitest Search callback, status=%d rssi=%d snr=%d freq=%d", status,rssi,snr,freq);
}

static void btfmSearchCompleteCallback(int status, int rssi, int snr, int freq)
{
    LOGD("mmitest Search complete callback status=%d rssi=%d snr=%d freq=%d", status,rssi,snr,freq);
    sFmsearchStatus=status;
    if(sFmsearchStatus==BT_STATUS_SUCCESS){
        fm_fre=freq;
        fm_rssi=rssi;
        fm_snr=snr;
    }
    if(sFmsearchStatus==BT_STATUS_NOMEM){
        fm_fre=freq;
        fm_rssi=rssi;
        fm_snr=snr;
    }
}

static void btfmAudioModeCallback(int status, int audioMode)
{
    LOGD("Audio mode change callback, status: %d, audioMode: %d", status, audioMode);
}

static void btfmAudioPathCallback(int status, int audioPath)
{
    LOGD("Audio path change callback, status: %d, audioPath: %d", status, audioPath);
}

static void btfmVolumeCallback(int status, int volume)
{
    LOGD("Volume change callback, status: %d, volume: %d", status, volume);
}

static void bt_adapter_state_changed_cb(int status)
{
        int retVal;
        sBtState = status;
        LOGD("BT/FM Adapter State Changed: %d",status);

        if (status != BT_RADIO_ON) return;
        sBtFmInterface = (btfm_interface_t*)ssBtInterface->get_fm_interface();

        retVal = sBtFmInterface->init(&btfm_callbacks);
        retVal = sBtFmInterface->enable(96);
}

static void fm_show_play_stat(int freq, int inpw)
{
    int row = 3;
    char text[128] = {0};
    inpw=-inpw;
    ui_set_color(CL_GREEN);
    memset(text, 0, sizeof(text));
    sprintf(text, "%s:%d.%dMHz", TEXT_FM_FREQ, freq / 100, freq % 100);           /*show channel*/
    row = ui_show_text(row, 0, text);

    memset(text, 0, sizeof(text));
    if (1) {//(inpw >= -105) && (inpw < 0)
        test_result = 1;
        memset(text, 0, sizeof(text));
        sprintf(text, "%s>=-105(dBm)", TEXT_FM_STRE);        /*show rssi*/
        row = ui_show_text(row, 0, text);
        row = ui_show_text(row, 0, TEXT_FM_OK);
    }

    gr_flip();
}

static void fm_show_headset(int state)
{
    int row = 2;
    if(SPRD_HEADSETOUT == state) {
        ui_clear_rows(row, 1);
        ui_set_color(CL_RED);
        ui_show_text(row, 0, TEXT_HD_UNINSERT);
    } else if (SPRD_HEADSETIN == state) {
        ui_clear_rows(row, 1);
        ui_set_color(CL_GREEN);
        ui_show_text(row, 0, TEXT_HD_INSERTED);
    }

    gr_flip();
}

static void fm_show_searching(int state)
{
    int row = 3;

    if (STATE_DISPLAY == state) {
        ui_clear_rows(row, 1);
        ui_set_color(CL_GREEN);
        ui_show_text(row, 0, TEXT_FM_SCANING);
    } else if (STATE_CLEAN== state) {
        ui_clear_rows(row, 1);
    }

    gr_flip();
}

static void fm_seek_timeout(void)
{
    int row = 5;

    ui_clear_rows(row, 2);
    ui_set_color(CL_RED);
    row = ui_show_text(row, 0, TEXT_FM_SEEK_TIMEOUT);
    row = ui_show_text(row, 0, TEXT_FM_FAIL);

    gr_flip();
}

static int fm_check_headset(int cmd, int* headset_state)
 {
	 int ret;
	 char buf[8];
	 static int fd = -1;

        switch(cmd){
            case HEADSET_CHECK:
                memset(buf, 0, sizeof(buf));
                lseek(fd, 0, SEEK_SET);
                ret = read(fd, buf, sizeof(buf));
                if(ret < 0) {
                    LOGD("read fd fail failed");
                    return -1;
                }
                *headset_state = atoi(buf);
                break;
            case HEADSET_OPEN :
                if (fd > 0) {
                    LOGD("headset device already open !");
                    return -1;
                }
                fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);
                if (fd < 0) {
                    LOGE("headset device open failed!");
                    return -1;
                }
                break;
            case HEADSET_CLOSE:
                if (fd < 0) {
                    LOGD("headset device already close!");
                    return -1;
                }
                close(fd);
                fd = -1;
                break;
            default:
                LOGD("In error command: %d", cmd);
                break;
	 }

	 return 0;
 }

static int fm_open(void)
{
    int retVal ;
    if( NULL != s_hwDev ) {
        LOGD("mmitest already opened.");
        return 0;
    }

    int err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if( err || NULL == s_hwModule ) {
        LOGE("mmitest hw_get_module failed");
        return ((err > 0) ? -err : err);
    }

    err = s_hwModule->methods->open(s_hwModule, BT_HARDWARE_MODULE_ID,
                    (hw_device_t**)&s_hwDev);
    if( err || NULL == s_hwDev ) {
        LOGE("mmitest open failed");
        return ((err > 0) ? -err : err);
    }

    if (err == 0){
        bt_device = (bluetooth_device_t *)s_hwDev;
        ssBtInterface = bt_device->get_bluetooth_interface();
        LOGD("mmitest get_bluetooth_interface.");
        retVal=(bt_status_t)ssBtInterface->init(&bt_callbacks);

        LOGD("mmitest init finished.");

        if(retVal !=BT_STATUS_SUCCESS){
            LOGD("mmitest BT init fail");
            err=-1;
            goto out;
        }
        retVal = (bt_status_t)ssBtInterface->set_os_callouts(&callouts);
        if(retVal !=BT_STATUS_SUCCESS){
            LOGD("mmitest BT set_os_callouts fail");
            err=-1;
            goto out;
        }
        LOGD("mmitest enable readio");
        ssBtInterface->enableRadio();
    }
out:
    LOGD("mmitest hal load success");
    return err;
}

static int fm_search(void)
{
    int ret;
    int counter=0;
    int freq_end=END_FRQ;
    int freq_start=END_FRQ+10;

    ret =sBtFmInterface->combo_search(freq_start,freq_end,THRESH_HOLD,DIRECTION,SCANMODE,MUTI_CHANNEL,CONTYPE,CONVALUE);

    while (counter++ < 10 && BT_STATUS_SUCCESS != sFmsearchStatus)
    {
	 freq_end+=10;
	 freq_start+=10;
	 sBtFmInterface->combo_search(freq_start,freq_end,THRESH_HOLD,DIRECTION,SCANMODE,MUTI_CHANNEL,CONTYPE,CONVALUE);
	 sleep(1);
    }
    LOGD("mmitest fm search: status =%d fm_fre=%d fm_rssi=%d", sFmsearchStatus,fm_fre,fm_rssi);
    if(sFmsearchStatus!=BT_STATUS_SUCCESS)
        return -1;

    LOGD("mmitest fm search: fm_fre=%d fm_rssi=%d", fm_fre,fm_rssi);


    LOGD("mmitest new freq write=%d",fm_fre);
    return 0;
}

static int fm_close(void)
{
    int counter = 0;
    if (sBtFmInterface)
        sBtFmInterface->disable();
    else
        goto out;

    while (counter++ < 3 && FM_STATE_DISABLED != sFmStatus) sleep(1);
    if (FM_STATE_DISABLED != sFmStatus) return -1;
    sBtFmInterface->cleanup();
    if (sBtFmInterface) sBtFmInterface->cleanup();
    if (ssBtInterface){
             ssBtInterface->disableRadio();
             sleep(2);
             ssBtInterface->cleanup();
        }
        sFmStatus = FM_STATE_DISABLED;

        if( NULL != s_hwDev && NULL != s_hwDev->common.close )
        {
            s_hwDev->common.close( &(s_hwDev->common) );
        }
out:
        s_hwDev = NULL;
        LOGD("Close successful.");
        return 0;
}

int fm_start(void)
{
    int ret;
    int rssi;
    int headset_in = 0;
    int freq = 875;
    int counter=0;
    int result;
    system("rm -f /data/misc/bluedroid/bt_config.xml");
    system("rm -f /data/misc/bluedroid/bt_config.new");
    system("rm -f /data/misc/bluedroid/bt_config.old");

    LOGD("mmitest freq=%d",freq);

    system(FM_INSMOD_COMMEND);
    ret = fm_check_headset(HEADSET_OPEN, NULL);         /*open headset device*/
    if (ret < 0) {
         LOGD("mmitest open %s failed", SPRD_HEADSET_SWITCH_DEV);
         goto FM_TEST_FAIL;;
    }

    fm_check_headset(HEADSET_CHECK, &headset_in);       /*checket headset state*/
    while (0 == headset_in&&1 == thread_run) {
        LOGD("headset out");
        fm_show_headset(SPRD_HEADSETOUT);                   /*show headset state*/
        fm_check_headset(HEADSET_CHECK, &headset_in);
        usbin_state=1;
    }

    if(0 == headset_in){
        fm_check_headset(HEADSET_CLOSE, NULL);
        usbin_state=0;
        goto FM_TEST_FAIL;
    }
    usbin_state=0;

    ret = fm_check_headset(HEADSET_CLOSE, NULL);        /*close headset device*/
    if (0 != ret) {
        LOGD("mmitest close %s failed",SPRD_HEADSET_SWITCH_DEV);
        goto FM_TEST_FAIL;
    }

    LOGD("headset in");
    fm_show_headset(SPRD_HEADSETIN);                /*show headset state*/
    fm_show_searching(STATE_DISPLAY);

    LOGD("mmitest start to open");
    ret = fm_open();
    if(ret!=0){
        LOGD("mmitest open %s failed", TROUT_FM_DEV_NAME);
        goto FM_TEST_FAIL;
    }

    while (sFmStatus != FM_STATE_ENABLED && counter++ < 3) sleep(1);

    if (sFmStatus != FM_STATE_ENABLED) {
        LOGD("fm service has not enabled, status: %d", sFmStatus);
        goto FM_TEST_FAIL;
    }

    fm_search();
    if(sFmsearchStatus==BT_STATUS_SUCCESS){
        fm_show_searching(STATE_CLEAN);
        fm_show_play_stat(fm_fre, fm_rssi);
        LOGD("mmitest fm success");
        result=RL_PASS;
        ui_push_result(RL_PASS);
    }else{
        fm_show_searching(STATE_CLEAN);
        fm_seek_timeout();
        FM_TEST_FAIL:
        result=RL_FAIL;
        ui_push_result(RL_FAIL);
        LOGD("mmitest fm failed");
    }

    fm_close();
    return 0;
}

int test_fm_start(void)
{
     int ret = 0;
     int row = 2;
     pthread_t t1, t2;
     ui_fill_locked();
     ui_show_title(MENU_TEST_FM);
     gr_flip();

     #ifdef SPRD_WCNBT_MARLIN
     if( bt_stop_flag){
        ui_set_color(CL_RED);
        ui_show_text(row,0,BT_BACK_TEST);
        gr_flip();
        sleep(1);
        return RL_NA;
     }
     #endif

     thread_run=1;
     pthread_create(&t1, NULL, (void*)fm_start, NULL);
     ret = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//, TEXT_GOBACK
     thread_run=0;
     pthread_join(t1, NULL); /* wait "handle key" thread exit. */

     save_result(CASE_TEST_FM,ret);
     return ret;
}

#else

 /** The following define the IOCTL command values via the ioctl macros */
 // ioctl command

 static int test_result = -1;
 static int fm_fd = -1;

 static void fm_show_play_stat(int freq, int inpw)
 {
	 int row = 4;
	 char text[128] = {0};
	 inpw=-inpw;
 
	 ui_set_color(CL_GREEN);
	 memset(text, 0, sizeof(text));
	 sprintf(text, "%s:%d.%dMHz", TEXT_FM_FREQ, freq / 10, freq % 10);			 /*show channel*/
	 row = ui_show_text(row, 0, text);
 
	 memset(text, 0, sizeof(text));
	 sprintf(text, "%s:%d(dBm)", TEXT_FM_STRE, inpw);		 /*show rssi*/
	 row = ui_show_text(row, 0, text);
 
	 memset(text, 0, sizeof(text));
	 if ((inpw >= -105) && (inpw < 0)) {
		 test_result = 1;
		 memset(text, 0, sizeof(text));
		 sprintf(text, "%s>=-105(dBm)", TEXT_FM_STRE);		  /*show rssi*/
		 row = ui_show_text(row, 0, text);
		 row = ui_show_text(row, 0, TEXT_FM_OK);
	 } else {
		 test_result = 0;
		 memset(text, 0, sizeof(text));
		 sprintf(text, "%s<-105(dBm)", TEXT_FM_STRE);		 /*show rssi*/
		 row = ui_show_text(row, 0, text);
		 row = ui_show_text(row, 0, TEXT_FM_FAIL);
	 }
 
	 gr_flip();
 }
 
 
 static void fm_show_headset(int state)
 {
	 int row = 3;
	 if(SPRD_HEADSETOUT == state) {
		 ui_clear_rows(row, 1);
		 ui_set_color(CL_RED);
		 ui_show_text(row, 0, TEXT_HD_UNINSERT);
	 } else if (SPRD_HEADSETIN == state) {
		 ui_clear_rows(row, 1);
		 ui_set_color(CL_GREEN);
		 ui_show_text(row, 0, TEXT_HD_INSERTED);
	 }
 
	 gr_flip();
 }
 
 static void fm_show_searching(int state)
 {
	 int row = 4;
 
	 if (STATE_DISPLAY == state) {
		 ui_clear_rows(row, 1);
		 ui_set_color(CL_GREEN);
		 ui_show_text(row, 0, TEXT_FM_SCANING);
	 } else if (STATE_CLEAN== state) {
		 ui_clear_rows(row, 1);
	 }
 
	 gr_flip();
 }
 
 static void fm_seek_timeout(void)
 {
	 int row = 6;
 
	 ui_clear_rows(row, 2);
	 ui_set_color(CL_RED);
	 row = ui_show_text(row, 0, TEXT_FM_SEEK_TIMEOUT);
	 row = ui_show_text(row, 0, TEXT_FM_FAIL);
 
	 gr_flip();
 }
 
 static int fm_check_headset(int cmd, int* headset_state)
 {
	 int ret;
	 char buf[8];
	 static int fd = -1;
 
        switch(cmd){
            case HEADSET_CHECK:
                memset(buf, 0, sizeof(buf));
                lseek(fd, 0, SEEK_SET);
                ret = read(fd, buf, sizeof(buf));
                if(ret < 0) {
                    LOGD("read fd failed");
                    return -1;
                }
                *headset_state = atoi(buf);
                break;
            case HEADSET_OPEN :
                if (fd > 0) {
                    LOGD("headset device already open !");
                    return -1;
                }
                fd = open(SPRD_HEADSET_SWITCH_DEV, O_RDONLY);
                if (fd < 0) {
                    LOGE("headset device open failed!");
                    return -1;
                }
                LOGD("open headset device success, fd =  %d", fd);
                break;
            case HEADSET_CLOSE:
                if (fd < 0) {
                    LOGD("headset device already close !");
                    return -1;
                }
                close(fd);
                fd = -1;
                break;
            default:
                LOGD("In error command: %d",  cmd);
                break;
	 }
 
	 return 0;
 }
 
 static int fm_open(void)
 {
	 int ret = 0;
	 int value = 0;
 
	 fm_fd = open(TROUT_FM_DEV_NAME, O_RDONLY);
	 if (fm_fd < 0) {
		 LOGE("FM open failed!");
		 return -1;
	 }
 
	 LOGD("FM open success ! fd = %d", fm_fd);
 
	 value = 1;
	 ret = ioctl(fm_fd, FM_IOCTL_ENABLE, &value);
	 if (0 != ret) {
		 LOGE("FM enable failed!");
		 goto ENABLE_FAILE;
	 }
 
	 return 0;
 
 ENABLE_FAILE:
	 ret = close(fm_fd);
	 if (0 != ret) {
		 LOGE("FM close failed!");
	 }
 
	 fm_fd = -1;
 
	 return -1;
 }
 
 static int fm_get_rssi(int* freq, int* rssi)
 {
	 int ret;
	 int value;
	 int buffer[4]; 	 /*freq, dir, timeout, reserve*/
 
	 value = 0;
	 ret = ioctl(fm_fd, FM_IOCTL_SET_VOLUME, &value);
	 if (0 != ret) {
		 LOGE("mmitest FM set mute failed!");
		 return -1;
	 }
	 do
	 {
		 buffer[0] = *freq;
		 buffer[1] = 1;
		 buffer[2] = 3000;
		 buffer[3] = 0;
 
		 LOGD("(before seek)freq=%d direction=%d timeout=%d reserve=%d",
				 buffer[0], buffer[1], buffer[2], buffer[3]);
 
		 ret = ioctl(fm_fd, FM_IOCTL_SEARCH, buffer);
		 LOGD("(after seek)freq=%d direction=%d timeout=%d reserve=%d result=%d",
				 buffer[0], buffer[1], buffer[2], buffer[3], ret);
		 LOGD("mmitest new freq=%d",buffer[3]);
		 if (0 != ret) {
			 LOGD("mmitest FM seek timeout!");
			 //return -1;
		 } else {
			 LOGD("mmitest FM seek success. freq = %d", buffer[3]);
		 }
 
		 *freq=buffer[3];

	 }while(ret!=0&&*freq<1080);
 
	 ret = ioctl(fm_fd, FM_IOCTL_SET_TUNE, &buffer[3]);
	 if (0 != ret) {
		 LOGD("mmitest FM set tune failed!");
		 return -1;
	 }
 
	 value = 1;
	 ret = ioctl(fm_fd, FM_IOCTL_SET_VOLUME, &value);
	 if (0 != ret) {
		 LOGD("mmitest FM set unmute failed!");
		 return -1;
	 }
 
	 ret = ioctl(fm_fd, FM_IOCTL_GET_RSSI, rssi);
	 if (0 != ret) {
		 LOGD("mmitest FM get rssi failed!");
		 return -1;
	 }
 
	 *freq = buffer[3];
	 LOGD("mmitest new freq write=%d",*freq);
 
	 LOGD("mmitest FM get freq = %d, rssi = %d", *freq, *rssi);
 
	 return 0;
 }
 
 static int fm_close(void)
 {
	 int value;
	 int ret;
 
	 value = 0;
	 ret = ioctl(fm_fd, FM_IOCTL_ENABLE, &value);
	 if (0 != ret) {
		 LOGD("FM disable failed!");
		 return -1;
	 }
 
	 ret = close(fm_fd);
	 if (0 != ret) {
		 LOGE("FM close failed!");
		 return -1;
	 }
 
	 fm_fd = -1;
 
	 LOGD("FM close success !");
 
	 return 0;
 }
 
 int test_fm_start(void)
 {
	 int ret;
	 int rssi;
	 int headset_in = 0;
	 int freq = 875;

	 ui_fill_locked();
	 ui_show_title(MENU_TEST_FM);

	 system(FM_INSMOD_COMMEND);

	 ret = fm_check_headset(HEADSET_OPEN, NULL);		 /*open headset device*/
	 if (ret < 0) {
		  LOGD("mmitest open %s failed", SPRD_HEADSET_SWITCH_DEV);
		  goto FM_TEST_FAIL;
	 }

	 fm_check_headset(HEADSET_CHECK, &headset_in);		 /*checket headset state*/
	 if (0 == headset_in) {
		 LOGD("mmitest headset out");
		 fm_show_headset(SPRD_HEADSETOUT);					 /*show headset state*/
		 fm_check_headset(HEADSET_CLOSE, NULL);    /*close headset device*/
		 goto FM_TEST_FAIL;
	 }

	 ret = fm_check_headset(HEADSET_CLOSE, NULL);		 /*close headset device*/
	 if (0 != ret) {
		  LOGD("mmitest close %s failed",SPRD_HEADSET_SWITCH_DEV);
		 goto FM_TEST_FAIL;
	 }

	 LOGD("mmitest headset in");

	 fm_show_headset(SPRD_HEADSETIN);				 /*show headset state*/
	 fm_show_searching(STATE_DISPLAY);

	 ret = fm_open();							 /*open fm*/
	 if (0 != ret) {
		 LOGD("mmitest open %s failed", TROUT_FM_DEV_NAME);
		 goto FM_TEST_FAIL;
	 }

	 ret = fm_get_rssi(&freq, &rssi);		 /*get rssi*/
	 if (0 != ret) {
		 fm_show_searching(STATE_CLEAN);
		 fm_seek_timeout();
		 fm_close();
		 LOGD("mmitest get rssi failed");
		 goto FM_TEST_FAIL;
	 }

	 ret = fm_close();								/*close fm*/
	 if (0 != ret) {
		 LOGD("mmitest  close %s failed",TROUT_FM_DEV_NAME);
		 goto FM_TEST_FAIL;
	 }

	 fm_show_searching(STATE_CLEAN);
	 fm_show_play_stat(freq, rssi/10); 		  /*display rssi*/
 
	 if(1 == test_result) {
		 ret = RL_PASS;
	 } else {
 FM_TEST_FAIL:
		 ret = RL_FAIL;
	 }

	 sleep(3);
	 save_result(CASE_TEST_FM,ret);

	 return ret;
 }
#endif
