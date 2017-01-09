#ifndef __CALIBRATION_H__

#define ADC_CHANNEL_PATH "/sys/class/misc/sprd-adc/adc_channel"
#define ADC_SCALE_PATH "/sys/class/misc/sprd-adc/adc_scale"
#define ADC_DATA_RAW_PATH "/sys/class/misc/sprd-adc/adc_data_raw"
#define ADC_CHANNEL_PATH_NEW "/sys/bus/iio/devices/iio:device0/adc_channel_set"
#define ADC_SCALE_PATH_NEW "/sys/bus/iio/devices/iio:device0/adc_scale_set"
#define ADC_DATA_RAW_PATH_NEW "/sys/bus/iio/devices/iio:device0/adc_info"
#define BATTERY_VOL_PATH "/sys/class/power_supply/battery/real_time_voltage"
#define BATTERY_ADC_PATH "/sys/class/power_supply/battery/real_time_vbat_adc"
#define CHARGER_STOP_PATH "/sys/class/power_supply/battery/stop_charge"
#define ADC_CHAN_FILE_PATH "/sys/kernel/debug/sc2713-regulator/adc_chan"
#define FGU_CURRENT_ADC_FILE_PATH \
  "/sys/class/power_supply/sprdfgu/fgu_current_adc"
#define FGU_VOL_ADC_FILE_PATH "/sys/class/power_supply/sprdfgu/fgu_vol_adc"
#define CHARGING_CURRENT_FILE_PATH \
  "sys/class/power_supply/battery/real_time_current"  // charging current
#define BATTERY_CURRENT_FILE_PATH \
  "sys/class/power_supply/sprdfgu/fgu_current"  // battery current
#define FGU_CURRENT_FILE_PATH "/sys/class/power_supply/sprdfgu/fgu_current"
#define FGU_VOL_FILE_PATH "/sys/class/power_supply/sprdfgu/fgu_vol"

#ifdef CONFIG_NAND
#define CALI_CTRL_FILE_PATH "/dev/ubi0_miscdata"
#else
#define CALI_CTRL_FILE_PATH "/dev/block/platform/sdio_emmc/by-name/miscdata"
#endif
#define BATTER_CALI_CONFIG_FILE CALI_CTRL_FILE_PATH
#define ADC_MAGIC (0x4144434D)  // ADCM, header flag of adc data
#define MISCDATA_BASE  (0)
#define ADC_DATA_OFFSET  (512 * 1024)
#define ADC_DATA_START (MISCDATA_BASE + ADC_DATA_OFFSET)
#define MAX_SN_LEN (24)
#define SP09_MAX_SN_LEN MAX_SN_LEN
#define SP09_MAX_STATION_NUM (15)
#define SP09_MAX_STATION_NAME_LEN (10)
#define SP09_MAX_LAST_DESCRIPTION_LEN (32)
typedef enum {
  DIAG_AP_CMD_ADC = 0x0001,
  DIAG_AP_CMD_LOOP,
  DIAG_AP_CMD_FILE_OPER,
  DIAG_AP_CMD_SWITCH_CP,
  DIAG_AP_CMD_BKLIGHT = 0x0005,
  DIAG_AP_CMD_PWMODE = 0x0007,
  DIAG_AP_CMD_CHANGE = 0x0010,
  DIAG_AP_CMD_READ_CURRENT = 0x0011,
  DIAG_AP_CMD_TSX_DATA = 0x0019,
  DIAG_AP_CMD_GET_MODEM_MODE = 0x0012,
  DIAG_AP_CMD_MODEM_DB_ATTR = 0x000e,
  DIAG_AP_CMD_MODEM_DB_READ = 0x000f,
  DIAG_AP_CMD_READ_MMI = 0x0013, //Read MMI
  DIAG_AP_CMD_WRITE_MMI = 0x0014, //Write MMI
  DIAG_AP_CMD_TEE_PRODUCTION = 0x001d,
  MAX_DIAG_AP_CMD
} DIAG_AP_CMD_E;

