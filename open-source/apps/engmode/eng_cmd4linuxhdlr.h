
#ifndef __ENG_CMD4LINUXHDLR_H__
#define __ENG_CMD4LINUXHDLR_H__

#define SPRDENG_OK			"OK"
#define SPRDENG_ERROR		"ERROR"
#define ENG_STREND			"\r\n"
#define ENG_TESTMODE		"engtestmode"
#define ENG_BATVOL			"/sys/class/power_supply/battery/real_time_voltage"
#define ENG_STOPCHG			"/sys/class/power_supply/battery/stop_charge"
#define ENG_CURRENT			"/sys/class/power_supply/battery/real_time_current"
#define ENG_BATTVOL_AVG 	"/sys/class/power_supply/battery/current_avg"
#define ENG_BATTVOL_NOW 	"/sys/class/power_supply/battery/voltage_now"
#define ENG_BATTCHG_STS 	"/sys/class/power_supply/battery/status"
#define ENG_BATTTEMP   		"/sys/class/power_supply/battery/temp"
#define ENG_BATTTEMP_ADC 	"/sys/class/power_supply/battery/temp_adc"
#define ENG_WPATEMP         "/sys/devices/platform/sprd_temprf/temprf_wpa_temp"
#define ENG_WPATEMP_ADC     "/sys/devices/platform/sprd_temprf/temprf_wpa_temp_adc"
#define ENG_DCXOTEMP		"/sys/devices/platform/sprd_temprf/temprf_dcxo_temp"
#define ENG_DCXOTEMP_ADC    "/sys/devices/platform/sprd_temprf/temprf_dcxo_temp_adc"
#define ENG_RECOVERYCMD		"/cache/recovery/command"
#define ENG_RECOVERYDIR		"/cache/recovery"
#define ENG_CHARGERTEST_FILE "/productinfo/chargertest.file"
#define IQMODE_FLAG_PATH        "/productinfo/iqmode"


typedef enum{
    CMD_SENDKEY = 0,
    CMD_GETICH,
    CMD_ETSRESET,
    CMD_RPOWERON,
    CMD_GETVBAT,
    CMD_STOPCHG,
    CMD_TESTMMI,
    CMD_BTTESTMODE,
    CMD_GETBTADDR,
    CMD_SETBTADDR,
    CMD_GSNR,
    CMD_GSNW,
    CMD_GETWIFIADDR,
    CMD_SETWIFIADDR,
    CMD_CGSNW,
    CMD_ETSCHECKRESET,
    CMD_SIMCHK,
    CMD_ATDIAG,
    CMD_INFACTORYMODE,
    CMD_FASTDEEPSLEEP,
    CMD_CHARGERTEST,
    CMD_SETUARTSPEED,
    CMD_SPBTTEST,
    CMD_SPWIFITEST,
    CMD_SPGPSTEST,
    CMD_BATTTEST,
    CMD_TEMPTEST,
    CMD_RTCTEST,
    CMD_SPWIQ,
    CMD_END
}ENG_CMD;


typedef enum{
    CMD_INVALID_TYPE,
    CMD_TO_AP, /* handled by AP */
    CMD_TO_APCP /* handled by AP and CP */
}eng_cmd_type;

struct eng_linuxcmd_str{
    int index;
    eng_cmd_type type;
    char *name;
    int (*cmd_hdlr)(char *, char *);
};

int eng_at2linux(char *buf);
int eng_linuxcmd_hdlr(int cmd, char *req, char* rsp);
eng_cmd_type eng_cmd_get_type(int cmd);

#endif
