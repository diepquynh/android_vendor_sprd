#ifndef __FMTEST_H__
#define __FMTEST_H__

struct fm_tune_parm {
    unsigned char err;
    unsigned char band;
    unsigned char space;
    unsigned char hilo;
    unsigned short freq;
};

struct fm_seek_parm {
	unsigned char err;
	unsigned char band;
	unsigned char space;
	unsigned char hilo;
	unsigned char seekdir;
	unsigned char seekth;
	unsigned short freq;
};

struct fm_scan_all_parm {
unsigned char band;//87.5~108,76~
unsigned char space;//50 or 100KHz
unsigned char chanel_num;
unsigned short freq[36]; // OUT parameter
};

struct fm_ctl_parm {
    unsigned char err;
    unsigned char addr;
    unsigned short val;
    unsigned short rw_flag; // 0:write, 1:read
};

struct rdslag {
	uint8_t TP;
	uint8_t TA;
	uint8_t music;
	uint8_t stereo;
	uint8_t artificial_head;
	uint8_t compressed;
	uint8_t dynamic_pty;
	uint8_t text_ab;
	uint32_t flag_status;
};

struct ct_info {
	uint16_t month;
	uint16_t day;
	uint16_t year;
	uint16_t hour;
	uint16_t minute;
	uint8_t local_time_offset_signbit;
	uint8_t local_time_offset_half_hour;
};

struct  af_info {
	int16_t AF_NUM;
	int16_t AF[2][25];
	uint8_t addr_cnt;
	uint8_t ismethod_a;
	uint8_t isafnum_get;
};

struct  ps_info {
	uint8_t PS[4][8];
	uint8_t addr_cnt;
};

struct  rt_info {
	uint8_t textdata[4][64];
	uint8_t getlength;
	uint8_t isrtdisplay;
	uint8_t textlength;
	uint8_t istypea;
	uint8_t bufcnt;
	uint16_t addr_cnt;
};

struct rds_group_cnt {
	unsigned long total;
	unsigned long groupA[16]; /* RDS groupA counter*/
	unsigned long groupB[16]; /* RDS groupB counter */
};

enum rds_group_cnt_opcode {
	RDS_GROUP_CNT_READ = 0,
	RDS_GROUP_CNT_WRITE,
	RDS_GROUP_CNT_RESET,
	RDS_GROUP_CNT_MAX
};

struct rds_group_cnt_req {
	int err;
	enum rds_group_cnt_opcode op;
	struct rds_group_cnt gc;
};

struct fm_rds_data {
	struct ct_info CT;
	struct rdslag RDSFLAG;
	uint16_t PI;
	uint8_t switch_tp;
	uint8_t PTY;
	struct  af_info af_data;
	struct  af_info afon_data;
	uint8_t radio_page_code;
	uint16_t program_item_number_code;
	uint8_t extend_country_code;
	uint16_t language_code;
	struct  ps_info ps_data;
	uint8_t ps_on[8];
	struct  rt_info rt_data;
	uint16_t event_status;
	struct rds_group_cnt gc;
};

/*Frequency offset, PDP_TH,PHP_TH, SNR_TH,RSS_THI*/
/** Frequency_Offset_Th        [0x0000 0xFFFF]   EXPERIENCE VALUES:0x5dc  */
/** Pilot_Power_Th RANGES:   [0x0000 0x1FFF]   EXPERIENCE VALUES:0x190  */
/** Noise_Power_Th RANGES:  [0x0000 0x1FFF]   EXPERIENCE VALUES:0xB0   */
struct fm_seek_criteria_parm {
	unsigned char rssi_th;
	unsigned char snr_th;
	unsigned short freq_offset_th;
	unsigned short pilot_power_th;
	unsigned short noise_power_th;
} __packed;

struct fm_audio_threshold_parm {
	unsigned short hbound;
	unsigned short lbound;
	unsigned short power_th;
	unsigned char phyt;
	unsigned char snr_th;
} __packed;
/*__attribute__ ((packed));*/

struct fm_reg_ctl_parm {
	unsigned char err;
	unsigned int addr;
	unsigned int val;
	/*0:write, 1:read*/
	unsigned char rw_flag;
} __packed;

struct fm_iq_data {
	/* defaut: 0xa [0x0-0x1f] */
	unsigned char level;
	/* defaut: 0x1000, max:0x2000 */
	unsigned short num;
} __packed;

