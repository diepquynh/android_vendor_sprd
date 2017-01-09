#ifndef _CONNECTIVITY_RF_PARAMETERS_H_
#define _CONNECTIVITY_RF_PARAMETERS_H_

/* type:
unsigned char   1
unsigned short  2
unsigned int    4*/
typedef struct
{
	char *itm;
	unsigned long mem_offset;
	int type;
} nvm_name_table;

typedef struct
{
	char data_rate_power;
	char channel_num;
	char channel_range[6];
	char b_tx_power_dr0[3];
	char b_tx_power_dr1[3];
	char g_tx_power_dr0[3];
	char g_tx_power_dr1[3];
	char g_tx_power_dr2[3];
	char g_tx_power_dr3[3];
	char n_tx_power_dr0[3];
	char n_tx_power_dr1[3];
	char n_tx_power_dr2[3];
	char n_tx_power_dr3[3];
	char power_reserved[10];
}tx_power_control_t;

typedef struct
{
	char phy0_init_num;
	unsigned short init_phy0_regs[16];
	char phy1_init_num;
	unsigned short init_phy1_regs[6];
	char rf_init_num;
	unsigned int init_rf_regs[16];
	char reserved_w16_num;
	unsigned short reserved_w16_regs[10];
	char reserved_w32_num;
	unsigned short reserved_w32_regs[10];
}init_register_t;

typedef struct
{
	char tpc_enable;
	char power_save_key;
	char enhance_reserved[4];
}enhance_config_t;

typedef struct
{
	char CoexExcutionMode;
	char CoexWifiScanCntPerChannel;
	char CoexWifiScanDurationOneTime;
	char CoexScoPeriodsToBlockDuringDhcp;
	char CoexA2dpDhcpProtectLevel;
	char CoexScoperiodsToBlockDuringEap;
	char CoexA2dpEapProtectLevel;
	char CoexScoPeriodsToBlockDuringWifiJoin;
	char CoexA2dpWifiJoinProtectLevel;
	unsigned short CoexEnterPMStateTime;
	unsigned short CoexAclA2dpBtWorkTime;
	unsigned short CoexAclA2dpWifiWorkTime;
	unsigned short CoexAclNoA2dpBtWorkTime;
	unsigned short CoexAclNoA2dpWifiWorkTime;
	unsigned short CoexAclMixBtWorkTime;
	unsigned short CoexAclMixWifiWorkTime;
	unsigned short CoexPageInqBtWorkTime;
	unsigned short CoexPageInqWifiWorkTime;
	unsigned short CoexScoSchema;
	unsigned short CoexDynamicScoSchemaEnable;
	unsigned short CoexScoPeriodsBtTakeAll;
	unsigned short CoexLteTxAdvancedTime;
	unsigned short CoexLteOneSubFrameLen;
	unsigned short CoexLteTxTimerLen;
	unsigned short CoexLteTxTimerFrameHeadLen;
	unsigned short CoexLteStrategyFlag;
	unsigned short CoexWifiDegradePowerValue;
	unsigned short CoexBtDegradePowerValue;
	unsigned short CoexWifi2300TxSpur2Lte[7];
	unsigned short CoexWifi2310TxSpur2Lte[7];
	unsigned short CoexWifi2320TxSpur2Lte[7];
	unsigned short CoexWifi2330TxSpur2Lte[7];
	unsigned short CoexWifi2340TxSpur2Lte[7];
	unsigned short CoexWifi2350TxSpur2Lte[7];
	unsigned short CoexWifi2360TxSpur2Lte[7];
	unsigned short CoexWifi2370TxSpur2Lte[7];
	unsigned short CoexWifi2380TxSpur2Lte[7];
	unsigned short CoexWifi2390TxSpur2Lte[7];
	unsigned short CoexWifi2400TxSpur2Lte[7];
	unsigned short CoexLteTxSpur2Wifi2300[7];
	unsigned short CoexLteTxSpur2Wifi2310[7];
	unsigned short CoexLteTxSpur2Wifi2320[7];
	unsigned short CoexLteTxSpur2Wifi2330[7];
	unsigned short CoexLteTxSpur2Wifi2340[7];
	unsigned short CoexLteTxSpur2Wifi2350[7];
	unsigned short CoexLteTxSpur2Wifi2360[7];
	unsigned short CoexLteTxSpur2Wifi2370[7];
	unsigned short CoexLteTxSpur2Wifi2380[7];
	unsigned short CoexLteTxSpur2Wifi2390[7];
	unsigned short CoexLteTxSpur2Wifi2400[7];
	unsigned short CoexReserved[16];
}coex_config_t;

typedef struct
{
	char public_reserved[10];
}public_config_t;

typedef struct
{
	char is_calibrated;
	char rc_cali_en;
	char dcoc_cali_en;
	char txiq_cali_en;
	char rxiq_cali_en;
	char txpower_cali_en;
	char dpd_cali_en;
	char config_reserved[4];
}cali_config_t;

