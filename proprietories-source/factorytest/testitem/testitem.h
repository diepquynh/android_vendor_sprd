#ifndef __TEST_ITEM_H__
#define __TEST_ITEM_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <linux/input.h>
#include <utils/Timers.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <hardware/sensors.h>
#include <hardware_legacy/wifi.h>
#include <netutils/ifc.h>

#include "audio.h"

#include "../minui/minui.h"
#include "../common.h"
#include "../resource.h"
#include "../ui.h"
#include "../eng_tok.h"
#include "factorytest.h"

enum {
	RL_NA = 0,
	RL_PASS = 1,
	RL_FAIL = 2,
	RL_NS = 3,
	RL_NEXT_PAGE = 4,
	RL_BACK = 5,
};

#define AUDCTL "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_DATA_CONTROL_PIPE "/data/local/media/mmi.audio.ctrl"
#define SPRD_AUDIO_FILE	"/system/media/engtest_sample.pcm"
#define SENCALI	"/sys/class/sprd_sensorhub/sensor_hub/calibrator_cmd"
#define SENDATA	"/sys/class/sprd_sensorhub/sensor_hub/calibrator_data"

typedef enum{
	SPEAKER,
	RECEIVER,
	HEADPHONE,
	FM_PLAY,
	PHONE
}eng_audio_type;
/*
typedef enum {
    AUDIO_DEVICE_NONE  = 0,
    AUDIO_DEVICE_OUT_EARPIECE                  = 0x1,
    AUDIO_DEVICE_OUT_SPEAKER                   = 0x2,
    AUDIO_DEVICE_OUT_WIRED_HEADSET             = 0x4,
    AUDIO_DEVICE_OUT_WIRED_HEADPHONE           = 0x8,
}aud_outdev_e ;

typedef enum {
    AUDIO_MODE_NORMAL = 0,
    AUDIO_MODE_IN_CALL = 2,
}aud_mode;
*/
typedef enum {
	CALIB_EN,
	CALIB_CHECK_STATUS,
	CALIB_DATA_WRITE,
	CALIB_DATA_READ,
	CALIB_FLASH_WRITE,
	CALIB_FLASH_READ,
} CALIBRATOR_CMD;

typedef struct eng_mode{
	eng_audio_type mode;
	int	finished;
	int phase;
	union{
		int data;
	}u;
}eng_audio_mode;

/************************/
void save_result(unsigned char id,char key_result);
int test_lcd_start(void);
int test_msensor_start(void);
int test_key_start(void);
int test_vibrator_start(void);
int test_version_show(void);
int test_phone_info_show(void);
int test_cali_info(void);
int test_backlight_start(void);
int test_vb_bl_start(void);
int test_led_start(void);
int test_tp_start(void);
int test_multi_touch_start(void);
int test_mainloopback_start(void);//+++++++++
int test_assisloopback_start(void);//++++++++++++
int test_sdcard_start(void);
int test_emmc_start(void);
int test_rtc_start(void);
int sdcard_read_fm(int *rd);
int sdcard_write_fm(int *rd);
int test_sdcard_pretest(void);
int test_gsensor_start(void);
int test_lsensor_start(void);
int test_psensor_start(void);
int test_asensor_start(void);
int cali_asensor_start(void);
int SenCaliCmd(const char * cmd);
int cali_gsensor_start(void);
int cali_msensor_start(void);
int cali_prosensor_start(void);
int cali_auto_prosensor_start(void);
int test_bcamera_start(void);
int test_acamera_start(void);
int test_flash_start(void);
int flash_start(void);
int test_facamera_start(void);
int test_fcamera_start(void);
int test_charge_start(void);
int test_headset_start(void);
int test_tel_start(void);
int test_otg_start(void);
int test_fm_start(void);
void eng_bt_scan_start(void);
int test_bt_start(void);
int test_bt_pretest(void);
int eng_wifi_scan_start(void);
int test_wifi_start(void);
int wifiClose( void );
int test_wifi_pretest(void);
int test_gps_start(void);
int test_gps_pretest(void);
int gpsOpen(void);
int test_sim_start(void);
int test_sim_pretest(void);
int test_receiver_start(void);
int test_speaker_start(void);
int SendAudioTestCmd(const uchar * cmd,int bytes);
int test_soundtrigger_start(void);
void test_fingersor_start(void);
int test_modem_init(void);
char* test_modem_get_ver(void);
int tel_send_at(int fd, char* cmd, char* buf,unsigned int buf_len, int wait);
int modem_send_at(int fd, char* cmd, char* buf,unsigned int buf_len, int wait);
int find_input_dev(int mode, const char *event_name);
int get_sensor_name(const char * name );
int sensor_load();
int sensor_disable(const char *sensorname);
int sensor_stop();
int sensor_enable(const char *sensorname);
int enable_sensor();

extern struct sensors_poll_device_t *device;
extern struct sensors_module_t *module;
extern struct sensor_t const *list;
extern int senloaded;



#endif /*__TEST_ITEM_H_*/