/* ********** ***********FM IOCTL define start ****************/
#define FM_IOC_MAGIC        0xf5

#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, int32_t*)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_SETVOL        _IOWR(FM_IOC_MAGIC, 4, uint32_t*)
#define FM_IOCTL_GETVOL        _IOWR(FM_IOC_MAGIC, 5, uint32_t*)
#define FM_IOCTL_MUTE          _IOWR(FM_IOC_MAGIC, 6, uint32_t*)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, int32_t*)
#define FM_IOCTL_SCAN          _IOWR(FM_IOC_MAGIC, 8, struct fm_scan_parm*)
#define FM_IOCTL_STOP_SCAN     _IO(FM_IOC_MAGIC,   9)

/* IOCTL and struct for test */
#define FM_IOCTL_GETCHIPID     _IOWR(FM_IOC_MAGIC, 10, uint16_t*)
#define FM_IOCTL_EM_TEST       _IOWR(FM_IOC_MAGIC, 11, struct fm_em_parm*)

#define FM_IOCTL_GETMONOSTERO  _IOWR(FM_IOC_MAGIC, 13, uint16_t*)
#define FM_IOCTL_GETCURPAMD    _IOWR(FM_IOC_MAGIC, 14, uint16_t*)
#define FM_IOCTL_GETGOODBCNT   _IOWR(FM_IOC_MAGIC, 15, uint16_t*)
#define FM_IOCTL_GETBADBNT     _IOWR(FM_IOC_MAGIC, 16, uint16_t*)
#define FM_IOCTL_GETBLERRATIO  _IOWR(FM_IOC_MAGIC, 17, uint16_t*)

/* IOCTL for RDS */
#define FM_IOCTL_RDS_ONOFF     _IOWR(FM_IOC_MAGIC, 18, uint16_t*)
#define FM_IOCTL_RDS_SUPPORT   _IOWR(FM_IOC_MAGIC, 19, int32_t*)

#define FM_IOCTL_RDS_SIM_DATA  _IOWR(FM_IOC_MAGIC, 23, uint32_t*)
#define FM_IOCTL_IS_FM_POWERED_UP  _IOWR(FM_IOC_MAGIC, 24, uint32_t*)

/* IOCTL for FM over BT */
#define FM_IOCTL_OVER_BT_ENABLE  _IOWR(FM_IOC_MAGIC, 29, int32_t*)

/* IOCTL for FM ANTENNA SWITCH */
#define FM_IOCTL_ANA_SWITCH     _IOWR(FM_IOC_MAGIC, 30, int32_t*)
#define FM_IOCTL_GETCAPARRAY      _IOWR(FM_IOC_MAGIC, 31, int32_t*)

/* IOCTL for FM I2S Setting  */
#define FM_IOCTL_I2S_SETTING  _IOWR(FM_IOC_MAGIC, 33, struct fm_i2s_setting*)

#define FM_IOCTL_RDS_GROUPCNT   _IOWR(FM_IOC_MAGIC, 34, \
				struct rds_group_cnt_req*)
#define FM_IOCTL_RDS_GET_LOG    _IOWR(FM_IOC_MAGIC, 35, struct rds_raw_data*)

#define FM_IOCTL_SCAN_GETRSSI   _IOWR(FM_IOC_MAGIC, 36, struct fm_rssi_req*)
#define FM_IOCTL_SETMONOSTERO   _IOWR(FM_IOC_MAGIC, 37, int32_t)
#define FM_IOCTL_RDS_BC_RST     _IOWR(FM_IOC_MAGIC, 38, int32_t*)
#define FM_IOCTL_CQI_GET     _IOWR(FM_IOC_MAGIC, 39, struct fm_cqi_req*)
#define FM_IOCTL_GET_HW_INFO    _IOWR(FM_IOC_MAGIC, 40, struct fm_hw_info*)
#define FM_IOCTL_GET_I2S_INFO   _IOWR(FM_IOC_MAGIC, 41, struct fm_i2s_info_t*)
#define FM_IOCTL_IS_DESE_CHAN   _IOWR(FM_IOC_MAGIC, 42, int32_t*)
//#define FM_IOCTL_TOP_RDWR _IOWR(FM_IOC_MAGIC, 43, struct fm_top_rw_parm*)
//#define FM_IOCTL_HOST_RDWR  _IOWR(FM_IOC_MAGIC, 44, struct fm_host_rw_parm*)

