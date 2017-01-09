#ifndef _FACTORY_H
#define _FACTORY_H


/**************Common define************/
#define AT_BUFFER_SIZE			2048
#define BUF_LEN 64
#define MAX_OPEN_TIMES  5

/**************Open Virtual key************/
#define SPRD_VIRTUAL_TOUCH

/**************Backlight************/
#define LCD_BACKLIGHT_DEV			"/sys/class/backlight/sprd_backlight/brightness"
#define KEY_BACKLIGHT_DEV 			"/sys/class/leds/keyboard-backlight/brightness"
#define LCD_BACKLIGHT_MAX_DEV		"/sys/class/backlight/sprd_backlight/max_brightness"
#define KEY_BACKLIGHT_MAX_DEV 		"/sys/class/leds/keyboard-backlight/max_brightness"
/*************end Backlight*********/

/************Camera************/
#define LIBRARY_PATH "/system/lib/hw/"
#define FLASH_SUPPORT "/sys/class/flash_test/flash_test/flash_value"
/*********end Bcamera***********/

/************BT*************/
#define BT_MAC_CONFIG_FILE_PATH  "/data/misc/bluedroid/btmac.txt"
#define BT_MAC_FACTORY_CONFIG_FILE_PATH  "/productinfo/btmac.txt"
#define BD_ADDR_LEN     6                         /* Device address length */
#define BD_NAME_LEN     248                       /* Device name length */
typedef unsigned char BD_ADDR[BD_ADDR_LEN];       /* Device address */
typedef unsigned char*BD_ADDR_PTR;                /* Pointer to Device Address */
typedef unsigned char BD_NAME[BD_NAME_LEN + 1];   /* Device name */
typedef unsigned char*BD_NAME_PTR;                /* Pointer to Device name */
#define MAC_LEN     (6)
#define MAX_REMOTE_DEVICES (10)
#define CASE_RETURN_STR(const) case const: return #const;
/******************BT end***************/

/**************Charge******************/
#define SPRD_CALI_MASK			0x00000200

#define ENG_BATVOL		"/sys/class/power_supply/battery/real_time_voltage"
#define ENG_CHRVOL		"/sys/class/power_supply/battery/charger_voltage"
#define ENG_CURRENT		"/sys/class/power_supply/battery/real_time_current"
#define ENG_USBONLINE	"/sys/class/power_supply/usb/online"
#define ENG_ACONLINE	"/sys/class/power_supply/ac/online"
#define ENG_BATONLINE   "/sys/class/power_supply/battery/health"
#define ENG_STOPCHG	    "/sys/class/power_supply/battery/stop_charge"
#define ENG_BATCURRENT  "/sys/class/power_supply/sprdfgu/fgu_current"
/****************end charge*************/

/**************FM5.1******************/

#define FM_INSMOD_COMMEND    "insmod /system/lib/modules/trout_fm.ko"

#define FM_IOCTL_BASE     	  'R'
#define FM_IOCTL_ENABLE       _IOW(FM_IOCTL_BASE, 0, int)
#define FM_IOCTL_GET_ENABLE   _IOW(FM_IOCTL_BASE, 1, int)
#define FM_IOCTL_SET_TUNE     _IOW(FM_IOCTL_BASE, 2, int)
#define FM_IOCTL_GET_FREQ     _IOW(FM_IOCTL_BASE, 3, int)
#define FM_IOCTL_SEARCH       _IOW(FM_IOCTL_BASE, 4, int[4])
#define FM_IOCTL_STOP_SEARCH  _IOW(FM_IOCTL_BASE, 5, int)
#define FM_IOCTL_SET_VOLUME   _IOW(FM_IOCTL_BASE, 7, int)
#define FM_IOCTL_GET_VOLUME   _IOW(FM_IOCTL_BASE, 8, int)
#define Trout_FM_IOCTL_CONFIG _IOW(FM_IOCTL_BASE, 9, int)
#define FM_IOCTL_GET_RSSI     _IOW(FM_IOCTL_BASE, 10, int)

#ifdef PLATFORM_VERSION6
#define TROUT_FM_DEV_NAME   		"/dev/fm"
#else
#define TROUT_FM_DEV_NAME               "/dev/Trout_FM"
#endif
/******************END FM5.1*******************/

/**************FM6.0******************/
typedef enum fm_bool {
    fm_false = 0,
    fm_true  = 1
} fm_bool;

typedef struct fm_tune_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint16_t freq;
}fm_tune_parm;

typedef struct fm_seek_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint8_t seekdir;
    uint8_t seekth;
    uint16_t freq;
}fm_seek_parm;

typedef struct fm_softmute_tune_t {
    int32_t rssi; // RSSI of current channel
    uint32_t freq; // current frequency
    fm_bool valid; // current channel is valid(true) or not(false)
}fm_softmute_tune_t;