#define AP_ADC_CALIB 1
#define AP_ADC_LOAD 2
#define AP_ADC_SAVE 3
#define AP_GET_VOLT 4
#define AP_DIAG_LOOP 5
#define AP_GET_FGU_VOL_ADC 6
#define AP_GET_FGU_CURRENT_ADC 7
#define AP_GET_FGU_TYPE 8
#define AP_GET_FGU_VOL_REAL 9
#define AP_GET_FGU_CURRENT_REAL 10

#define CALI_MAGIC (0x49424143)  // CALI
#define CALI_COMP (0x504D4F43)  // COMP

typedef struct {
  unsigned int adc[2];  // calibration of ADC, two test point
  unsigned int
      battery[2];  // calibraton of battery(include resistance), two test point
  unsigned int reserved[8];  // reserved for feature use.
} AP_ADC_T;

typedef struct {
  unsigned int magic;      // when create ,magic = "CALI"
  unsigned int cali_flag;  // cali_flag   default 0xFFFFFFFF, when calibration
                           // finished,it is set "COMP"
  AP_ADC_T adc_para;       // ADC calibration data.
} CALI_INFO_DATA_T;

typedef struct {
  unsigned short cmd;     // DIAG_AP_CMD_E
  unsigned short length;  // Length of structure
} TOOLS_DIAG_AP_CMD_T;

typedef struct {
  unsigned int operate;  // 0: Get ADC   1: Load ADC    2: Save ADC
  unsigned int parameters[12];
} TOOLS_AP_ADC_REQ_T;

typedef struct {
  unsigned short status;  // ==0: success, != 0: fail
  unsigned short length;  // length of  result
} TOOLS_DIAG_AP_CNF_T;

typedef struct
{
  unsigned short  uType; //00: whole phone test; 01: PCBA test ; 02: BBAT test
  char uBuff[256]; //16*16
}TOOLS_DIAG_MMI_CIT_T;

typedef struct {
  int charging;
  int battery;
} TOOLS_DIAG_CHARGE_CURRENT_CNF_T;

typedef struct
{
  unsigned int freq;
  unsigned int temprature;
}TSX_DATA_T;

typedef struct
{
  unsigned int cmd;
  unsigned int res_status;
  TSX_DATA_T value[2];
}TOOLS_DIAG_AP_TSX_DATA_T;

typedef struct {
  MSG_HEAD_T msg_head;
  TOOLS_DIAG_AP_CNF_T diag_ap_cnf;
  TOOLS_AP_ADC_REQ_T ap_adc_req;
} MSG_AP_ADC_CNF;

typedef struct {
  unsigned int on_off;  // 1:0n    0:offf
} TOOLS_DIAG_AP_CHARGE_T;

typedef struct { char modem_mode[64]; } TOOLS_DIAG_AP_MODULE_T;

typedef struct _tagSP09_PHASE_CHECK {
  uint32_t Magic;             // "SP09"
  char SN1[SP09_MAX_SN_LEN];  // SN , SN_LEN=24
  char SN2[SP09_MAX_SN_LEN];  // add for Mobile
  int StationNum;             // the test station number of the testing
  char StationName[SP09_MAX_STATION_NUM][SP09_MAX_STATION_NAME_LEN];
  unsigned char Reserved[13];  //
  unsigned char SignFlag;
  char szLastFailDescription[SP09_MAX_LAST_DESCRIPTION_LEN];
  unsigned short iTestSign;  // Bit0~Bit14 ---> station0~station 14
  // if tested. 0: tested, 1: not tested
  unsigned short iItem;  // part1: Bit0~ Bit_14 indicate test Station,1 : Pass,

} SP09_PHASE_CHECK_T, *LPSP09_PHASE_CHECK_T;

void initialize_ctrl_file(void);

#define __CALIBRATION_H__

#endif