#define FM_IOCTL_PRE_SEARCH _IOWR(FM_IOC_MAGIC, 45, int32_t)
#define FM_IOCTL_RESTORE_SEARCH _IOWR(FM_IOC_MAGIC, 46, int32_t)

#define FM_IOCTL_SET_SEARCH_THRESHOLD   _IOWR(FM_IOC_MAGIC, 47, \
		fm_search_threshold_t*)

#define FM_IOCTL_GET_AUDIO_INFO _IOWR(FM_IOC_MAGIC, 48, struct fm_audio_info_t*)

#define FM_IOCTL_SCAN_NEW       _IOWR(FM_IOC_MAGIC, 60, struct fm_scan_t*)
#define FM_IOCTL_SEEK_NEW       _IOWR(FM_IOC_MAGIC, 61, struct fm_seek_t*)
#define FM_IOCTL_TUNE_NEW       _IOWR(FM_IOC_MAGIC, 62, struct fm_tune_t*)

#define FM_IOCTL_SOFT_MUTE_TUNE _IOWR(FM_IOC_MAGIC, 63, \
	struct fm_softmute_tune_t*)
#define FM_IOCTL_DESENSE_CHECK   _IOWR(FM_IOC_MAGIC, 64, \
	struct fm_desense_check_t*)


/*IOCTL for SPRD SPECIAL */
/*audio mode:0:mono, 1:stereo; 2:blending*/
#define FM_IOCTL_SET_AUDIO_MODE       _IOWR(FM_IOC_MAGIC, 0x47, int32_t*)
#define FM_IOCTL_SET_REGION       _IOWR(FM_IOC_MAGIC, 0x48, int32_t*)
#define FM_IOCTL_SET_SCAN_STEP       _IOWR(FM_IOC_MAGIC, 0x49, int32_t*)
#define FM_IOCTL_CONFIG_DEEMPHASIS       _IOWR(FM_IOC_MAGIC, 0x4A, int32_t*)
#define FM_IOCTL_GET_AUDIO_MODE       _IOWR(FM_IOC_MAGIC, 0x4B, int32_t*)
#define FM_IOCTL_GET_CUR_BLER       _IOWR(FM_IOC_MAGIC, 0x4C, int32_t*)
#define FM_IOCTL_GET_SNR       _IOWR(FM_IOC_MAGIC, 0x4D, int32_t*)
#define FM_IOCTL_SOFTMUTE_ONOFF       _IOWR(FM_IOC_MAGIC, 0x4E, int32_t*)
/*Frequency offset, PDP_TH,PHP_TH, SNR_TH,RSS_THI*/
#define FM_IOCTL_SET_SEEK_CRITERIA       _IOWR(FM_IOC_MAGIC, 0x4F, \
			struct fm_seek_criteria_parm*)
/*softmute ,blending ,snr_th*/
#define FM_IOCTL_SET_AUDIO_THRESHOLD _IOWR(FM_IOC_MAGIC, 0x50, \
			struct fm_audio_threshold_parm*)
/*Frequency offset, PDP_TH,PHP_TH, SNR_TH,RSS_THI*/
#define FM_IOCTL_GET_SEEK_CRITERIA       _IOWR(FM_IOC_MAGIC, 0x51, \
			struct fm_seek_criteria_parm*)
/*softmute ,blending ,snr_th*/
#define FM_IOCTL_GET_AUDIO_THRESHOLD _IOWR(FM_IOC_MAGIC, 0x52, \
			struct fm_audio_threshold_parm*)
#define FM_IOCTL_RW_REG        _IOWR(FM_IOC_MAGIC, 0xC, struct fm_reg_ctl_parm*)
#define FM_IOCTL_AF_ONOFF     _IOWR(FM_IOC_MAGIC, 0x53, uint16_t*)
#define FM_IOCTL_GET_IQ_LOG	_IOWR(FM_IOC_MAGIC, 0x54, struct fm_iq_data *)

/* IOCTL for EM */
#define FM_IOCTL_FULL_CQI_LOG _IOWR(FM_IOC_MAGIC, 70, \
	struct fm_full_cqi_log_t *)

#define FM_IOCTL_DUMP_REG   _IO(FM_IOC_MAGIC, 0xFF)


#endif