typedef struct
{
	char rctune_value;
	char rctune_reserved[2];
}rctune_cali_t;

typedef struct
{
	unsigned short dcoc_cali_code[18];
	unsigned int dcoc_reserved[4];
}dcoc_cali_t;

typedef struct
{
	unsigned int rf_txiq_c11;
	unsigned int rf_txiq_c12;
	unsigned int rf_txiq_c22;
	unsigned int rf_txiq_dc;
	unsigned int txiq_reserved[4];
}txiq_cali_t;

typedef struct
{
	unsigned int rf_rxiq_coef21_22;
	unsigned int rf_rxiq_coef11_12;
	unsigned int rxiq_reserved[2];
}rxiq_cali_t;

typedef struct
{
	int txpower_psat_temperature;
	char txpower_psat_gainindex;
    unsigned short txpower_psat_power;
	char txpower_psat_backoff;
	unsigned char txpower_psat_upper_limit;
    unsigned char txpower_psat_lower_limit;
	char txpower_freq_delta_gainindex[14];
	char txpower_temperature_delta_gm_channel1[32];
	char txpower_temperature_delta_gm_channel2[32];
	char txpower_temperature_delta_gm_channel3[32];
	char txpower_temperature_delta_gm_channel4[32];
	char txpower_temperature_delta_gm_channel5[32];
	char txpower_temperature_delta_gm_channel6[32];
	char txpower_temperature_delta_gm_channel7[32];
	char txpower_temperature_delta_gm_channel8[32];
	char txpower_temperature_delta_gm_channel9[32];
	char txpower_temperature_delta_gm_channel10[32];
	char txpower_temperature_delta_gm_channel11[32];
	char txpower_temperature_delta_gm_channel12[32];
	char txpower_temperature_delta_gm_channel13[32];
	char txpower_temperature_delta_gm_channel14[32];
	unsigned int txpower_reserved[4];
}txpower_cali_t;


typedef struct
{
    unsigned char rf_ctune[14];
    unsigned int rf_reserved[4];
}rf_para_t;

typedef struct
{
    unsigned int tpc_cfg[37];
    unsigned int tpc_reserved[4];
}tpc_cfg_t;

typedef struct
{
	char dpd_cali_channel_num;
	char dpd_cali_channel[3];
	unsigned int channel1_dpd_cali_table[182];
	unsigned int channel2_dpd_cali_table[182];
	unsigned int channel3_dpd_cali_table[182];
	unsigned int dpd_reserved[4];
}dpd_cali_t;

typedef struct
{
	char config_version;
	tx_power_control_t tx_power_control;
	init_register_t init_register;
	enhance_config_t enhance_config;
	coex_config_t coex_config;
	public_config_t public_config;
}wifi_config_t;

typedef struct
{
	char cali_version;
	cali_config_t cali_config;
	rctune_cali_t rctune_cali;
	dcoc_cali_t dcoc_cali;
	txiq_cali_t txiq_cali;
	rxiq_cali_t rxiq_cali;
	txpower_cali_t txpower_cali;
	dpd_cali_t dpd_cali;
	rf_para_t rf_para;
	tpc_cfg_t tpc_cfg;
}wifi_cali_t;

typedef struct
{
    wifi_cali_t wifi_cali;      //3008 bytes
    unsigned char filler[1088]; //1088 bytes
}wifi_cali_cp_t;                //4096 bytes total

typedef struct
{
    wifi_config_t wifi_config;  //616  bytes
    wifi_cali_t wifi_cali;      //3008 bytes
    unsigned char filler[472];  //472  bytes
}wifi_rf_t;                     //4096 bytes total

/*
API:
Get connectivity configure parameters from connectivity_configure.ini
parameter: wifi_config_t* caller should provide memory.
return 0 if success, or -1 if an error occurred
*/
int get_connectivity_config_param(wifi_config_t* p);

/*
API:
Get connectivity calibration parameters from connectivity_calibration.ini
parameter: wifi_cali_t* caller should provide memory.
return 0 if success, or -1 if an error occurred
*/
int get_connectivity_cali_param(wifi_cali_t* p);

/*
API:
Get connectivity RF parameters including configure parameters and calibration parameters
parameter: wifi_rf_t* caller should provide memory.
return 0 if success, or negative if an error occurred
*/
int get_connectivity_rf_param(wifi_rf_t* p);

/*
API:
save connectivity calibration paramters into connectivity_calibration.ini
parameter: wifi_cali_t *cali_param_ptr, caller should provide memory.
return: false/true
*/
int wlan_save_cali_data_to_file(wifi_cali_cp_t *cali_param_ptr);

#define  WIFI_CONFIG_TABLE(NAME, MEM_OFFSET, TYPE)  { NAME, (unsigned long)( &( ((wifi_config_t *)(0))->MEM_OFFSET )), TYPE}
#define  WIFI_CALI_TABLE(NAME, MEM_OFFSET, TYPE)    { NAME, (unsigned long)( &( ((wifi_cali_t *)(0))->MEM_OFFSET )), TYPE}

#endif
