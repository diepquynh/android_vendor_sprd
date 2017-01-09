#ifndef __ENG_DIAG_H__
#define __ENG_DIAG_H__

/*got it from tool*/
#define DIAG_CHANGE_MODE_F  12
typedef enum
{
    NORMAL_MODE                         = 0,
    LAYER1_TEST_MODE                    = 1,
    ASSERT_BACK_MODE                    = 2,
    CALIBRATION_MODE                    = 3,
    DSP_CODE_DOWNLOAD_BACK              = 4,
    DSP_CODE_DOWNLOAD_BACK_CALIBRATION  = 5,
    BOOT_RESET_MODE                     = 6,
    PRODUCTION_MODE                     = 7,
    RESET_MODE                          = 8,
    CALIBRATION_POST_MODE               = 9,
    PIN_TEST_MODE                       = 10,
    IQC_TEST_MODE                       = 11,
    WATCHDOG_RESET_MODE                 = 12,

    CALIBRATION_NV_ACCESS_MODE          = 13,
    CALIBRATION_POST_NO_LCM_MODE        = 14,

    TD_CALIBRATION_POST_MODE            = 15,
    TD_CALIBRATION_MODE                 = 16,
    TD_CALIBRATION_POST_NO_LCM_MODE     = 17,

    MODE_MAX_TYPE,

    MODE_MAX_MASK                       = 0x7F

}MCU_MODE_E;
typedef enum
{
    ENG_WARM_START = 0x1,
    ENG_COLD_START = 0x7D,
    ENG_HOT_START = 0x400,
    ENG_FAC_START = 0xFFFF,
    ENG_GPS_START_MAX
}ENG_GPS_START_TYPE_E;

#define MAX_IMEI_LENGTH		8
#define MAX_IMEI_STR_LENGTH 15
#define MAX_BTADDR_LENGTH	6
#define MAX_WIFIADDR_LENGTH	6
#define GPS_NVINFO_LENGTH	44
#define DIAG_HEADER_LENGTH	8

#define DIAG_CMD_VER		0x00
#define DIAG_CMD_IMEIBTWIFI	0x5E
#define DIAG_CMD_READ		0x80
#define DIAG_CMD_GETVOLTAGE	0x1E
#define DIAG_CMD_APCALI		0x62
#define DIAG_CMD_FACTORYMODE	0x0D
#define DIAG_CMD_ADC_F		0x0F  //add by kenyliu on 2013 07 12 for get ADCV  bug 188809
#define DIAG_FM_TEST_F          0x41  //FM pandora
#define DIAG_CMD_AT 		0x68
#define DIAG_CMD_CHANGEMODE     DIAG_CHANGE_MODE_F

#define DIAG_CMD_DIRECT_PHSCHK  0x5F

#define DIAG_CMD_IMEI1BIT       0x01
#define DIAG_CMD_IMEI2BIT       0x02
#define DIAG_CMD_IMEI3BIT       0x10
#define DIAG_CMD_IMEI4BIT       0x20
#define DIAG_CMD_BTBIT			0x04
#define DIAG_CMD_WIFIBIT		0x40
#define DIAG_CMD_AUTOTEST       0x38

#define DIAG_CMD_CURRENT_TEST       0x11

#define DIAG_SYSTEM_F         0x5
#define DIAG_CMD_WIFI_TEST_F	    0x36
#define DIAG_CMD_BUSMONITOR        0xFD

#define AUDIO_NV_ARM_INDI_FLAG          0x02
#define AUDIO_ENHA_EQ_INDI_FLAG         0x04
#define AUDIO_DATA_READY_INDI_FLAG      (AUDIO_NV_ARM_INDI_FLAG|AUDIO_ENHA_EQ_INDI_FLAG)

#define DIAG_SUB_MMICIT_READ    0x1A 