// band
#define FM_BAND_UNKNOWN 0
#define FM_BAND_UE      1 // US/Europe band  87.5MHz ~ 108MHz (DEFAULT)
#define FM_BAND_JAPAN   2 // Japan band      76MHz   ~ 90MHz
#define FM_BAND_JAPANW  3 // Japan wideband  76MHZ   ~ 108MHz
#define FM_BAND_SPECIAL 4 // special   band  between 76MHZ   and  108MHz
#define FM_BAND_DEFAULT FM_BAND_UE

#define FM_UE_FREQ_MIN  875
#define FM_UE_FREQ_MAX  1080
#define FM_JP_FREQ_MIN  760
#define FM_JP_FREQ_MAX  1080
#define FM_FREQ_MIN  FM_UE_FREQ_MIN
#define FM_FREQ_MAX  FM_JP_FREQ_MAX

// auto HiLo
#define FM_AUTO_HILO_OFF    0
#define FM_AUTO_HILO_ON     1

// seek direction
#define FM_SEEK_UP          1
#define FM_SEEK_DOWN        0

// seek threshold
#define FM_SEEKTH_LEVEL_DEFAULT 4

#define FM_SEEK_SPACE      1 // FM radio seek space,1:100KHZ; 2:200KHZ
#define FM_VOICE_MUTE	1
#define FM_VOICE_ON	0
#define FM_IOC_MAGIC        0xf5

#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, int32_t*)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_SETVOL        _IOWR(FM_IOC_MAGIC, 4, uint32_t*)
#define FM_IOCTL_GETVOL        _IOWR(FM_IOC_MAGIC, 5, uint32_t*)
#define FM_IOCTL_MUTE          _IOWR(FM_IOC_MAGIC, 6, uint32_t*)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, int32_t*)
//#define FM_IOCTL_SCAN          _IOWR(FM_IOC_MAGIC, 8, struct fm_scan_parm*)
#define FM_IOCTL_STOP_SCAN     _IO(FM_IOC_MAGIC,   9)
#define FM_IOCTL_SOFT_MUTE_TUNE _IOWR(FM_IOC_MAGIC, 63, struct fm_softmute_tune_t*)/*for soft mute tune*/

/******************END FM6.0*******************/


#define SPRD_HEADSETOUT             0
#define SPRD_HEADSETIN              1

typedef enum
{
    HEADSET_CLOSE = 0,
    HEADSET_OPEN ,
    HEADSET_CHECK,
    HEADSET_INVALID
}HEADSET_CMD_TYPE;

#define STATE_CLEAN     			0
#define STATE_DISPLAY   			1
#define MAX_NAME_LEN 4096

#define START_FRQ	10410
#define END_FRQ     8750
#define THRESH_HOLD 100
#define DIRECTION   128
#define SCANMODE    0
#define MUTI_CHANNEL false
#define CONTYPE     0
#define CONVALUE    0
/******************END FM*******************/

/****************GPS****************/
#ifdef FUNCTION_DEBUG
#define FUN_ENTER	LOGD("%s enter \n",__FUNCTION__);
#define FUN_EXIT 	LOGD("%s exit \n",__FUNCTION__);
#else
#define FUN_ENTER
#define FUN_EXIT
#endif

#define GPSNATIVETEST_WAKE_LOCK_NAME  "gps_native_test"
#define GPS_TEST_PASS  TEXT_TEST_PASS   //"PASS"
#define GPS_TEST_FAILED  TEXT_TEST_FAIL //"FAILED"
#define GPS_TESTING  TEXT_BT_SCANING    //"TESTING......"
#define GPS_TEST_TIME_OUT	     (30) // s120
/********************end GPS*************/

/*****************Gsensor*************/
#define SPRD_GSENSOR_OFFSET					100
#define SPRD_GSENSOR_1G						1024
#define GSENSOR_TIMEOUT                     20
/***************end Gsensor************/

/****************Headerset***********/
#define SPRD_HEADSET_SWITCH_DEV 		"/sys/class/switch/h2w/state"
#define SPRD_HEASETKEY_DEV				"headset-keyboard"
#define SPRD_HEADSET_KEY	   			KEY_MEDIA
#define SPRD_HEADSET_KEYLONGPRESS		KEY_END
/**************end Headerset***********/

/***************key**************/
#define KEY_TIMEOUT 20
/***************end key**********/

/**************TP/Sensor**********/
static const char INPUT_EVT_NAME[]  = "/dev/input/event";
char TS_DEV_NAME[BUF_LEN];
char ACC_DEV_NAME[BUF_LEN];
char LUX_DEV_NAME[BUF_LEN];
char PXY_DEV_NAME[BUF_LEN];

