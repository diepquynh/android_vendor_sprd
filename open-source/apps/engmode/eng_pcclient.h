#ifndef __ENG_PCCLIENT_H__
#define __ENG_PCCLIENT_H__

#define ENG_CALISTR				"calibration"
#define ENG_TESTMODE			"engtestmode"
#define ENG_STREND				"\r\n"
#define ENG_USBIN				"/sys/class/android_usb/android0/state"
#define ENG_USBCONNECTD			"CONFIGURED"
#define ENG_FACOTRYMODE_FILE	"/productinfo/factorymode.file"
#define ENG_FACOTRYSYNC_FILE	"/factorysync.file"
#define ENG_WIFI_CONFIG_FILE    "/productinfo/2351_connectivity_configure.ini"
#define ENG_IMEI1_CONFIG_FILE   "/productinfo/imei1.txt"
#define ENG_IMEI2_CONFIG_FILE   "/productinfo/imei2.txt"
#define ENG_IMEI3_CONFIG_FILE   "/productinfo/imei3.txt"
#define ENG_IMEI4_CONFIG_FILE   "/productinfo/imei4.txt"
#define ENG_MODEMRESET_PROPERTY	"persist.sys.sprd.modemreset"
#define ENG_USB_PROPERTY	    "persist.sys.sprd.usbfactorymode"
#define RAWDATA_PROPERTY		"sys.rawdata.ready"
#define ENG_ATDIAG_AT			"AT+SPBTWIFICALI="
#define ENG_BUFFER_SIZE			2048
#define ENG_CMDLINE_LEN			1024
#define ENG_DEV_PATH_LEN        260

enum {
    ENG_CMDERROR = -1,
    ENG_CMD4LINUX = 0,
    ENG_CMD4MODEM
};

enum {
    ENG_RUN_TYPE_WCDMA = 0,
    ENG_RUN_TYPE_TD,
    ENG_RUN_TYPE_WCN,
    ENG_RUN_TYPE_LTE,
    ENG_RUN_TYPE_MAX
};

typedef struct eng_host_int {
    char dev_at[ENG_DEV_PATH_LEN];
    char dev_diag[ENG_DEV_PATH_LEN];
    char dev_log[ENG_DEV_PATH_LEN];
    int dev_type;
    int cali_flag;
}eng_host_int_t;

typedef struct eng_modem_int {
    char at_chan[ENG_DEV_PATH_LEN];
    char diag_chan[ENG_DEV_PATH_LEN];
    char log_chan[ENG_DEV_PATH_LEN];
}eng_modem_int_t;

typedef struct eng_dev_info {
    eng_host_int_t host_int;
    eng_modem_int_t modem_int;
}eng_dev_info_t;

#endif