#define ENG_TESTMODE			"engtestmode"
#define ENG_SPRD_VERS			"ro.build.description"
#define ENG_SET_BACKLIGHT	        "/sys/class/backlight/sprd_backlight/brightness"
#define ENG_TXDATA_FILE  "/productinfo/txdata.txt"
typedef enum
{
    CMD_COMMON=-1,
    CMD_USER_VER,
    CMD_USER_BTWIFIIMEI,
    CMD_USER_FACTORYMODE,
    CMD_USER_AUDIO,
    CMD_USER_RESET,
    CMD_USER_GETVOLTAGE,
    CMD_USER_APCALI,
    CMD_USER_APCMD,
    CMD_USER_ADC,
    CMD_USER_FM,
    CMD_USER_PRODUCT_CTRL,
    CMD_USER_DIRECT_PHSCHK,
    CMD_USER_MMICIT_READ,
    CMD_USER_DEEP_SLEEP,
    CMD_USER_FILE_OPER,
    CMD_USER_CFT_SWITCH,
    CMD_USER_BKLIGHT,
    CMD_USER_TXDATA,
    CMD_USER_PWMODE,
    CMD_USER_SET_CONFIGURE_IP,
    CMD_USER_READ_REGISTER,
    CMD_USER_WRITE_REGISTER,
    CMD_USER_SHUT_DOWN,
    CMD_USER_GPS_AUTO_TEST,
    CMD_USER_AUTOTEST,
    CMD_USER_AUTOTEST_PATH_CONFIRM = 0x1c,
    CMD_USER_ENABLE_CHARGE_ONOFF,
    CMD_USER_GET_CHARGE_CURRENT,
    CMD_USER_READ_EFUSE,
    CMD_USER_WRITE_EFUSE,
    CMD_USER_READ_PUBLICKEY,
    CMD_USER_ENABLE_SECURE,
    CMD_USER_READ_ENABLE_SECURE_BIT,
    CMD_USER_GET_MODEM_MODE,
    CMD_USER_ENABLE_BUSMOINITOR,
    CMD_USER_DISABLE_BUSMOINITOR,
    CMD_USER_GET_CHANINFO,
    CMD_USER_GET_RTCTIME,
    CMD_USER_GET_MONITORDATA,
    CMD_USER_MODEM_DB_ATTR,
    CMD_USER_MODEM_DB_READ,
    CMD_USER_READ_MMI = 0x3c,/* 0x3c*/
    CMD_USER_WRITE_MMI,
    CMD_INVALID
}DIAG_CMD_TYPE;

typedef enum
{
    CMD_AUTOTEST_DUMMY,
    CMD_AUTOTEST_KEYPAD,
    CMD_AUTOTEST_LCD_PARALLEL,
    CMD_AUTOTEST_LCD_SPI,
    CMD_AUTOTEST_CAMERA_IIC,
    CMD_AUTOTEST_CAMERA_PARALLEL,
    CMD_AUTOTEST_CAMERA_SPI,
    CMD_AUTOTEST_GPIO, // and TP test
    CMD_AUTOTEST_TF,
    CMD_AUTOTEST_SIM,
    CMD_AUTOTEST_MIC,
    CMD_AUTOTEST_SPEAK,
    CMD_AUTOTEST_MISC,
    CMD_AUTOTEST_FM,
    CMD_AUTOTEST_ATV,
    CMD_AUTOTEST_BT,
    CMD_AUTOTEST_WIFI,
    CMD_AUTOTEST_IIC_DEV,
    CMD_AUTOTEST_CHARGE,
	//-- [[
	CMD_AUTOTEST_RSV01,  // 19
	CMD_AUTOTEST_RSV02,  // 20
	CMD_AUTOTEST_SENSOR, // 21
	//-- ]]
    CMD_AUTOTEST_GPS,
    CMD_AUTOTEST_END
}DIAG_AUTOTEST_CMD_TYPE;
struct eng_autotestcmd_str{
    int index;
    int (*cmd_hdlr)(char *, char *);
};

#define ENG_DIAG_SIZE 4096
#define ENG_LINUX_VER	"/proc/version"
#define ENG_ANDROID_VER "ro.build.version.release"
#define ENG_AUDIO       "/sys/class/vbc_param_config/vbc_param_store"
#define ENG_FM_DEVSTAT	"/sys/class/fm_devstat_config/fm_devstat_store"