enum sensor_type
{
	SENSOR_TS,
	SENSOR_ACC,
	SENSOR_PXY,
	SENSOR_LUX,
	SENSOR_NUM
};

/**************Modem**********/
#define PROP_MODEM_W_COUNT  "ro.modem.w.count"
#define PROP_MODEM_LTE_COUNT  "ro.modem.l.count"
#define PROP_MODEM_LTE_ENABLE  "persist.modem.l.enable"


#define MAX_MODEM_COUNT 	2
/*************end modem************/

/***************key*************/
struct test_key {
	int	key;
	int done;
	char key_name[BUF_LEN];
	char key_shown[BUF_LEN];
};

enum key_type
{
	KEY_TYPE_POWER,
	KEY_TYPE_VOLUMEDOWN,
	KEY_TYPE_VOLUMEUP,
	KEY_TYPE_CAMERA,
	KEY_TYPE_HOME,
	KEY_TYPE_MENU,
	KEY_TYPE_BACK,
	KEY_TYPE_NUM
};
/***************end key*************/

/***************sdcard*************/
#ifdef CONFIG_NAND
#define SPRD_SD_DEV			"/dev/block/mmcblk0p1"
#define SPRD_SD_DEV_SIZE	"/sys/block/mmcblk1/size"
#define SPRD_MOUNT_DEV		"mount -t vfat /dev/block/mmcblk0p1  /sdcard"
#else
#define SPRD_SD_DEV			"/dev/block/mmcblk1p1"
#define SPRD_SD_DEV_SIZE	"/sys/block/mmcblk1/size"
#define SPRD_MOUNT_DEV		"mount -t vfat /dev/block/mmcblk1p1  /sdcard"
#define SPRD_SD_MOUNT_DEV	"/dev/block/platform/sdio_sd/mmcblk1p1"
#endif

#define RW_LEN	512
/******************end sdcard*************/

#define ABS_MT_POSITION_X			0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y			0x36	/* Center Y ellipse position */
#define ABS_MT_TOUCH_MAJOR			0x30
#define SYN_REPORT					0
#define SYN_MT_REPORT				2
/****************end tp***************/

/**************UI*****************/
#define MAX_COLS 64
#define MAX_ROWS 48

#define CHAR_WIDTH 18
#ifdef SPRD_VIRTUAL_TOUCH
#define CHAR_HEIGHT 80
#else
#define CHAR_HEIGHT 35
#endif

#define PASS_POSITION (8*CHAR_HEIGHT)
#define FAIL_POSITION (16*CHAR_WIDTH)

#define PROGRESSBAR_INDETERMINATE_STATES 6
#define PROGRESSBAR_INDETERMINATE_FPS 15
/**************end UI**************/


/****************version**********/
#define PROP_ANDROID_VER	"ro.build.version.release"
#define PROP_SPRD_VER		"ro.build.description"
#define PROP_PRODUCT_VER	"ro.product.model"
#define PATH_LINUX_VER		"/proc/version"
/**************end version***********/


/***************vibrator***********/
#define VIBRATOR_ENABLE_DEV			"/sys/class/timed_output/vibrator/enable"

/************wifi*****************/
#define WIFI_ADDRESS        "/sys/class/net/wlan0/address"
#define WIFI_MAC_CONFIG_FILE_PATH  "/data/misc/wifi/wifimac.txt"
#define WIFI_MAC_FACTORY_CONFIG_FILE_PATH  "/productinfo/wifimac.txt"
/*************end wifi********/

/****************OTG*************/
#define OTG_FILE_PATH  "/sys/bus/platform/drivers/dwc_otg/is_support_otg"
#define OTG_INSERT_STATUS "sys/devices/20200000.usb/otg_status"
#define OTG_DEVICE_HOST "sys/devices/20200000.usb/mode"
#define OTG_TESTFILE_PATH "/system/bin/test"
/****************end OTG**********/

/******************Phase check*************/
#define  MAX_SN_LEN  24
#define SP09_MAX_SN_LEN   MAX_SN_LEN
#define SP09_MAX_STATION_NUM   15
#define SP09_MAX_STATION_NAME_LEN   10
#define SP09_SPPH_MAGIC_NUMBER   0x53503039
#define SP05_SPPH_MAGIC_NUMBER   0x53503035
#define SP09_MAX_LAST_DESCRIPTION_LEN   32

#define SN1_START_INDEX   4
#define SN2_START_INDEX  (SN1_START_INDEX + SP09_MAX_SN_LEN)

#define STATION_START_INDEX   56
#define TESTFLAG_START_INDEX  252
#define RESULT_START_INDEX   254
/****************end Phase check*************/

/******************wifi*************/
typedef struct wifi_ap_t {
    char smac[32]; // string mac
    char name[64]; //wifi name
    int  sig_level;
    int frequency;
}wifi_ap_t;

#define WIFI_STATUS_UNKNOWN     0x0001
#define WIFI_STATUS_OPENED      0x0002
#define WIFI_STATUS_SCANNING    0x0004
#define WIFI_STATUS_SCAN_END    0x0008

#define WIFI_CMD_RETRY_NUM      3
#define WIFI_CMD_RETRY_TIME     1 // second
#define WIFI_MAX_AP             20
#define EVT_MAX_LEN 127
/******************wifi end*************/

/***************Result Show************/
#define PCBATXTPATH "/productinfo/PCBAtest.txt"
#define PHONETXTPATH   "/productinfo/wholephonetest.txt"
typedef struct mmitest_result
{
    unsigned char type_id;
    unsigned char function_id;
    unsigned char eng_support;
    unsigned char pass_faild;   //0:not test,1:pass,2:fail
}mmi_result;

typedef struct mmi_show_data
{
	unsigned char id;
	char* name;
	int (*func)(void);
	mmi_result* mmi_result_ptr;
}mmi_show_data;

typedef struct hardware_result{
    char id;
    char support;
}hardware_result;

typedef struct {
	unsigned char num;
	char* title;
	int (*func)(void);
}menu_info;

typedef struct {
	char* result;
	char* title;
	int (*func)(void);
}menu_result_info;

typedef enum
{
    CASE_TEST_LCD,
    CASE_TEST_TP,
    CASE_TEST_MULTITOUCH,
    CASE_TEST_KEY,
    CASE_TEST_VIBRATOR,
    CASE_TEST_BACKLIGHT,
    CASE_TEST_FCAMERA,
    CASE_TEST_BCAMERA,
    CASE_TEST_FLASH,
    CASE_TEST_MAINLOOP,
    CASE_TEST_ASSISLOOP,
    CASE_TEST_SPEAKER,
    CASE_TEST_RECEIVER,
    CASE_TEST_HEADSET,
    CASE_TEST_SDCARD,
    CASE_TEST_SIMCARD,
    CASE_TEST_CHARGE,
    CASE_TEST_WIRELESSCHARGER,
    CASE_TEST_FM,
    CASE_TEST_ATV,
    CASE_TEST_DTV,
    CASE_TEST_BT,
    CASE_TEST_WIFI,
    CASE_TEST_GPS,
    CASE_TEST_RTC,
    CASE_TEST_OTG,
    CASE_TEST_TEL,
    CASE_TEST_NFC,
    CASE_TEST_CALIBINFO,
    CASE_TEST_SOFTCHECK,
    CASE_TEST_IRREMOTE,
    CASE_TEST_ACCSOR,
    CASE_TEST_MAGSOR,
    CASE_TEST_ORISOR,
    CASE_TEST_GYRSOR,
    CASE_TEST_LPSOR,
    CASE_TEST_PRESSOR,
    CASE_TEST_TEMPESOR,
    CASE_TEST_GSENSOR,
    CASE_TEST_LSENSOR,
    CASE_TEST_RVSOR,
    CASE_TEST_FINGERSOR,
    CASE_TEST_HUMISOR,
    CASE_TEST_HALLSOR,
    CASE_TEST_LED,
    CASE_TEST_EMMC,
    CASE_TEST_RESERVED2,
    CASE_TEST_RESERVED3,
    CASE_TEST_RESERVED4,
    CASE_TEST_RESERVED5,
    CASE_TEST_RESERVED6,
    CASE_TEST_FORCUST,
    CASE_TEST_FORCUST1,
    CASE_TEST_FORCUST2,
    CASE_TEST_FORCUST3,
    CASE_TEST_FORCUST4,
    CASE_TEST_FORCUST5,
    CASE_TEST_FORCUST6,
    CASE_TEST_FORCUST7,
    CASE_TEST_FORCUST8,
    CASE_TEST_FORCUST9,
    CASE_TEST_FORCUST10,
    CASE_TEST_FORCUST11,
    FINAL_RESULT_FLAG,
    TOTAL_NUM
}CASE_TEST_LIST;

typedef enum
{
	RESULT_NOT_TEST = 0,
	RESULT_PASS,
	RESULT_FAIL,
	RESULT_INVALID
}RESULT_TEST_LIST;

mmi_result phone_result[TOTAL_NUM];
mmi_result pcba_result[TOTAL_NUM];
hardware_result support_result[TOTAL_NUM];

menu_result_info menu_phone_result_menu[64];
menu_result_info menu_pcba_result_menu[64];
menu_result_info menu_not_suggestion_result_menu[64];

#endif