/*the add the file result*/
#define BBAT_TEST_FILE_PATH "/productinfo/BBATtest.txt"
#define PCBA_TEST_FILE_PATH "/productinfo/PCBAtest.txt"
#define WHOLE_PHONE_TEST_FILE_PATH "/productinfo/wholephonetest.txt"

#define HASH_LEN                    40

#define MAX_SN_LEN                  24
#define MAX_STATION_NUM             15
#define MAX_STATION_NAME_LEN        10
#define SP09_SPPH_MAGIC_NUMBER   0x53503039

#define SP15_MAX_SN_LEN                  64
#define SP15_MAX_STATION_NUM             20
#define SP15_MAX_STATION_NAME_LEN        15
#define SP15_SPPH_MAGIC_NUMBER   0x53503135

#define MAX_LAST_DESCRIPTION_LEN    32

#define RW_MASK                     0x80 //(BIT_7)
#define WRITE_MODE                  0
#define RM_VALID_CMD_MASK           0x7f
#define DIAG_CMD_GPS_AUTO_TEST      0x3A
#define MSG_NACK                    0
#define MSG_ACK                     1

#define ENG_MAX_NAME_LEN            260
#define MAX_DIAG_TRANSMIT_FILE_LEN  8192

#define SHA1_SIZE       20
#define ENG_MODEMDB_DIAG_SIZE       (64*1024)
#define MAX_DIAG_TRANSMIT_MODEMDB_LEN	(ENG_MODEMDB_DIAG_SIZE-128)

typedef enum{
    IMEI_ERR_NONE = 0,
    IMEI_CRC_ERR,
    IMEI_CMD_ERR,
    IMEI_SAVE_ERR,
    IMEI_READ_ERR
}ERR_IMEI_E;
// This is the communication frame head
typedef struct msg_head_tag
{
    unsigned int  seq_num;      // Message sequence number, used for flow control
    unsigned short  len;          // The totoal size of the packet "sizeof(MSG_HEAD_T)
    // + packet size"
    unsigned char   type;         // Main command type
    unsigned char   subtype;      // Sub command type
}__attribute__((packed)) MSG_HEAD_T;

typedef struct {
    unsigned char byEngineSn[24];
    unsigned int  dwMapVersion;
    unsigned char byActivatecode[16];
}GPS_NV_INFO_T;

typedef struct {
	char type_id;
	char function_id;
	char support;
	char status;
}TEST_NEW_RESULT_INFO;

typedef struct hardware_result{
	char id;
	char support;
}hardware_result;

typedef struct {
    unsigned char imei1[MAX_IMEI_LENGTH];
    unsigned char imei2[MAX_IMEI_LENGTH];
    unsigned char btaddr[MAX_BTADDR_LENGTH];
    unsigned char gpsinfo[GPS_NVINFO_LENGTH];
    unsigned char wifiaddr[MAX_WIFIADDR_LENGTH];
    unsigned char reserved1[2];
    unsigned char imei3[MAX_IMEI_LENGTH];
    unsigned char imei4[MAX_IMEI_LENGTH];
    unsigned char reserved2[16];
}REF_NVWriteDirect_T;

typedef struct _SP09_PHASE_CHECK_HEADER
{
    unsigned int Magic; //"SP09"
    unsigned char SN[MAX_SN_LEN];   //SN,SN_LEN=24
    unsigned char SN2[MAX_SN_LEN];  //Add for Mobile

    unsigned int StationNum;   //The test station number of the testing
    unsigned char StationName[MAX_STATION_NUM][MAX_STATION_NAME_LEN];

    unsigned char Reserved[13]; //value: 0
    unsigned char SignFlag; // internal flag
    char szLastFailDescription[MAX_LAST_DESCRIPTION_LEN];
    unsigned short iTestSign; // Bit0~Bit14 --> station0 ~ station14 if tested. 0:tested,1:not tested.
    unsigned short iItem; // Part1:Bit0~Bit14 indicate test station,0:pass,1:fail
    // Part2:Bit15 set to 0;
}SP09_TEST_TRACK_HEADER_T;

typedef struct _SP09_PHASE_CHECK_S
{
    SP09_TEST_TRACK_HEADER_T header;
}SP09_TEST_DATA_INFO_T;

/**/
typedef struct _SP15_PHASE_CHECK_HEADER
{
    unsigned int Magic; //"SP15"
    unsigned char SN[SP15_MAX_SN_LEN];   //SN,SN_LEN=24
    unsigned char SN2[SP15_MAX_SN_LEN];  //Add for Mobile

    unsigned int StationNum;   //The test station number of the testing
    unsigned char StationName[SP15_MAX_STATION_NUM][SP15_MAX_STATION_NAME_LEN];

    unsigned char Reserved[13]; //value: 0
    unsigned char SignFlag; // internal flag
    char szLastFailDescription[MAX_LAST_DESCRIPTION_LEN];
    unsigned long iTestSign; // Bit0~Bit14 --> station0 ~ station14 if tested. 0:tested,1:not tested.
    unsigned long iItem; // Part1:Bit0~Bit14 indicate test station,0:pass,1:fail
    // Part2:Bit15 set to 0;
}SP15_TEST_TRACK_HEADER_T;

typedef struct _SP15_PHASE_CHECK_S
{
    SP15_TEST_TRACK_HEADER_T header;
}SP15_TEST_DATA_INFO_T;

typedef struct
{
    unsigned int file_cmd;
    unsigned char file_name[ENG_MAX_NAME_LEN];
}__attribute__((packed))TOOLS_DIAG_AP_FILEOPER_REQ_T;

typedef struct
{
    unsigned int file_size;
}TOOLS_DIAG_AP_FILE_STATUS_T;

typedef struct
{
    unsigned int status; // 0: finished, 1: under reading or writing
    unsigned int data_len;// specifies the data length to read/write
    unsigned char data[MAX_DIAG_TRANSMIT_FILE_LEN];
}__attribute__((packed))TOOLS_DIAG_AP_FILE_DATA_T;

typedef struct
{
    unsigned short cp_no;
}TOOLS_DIAG_AP_SWITCH_CP_T;

#pragma pack(1)
typedef struct {
   unsigned short blockid;
   unsigned long data;
}EFUSE_READ_INFO_T_RES;
typedef struct{
   unsigned short blockid;
   unsigned char flag;
}EFUSE_INFO_T_RES;
#pragma pack()
typedef struct wifi_configure_ip_t
{
    char szSSID[16];
    char szIPSetting[32];
    char szIPAddress[16];
    char szGateway[16];
    char szDNS[16];
    char szSubnet[16];
}__attribute__((packed))WIFI_CONFIGURE_IP_T;

typedef struct wifi_register_req_t
{
    char            szType[8]; //MAC, PHY , RF
    unsigned int    nRegAddr;
    unsigned int    nCount;
    unsigned int nUit; //BYTE:1, WORD:2, DWORD:4
}__attribute__((packed))WIFI_REGISTER_REQ_T;

typedef struct  __attribute__((packed))
{
	uint16_t cmd;     // AP sub cmd
	uint16_t length;  // Structure length of related command response
	uint16_t status;  // Operation status,  0: success, others fail
}TOOLS_DIAG_AP_CNF_MD_T;

typedef struct __attribute__((packed))
{
	uint32_t type_flags;
	uint32_t offset;
	uint32_t length;
}DataBlockHeader_T;

typedef struct __attribute__((packed))
{
	uint8_t sn;
	uint8_t total;
	uint8_t data[MAX_DIAG_TRANSMIT_MODEMDB_LEN];
}ModemDB_T;

typedef struct
{
    uint8_t lpSHA1[SHA1_SIZE];
}DBSHA1_T;

int eng_diag(char *buf,int len);
int eng_diag_writeimei(char *req, char *rsp);
void *eng_vlog_thread(void *x);
void *eng_vdiag_wthread(void *x);
void *eng_vdiag_rthread(void *x);
void * eng_sd_log(void * args);
void *eng_gps_log_thread(void *x);
int eng_diag_decode7d7e(char *buf,int len);
#endif
