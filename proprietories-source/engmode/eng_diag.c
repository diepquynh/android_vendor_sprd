#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>  //-- for system command
#include <semaphore.h>
#include <cutils/android_reboot.h>
#include <cutils/sockets.h>
#include <poll.h>
#include <sys/sysinfo.h>
#include "engopt.h"
#include "eng_attok.h"
#include "eng_pcclient.h"
#include "eng_autotest.h"  //-- for autotest 20130220
#include "eng_diag.h"
#include "eng_sqlite.h"
#include "vlog.h"
#include "crc16.h"
#include "string.h"
#ifndef USE_AUDIO_WHALE_HAL
#include "eng_audio.h"
#include "eng_audio_ext.h"
#endif
#include "eut_opt.h"
#include <ctype.h>
#include "cutils/properties.h"
#include <sys/reboot.h>
#include "eng_btwifiaddr.h"
#include "vlog.h"
#include "eng_productdata.h"
#include "eng_sqlite.h"
#ifndef CONFIG_MINIENGPC
#ifdef ENG_AT_CHANNEL
#include "atci.h"
#endif
#endif
#include "calibration.h"
#include "eng_cmd4linuxhdlr.h"
#include "wifi_eut_sprd.h"
#include "eng_ap_modem_time_sync.h"

#if defined(ENGMODE_EUT_BCM)
#include "bt_eut_pandora.h"
#endif

#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
#include "bt_engpc_sprd_if.h"
#endif
#ifndef CONFIG_MINIENGPC
#include "gps_pc_mode.h"
#endif
#include "eng_busmonitor.h"

#if (defined TEE_PRODUCTION_CONFIG) && (!defined CONFIG_MINIENGPC)
#include "tee_production.h"
#endif
#define ENG_TEE_RSP_LEN (1024*2)

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#define NVITEM_ERROR_E int
#define NVERR_NONE 0
#define IMEI_NUM 4
#define MAX_LINE_LEN 256

#define CMD_SECURESTRING "securesha1="
#define CMD_PUBLICKEYPATH "primpukpath="
#define CMD_PUBLICKEYSTART "primpukstart="
#define CMD_PUBLICKEYLEN "primpuklen="
#define CMD_REGISTERSTRING "regvaluesis:"

#define PERSIST_MODEM_CHAR "persist.modem."

#define TD_MODEM 0x3434
#define W_MODEM 0x5656
#define LTE_MODEM 0x7878
#define LF_MODEM 0x6868
#define TL_MODEM 0x9898

#define TD_PROC_PROP "ro.modem.t.dev"
#define W_PROC_PROP "ro.modem.w.dev"
#define LTE_PROC_PROP "ro.modem.l.dev"
#define LF_PROC_PROP "ro.modem.lf.dev"
#define TL_PROC_PROP "ro.modem.tl.dev"

#define TD_MODEM_ENABLE_PROP "persist.modem.t.enable"
#define W_MODEM_ENABLE_PROP "persist.modem.w.enable"
#define LTE_MODEM_ENABLE_PROP "persist.modem.l.enable"
#define LF_MODEM_ENABLE_PROP "persist.modem.lf.enable"
#define TL_MODEM_ENABLE_PROP "persist.modem.tl.enable"

#define DBHEADER_FLAG_MODEMIMG 1
#define DBHEADER_FLAG_MODEMDB 2
#define DBHEADER_FLAG_ISEND 1
#define DBHEADER_FLAG_ISNEXT 0
#define DBHEADER_FLAG_SHA1 1
#define DBHEADER_FLAG_NOSHA1 0

#ifdef SECURE_BOOT_ENABLE
#define BOOT_INFO_SIZE (512)
#define VLR_INFO_SIZE (512)
#endif

#define ENG_PCBA_SUPPORT_CONFIG "/system/etc/factorytest/PCBA.conf"
#define ENG_BBAT_SUPPORT_CONFIG "/system/etc/engmode/BBAT.conf"

// SIPC interfaces in AP linux for AT CMD
char *at_sipc_devname[] = {
    "/dev/stty_td30",  // AT channel in TD mode
    "/dev/stty_w30"    // AT channel in W mode
};

int g_reset = 0;
int g_setuart_ok = 0;
sem_t g_gps_sem;
int g_gps_log_enable = 0;
int g_assert_cmd = 0;
extern int g_run_mode;
extern int g_ap_cali_flag;
#ifndef USE_AUDIO_WHALE_HAL
extern AUDIO_TOTAL_T *audio_total;
#endif
extern void eng_check_factorymode(int normal_cali);
#ifndef USE_AUDIO_WHALE_HAL
extern int parse_vb_effect_params(void *audio_params_ptr,
                                  unsigned int params_size);
extern int SetAudio_pga_parameter_eng(AUDIO_TOTAL_T *aud_params_ptr,
                                      unsigned int params_size,
                                      uint32_t vol_level);
#endif
extern int eng_battery_calibration(char *data, unsigned int count,
                                   char *out_msg, int out_len);
extern void adc_get_result(char *chan);
extern void disable_calibration(void);
extern void enable_calibration(void);
extern char *get_ser_diag_path(void);
extern int disconnect_vbus_charger(void);
extern int connect_vbus_charger(void);
#if 0
extern int  start_fm_test(char *,int,char *);
#endif

static int eng_diag_read_mmi(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_write_mmi(char *buf, int len, char *rsp, int rsplen);
int eng_init_test_file(void);

static int parse_config(void);

extern struct eng_bt_eutops bt_eutops;
extern struct eng_wifi_eutops wifi_eutops;
extern struct eng_gps_eutops gps_eutops;

extern TIME_SYNC_T g_time_sync;
extern pthread_mutex_t g_time_sync_lock;

static unsigned char tun_data[376];
static int diag_pipe_fd = 0;
static char eng_atdiag_buf[2 * ENG_DIAG_SIZE];
static char eng_diag_buf[2 * ENG_DIAG_SIZE];
static char eng_audio_diag_buf[2 * ENG_DIAG_SIZE];
static char eng_modemdb_diag_buf[2 * ENG_MODEMDB_DIAG_SIZE];
static int eng_diag_len = 0;
static int g_is_data = 0;
static int g_indicator = 0;
static int g_index = 0;
static int cmd_type;
static int eq_or_tun_type, eq_mode_sel_type;
static int s_cmd_index = -1;
static int s_cp_ap_proc = 0;
static int s_cur_filepos = 0;
static eng_thread_t gps_thread_hdlr;
static int loop_enable,loop_route;
static hardware_result support_result[64];
static hardware_result bbat_support_result[64];

static int write_productnvdata(char *buffer, int size);
static int read_productnvdata(char *buffer, int size);
static int eng_diag_getver(unsigned char *buf, int len, char *rsp);
static int eng_diag_bootreset(unsigned char *buf, int len, char *rsp);
static int eng_diag_getband(char *buf, int len, char *rsp);
static int eng_diag_btwifiimei(char *buf, int len, char *rsp, int rsplen);
#if (!defined USE_AUDIO_WHALE_HAL) && (!defined CONFIG_MINIENGPC)
static int eng_diag_audio(char *buf, int len, char *rsp);
#endif
static int eng_diag_product_ctrl(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_direct_phschk(char *buf, int len, char *rsp, int rsplen);
static void eng_diag_reboot(int reset);
static int eng_diag_deep_sleep(char *buf, int len, char *rsp);

static int eng_diag_modem_db_attr(char *buf, int len, char *rsp);
static int read_modem_db_attr(char *buffer);
static int eng_diag_modem_db_read(char *buf, int len, char *rsp);
static int get_modem_readpath(char *modem_file);

static int eng_diag_gps_autotest_hdlr(char *buf, int len, char *rsp,
                                      int rsplen);
static int eng_diag_fileoper_hdlr(char *buf, int len, char *rsp);
static int eng_diag_ap_req(char *buf, int len);
static int eng_diag_read_imei(REF_NVWriteDirect_T *direct, int num);
static int eng_diag_write_imei(REF_NVWriteDirect_T *direct, int num);
static void ImeiConvStr2NV(unsigned char *szImei, unsigned char *nvImei);
static void ImeiConvNV2Str(unsigned char *nvImei, unsigned char *szImei);
static char MAKE1BYTE2BYTES(unsigned char high4bit, unsigned char low4bit);
#if (!defined USE_AUDIO_WHALE_HAL) && (!defined CONFIG_MINIENGPC)
int is_audio_at_cmd_need_to_handle(char *buf, int len);
#endif
int is_rm_cali_nv_need_to_handle(char *buf, int len);
int eng_diag_factorymode(char *buf, int len, char *rsp);
int eng_diag_mmicit_read(char *buf, int len, char *rsp, int rsplen);
#if defined(ENGMODE_EUT_BCM)
int get_sub_str(char *buf, char **revdata, char a, char b);
#elif defined(ENGMODE_EUT_SPRD)
int get_sub_str(const char *buf, char **revdata, char a, char *delim,
                unsigned char count, unsigned char substr_max_len);
#endif
int get_cmd_index(char *buf);
int eng_diag_adc(
    char *buf,
    int *Irsp);  // add by kenyliu on 2013 07 12 for get ADCV  bug 188809
void At_cmd_back_sig(void);  // add by kenyliu on 2013 07 15 for set calibration
                             // enable or disable  bug 189696
static void eng_diag_cft_switch_hdlr(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_enable_charge(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_get_charge_current(char *buf, int len, char *rsp,
                                       int rsplen);
static int get_charging_current(int *value);
static int get_battery_current(int *value);
static int eng_diag_get_modem_mode(char *buf, int len, char *rsp, int rsplen);

static int eng_diag_read_efuse(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_write_efuse(char *buf, int len, char *rsp, int rsplen);
static int eng_parse_hash_cmdline(unsigned char cmdvalue[]);
static int eng_parse_publickey_cmdline(char *path, int *pos, int *len);
static int eng_diag_read_publickey(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_enable_secure(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_read_enable_secure_bit(char *buf, int len, char *rsp,
                                           int rsplen);
static int eng_detect_process(char *process_name);
static int eng_open_wifi_switch();
static int eng_diag_set_backlight(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_txdata(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_set_powermode(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_set_ipconfigure(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_read_register(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_write_register(char *buf, int len, char *rsp, int rsplen);
static int eng_diag_get_time_sync_info(char *buf,int len,char *rsp, int rsplen);
#if (defined TEE_PRODUCTION_CONFIG) && (!defined CONFIG_MINIENGPC)
static int eng_diag_tee_production(char *buf, int len, char *rsp, int rsplen);
#endif

static const char *at_sadm = "AT+SADM4AP";
static const char *at_spenha = "AT+SPENHA";
static const char *at_calibr = "AT+CALIBR";
static const char *at_phoneinfo = "AT+PEINFO";
static const char *at_phoneloop = "AT+PELOOP";

// static int at_sadm_cmd_to_handle[] = {7,8,9,10,11,12,-1};
static int at_sadm_cmd_to_handle[] = {7, 8, 9, 10, 11, 12, -1};
// static int at_spenha_cmd_to_handle[] = {0,1,2,3,4,-1};
static int at_spenha_cmd_to_handle[] = {0, 1, 2, 3, 4, -1};

static int eng_autotest_dummy(char *req, char *rsp);
static int eng_autotest_keypad(char *req, char *rsp);
static int eng_autotest_lcd_parallel(char *req, char *rsp);
static int eng_autotest_lcd_spi(char *req, char *rsp);
static int eng_autotest_camera_iic(char *req, char *rsp);
static int eng_autotest_camera_parallel(char *req, char *rsp);
static int eng_autotest_camera_spi(char *req, char *rsp);
static int eng_autotest_gpio(char *req, char *rsp);
static int eng_autotest_tf(char *req, char *rsp);
static int eng_autotest_sim(char *req, char *rsp);
static int eng_autotest_mic(char *req, char *rsp);
static int eng_autotest_speak(char *req, char *rsp);
static int eng_autotest_misc(char *req, char *rsp);
static int eng_autotest_fm(char *req, char *rsp);
static int eng_autotest_atv(char *req, char *rsp);
static int eng_autotest_bt(char *req, char *rsp);
static int eng_autotest_wifi(char *req, char *rsp);
static int eng_autotest_iic_dev(char *req, char *rsp);
static int eng_autotest_charge(char *req, char *rsp);
//-- [[ 2013-01-22
static int eng_autotest_reserve(char *req, char *rsp);
static int eng_autotest_sensor(char *req, char *rsp);
//-- ]]
static int eng_autotest_gps(char *req, char *rsp);

static struct eng_autotestcmd_str eng_autotestcmd[] = {
    {CMD_AUTOTEST_DUMMY, eng_autotest_dummy},
    {CMD_AUTOTEST_KEYPAD, eng_autotest_keypad},
    {CMD_AUTOTEST_LCD_PARALLEL, eng_autotest_lcd_parallel},
    {CMD_AUTOTEST_LCD_SPI, eng_autotest_lcd_spi},
    {CMD_AUTOTEST_CAMERA_IIC, eng_autotest_camera_iic},
    {CMD_AUTOTEST_CAMERA_PARALLEL, eng_autotest_camera_parallel},
    {CMD_AUTOTEST_CAMERA_SPI, eng_autotest_camera_spi},
    {CMD_AUTOTEST_GPIO, eng_autotest_gpio},
    {CMD_AUTOTEST_TF, eng_autotest_tf},
    {CMD_AUTOTEST_SIM, eng_autotest_sim},
    {CMD_AUTOTEST_MIC, eng_autotest_mic},
    {CMD_AUTOTEST_SPEAK, eng_autotest_speak},
    {CMD_AUTOTEST_MISC, eng_autotest_misc},
    {CMD_AUTOTEST_FM, eng_autotest_fm},
    {CMD_AUTOTEST_ATV, eng_autotest_atv},
    {CMD_AUTOTEST_BT, eng_autotest_bt},
    {CMD_AUTOTEST_WIFI, eng_autotest_wifi},
    {CMD_AUTOTEST_IIC_DEV, eng_autotest_iic_dev},
    {CMD_AUTOTEST_CHARGE, eng_autotest_charge},
    //-- [[ 2013-01-22
    {CMD_AUTOTEST_RSV01, eng_autotest_reserve},
    {CMD_AUTOTEST_RSV02, eng_autotest_reserve},
    {CMD_AUTOTEST_SENSOR, eng_autotest_sensor},
    //-- ]]
    {CMD_AUTOTEST_GPS, eng_autotest_gps},
};

#if defined(ENGMODE_EUT_BCM)
struct eut_cmd eut_cmds[] = {
    {EUT_REQ_INDEX, ENG_EUT_REQ},
    {EUT_INDEX, ENG_EUT},

    /* BT TXCH*/
    {BT_TXCH_REQ_INDEX, ENG_BT_TXCH_REQ},
    {BT_TXCH_INDEX, ENG_BT_TXCH},

    /* BT RXCH*/
    {BT_RXCH_REQ_INDEX, ENG_BT_RXCH_REQ},
    {BT_RXCH_INDEX, ENG_BT_RXCH},

    /* BT TX PATTERN */
    {BT_TXPATTERN_REQ_INDEX, ENG_BT_TXPATTERN_REQ},
    {BT_TXPATTERN_INDEX, ENG_BT_TXPATTERN},

    /* BT RX PATTERN */
    {BT_RXPATTERN_REQ_INDEX, ENG_BT_RXPATTERN_REQ},
    {BT_RXPATTERN_INDEX, ENG_BT_RXPATTERN},

    /* TXPKTTYPE */
    {BT_TXPKTTYPE_REQ_INDEX, ENG_BT_TXPKTTYPE_REQ},  // 10
    {BT_TXPKTTYPE_INDEX, ENG_BT_TXPKTTYPE},

    /* RXPKTTYPE */
    {BT_RXPKTTYPE_REQ_INDEX, ENG_BT_RXPKTTYPE_REQ},  // brcm
    {BT_RXPKTTYPE_INDEX, ENG_BT_RXPKTTYPE},

    /* TXPKTLEN */
    {BT_TXPKTLEN_REQ_INDEX, ENG_BT_TXPKTLEN_REQ},
    {BT_TXPKTLEN_INDEX, ENG_BT_TXPKTLEN},

    /* TXPWR */
    {BT_TXPWR_REQ_INDEX, ENG_BT_TXPWR_REQ},
    {BT_TXPWR_INDEX, ENG_BT_TXPWR},

    /* RX Gain */
    {BT_RXGAIN_REQ_INDEX, ENG_BT_RXGAIN_REQ},
    {BT_RXGAIN_INDEX, ENG_BT_RXGAIN},

    /* ADDRESS */
    {BT_ADDRESS_REQ_INDEX, ENG_BT_ADDRESS_REQ},  // 20
    {BT_ADDRESS_INDEX, ENG_BT_ADDRESS},

    /* RXDATA */
    {BT_RXDATA_REQ_INDEX, ENG_BT_RXDATA_REQ},

    /* TESTMODE */
    {BT_TESTMODE_REQ_INDEX, ENG_BT_TESTMODE_REQ},
    {BT_TESTMODE_INDEX, ENG_BT_TESTMODE},

    /* TX */
    {TX_REQ_INDEX, ENG_TX_REQ},
    {TX_INDEX, ENG_TX},  // 26

    /* RX */
    {RX_REQ_INDEX, ENG_RX_REQ},
    {RX_INDEX, ENG_RX},

    {WIFI_BAND_REQ_INDEX, ENG_WIFI_BAND_REQ},
    {WIFI_BAND_INDEX, ENG_WIFI_BAND},
    {GPSSEARCH_REQ_INDEX, ENG_GPSSEARCH_REQ},
    {GPSSEARCH_INDEX, ENG_GPSSEARCH},
    {WIFICH_REQ_INDEX, ENG_WIFICH_REQ},
    {WIFICH_INDEX, ENG_WIFICH},
    {WIFITX_MODE_REQ_INDEX, ENG_WIFITX_MODE_REQ},
    {WIFITX_MODE_INDEX, ENG_WIFITX_MODE},
    {WIFIMODE_INDEX, ENG_WIFIMODE},  // brcm
    {WIFIRATIO_REQ_INDEX, ENG_WIFIRATIO_REQ},
    {WIFIRATIO_INDEX, ENG_WIFIRATIO},
    {WIFITX_FACTOR_REQ_INDEX, ENG_WIFITX_FACTOR_REQ},
    {WIFITX_FACTOR_INDEX, ENG_WIFITX_FACTOR},
    {WIFITX_PWRLV_REQ_INDEX, ENG_WIFITX_PWRLV_REQ},
    {WIFITX_PWRLV_INDEX, ENG_WIFITX_PWRLV},
    {ENG_WIFITXGAININDEX_REQ_INDEX, ENG_WIFITXGAININDEX_REQ},
    {ENG_WIFITXGAININDEX_INDEX, ENG_WIFITXGAININDEX},
    {WIFITX_REQ_INDEX, ENG_WIFITX_REQ},
    {WIFITX_INDEX, ENG_WIFITX},
    {WIFIRX_PACKCOUNT_INDEX, ENG_WIFIRX_PACKCOUNT},
    {WIFICLRRXPACKCOUNT_INDEX, ENG_WIFI_CLRRXPACKCOUNT},
    {WIFIRX_REQ_INDEX, ENG_WIFIRX_REQ},
    {WIFIRX_INDEX, ENG_WIFIRX},
    {GPSPRNSTATE_REQ_INDEX, ENG_GPSPRNSTATE_REQ},
    {GPSSNR_REQ_INDEX, ENG_GPSSNR_REQ},
    {GPSPRN_INDEX, ENG_GPSPRN},
    {ENG_WIFIRATE_REQ_INDEX, ENG_WIFIRATE_REQ},
    {ENG_WIFIRATE_INDEX, ENG_WIFIRATE},
    {ENG_WIFIRSSI_REQ_INDEX, ENG_WIFIRSSI_REQ},
    /* Pkt Length */
    {WIFIPKTLEN_REQ_INDEX, ENG_WIFIPKTLEN_REQ},
    {WIFIPKTLEN_INDEX, ENG_WIFIPKTLEN},
    /* Band Width */
    {WIFIBANDWIDTH_REQ_INDEX, ENG_WIFIBANDWIDTH_REQ},
    {WIFIBANDWIDTH_INDEX, ENG_WIFIBANDWIDTH},
    /* LNA */
    {WIFILNA_REQ_INDEX, ENG_WIFILNA_REQ},
    {WIFILNA_INDEX, ENG_WIFILNA},
    /* Preamble */
    {WIFIPREAMBLE_INDEX, ENG_WIFIPREAMBLE},  // brcm

};
#elif defined(ENGMODE_EUT_SPRD)
struct eut_cmd eut_cmds[] = {
    {EUT_REQ_INDEX, ENG_EUT_REQ},  // sprd
    {EUT_INDEX, ENG_EUT},
    {GPSSEARCH_REQ_INDEX, ENG_GPSSEARCH_REQ},
    {GPSSEARCH_INDEX, ENG_GPSSEARCH},
    {WIFICH_REQ_INDEX, ENG_WIFICH_REQ},
    {WIFICH_INDEX, ENG_WIFICH},

    {WIFIRATIO_REQ_INDEX, ENG_WIFIRATIO_REQ},
    {WIFIRATIO_INDEX, ENG_WIFIRATIO},

    /* Tx Power Level */
    {WIFITXPWRLV_REQ_INDEX, ENG_WIFITXPWRLV_REQ},
    {WIFITXPWRLV_INDEX, ENG_WIFITXPWRLV},

    /* Tx FAC */
    {WIFITX_FACTOR_REQ_INDEX, ENG_WIFITX_FACTOR_REQ},
    {WIFITX_FACTOR_INDEX, ENG_WIFITX_FACTOR},

    /* TX Mode */
    {WIFITXMODE_REQ_INDEX, ENG_WIFITXMODE_REQ},
    {WIFITXMODE_INDEX, ENG_WIFITXMODE},

    /* TX Gain */
    {ENG_WIFITXGAININDEX_REQ_INDEX, ENG_WIFITXGAININDEX_REQ},  // sprd
    {ENG_WIFITXGAININDEX_INDEX, ENG_WIFITXGAININDEX},

    /* TX */
    {TX_REQ_INDEX, ENG_TX_REQ},
    {TX_INDEX, ENG_TX},

    /* RX */
    {RX_REQ_INDEX, ENG_RX_REQ},
    {RX_INDEX, ENG_RX},

    /* Mode, is not TX Mode*/
    {WIFIMODE_INDEX, ENG_WIFIMODE},

    {WIFIRX_PACKCOUNT_INDEX, ENG_WIFIRX_PACKCOUNT},
    {WIFICLRRXPACKCOUNT_INDEX, ENG_WIFI_CLRRXPACKCOUNT},
    {GPSPRNSTATE_REQ_INDEX, ENG_GPSPRNSTATE_REQ},
    {GPSSNR_REQ_INDEX, ENG_GPSSNR_REQ},
    {GPSPRN_INDEX, ENG_GPSPRN},
    {ENG_WIFIRATE_REQ_INDEX, ENG_WIFIRATE_REQ},
    {ENG_WIFIRATE_INDEX, ENG_WIFIRATE},
    {ENG_WIFIRSSI_REQ_INDEX, ENG_WIFIRSSI_REQ},

    /* LNA */
    {WIFILNA_REQ_INDEX, ENG_WIFILNA_REQ},
    {WIFILNA_INDEX, ENG_WIFILNA},

    /* Band */
    {WIFIBAND_REQ_INDEX, ENG_WIFIBAND_REQ},
    {WIFIBAND_INDEX, ENG_WIFIBAND},

    /* Band Width */
    {WIFIBANDWIDTH_REQ_INDEX, ENG_WIFIBANDWIDTH_REQ},
    {WIFIBANDWIDTH_INDEX, ENG_WIFIBANDWIDTH},

    /* Pkt Length */
    {WIFIPKTLEN_REQ_INDEX, ENG_WIFIPKTLEN_REQ},
    {WIFIPKTLEN_INDEX, ENG_WIFIPKTLEN},

    /* Preamble */
    {WIFIPREAMBLE_REQ_INDEX, ENG_WIFIPREAMBLE_REQ},
    {WIFIPREAMBLE_INDEX, ENG_WIFIPREAMBLE},

    /* Payload */
    {WIFIPAYLOAD_REQ_INDEX, ENG_WIFIPAYLOAD_REQ},
    {WIFIPAYLOAD_INDEX, ENG_WIFIPAYLOAD},

    /* Guard Interval */
    {WIFIGUARDINTERVAL_REQ_INDEX, ENG_GUARDINTERVAL_REQ},
    {WIFIGUARDINTERVAL_INDEX, ENG_GUARDINTERVAL},

    /* MAC Filter */
    {WIFIMACFILTER_REQ_INDEX, ENG_MACFILTER_REQ},
    {WIFIMACFILTER_INDEX, ENG_MACFILTER},  // sprd

};
#endif

#if 0
static int eng_diag_write2pc(unsigned char* buf, unsigned int len)
{
    int ret;
    int i=0;
    int fd=0;
    int error = 0;

    do{
        fd = get_ser_diag_fd();
        error = 0;
        ret = write(fd,buf,len);
        if(ret < 0){
            error = errno;
            ENG_LOG("%s error: %s \n",__func__,strerror(errno));
            if(error == EBUSY)
                usleep(200000);
        }
        i++;
    }while(( error == EBUSY)&&(i<25));
    return ret;
}
#endif

static void print_log(char *log_data, int cnt) {
  int i;

  if (cnt > ENG_DIAG_SIZE) cnt = ENG_DIAG_SIZE;

  ENG_LOG("vser receive:\n");
  for (i = 0; i < cnt; i++) {
    if (isalnum(log_data[i])) {
      ENG_LOG("%c ", log_data[i]);
    } else {
      ENG_LOG("%2x ", log_data[i]);
    }
  }
  ENG_LOG("\n");
}

char *eng_diag_at_channel() { return at_sipc_devname[g_run_mode]; }

int eng_diag_parse(char *buf, int len, int *num) {
  int i;
  int ret = CMD_COMMON;
  MSG_HEAD_T *head_ptr = NULL;
  *num = eng_diag_decode7d7e(
      (char *)(buf + 1), (len - 2));  // remove the start 0x7e and the last 0x7e
  head_ptr = (MSG_HEAD_T *)(buf + 1);
  ENG_LOG("%s: cmd=0x%x; subcmd=0x%x\n", __FUNCTION__, head_ptr->type,
          head_ptr->subtype);
  // ENG_LOG("%s: cmd is:%s \n", __FUNCTION__, (buf + DIAG_HEADER_LENGTH + 1));

  switch (head_ptr->type) {
    case DIAG_CMD_CHANGEMODE:
      ENG_LOG("%s: Handle DIAG_CMD_CHANGEMODE\n", __FUNCTION__);
      if (head_ptr->subtype == RESET_MODE) {
        ret = CMD_USER_RESET;
      } else {
        ret = CMD_COMMON;
      }
      break;
    case DIAG_CMD_FACTORYMODE:
      ENG_LOG("%s: Handle DIAG_CMD_FACTORYMODE\n", __FUNCTION__);
      if (head_ptr->subtype == 0x07) {
        ret = CMD_USER_FACTORYMODE;
      } else if (head_ptr->subtype >= 0x2 && head_ptr->subtype <= 0x4) {
        // 2: NVITEM_PRODUCT_CTRL_READ
        // 3: NVITEM_PRODUCT_CTRL_WRITE
        // 4: NVITEM_PRODUCT_CTRL_ERASE
        ret = CMD_USER_PRODUCT_CTRL;
      } else if (head_ptr->subtype == DIAG_SUB_MMICIT_READ) {
        ret = CMD_USER_MMICIT_READ;
      } else {
        ret = CMD_COMMON;
      }
      break;
    case DIAG_CMD_DIRECT_PHSCHK:
      ENG_LOG("%s: Handle DIAG_CMD_DIRECT_PHSCHK\n", __FUNCTION__);
      ret = CMD_USER_DIRECT_PHSCHK;
      break;
#if 0
        case DIAG_CMD_ADC_F:
            ret =CMD_USER_ADC;
            break;
#endif
    case DIAG_FM_TEST_F:
      ret = CMD_USER_FM;
      break;
    case DIAG_CMD_AT:
#if (!defined USE_AUDIO_WHALE_HAL) && (!defined CONFIG_MINIENGPC)
      if (is_audio_at_cmd_need_to_handle(buf, len)) {
        ENG_LOG("%s: Handle DIAG_CMD_AUDIO\n", __FUNCTION__);
        ret = CMD_USER_AUDIO;
#else
      if (1 == 0) {
#endif
      } else if (is_ap_at_cmd_need_to_handle(buf, len)) {
        if (CMD_TO_APCP == eng_cmd_get_type(s_cmd_index)) {
          s_cp_ap_proc = 1;
        }
        ret = CMD_USER_APCMD;
      } else {
        ret = CMD_COMMON;
      }
      break;
    case DIAG_CMD_GETVOLTAGE:
      ret = CMD_USER_GETVOLTAGE;
      break;
    case DIAG_CMD_APCALI:
      ret = eng_diag_ap_req(buf, len);
      break;
    case DIAG_CMD_AUTOTEST:
      ENG_LOG("%s: Handle DIAG_CMD_AUTOTEST , eng_autotestIsConnect = %d \n",
              __FUNCTION__, eng_autotestIsConnect());
      if (head_ptr->subtype == 0x1c) {
        ret = CMD_USER_AUTOTEST_PATH_CONFIRM;
      } else {
        ret = CMD_USER_AUTOTEST;
      }
      break;
    case DIAG_CMD_VER:
      ENG_LOG("%s: Handle DIAG_CMD_VER", __FUNCTION__);
      if (head_ptr->subtype == 0x2) {
        ret = CMD_USER_VER;
      }
      break;
    case DIAG_CMD_IMEIBTWIFI:
      ret = is_rm_cali_nv_need_to_handle(buf, len);
      if (ret) {
        if (2 == ret) {
          s_cp_ap_proc = 1;  // This command should send to AP and CP.
        }
        ret = CMD_USER_BTWIFIIMEI;
      } else {
        ret = CMD_COMMON;
      }
      break;
    case DIAG_CMD_CURRENT_TEST:
      ENG_LOG("%s: Handle DIAG_CMD_CURRENT_TEST", __FUNCTION__);
      if (head_ptr->subtype == 0x2) {
        ret = CMD_USER_DEEP_SLEEP;
      } else if (head_ptr->subtype == 0xe) {
        ret = CMD_USER_SHUT_DOWN;
      }
      break;
    case DIAG_CMD_GPS_AUTO_TEST:
      ENG_LOG("%s: Handle DIAG_CMD_GPS_AUTO_TEST", __FUNCTION__);
      ret = CMD_USER_GPS_AUTO_TEST;
      break;
    case DIAG_CMD_WIFI_TEST_F:
      if (head_ptr->subtype == 0x10) {
        ret = CMD_USER_SET_CONFIGURE_IP;
      } else if (head_ptr->subtype == 0x0E) {
        ret = CMD_USER_READ_REGISTER;
      } else if (head_ptr->subtype == 0x0F) {
        ret = CMD_USER_WRITE_REGISTER;
      }
      break;
    case DIAG_SYSTEM_F:
      if (head_ptr->subtype == 0x4) {
        ENG_LOG("%s: Handle DIAG_CMD_ASSERT", __FUNCTION__);
        g_assert_cmd = 1;
      }else if(head_ptr->subtype==0x11)
        ret = CMD_USER_GET_TIME_SYNC_INFO;
      else if(head_ptr->subtype==0x20)
        ret = CMD_USER_READ_EFUSE;
      else if (head_ptr->subtype == 0x21)
        ret = CMD_USER_WRITE_EFUSE;
      else if (head_ptr->subtype == 0x22)
        ret = CMD_USER_ENABLE_SECURE;
      else if (head_ptr->subtype == 0x23)
        ret = CMD_USER_READ_PUBLICKEY;
      else if (head_ptr->subtype == 0x24)
        ret = CMD_USER_READ_ENABLE_SECURE_BIT;
      break;
    case DIAG_CMD_BUSMONITOR:
      if (head_ptr->subtype == 0x00) {
        ENG_LOG("%s: Handle CMD_USER_ENABLE_BUSMOINITOR", __FUNCTION__);
        ret = CMD_USER_ENABLE_BUSMOINITOR;
      } else if (head_ptr->subtype == 0x01) {
        ENG_LOG("%s: Handle CMD_USER_DISABLE_BUSMOINITOR", __FUNCTION__);
        ret = CMD_USER_DISABLE_BUSMOINITOR;
      } else if (head_ptr->subtype == 0x02) {
        ENG_LOG("%s: Handle CMD_USER_GET_CHANINFO", __FUNCTION__);
        ret = CMD_USER_GET_CHANINFO;
      } else if (head_ptr->subtype == 0x03) {
        ENG_LOG("%s: Handle CMD_USER_GET_RTCTIME", __FUNCTION__);
        ret = CMD_USER_GET_RTCTIME;
      } else if (head_ptr->subtype == 0x04) {
        ENG_LOG("%s: Handle CMD_USER_GET_MONITORDATA", __FUNCTION__);
        ret = CMD_USER_GET_MONITORDATA;
      }
      break;
    default:
      ENG_LOG("%s: Default\n", __FUNCTION__);
      ret = CMD_COMMON;
      break;
  }
  return ret;
}

static void eng_attest_build_rsphead(char *buf, MSG_HEAD_T *head, int rlen) {
  int i = 0;
  char *ptr = NULL;
  memcpy((char *)head, buf + 1, sizeof(MSG_HEAD_T));
  if (rlen >= 0) {
    head->len = sizeof(MSG_HEAD_T) + rlen;
    head->subtype = 0;
  } else {
    head->len = sizeof(MSG_HEAD_T);
    head->subtype = 1;
  }

  ptr = (char *)head;
  for (i = 0; i < sizeof(MSG_HEAD_T); i++) {
    ENG_LOG("%s [%d]: 0x%x", __FUNCTION__, i, ptr[i]);
  }
}

static int eng_diag_attest(char *data, int count, char *out_msg) {
#ifdef USE_AUDIO_WHALE_HAL
  char sub_type;
#else
  uint8 sub_type;
#endif
  int rlen;
  MSG_HEAD_T head, *head_ptr = NULL;
  char rsp[512];
  char rsp_buf[1024];
  memset(rsp, 0, sizeof(rsp));
  memset(rsp_buf, 0, sizeof(rsp_buf));
  head_ptr = (MSG_HEAD_T *)(data + 1);
  sub_type = head_ptr->subtype;
  if (sub_type >= (int)NUM_ELEMS(eng_autotestcmd)) {
    ENG_LOG("%s: no handler for CMD_USER_AUTOTEST sub_type = %d", __FUNCTION__,
            sub_type);
    return -1;
  }
  //-- [[ for autotest 20120220
  if (eng_autotestIsConnect() >= 0) {
    rlen = eng_autotestDoTest((const uchar *)data + 1, count - 1, (uchar *)rsp,
                              ENG_BUFFER_SIZE);
  } else {
    rlen = -1;  // eng_autotestcmd[sub_type].cmd_hdlr(data, rsp);
  }
  //-- ]]
  ENG_LOG("%s: rlen=%d\n", __FUNCTION__, rlen);
  // Send response
  eng_attest_build_rsphead(data, &head, rlen);
  if (rlen > 0) {
    head.len = sizeof(MSG_HEAD_T) + rlen;
    memcpy(rsp_buf, &head, sizeof(MSG_HEAD_T));
    memcpy(rsp_buf + sizeof(MSG_HEAD_T), rsp, rlen);
  } else {
    memcpy(rsp_buf, &head, sizeof(MSG_HEAD_T));
  }
  return translate_packet(out_msg, rsp_buf, head.len);
}

static int get_modem_readpath(char *modem_file) {
  int modem = -1;
  char modem_path[128] = {0};
  char persist_prop[128] = {0};
  char prop_nvp[128] = {0};
  char prop[128] = {0};
  char modem_enable[128] = {0};

  if (-1 == property_get("ro.product.partitionpath", modem_path, "")) {
    ENG_LOG("%s: get partitionpath fail\n", __FUNCTION__);
    return 0;
  }

  /*get modem mode from property*/
  strcpy(prop, TD_MODEM_ENABLE_PROP);
  property_get(prop, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    modem = TD_MODEM;
  }
  memset(prop, 0, sizeof(prop));
  strcpy(prop, W_MODEM_ENABLE_PROP);
  property_get(prop, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    modem = W_MODEM;
  }
  memset(prop, 0, sizeof(prop));
  strcpy(prop, LTE_MODEM_ENABLE_PROP);
  property_get(prop, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    modem = LTE_MODEM;
  }
  memset(prop, 0, sizeof(prop));
  strcpy(prop, LF_MODEM_ENABLE_PROP);
  property_get(prop, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    modem = LF_MODEM;
  }
  memset(prop, 0, sizeof(prop));
  strcpy(prop, TL_MODEM_ENABLE_PROP);
  property_get(prop, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    modem = TL_MODEM;
  }
  if (modem < 0) {
    ENG_LOG("%s: ALL_MODEM disable\n", __FUNCTION__);
    return 0;
  }
  strcpy(persist_prop, PERSIST_MODEM_CHAR);

  switch (modem) {
    case TD_MODEM: {
      strcat(persist_prop, "t.nvp");
      property_get(persist_prop, prop_nvp, "");
      strcat(modem_path, prop_nvp);
      if (0 == strlen(modem_path)) {
        ENG_LOG("invalid ro.modem.x.nvp path_char %s\n", modem_path);
        return 0;
      }
    } break;
    case W_MODEM: {
      strcat(persist_prop, "w.nvp");
      property_get(persist_prop, prop_nvp, "");
      strcat(modem_path, prop_nvp);
      if (0 == strlen(modem_path)) {
        ENG_LOG("invalid ro.modem.x.nvp path_char %s\n", modem_path);
        return 0;
      }
    } break;
    case LF_MODEM: {
      strcat(persist_prop, "lf.nvp");
      property_get(persist_prop, prop_nvp, "");
      strcat(modem_path, prop_nvp);
      if (0 == strlen(modem_path)) {
        ENG_LOG("invalid ro.modem.x.nvp path_char %s\n", modem_path);
        return 0;
      }
    } break;
    case LTE_MODEM: {
      strcat(persist_prop, "l.nvp");
      property_get(persist_prop, prop_nvp, "");
      strcat(modem_path, prop_nvp);
      if (0 == strlen(modem_path)) {
        ENG_LOG("invalid ro.modem.x.nvp path_char %s\n", modem_path);
        return 0;
      }
    } break;
    case TL_MODEM: {
      strcat(persist_prop, "tl.nvp");
      property_get(persist_prop, prop_nvp, "");
      strcat(modem_path, prop_nvp);
      if (0 == strlen(modem_path)) {
        ENG_LOG("invalid ro.modem.x.nvp path_char %s\n", modem_path);
        return 0;
      }
    } break;
  }

  strcat(strcpy(modem_file, modem_path), "modem");
  ENG_LOG("%s: line=%d, modem_file=%s\n", __FUNCTION__, __LINE__, modem_file);

  return -1;
}

static int read_modem_db_attr(char *buffer) {
  int ret = 0;
  int len = 0;
  int heads_len = 0;
  int secure_offset = 0;
  char modem_file[256] = {0};
  DataBlockHeader_T dbheader = {0};
  FILE *fp;
  DBSHA1_T dbsha1 = {0};
  TOOLS_DIAG_AP_CNF_MD_T cnf_md = {0};
  memset(&cnf_md, 0, sizeof(TOOLS_DIAG_AP_CNF_MD_T));

  if (!get_modem_readpath(modem_file)) {
    ENG_LOG("%s: can not get modem readpath\n", __FUNCTION__);
    return 0;
  }

  fp = fopen(modem_file, "r");
  if (NULL == fp) {
    ENG_LOG("%s: can not open file: %s \n", __FUNCTION__, modem_file);
    return 0;
  }

  cnf_md.cmd = DIAG_AP_CMD_MODEM_DB_ATTR;

#ifdef SECURE_BOOT_ENABLE
  secure_offset = BOOT_INFO_SIZE + VLR_INFO_SIZE;
  ENG_LOG("%s: secure boot enable, fseek offset\n", __FUNCTION__);
  if (fseek(fp, secure_offset, SEEK_SET)) {
    ENG_LOG("%s: fseek secure_offset fail\n", __FUNCTION__);
    fclose(fp);
    return 0;
  };
#endif

  if (fread(&dbheader, 1, sizeof(DataBlockHeader_T), fp) !=
      sizeof(DataBlockHeader_T)) {
    ENG_LOG("%s: read DataBlockHeader fail\n", __FUNCTION__);
    fclose(fp);
    return 0;
  }
  if (strncmp((const char *)&dbheader, "SCI1", 4) != 0) {
    ENG_LOG("%s: no modem db\n", __FUNCTION__);
    goto NOMDDB_ERROR;
  }

  do {
    memset(&dbheader, 0, sizeof(DataBlockHeader_T));
    if (fread(&dbheader, 1, sizeof(DataBlockHeader_T), fp) !=
        sizeof(DataBlockHeader_T)) {
      ENG_LOG("%s: read DataBlockHeader fail\n", __FUNCTION__);
      fclose(fp);
      return 0;
    }
    if ((dbheader.type_flags & 0xff) == DBHEADER_FLAG_MODEMIMG) {
      // modem img
    } else if ((dbheader.type_flags & 0xff) == DBHEADER_FLAG_MODEMDB) {
      // modem db
      if (((dbheader.type_flags >> 10) & 0x01) == DBHEADER_FLAG_SHA1) {
        if (fseek(fp, dbheader.offset, SEEK_SET)) {
          ENG_LOG("%s: fseek dbheader.offset fail\n", __FUNCTION__);
          fclose(fp);
          return 0;
        };
        if (fread((&dbsha1)->lpSHA1, 1, SHA1_SIZE, fp) != SHA1_SIZE) {
          ENG_LOG("%s: read sha1 data fail\n", __FUNCTION__);
          fclose(fp);
          return 0;
        }

        cnf_md.length = SHA1_SIZE;
        cnf_md.status = 0;  // success
        memcpy(buffer, &cnf_md, sizeof(TOOLS_DIAG_AP_CNF_MD_T));
        memcpy(buffer + sizeof(TOOLS_DIAG_AP_CNF_MD_T), &dbsha1, cnf_md.length);
        fclose(fp);
        return sizeof(TOOLS_DIAG_AP_CNF_MD_T) + cnf_md.length;
      } else {
        ENG_LOG("%s: no SHA-1\n", __FUNCTION__);
        cnf_md.length = 0;
        cnf_md.status = 2;  // no SHA-1
        memcpy(buffer, &cnf_md, sizeof(TOOLS_DIAG_AP_CNF_MD_T));
        fclose(fp);
        return sizeof(TOOLS_DIAG_AP_CNF_MD_T);
      }
    }
    // ENG_LOG("%s: line=%d, dbheader.type_flags=x%02x\n",__FUNCTION__,
    // __LINE__, dbheader.type_flags);

  } while (((dbheader.type_flags >> 8) & 0x01) != DBHEADER_FLAG_ISEND);

NOMDDB_ERROR:
  cnf_md.length = 0;
  cnf_md.status = 1;  // no modem.db
  memcpy(buffer, &cnf_md, sizeof(TOOLS_DIAG_AP_CNF_MD_T));
  fclose(fp);
  return sizeof(TOOLS_DIAG_AP_CNF_MD_T);
}

static int eng_diag_modem_db_attr(char *buf, int len, char *rsp) {
  int data_len = 0, i;
  int head_len = 0;
  int diag_len = 0;
  char rsptmp[512] = {0};

  MSG_HEAD_T *msg_head = (MSG_HEAD_T *)(buf + 1);
  head_len = sizeof(MSG_HEAD_T);
  memcpy(rsptmp, msg_head, head_len);

  data_len = read_modem_db_attr((char *)rsptmp + head_len);

  MSG_HEAD_T *msg_head_p = (MSG_HEAD_T *)rsptmp;
  msg_head_p->len = head_len + data_len;

  diag_len = translate_packet(rsp, rsptmp, head_len + data_len);

  for (i = 0; i < diag_len; i++) {
    ENG_LOG("%s: rsp[%d]=%x\n", __FUNCTION__, i, rsp[i]);
  }

  return diag_len;
}

static int eng_diag_modem_db_read(char *buf, int len, char *rsp) {
  int file_size = 0;
  int read_size = 0;
  int read_len = 0;
  int total_size = 0;
  int db_offset = 0, secure_offset = 0;
  int sha1_flag = 0;
  int ret = 0;
  int fd_ser = -1;
  char modem_file[256] = {0};
  int diag_size =
      sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_MD_T) + sizeof(ModemDB_T);
  unsigned char *tmp;
  unsigned char total = 0;
  DataBlockHeader_T dbheader = {0};
  FILE *fp;

  tmp = (unsigned char *)malloc(diag_size);
  if (NULL == tmp) {
    ENG_LOG("%s: no free heap memory!\n", __FUNCTION__);
    return 0;
  }

  memset(tmp, 0, diag_size);
  MSG_HEAD_T *header = (MSG_HEAD_T *)tmp;
  TOOLS_DIAG_AP_CNF_MD_T *mbrsp =
      (TOOLS_DIAG_AP_CNF_MD_T *)(tmp + sizeof(MSG_HEAD_T));
  ModemDB_T *filedata =
      (ModemDB_T *)(tmp + sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_MD_T));

  memcpy(tmp, buf + 1, sizeof(MSG_HEAD_T));

  if (!get_modem_readpath(modem_file)) {
    ENG_LOG("%s: can not get modem readpath\n", __FUNCTION__);
    free(tmp);
    return 0;
  }

  fp = fopen(modem_file, "r");
  if (NULL == fp) {
    ENG_LOG("%s: can not open file: %s \n", __FUNCTION__, modem_file);
    free(tmp);
    return 0;
  }

  mbrsp->cmd = DIAG_AP_CMD_MODEM_DB_READ;

#ifdef SECURE_BOOT_ENABLE
  secure_offset = BOOT_INFO_SIZE + VLR_INFO_SIZE;
  ENG_LOG("%s: secure boot enable, fseek offset\n", __FUNCTION__);
  if (fseek(fp, secure_offset, SEEK_SET)) {
    ENG_LOG("%s: fseek secure_offset fail\n", __FUNCTION__);
    free(tmp);
    fclose(fp);
    return 0;
  };
#endif

  if (fread(&dbheader, 1, sizeof(DataBlockHeader_T), fp) !=
      sizeof(DataBlockHeader_T)) {
    ENG_LOG("%s: read DataBlockHeader fail\n", __FUNCTION__);
    free(tmp);
    fclose(fp);
    return 0;
  }
  if (strncmp((const char *)&dbheader, "SCI1", 4) != 0) {
    ENG_LOG("%s: no modem db\n", __FUNCTION__);
    free(tmp);
    fclose(fp);
    return 0;
  }

  do {
    memset(&dbheader, 0, sizeof(DataBlockHeader_T));
    if (fread(&dbheader, 1, sizeof(DataBlockHeader_T), fp) !=
        sizeof(DataBlockHeader_T)) {
      ENG_LOG("%s: read DataBlockHeader fail\n", __FUNCTION__);
      free(tmp);
      fclose(fp);
      return 0;
    }
    if ((dbheader.type_flags & 0xff) == DBHEADER_FLAG_MODEMIMG) {
      // modem img
    } else if ((dbheader.type_flags & 0xff) == DBHEADER_FLAG_MODEMDB) {
      // modem db
      if (((dbheader.type_flags >> 10) & 0x01) == DBHEADER_FLAG_SHA1) {
        sha1_flag = 1;

      } else {  // no sha-1
        sha1_flag = 0;
        mbrsp->status = 2;
      }

      sha1_flag
          ? (file_size = dbheader.length - SHA1_SIZE)
          : (file_size = dbheader.length);  // modem.db size(without sha1 size)

      ENG_LOG("%s: line=%d, file_size=%d\n", __FUNCTION__, __LINE__, file_size);
      /*total(uchar) must be less then 255*/
      total = file_size / MAX_DIAG_TRANSMIT_MODEMDB_LEN;
      if (total > 254) {
        ENG_LOG("%s: couts of modemdb pkg too many: %d\n", __FUNCTION__, total);
        free(tmp);
        fclose(fp);
        return 0;
      }

      if (0 == (file_size % MAX_DIAG_TRANSMIT_MODEMDB_LEN)) {
        filedata->total = total;
      } else {
        filedata->total = total + 1;
      }

      sha1_flag ? (db_offset = dbheader.offset + SHA1_SIZE)
                : (db_offset = dbheader.offset);
      if (fseek(fp, dbheader.offset + SHA1_SIZE, SEEK_SET)) {
        ENG_LOG("%s: fseek dbheader.offset + SHA1_SIZE fail\n", __FUNCTION__);
        free(tmp);
        fclose(fp);
        return 0;
      };

      do {
        if (filedata->total - filedata->sn == 1) {
          read_len =
              file_size - MAX_DIAG_TRANSMIT_MODEMDB_LEN * (filedata->total - 1);
        } else {
          read_len = MAX_DIAG_TRANSMIT_MODEMDB_LEN;
        }

        read_size = fread(filedata->data, 1, read_len, fp);
        total_size += read_size;
        filedata->sn += 1;
        mbrsp->length =
            sizeof(ModemDB_T) - MAX_DIAG_TRANSMIT_MODEMDB_LEN + read_size;
        mbrsp->status = 0;
        header->len =
            sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_MD_T) + mbrsp->length;

        ret = translate_packet(rsp, tmp, header->len);
        fd_ser = get_ser_diag_fd();
        ret = eng_diag_write2pc(rsp, ret, fd_ser);
        if(ret < 0) {
          ENG_LOG("%s: write to pc modem db fail\n", __FUNCTION__);
          break;
        }
        ENG_LOG("%s: line=%d, filedata->sn=%d, filedata->total=%d\n",
                __FUNCTION__, __LINE__, filedata->sn, filedata->total);
        ENG_LOG("%s: response length(read): %d\n", __FUNCTION__, ret);

      } while (filedata->sn < filedata->total);

      fclose(fp);
      free(tmp);
      return 0;
    }

  } while (((dbheader.type_flags >> 8) & 0x01) != DBHEADER_FLAG_ISEND);
  fclose(fp);
  free(tmp);
  return 0;
}

int eng_diag_user_handle(int type, char *buf, int len) {
  int rlen = 0, i;
  int extra_len = 0;
  int ret;
  MSG_HEAD_T head, *head_ptr = NULL;
  char rsp[512];
  int adc_rsp[8];
  int fd = -1;

  memset(rsp, 0, sizeof(rsp));
  memset(adc_rsp, 0, sizeof(adc_rsp));
  fd = get_ser_diag_fd();

  ENG_LOG("%s: type=%d\n", __FUNCTION__, type);

  switch (type) {
    case CMD_USER_VER:
      rlen = eng_diag_getver((unsigned char *)buf, len, rsp);
      break;
    case CMD_USER_RESET:
      rlen = eng_diag_bootreset((unsigned char *)buf, len, rsp);
      g_reset = 1;
      break;
    case CMD_USER_BTWIFIIMEI:
      rlen = eng_diag_btwifiimei(buf, len, rsp, sizeof(rsp));
      if (!s_cp_ap_proc) {
        eng_diag_write2pc(rsp, rlen, fd);
      }
      return 0;
    case CMD_USER_FACTORYMODE:
      rlen = eng_diag_factorymode(buf, len, rsp);
      break;
    case CMD_USER_ADC:
      rlen = eng_diag_adc(buf, adc_rsp);
      break;
/* add FM pandora for marlin */
#if 0  //
	case CMD_USER_FM:
            rlen=start_fm_test(buf,len,rsp);
            eng_diag_write2pc(rsp,rlen, fd);

	    return 0;
	    break;
#endif
    case CMD_USER_AUTOTEST:
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_attest(buf, len, eng_diag_buf);
      eng_diag_len = rlen;
      if (eng_diag_len > 0) {
        for (i = 0; i < eng_diag_len; i++) {
          ENG_LOG("%s: eng_diag_buf[%d]=%x\n", __FUNCTION__, i,
                  eng_diag_buf[i]);
        }
        eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      }
      return 0;
      break;
    /* add read modem.db attr */
    case CMD_USER_MODEM_DB_ATTR:
      ENG_LOG("%s: CMD_USER_MODEM_DB_ATTR\n", __FUNCTION__);
      rlen = eng_diag_modem_db_attr(buf, len, eng_diag_buf);
      ENG_LOG("%s: rlen=%d\n", __FUNCTION__, rlen);
      eng_diag_len = rlen;
      if (eng_diag_len > 0) {
        eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      }
      return 0;
      break;
    /* add read modem.db data */
    case CMD_USER_MODEM_DB_READ:
      ENG_LOG("%s: CMD_USER_MODEM_DB_READ\n", __FUNCTION__);
      eng_diag_modem_db_read(buf, len, eng_modemdb_diag_buf);
      return 0;
      break;
    case CMD_USER_AUTOTEST_PATH_CONFIRM:
      /*eng_attest_build_rsphead(buf, &head, 0);
      memcpy(eng_diag_buf, &head, sizeof(MSG_HEAD_T));
      rlen = eng_hex2ascii(eng_diag_buf, eng_atdiag_buf, head.len);
      ENG_LOG("%s: after hex rlen=%d\n",__FUNCTION__, rlen);
      rlen = eng_atdiag_rsp(eng_get_csclient_fd(), eng_atdiag_buf, rlen);*/
      break;
#if (!defined USE_AUDIO_WHALE_HAL) && (!defined CONFIG_MINIENGPC)
    case CMD_USER_AUDIO:
      memset(eng_audio_diag_buf, 0, sizeof(eng_audio_diag_buf));
      rlen = eng_diag_audio(buf, len, eng_audio_diag_buf);
      break;
#endif
    case CMD_USER_GETVOLTAGE:
    case CMD_USER_APCALI:
      rlen =
          eng_battery_calibration(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
      break;
    case CMD_USER_APCMD:
      // For compatible with pc tool: MOBILETEST,
      // send a empty diag framer first.
      {
        char emptyDiag[] = {0x7e, 0x00, 0x00, 0x00, 0x00,
                            0x08, 0x00, 0xd5, 0x00, 0x7e};
        write(get_ser_diag_fd(), emptyDiag, sizeof(emptyDiag));
      }
      rlen = eng_diag_apcmd_hdlr(buf, len, rsp);
      break;
    case CMD_USER_PRODUCT_CTRL:
      ENG_LOG("%s: CMD_USER_PRODUCT_CTRL\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen =
          eng_diag_product_ctrl(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_DIRECT_PHSCHK:
      ENG_LOG("%s: CMD_USER_DIRECT_PHSCHK\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen =
          eng_diag_direct_phschk(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_MMICIT_READ:
      ENG_LOG("%s: CMD_USER_MMICIT_READ Req !\n", __FUNCTION__);
      rlen = eng_diag_mmicit_read(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_DEEP_SLEEP:
      ENG_LOG("%s: CMD_USER_DEEP_SLEEP Req!\n", __FUNCTION__);
      rlen = eng_diag_deep_sleep(buf, len, rsp);
      s_cp_ap_proc = 1;  // This cmd need cp proc.
      return 0;
    case CMD_USER_FILE_OPER:
      ENG_LOG("%s: CMD_USER_FILE_OPER Req!\n", __FUNCTION__);
      rlen = eng_diag_fileoper_hdlr(buf, len, rsp);
      break;
    case CMD_USER_SHUT_DOWN:
      ENG_LOG("%s: CMD_USER_SHUT_DOWN Req!\n", __FUNCTION__);
      reboot(LINUX_REBOOT_CMD_POWER_OFF);
      break;
    case CMD_USER_CFT_SWITCH:
      ENG_LOG("%s: CMD_USER_CFT_SWITCHReq!\n", __FUNCTION__);
      eng_diag_cft_switch_hdlr(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      break;
#ifndef CONFIG_MINIENGPC
    case CMD_USER_GPS_AUTO_TEST:
      ENG_LOG("%s: CMD_USER_GPS_AUTO_TEST Req!\n", __FUNCTION__);
      rlen = eng_diag_gps_autotest_hdlr(buf, len, eng_diag_buf,
                                        sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#endif
    case CMD_USER_ENABLE_CHARGE_ONOFF:
      rlen = eng_diag_enable_charge(buf, len, rsp, sizeof(rsp));
      eng_diag_write2pc(rsp, rlen, fd);
      return 0;
    case CMD_USER_GET_CHARGE_CURRENT:
      rlen = eng_diag_get_charge_current(buf, len, rsp, sizeof(rsp));
      eng_diag_write2pc(rsp, rlen, fd);
      return 0;
#ifndef CONFIG_MINIENGPC
    case CMD_USER_READ_EFUSE:
      ENG_LOG("%s: CMD_USER_READ_EFUSE Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_read_efuse(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_WRITE_EFUSE:
      ENG_LOG("%s: CMD_USER_WRITE_EFUSE Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_write_efuse(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#endif
    case CMD_USER_READ_PUBLICKEY:
      ENG_LOG("%s: CMD_USER_READ_PUBLICKEY Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen =
          eng_diag_read_publickey(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#ifndef CONFIG_MINIENGPC
    case CMD_USER_ENABLE_SECURE:
      ENG_LOG("%s: CMD_USER_SCURE_ENABLE Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen =
          eng_diag_enable_secure(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_READ_ENABLE_SECURE_BIT:
      ENG_LOG("%s: CMD_USER_SCURE_ENABLE Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_read_enable_secure_bit(buf, len, eng_diag_buf,
                                             sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#if defined(TEE_PRODUCTION_CONFIG)
    case CMD_USER_TEE_PRODUCTION:
      ENG_LOG("%s: CMD_USER_TEE_PRODUCTION Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_tee_production(buf, len, eng_diag_buf,
              sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      for(i=0;i<eng_diag_len;i++)
      {
          ENG_LOG("%s: eng_diag_buf[%d]=%x\n",__FUNCTION__, i,eng_diag_buf[i]);
      }
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#endif
#endif
    case CMD_USER_GET_MODEM_MODE:
      rlen = eng_diag_get_modem_mode(buf, len, rsp, sizeof(rsp));
      eng_diag_write2pc(rsp, rlen, fd);
      return 0;
    case CMD_USER_READ_MMI:
      ENG_LOG("%s: CMD_USER_READ_MMI Req!\n", __FUNCTION__);
      memset(eng_diag_buf,0,sizeof(eng_diag_buf));
      rlen = eng_diag_read_mmi(buf, len, eng_diag_buf,sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf,eng_diag_len ,fd);
      return 0;
    case CMD_USER_WRITE_MMI:
      ENG_LOG("%s: CMD_USER_READ_MMI Req!\n", __FUNCTION__);
      memset(eng_diag_buf,0,sizeof(eng_diag_buf));
      rlen = eng_diag_write_mmi(buf, len, eng_diag_buf,sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf,eng_diag_len ,fd);
      return 0;
    case CMD_USER_GET_TIME_SYNC_INFO:
      ENG_LOG("%s: CMD_USER_GET_TIME_SYNC_INFO Req!\n", __FUNCTION__);
      memset(eng_diag_buf,0,sizeof(eng_diag_buf));
      rlen = eng_diag_get_time_sync_info(buf, len, eng_diag_buf,sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf,eng_diag_len ,fd);
      return 0;
    case CMD_USER_BKLIGHT:
      ENG_LOG("%s: CMD_USER_BKLIGHT Req!\n", __FUNCTION__);
      rlen =
          eng_diag_set_backlight(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_PWMODE:
      ENG_LOG("%s: CMD_USER_PWMODE Req!\n", __FUNCTION__);
      rlen =
          eng_diag_set_powermode(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_SET_CONFIGURE_IP:
      ENG_LOG("%s: CMD_USER_SET_CONFIGURE_IP Req!\n", __FUNCTION__);
      rlen = eng_diag_set_ipconfigure(buf, len, eng_diag_buf,
                                      sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_READ_REGISTER:
      ENG_LOG("%s: CMD_USER_READ_REGISTER Req!\n", __FUNCTION__);
      rlen =
          eng_diag_read_register(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_WRITE_REGISTER:
      ENG_LOG("%s: CMD_USER_WRITE_REGISTER Req!\n", __FUNCTION__);
      rlen =
          eng_diag_write_register(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#ifndef CONFIG_MINIENGPC
    case CMD_USER_ENABLE_BUSMOINITOR:
      ENG_LOG("%s: CMD_USER_ENABLE_BUSMOINITOR Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_busmonitor_enable(buf, len, eng_diag_buf,
                                        sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_DISABLE_BUSMOINITOR:
      ENG_LOG("%s: CMD_USER_DISABLE_BUSMOINITOR Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_busmonitor_disable(buf, len, eng_diag_buf,
                                         sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_GET_CHANINFO:
      ENG_LOG("%s: CMD_USER_GET_CHANINFO Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_busmonitor_get_chaninfo(buf, len, eng_diag_buf,
                                              sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_GET_RTCTIME:
      ENG_LOG("%s: CMD_USER_GET_RTCTIME Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_busmonitor_get_rtctime(buf, len, eng_diag_buf,
                                             sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
    case CMD_USER_GET_MONITORDATA:
      ENG_LOG("%s: CMD_USER_GET_MONITORDATA Req!\n", __FUNCTION__);
      memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
      rlen = eng_diag_busmonitor_get_monitordata(buf, len, eng_diag_buf,
                                                 sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      return 0;
#endif
    case CMD_USER_TXDATA:
      ENG_LOG("%s: CMD_USER_TXDATA Req!\n", __FUNCTION__);
      memset(eng_diag_buf,0,sizeof(eng_diag_buf));
      rlen = eng_diag_txdata(buf, len, eng_diag_buf, sizeof(eng_diag_buf));
      eng_diag_len = rlen;
      eng_diag_write2pc(eng_diag_buf, eng_diag_len,fd);
      return 0;
    default:
      break;
  }

  memcpy((char *)&head, buf + 1, sizeof(MSG_HEAD_T));
  head.len = sizeof(MSG_HEAD_T) + rlen - extra_len;
  ENG_LOG("%s: head.len=%d\n", __FUNCTION__, head.len);
  eng_diag_buf[0] = 0x7e;
#ifdef USE_AUDIO_WHALE_HAL
  if (type == CMD_USER_APCMD) {
#else
  if ((type == CMD_USER_AUDIO) || (type == CMD_USER_APCMD)) {
#endif
    head.seq_num = 0;
    head.type = 0x9c;
    head.subtype = 0x00;
  }

  if (type == CMD_USER_ADC) {
    head.subtype = 0x00;
  }

  memcpy(eng_diag_buf + 1, &head, sizeof(MSG_HEAD_T));

#ifdef USE_AUDIO_WHALE_HAL
  if (type == CMD_USER_ADC) {
#else
  if (type == CMD_USER_AUDIO) {
    memcpy(eng_diag_buf + sizeof(MSG_HEAD_T) + 1, eng_audio_diag_buf,
           strlen(eng_audio_diag_buf));
  } else if (type == CMD_USER_ADC) {
#endif
    memcpy(eng_diag_buf + sizeof(MSG_HEAD_T) + 1, adc_rsp, sizeof(adc_rsp));
  } else {
    memcpy(eng_diag_buf + sizeof(MSG_HEAD_T) + 1, rsp, sizeof(rsp));
  }
  eng_diag_buf[head.len + extra_len + 1] = 0x7e;
  eng_diag_len = head.len + extra_len + 2;
  ENG_LOG("%s: eng_diag_write2pc eng_diag_buf=%s;eng_diag_len:%d !\n",
          __FUNCTION__, eng_diag_buf, eng_diag_len);
  eng_diag_encode7d7e((char *)(eng_diag_buf+1), (eng_diag_len-2), &eng_diag_len);
  eng_diag_buf[eng_diag_len-1] = 0x7e;
  ret = eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
  if (ret <= 0) {
    ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
  }

  if (g_reset) {
    eng_diag_reboot(g_reset);
  }

#ifdef USE_AUDIO_WHALE_HAL
  if (CMD_USER_APCMD == type) {
    ENG_LOG("%s: this is audio type !\n", __FUNCTION__);
#else
  if (type == CMD_USER_AUDIO || (CMD_USER_APCMD == type)) {
    ENG_LOG("%s: this is audio type !\n", __FUNCTION__);
#endif
    return 1;
  }

  return 0;
}

static int eng_atdiag_parse(unsigned char *buf, int len) {
  int i;
  int ret = CMD_COMMON;
  MSG_HEAD_T *head_ptr = NULL;
  head_ptr = (MSG_HEAD_T *)(buf + 1);

  ENG_LOG("%s: cmd=0x%x; subcmd=0x%x", __FUNCTION__, head_ptr->type,
          head_ptr->subtype);

  switch (head_ptr->type) {
    case DIAG_CMD_VER:
      ENG_LOG("%s: Handle DIAG_CMD_VER", __FUNCTION__);
      if (head_ptr->subtype == 0x00) {
        ret = CMD_USER_VER;
      }
      break;
    default:
      ENG_LOG("%s: Default", __FUNCTION__);
      ret = CMD_COMMON;
      break;
  }
  return ret;
}

int eng_hex2ascii(char *input, char *output, int length) {
  int i;
  char tmp[3];
  for (i = 0; i < length; i++) {
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%02x", input[i]);
    strcat(output, tmp);
  }

  ENG_LOG("%s: %s", __func__, output);
  return strlen(output);
}

/********************************************************************
*   name   eng_atdiag_euthdlr
*   ---------------------------
*   description: \C8\EB\BFں\AF\CA\FD.
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   ------------------
*   other:
*   ע\D2\E2,\D5\E2\C0\EF\B0\FC\BA\AC2\CC׷\BD\B0\B8.
*   ENGMODE_EUT_BCM\CA\C7BRCM\B5ķ\BD\B0\B8,
*   ENGMODE_EUT_SPRD\CA\C7SPRD\B7\BD\B0\B8,\D4\DAengmode\B5\C4mk\D6п\D8\D6Ʊ\E0\D2\EB
********************************************************************/
#if defined(ENGMODE_EUT_BCM)
int eng_atdiag_euthdlr(char *buf, int len, char *rsp, int module_index) {
  // brcm
  char args0[20] = {0};
  char args1[20] = {0};
  char *data[2] = {args0, args1};
  int cmd_index = -1;
  ENG_LOG("brcm buf =%s\n", buf);
  get_sub_str(buf, data, '=', ',');
  ENG_LOG("brcm sub str is %s\n", buf);
  cmd_index = get_cmd_index(buf);
  /*
  if (module_index == BT_MODULE_INDEX) {
        cmd_index = get_brcm_bt_cmd_index(buf);
  }
  */
  ENG_LOG("\r\n");
  ENG_LOG("brcm: eng_atdiag_euthdlr(), args0 =%s, args1=%s, cmd_index=%d\n",
          args0, args1, cmd_index);
  switch (cmd_index) {
    case EUT_REQ_INDEX:
      ENG_LOG("wsh EUT_REQ_INDEX\n");
      if (module_index == BT_MODULE_INDEX) {
        ALOGD("case BT_EUT_REQ_INDEX");
        bt_eutops.bteut_req(rsp);
      } else if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("case WIFIEUT_INDEX");
        // wifi_eut_get(rsp);
        if (wifi_eutops.wifieut_req != NULL)
          wifi_eutops.wifieut_req(rsp);
        else
          ALOGE("wifi_eutops.wifieut_req not support!");
      } else {
        ALOGD("case GPS_INDEX");
        gps_eutops.gpseut_req(rsp);
      }
      break;
    case EUT_INDEX:
      ENG_LOG("wsh EUT_INDEX\n");
      if (module_index == BT_MODULE_INDEX) {
        ALOGD("case BTEUT_INDEX");
        bt_eutops.bteut(atoi(data[1]), rsp);
      } else if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("case WIFIEUT_INDEX");
        // wifi_eut_set(atoi(data[1]), rsp);
        if (wifi_eutops.wifieut != NULL)
          wifi_eutops.wifieut(atoi(data[1]), rsp);
        else
          ALOGE("wifi_eutops.wifieut not support!");
      } else {
        ALOGD("case GPS_INDEX");
        gps_eutops.gpseut(atoi(data[1]), rsp);
      }
      break;
    case WIFICH_REQ_INDEX:
      ENG_LOG("case WIFIECH_REQ_INDEX");
      // wifi_channel_get(rsp);
      if (wifi_eutops.wifi_ch_req != NULL)
        wifi_eutops.wifi_ch_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_ch_req not support!");
      break;
    case WIFICH_INDEX:
      ENG_LOG("case WIFICH_INDEX   %d", WIFICH_INDEX);
      // wifi_channel_set(atoi(data[1]),rsp);
      if (wifi_eutops.set_wifi_ch != NULL)
        wifi_eutops.set_wifi_ch(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.set_wifi_ch not support!");
      break;
    case WIFIMODE_INDEX:
      ENG_LOG("wsh WIFIMODE_INDEX\n");
      // wifi_eutops.set_wifi_mode(data[1],rsp);
      if (wifi_eutops.set_wifi_mode != NULL)
        wifi_eutops.set_wifi_mode(data[1], rsp);
      else
        ALOGE("wifi_eutops.set_wifi_mode not support!");
      break;
    case WIFIRATIO_INDEX:
      ALOGD("case WIFIRATIO_INDEX   %d", WIFIRATIO_INDEX);
      // wifi_eutops.set_wifi_ratio(atof(data[1]),rsp);
      if (wifi_eutops.set_wifi_ratio != NULL)
        wifi_eutops.set_wifi_ratio(atof(data[1]), rsp);
      else
        ALOGE("wifi_eutops.set_wifi_ratio not support!");
      break;
    case WIFITX_FACTOR_INDEX:
      // wifi_eutops.set_wifi_tx_factor(atol(data[1]),rsp);
      if (wifi_eutops.set_wifi_tx_factor != NULL)
        wifi_eutops.set_wifi_tx_factor(atol(data[1]), rsp);
      else
        ALOGE("wifi_eutops.set_wifi_tx_factor not support!");
      break;

    case WIFITX_FACTOR_REQ_INDEX:
      // wifi_eutops.wifi_tx_factor_req(rsp);
      if (wifi_eutops.wifi_tx_factor_req != NULL)
        wifi_eutops.wifi_tx_factor_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_factor_req not support!");
      break;
    case WIFIRATIO_REQ_INDEX:
      // wifi_eutops.wifi_ratio_req(rsp);
      if (wifi_eutops.wifi_ratio_req != NULL)
        wifi_eutops.wifi_ratio_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_ratio_req not support!");
      break;
    case WIFIRX_PACKCOUNT_INDEX:
      // wifi_rxpktcnt_get(rsp);
      if (wifi_eutops.wifi_rxpackcount != NULL)
        wifi_eutops.wifi_rxpackcount(rsp);
      else
        ALOGE("wifi_eutops.wifi_rxpackcount not support!");
      break;
    case WIFICLRRXPACKCOUNT_INDEX:
      // wifi_eutops.wifi_clr_rxpackcount(rsp);
      if (wifi_eutops.wifi_clr_rxpackcount != NULL)
        wifi_eutops.wifi_clr_rxpackcount(rsp);
      else
        ALOGE("wifi_eutops.wifi_clr_rxpackcount not support!");
      break;
    case WIFI_BAND_REQ_INDEX:
      if (wifi_eutops.wifiband_req != NULL)
        wifi_eutops.wifiband_req(rsp);
      else
        ALOGE("wifi_eutops.wifiband_req not support!");
      break;
    case WIFI_BAND_INDEX:
      if (wifi_eutops.wifiband != NULL)
        wifi_eutops.wifiband(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifiband not support!");
      break;
    case WIFITX_MODE_REQ_INDEX:
      if (wifi_eutops.wifi_tx_mode_req != NULL)
        wifi_eutops.wifi_tx_mode_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_mode_req not support!");
      break;
    case WIFITX_MODE_INDEX:
      if (wifi_eutops.wifi_tx_mode_bcm != NULL)
        wifi_eutops.wifi_tx_mode_bcm(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_mode not support!");
      break;
    case WIFITX_PWRLV_REQ_INDEX:
      ENG_LOG("case WIFITX_PWRLV_REQ_INDEX   %d", WIFITX_PWRLV_REQ_INDEX);
      if (wifi_eutops.wifi_tx_pwrlv_req != NULL)
        wifi_eutops.wifi_tx_pwrlv_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_pwrlv_req not support!");
      break;
    case WIFITX_PWRLV_INDEX:
      if (wifi_eutops.wifi_tx_pwrlv != NULL)
        wifi_eutops.wifi_tx_pwrlv(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_pwrlv not support!");
      break;
    case GPSSEARCH_REQ_INDEX:
      gps_eutops.gps_search_req(rsp);
      break;
    case GPSSEARCH_INDEX:
      gps_eutops.gps_search(atoi(data[1]), rsp);
      break;
    case GPSPRNSTATE_REQ_INDEX:
      gps_eutops.gps_prnstate_req(rsp);
      break;
    case GPSSNR_REQ_INDEX:
      gps_eutops.gps_snr_req(rsp);
      break;
    case GPSPRN_INDEX:
      gps_eutops.gps_setprn(atoi(data[1]), rsp);
      break;
    //-----------------------------------------------------
    case ENG_WIFIRATE_INDEX:
      ENG_LOG("%s(), case:ENG_WIFIRATE_INDEX\n", __FUNCTION__);
      // wifi_rate_set(data[1], rsp);
      if (wifi_eutops.set_wifi_rate != NULL)
        wifi_eutops.set_wifi_rate(data[1], rsp);
      else
        ALOGE("wifi_eutops.set_wifi_rate not support!");
      break;
    case ENG_WIFIRATE_REQ_INDEX:
      ENG_LOG("%s(), case:ENG_WIFIRATE_REQ_INDEX\n", __FUNCTION__);
      // wifi_rate_get(rsp);
      if (wifi_eutops.wifi_rate_req != NULL)
        wifi_eutops.wifi_rate_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_rate_req not support!");
      break;
    case ENG_WIFITXGAININDEX_INDEX:
      ENG_LOG("%s(), case:ENG_WIFITXGAININDEX_INDEX\n", __FUNCTION__);
      // wifi_txgainindex_set(atoi(data[1]),rsp);
      if (wifi_eutops.set_wifi_txgainindex != NULL)
        wifi_eutops.set_wifi_txgainindex(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.set_wifi_txgainindex not support!");
      break;
    case ENG_WIFITXGAININDEX_REQ_INDEX:
      ENG_LOG("%s(), case:ENG_WIFITXGAININDEX_REQ_INDEX\n", __FUNCTION__);
      // wifi_txgainindex_get(rsp);
      if (wifi_eutops.wifi_txgainindex_req != NULL)
        wifi_eutops.wifi_txgainindex_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_txgainindex_req not support!");
      break;
    case ENG_WIFIRSSI_REQ_INDEX:
      ENG_LOG("%s(), case:ENG_WIFIRSSI_REQ_INDEX\n", __FUNCTION__);
      // wifi_rssi_get(rsp);
      if (wifi_eutops.wifi_rssi_req != NULL)
        wifi_eutops.wifi_rssi_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_rssi_req not support!");
      break;
    //-----------------------------------------------------
    case WIFIPKTLEN_REQ_INDEX:
      ENG_LOG("case WIFIPKTLEN_REQ_INDEX");
      if (wifi_eutops.wifi_pkt_len_get != NULL)
        wifi_eutops.wifi_pkt_len_get(rsp);
      else
        ALOGE("wifi_eutops.wifi_pkt_len_get not support!");
      break;
    case WIFIPKTLEN_INDEX:
      ENG_LOG("case WIFIPKTLEN_INDEX");
      if (wifi_eutops.wifi_pkt_len_set != NULL)
        wifi_eutops.wifi_pkt_len_set(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_pwrlv not support!");
      break;
    case WIFIBANDWIDTH_REQ_INDEX:
      ENG_LOG("case WIFIBANDWIDTH_REQ_INDEX");
      if (wifi_eutops.wifi_bw_get != NULL)
        wifi_eutops.wifi_bw_get(rsp);
      else
        ALOGE("wifi_eutops.wifi_bw_get not support!");
      break;
    case WIFIBANDWIDTH_INDEX:
      ENG_LOG("case WIFIBANDWIDTH_INDEX");
      if (wifi_eutops.wifi_bw_set != NULL)
        wifi_eutops.wifi_bw_set(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifi_bw_set not support!");
      break;
    case WIFILNA_INDEX:
      ENG_LOG("case WIFILNA_INDEX");
      if (wifi_eutops.wifi_pm != NULL)
        wifi_eutops.wifi_pm(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifi_pm not support!");
      break;
    case WIFIPREAMBLE_INDEX:
      ENG_LOG("case WIFILNA_INDEX");
      if (wifi_eutops.wifi_eut_preamble_set != NULL)
        wifi_eutops.wifi_eut_preamble_set(atoi(data[1]), rsp);
      else
        ALOGE("wifi_eutops.wifi_eut_preamble_set not support!");
      break;
    // for BRCM Bluetooth
    /* TX Channel */
    case BT_TXCH_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXCH_REQ_INDEX", __func__);
      bt_channel_get(BTEUT_CMD_TYPE_TX, rsp);
    } break;

    case BT_TXCH_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXCH_INDEX", __func__);
      bt_channel_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
    } break;

    /* RX Channel */
    case BT_RXCH_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXCH_REQ_INDEX", __func__);
      bt_channel_get(BTEUT_CMD_TYPE_RX, rsp);
    } break;

    case BT_RXCH_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXCH_INDEX", __func__);
      bt_channel_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
    } break;

    /* Pattern */
    case BT_TXPATTERN_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPATTERN_REQ_INDEX", __func__);
      bt_pattern_get(BTEUT_CMD_TYPE_TX, rsp);
    } break;

    case BT_TXPATTERN_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPATTERN_INDEX", __func__);
      bt_pattern_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
    } break;

    case BT_RXPATTERN_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXPATTERN_REQ_INDEX", __func__);
      bt_pattern_get(BTEUT_CMD_TYPE_RX, rsp);
    } break;

    case BT_RXPATTERN_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXPATTERN_INDEX", __func__);
      bt_pattern_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
    } break;

    /* PKT Type */
    case BT_TXPKTTYPE_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPKTTYPE_REQ_INDEX", __func__);
      bt_pkttype_get(BTEUT_CMD_TYPE_TX, rsp);
    } break;

    case BT_TXPKTTYPE_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPATTERN_INDEX", __func__);
      bt_pkttype_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
    } break;

    case BT_RXPKTTYPE_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXPATTERN_REQ_INDEX", __func__);
      bt_pkttype_get(BTEUT_CMD_TYPE_RX, rsp);
    } break;

    case BT_RXPKTTYPE_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXPATTERN_INDEX", __func__);
      bt_pkttype_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
    } break;

    /* PKT Length */
    case BT_TXPKTLEN_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPKTLEN_REQ_INDEX", __func__);
      bt_txpktlen_get(rsp);
    } break;

    case BT_TXPKTLEN_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPKTLEN_INDEX", __func__);
      bt_txpktlen_set(atoi(data[1]), rsp);
    } break;

    /* TX Power */
    case BT_TXPWR_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPKTLEN_REQ_INDEX", __func__);
      bt_txpwr_get(rsp);
    } break;

    case BT_TXPWR_INDEX: {
      ENG_LOG("ADL %s(), case BT_TXPKTLEN_INDEX", __func__);
      int a1, a2;
      ENG_LOG("brcm BT_TXPWR_INDEX debug\n");
      sscanf(data[1], "%d,%d", &a1, &a2);
      bt_txpwr_set(a1, a2, rsp);
    } break;

    /* RX Gain */
    case BT_RXGAIN_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXGAIN_REQ_INDEX", __func__);
      bt_rxgain_get(rsp);
    } break;

    case BT_RXGAIN_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXGAIN_INDEX", __func__);
      int a1, a2;
      ENG_LOG("brcm BT_RXGAIN_INDEX debug\n");
      sscanf(data[1], "%d,%d", &a1, &a2);
      bt_rxgain_set(a1, a2, rsp);
    } break;

    /* ADDRESS */
    case BT_ADDRESS_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_ADDRESS_REQ_INDEX", __func__);
      bt_address_get(rsp);
    } break;

    case BT_ADDRESS_INDEX: {
      ENG_LOG("ADL %s(), case BT_ADDRESS_INDEX", __func__);
      bt_address_set(data[1], rsp);
    } break;

    /* RX received data */
    case BT_RXDATA_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_RXDATA_REQ_INDEX", __func__);
      bt_rxdata_get(rsp);
    } break;

    /* Testmode */
    case BT_TESTMODE_REQ_INDEX: {
      ENG_LOG("ADL %s(), case BT_TESTMODE_REQ_INDEX", __func__);

      if (BT_MODULE_INDEX == module_index) {
        bt_testmode_get(BTEUT_BT_MODE_CLASSIC, rsp);
      } else if (BLE_MODULE_INDEX == module_index) {
        bt_testmode_get(BTEUT_BT_MODE_BLE, rsp);
      }
    } break;

    /* Testmode */
    case BT_TESTMODE_INDEX: {
      ENG_LOG("ADL %s(), case BT_TESTMODE_INDEX, module_index = %d", __func__,
              module_index);
      if (BT_MODULE_INDEX == module_index) {
        bt_testmode_set(BTEUT_BT_MODE_CLASSIC, atoi(data[1]), rsp);
      } else if (BLE_MODULE_INDEX == module_index) {
        bt_testmode_set(BTEUT_BT_MODE_BLE, atoi(data[1]), rsp);
      }
    } break;

    case TX_INDEX: {
      ENG_LOG("ADL %s(), case TX_INDEX, module_index = %d", __func__,
              module_index);
      if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
        int pktcnt = 0;
        int a1, a2, a3;
        ENG_LOG("brcm TX_INDEX debug\n");
        sscanf(data[1], "%d,%d,%d", &a1, &a2, &a3);

        ENG_LOG(
            "ADL %s(), case TX_INDEX, call bt_tx_set(), a1 = %d,a2=%d,a3=%d\n",
            __func__, a1, a2, a3);
        bt_tx_set(a1, a2, a3, rsp);
      } else if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("case WIFITX_INDEX   %d", WIFITX_INDEX);
         // wifi_tx_set(atoi(data[1]),rsp);
         if (wifi_eutops.wifi_tx != NULL)
              wifi_eutops.wifi_tx(atoi(data[1]), rsp);
          else
              ALOGE("wifi_eutops.wifi_tx not support!");
      } else {
        ENG_LOG("ADL %s(), case TX_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case RX_INDEX: {
      ENG_LOG("ADL %s(), case RX_INDEX, module_index = %d", __func__,
              module_index);

      if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case RX_INDEX, call bt_rx_set()", __func__);
        bt_rx_set(atoi(data[1]), rsp);
      } else

          if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("ADL %s(), case RX_INDEX, call wifi_rx_set()", __func__);
	 // wifi_rx_set(atoi(data[1]),rsp);
        if (wifi_eutops.wifi_rx != NULL)
           wifi_eutops.wifi_rx(atoi(data[1]), rsp);
         else
            ALOGE("wifi_eutops.wifi_rx not support!");
      } else {
        ENG_LOG("ADL %s(), case RX_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case TX_REQ_INDEX: {
      ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index = %d", __func__,
              module_index);

      if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case TX_REQ_INDEX, call bt_tx_get()", __func__);
        bt_tx_get(rsp);
      } else if (WIFI_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case TX_REQ_INDEX, call wifi_tx_get()", __func__);
        // wifi_tx_get(rsp);
       if (wifi_eutops.wifi_tx_req != NULL)
         wifi_eutops.wifi_tx_req(rsp);
      else
        ALOGE("wifi_eutops.wifi_tx_req not support!");
      } else {
        ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case RX_REQ_INDEX: {
      ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index = %d", __func__,
              module_index);

      if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case RX_REQ_INDEX, call bt_rx_get()", __func__);
        bt_rx_get(rsp);
      } else if (WIFI_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case RX_REQ_INDEX, call wifi_rx_get()", __func__);
        // wifi_rx_get(rsp);
       if (wifi_eutops.wifi_rx_req != NULL)
          wifi_eutops.wifi_rx_req(rsp);
      else
          ALOGE("wifi_eutops.wifi_rx_req not support!");
      } else {
        ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index is ERROR", __func__);
      }
    } break;

    default:
      strcpy(rsp, "can not match the at command");
      return 0;
  }

  // @alvin:
  // Here: I think it will response to pc directly outside
  // this function and should not send to modem again.
  ALOGD(" eng_atdiag_rsp   %s", rsp);
  ENG_LOG("eng_atdiag_rsp %s", rsp);
  return 0;
}
int get_sub_str(char *buf, char **revdata, char a, char b) {
  int len, len1;
  char *start;
  char *current;
  char *end = buf;
  start = strchr(buf, a);
  current = strchr(buf, b);
  ALOGD("get_sub_str ----->>  %d", (int)current);
  if (!current) {
    return 0;
  }
  while (end && *end != '\0') end++;
  if ((start != NULL) & (end != NULL)) {
    start++;
    current++;
    len = current - start - 1;
    len1 = end - current;
    ALOGD("get_sub_str  len1= %d", len1);
    memcpy(revdata[0], start, len);
    memcpy(revdata[1], current, len1);
  }
  return 0;
}
int get_cmd_index(char *buf) {
  int index = -1;
  int i;
  ENG_LOG("%s(), buf = %s", __func__, buf);
  for (i = 0; i < (int)NUM_ELEMS(eut_cmds); i++) {
    if (strstr(buf, eut_cmds[i].name) != NULL) {
      ENG_LOG(
          "i=%d, eut_cmds[i].index= "
          "%d, eut_cmds[i].name = %s\n",
          i, eut_cmds[i].index, eut_cmds[i].name);
      index = eut_cmds[i].index;
      break;
    }
  }
  return index;
}

int get_brcm_bt_cmd_index(char *buf) {
  int index = -1;
  int i;
  ALOGD("brcm get cmd index\n");
  for (i = 0; i < (int)NUM_ELEMS(eut_cmds); i++) {
    if (!(strcmp(buf, eut_cmds[i].name))) {
      ENG_LOG(
          "i=%d, eut_cmds[i].index= "
          "%d, eut_cmds[i].name = %s\n",
          i, eut_cmds[i].index, eut_cmds[i].name);
      index = eut_cmds[i].index;
      break;
    } else {
      ENG_LOG(
          "+++++++++++i=%d, eut_cmds[i].index= "
          "%d, eut_cmds[i].name = %s\n",
          i, eut_cmds[i].index, eut_cmds[i].name);
    }
  }
  return index;
}

#elif defined(ENGMODE_EUT_SPRD)
int eng_atdiag_euthdlr(char *buf, int len, char *rsp, int module_index) {
  // spreadtrum
  char args0[32 + 1] = {0x00};
  char args1[32 + 1] = {0x00};
  char args2[32 + 1] = {0x00};
  char args3[32 + 1] = {0x00};

  char *data[4] = {args0, args1, args2, args3};
  int cmd_index = -1;

  ENG_LOG("\r\n");
  ENG_LOG("ADL entry %s(), module=%d, buf = %s", __func__, module_index, buf);

  if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
    bt_eut_parse(module_index, buf, rsp);
    return 0;
  }

  get_sub_str(buf, data, '=', ",", 4, 32);
  cmd_index = get_cmd_index(buf);
  ENG_LOG(
      "ADL %s(), args0 = %s, args1 = %s, args2 = %s, args3 = %s cmd_index = "
      "%d, module_index = %d",
      __func__, args0, args1, args2, args3, cmd_index, module_index);

  if (module_index == GPS_MODULE_INDEX) {
#ifndef CONFIG_MINIENGPC
    gps_eut_parse(buf, rsp);
#endif
    return 0;
  }

  switch (cmd_index) {
    case EUT_REQ_INDEX:
      if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("case WIFIEUT_INDEX");
        wifi_eut_get(rsp);
      }

      break;

    case EUT_INDEX:
      if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("case WIFIEUT_INDEX");
        wifi_eut_set(atoi(data[1]), rsp);
      }

      break;
    case WIFICH_REQ_INDEX:
      wifi_channel_get(rsp);
      break;
    case WIFICH_INDEX:
      ENG_LOG("case WIFICH_INDEX   %d", WIFICH_INDEX);
      wifi_channel_set(atoi(data[1]), rsp);
      break;
    case WIFIMODE_INDEX:
      // wifi_eutops.set_wifi_mode(data[1],rsp);
      break;
    case WIFIRATIO_INDEX:
      ALOGD("case WIFIRATIO_INDEX   %d", WIFIRATIO_INDEX);
      // wifi_eutops.set_wifi_ratio(atof(data[1]),rsp);
      break;
    case WIFITX_FACTOR_INDEX:
      // wifi_eutops.set_wifi_tx_factor(atol(data[1]),rsp);
      break;

    case TX_INDEX: {
      ENG_LOG("ADL %s(), case TX_INDEX, module_index = %d", __func__,
              module_index);

      if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("ADL %s(), case TX_INDEX, call wifi_tx_set()", __func__);
        wifi_tx_set(atoi(data[1]), atoi(data[2]), atoi(data[3]), rsp);
      } else {
        ENG_LOG("ADL %s(), case TX_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case RX_INDEX: {
      ENG_LOG("ADL %s(), case RX_INDEX, module_index = %d", __func__,
              module_index);

      if (module_index == WIFI_MODULE_INDEX) {
        ENG_LOG("ADL %s(), case RX_INDEX, call wifi_rx_set()", __func__);
        wifi_rx_set(atoi(data[1]), rsp);
      } else {
        ENG_LOG("ADL %s(), case RX_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case TX_REQ_INDEX: {
      ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index = %d", __func__,
              module_index);

      if (WIFI_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case TX_REQ_INDEX, call wifi_tx_get()", __func__);
        wifi_tx_get(rsp);
      } else {
        ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case RX_REQ_INDEX: {
      ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index = %d", __func__,
              module_index);

      if (WIFI_MODULE_INDEX == module_index) {
        ENG_LOG("ADL %s(), case RX_REQ_INDEX, call wifi_rx_get()", __func__);
        wifi_rx_get(rsp);
      } else {
        ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index is ERROR", __func__);
      }
    } break;

    case WIFITX_FACTOR_REQ_INDEX:
      // wifi_eutops.wifi_tx_factor_req(rsp);

      break;
    case WIFIRATIO_REQ_INDEX:
      // wifi_eutops.wifi_ratio_req(rsp);

      break;
    case WIFIRX_PACKCOUNT_INDEX:
      wifi_rxpktcnt_get(rsp);
      break;
    case WIFICLRRXPACKCOUNT_INDEX:
      // wifi_eutops.wifi_clr_rxpackcount(rsp);
      break;

    //-----------------------------------------------------
    case ENG_WIFIRATE_INDEX:
      ENG_LOG("%s(), case:ENG_WIFIRATE_INDEX\n", __FUNCTION__);
      wifi_rate_set(data[1], rsp);
      break;
    case ENG_WIFIRATE_REQ_INDEX:
      ENG_LOG("%s(), case:ENG_WIFIRATE_REQ_INDEX\n", __FUNCTION__);
      wifi_rate_get(rsp);
      break;
    case ENG_WIFITXGAININDEX_INDEX:
      ENG_LOG("%s(), case:ENG_WIFITXGAININDEX_INDEX\n", __FUNCTION__);
      wifi_txgainindex_set(atoi(data[1]), rsp);
      break;
    case ENG_WIFITXGAININDEX_REQ_INDEX:
      ENG_LOG("%s(), case:ENG_WIFITXGAININDEX_REQ_INDEX\n", __FUNCTION__);
      wifi_txgainindex_get(rsp);
      break;
    case ENG_WIFIRSSI_REQ_INDEX:
      ENG_LOG("%s(), case:ENG_WIFIRSSI_REQ_INDEX\n", __FUNCTION__);
      wifi_rssi_get(rsp);
      break;

    /* lna */
    case WIFILNA_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFILNA_REQ_INDEX", __func__);
      wifi_lna_get(rsp);
    } break;

    case WIFILNA_INDEX: {
      ENG_LOG("%s(), case:WIFILNA_INDEX", __func__);
      wifi_lna_set(atoi(data[1]), rsp);
    } break;

    /* Band */
    case WIFIBAND_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIBAND_REQ_INDEX", __func__);
      wifi_band_get(rsp);
    } break;

    case WIFIBAND_INDEX: {
      ENG_LOG("%s(), case:WIFIBAND_INDEX", __func__);
      wifi_band_set((wifi_band)atoi(data[1]), rsp);
    } break;

    /* Band Width */
    case WIFIBANDWIDTH_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIBANDWIDTH_REQ_INDEX", __func__);
      wifi_bandwidth_get(rsp);
    } break;

    case WIFIBANDWIDTH_INDEX: {
      ENG_LOG("%s(), case:WIFIBANDWIDTH_INDEX", __func__);
      wifi_bandwidth_set((wifi_bandwidth)atoi(data[1]), rsp);
    } break;

    /* Tx Power Level */
    case WIFITXPWRLV_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFITXPWRLV_REQ_INDEX", __func__);
      wifi_tx_power_level_get(rsp);
    } break;

    case WIFITXPWRLV_INDEX: {
      ENG_LOG("%s(), case:WIFITXPWRLV_INDEX", __func__);
      wifi_tx_power_level_set(atoi(data[1]), rsp);
    } break;

    /* Pkt Length */
    case WIFIPKTLEN_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIPKTLEN_REQ_INDEX", __func__);
      wifi_pkt_length_get(rsp);
    } break;

    case WIFIPKTLEN_INDEX: {
      ENG_LOG("%s(), case:WIFIPKTLEN_INDEX", __func__);
      wifi_pkt_length_set(atoi(data[1]), rsp);
    } break;

    /* TX Mode */
    case WIFITXMODE_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFITXMODE_REQ_INDEX", __func__);
      wifi_tx_mode_get(rsp);
    } break;

    case WIFITXMODE_INDEX: {
      ENG_LOG("%s(), case:WIFIPKTLEN_INDEX", __func__);
      wifi_tx_mode_set((wifi_tx_mode)atoi(data[1]), rsp);
    } break;

    /* Preamble */
    case WIFIPREAMBLE_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIPREAMBLE_REQ_INDEX", __func__);
      wifi_preamble_get(rsp);
    } break;

    case WIFIPREAMBLE_INDEX: {
      ENG_LOG("%s(), case:WIFIPREAMBLE_INDEX", __func__);
      wifi_preamble_set((wifi_tx_mode)atoi(data[1]), rsp);
    } break;

    /* Payload */
    case WIFIPAYLOAD_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIPAYLOAD_REQ_INDEX", __func__);
      wifi_payload_get(rsp);
    } break;

    case WIFIPAYLOAD_INDEX: {
      ENG_LOG("%s(), case:WIFIPAYLOAD_INDEX", __func__);
      wifi_payload_set((wifi_payload)atoi(data[1]), rsp);
    } break;

    /* Payload */
    case WIFIGUARDINTERVAL_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIGUARDINTERVAL_REQ_INDEX", __func__);
      wifi_guardinterval_get(rsp);
    } break;

    case WIFIGUARDINTERVAL_INDEX: {
      ENG_LOG("%s(), case:WIFIGUARDINTERVAL_INDEX", __func__);
      wifi_guardinterval_set((wifi_guard_interval)atoi(data[1]), rsp);
    } break;

    /* MAC Filter */
    case WIFIMACFILTER_REQ_INDEX: {
      ENG_LOG("%s(), case:WIFIMACFILTER_REQ_INDEX", __func__);
      wifi_mac_filter_get(rsp);
    } break;

    case WIFIMACFILTER_INDEX: {
      ENG_LOG("%s(), case:WIFIMACFILTER_INDEX", __func__);
      wifi_mac_filter_set(atoi(data[1]), data[2], rsp);
    } break;

    //-----------------------------------------------------
    default:
      strcpy(rsp, "can not match the at command");
      return 0;
  }

  // @alvin:
  // Here: I think it will response to pc directly outside
  // this function and should not send to modem again.
  ALOGD(" eng_atdiag_rsp   %s", rsp);

  return 0;
}

/********************************************************************
*   name   get_sub_str
*   ---------------------------
*   description: delimeter sub string from command string user inputed by AT
*Command
*   ----------------------------
*   para        IN/OUT      type            note
*   buf         IN          char *          user inputed command string
*   revdata     OUT         char **         receive delimeted sub string
*   a           IN          char            get command name by this parameter
*   delim       IN          char *          delimter sub string by this
*parameter
*   count       IN          unsigned char   revdata can stored count.
*   substr_max_len  IN      unsigned char   sub string's max length
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int get_sub_str(const char *buf, char **revdata, char a, char *delim,
                unsigned char count, unsigned char substr_max_len) {
  int len, len1, len2;
  char *start = NULL;
  char *substr = NULL;
  char *end = buf;
  int str_len = strlen(buf);

  start = strchr(buf, a);
  substr = strstr(buf, delim);

  if (!substr) {
    /* if current1 not exist, return this function.*/
    return 0;
  }

  while (end && *end != '\0') {
    end++;
  }

  if ((NULL != start) && (NULL != end)) {
    char *tokenPtr = NULL;
    unsigned int index =
        1; /*must be inited by 1, because data[0] is command name */

    start++;
    substr++;
    len = substr - start - 1;

    /* get cmd name */
    memcpy(revdata[0], start, len);

    /* get sub str by delimeter */
    tokenPtr = strtok(substr, delim);
    while (NULL != tokenPtr && index < count) {
      strncpy(revdata[index++], tokenPtr, substr_max_len);

      /* next */
      tokenPtr = strtok(NULL, delim);
    }
  }

  return 0;
}

int get_cmd_index(char *buf) {
  int i = 0;
  int index = -1;
  char *start = NULL;
  char *cur = NULL;
  int str_len = 0;
  char name_str[64 + 1] = {0x00};

  start = strchr(buf, '=');
  if (NULL == start) {
    ENG_LOG("ADL leaving %s(), start is NULL, buf = %s", __func__, buf);
    return -1;
  }

  /* search ',' '?' '\r' */
  /* skip '=' */
  start++;

  if (NULL != (cur = strchr(buf, '?'))) {
    cur++; /* include '?' to name_str */
    str_len = cur - start;
    strncpy(name_str, (char *)start, str_len);
  } else if (NULL != (cur = strchr(buf, ',')) ||
             NULL != (cur = strchr(buf, '\r'))) {
    str_len = cur - start;
    strncpy(name_str, (char *)start, str_len);
  } else {
    ENG_LOG("ADL leaving %s(), cmd is error, buf = %s", __func__, buf);
    return -1;
  }

  for (i = 0; i < (int)NUM_ELEMS(eut_cmds); i++) {
    if (0 == strcmp(name_str, eut_cmds[i].name)) {
      index = eut_cmds[i].index;
      break;
    }
  }

  return index;
}
#endif

/********************************************************************
*   name   get_gps_sub_str
*   ---------------------------
*   description: parse gps AT command
********************************************************************/
int get_gps_sub_str(const char *buf, char **revdata, char a, char *delim,
                    unsigned char count, unsigned char substr_max_len) {
  int len, len1, len2;
  char *start = NULL;
  char *substr = NULL;
  char *end = buf;
  int str_len = strlen(buf);

  start = strchr(buf, a);
  substr = strstr(buf, delim);

  if (!substr) {
    /* if current1 not exist, return this function.*/
    return 0;
  }

  while (end && *end != '\0') {
    end++;
  }

  if ((NULL != start) && (NULL != end)) {
    char *tokenPtr = NULL;
    unsigned int index =
        1; /*must be inited by 1, because data[0] is command name */

    start++;
    substr++;
    len = substr - start - 1;

    /* get cmd name */
    memcpy(revdata[0], start, len);

    /* get sub str by delimeter */
    tokenPtr = strtok(substr, delim);
    while (NULL != tokenPtr && index < count) {
      strncpy(revdata[index++], tokenPtr, substr_max_len);

      /* next */
      tokenPtr = strtok(NULL, delim);
    }
  }

  return 0;
}

/********************************************************************
*   name   eng_gps_atdiag_euthdlr
*   ---------------------------
*   description: gps at command in engmode
********************************************************************/
int eng_gps_atdiag_euthdlr(char *buf, int len, char *rsp, int module_index) {
  char args0[32 + 1] = {0x00};
  char args1[32 + 1] = {0x00};
  char args2[32 + 1] = {0x00};
  char args3[32 + 1] = {0x00};

  char *data[4] = {args0, args1, args2, args3};
  // int cmd_index = -1;

  get_gps_sub_str(buf, data, '=', ",", 4, 32);
  // cmd_index = get_gps_cmd_index(buf);

  if (module_index == GPS_MODULE_INDEX) {
#ifndef CONFIG_MINIENGPC
    gps_eut_parse(buf, rsp);
#endif
    return 0;
  }

  ALOGD(" eng_atdiag_rsp   %s", rsp);

  return 0;
}

int eng_autotest_dummy(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_keypad(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_lcd_parallel(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_lcd_spi(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_camera_iic(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_camera_parallel(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_camera_spi(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_gpio(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_tf(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_sim(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_mic(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_speak(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_misc(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_fm(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_atv(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_bt(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_wifi(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_iic_dev(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_charge(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

//-- [[ 2013-01-22
int eng_autotest_reserve(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_autotest_sensor(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}
//-- ]]

int eng_autotest_gps(char *req, char *rsp) {
  int ret = 0;
  ENG_LOG("%s Enter", __FUNCTION__);
  return ret;
}

int eng_atdiag_hdlr(unsigned char *buf, int len, char *rsp) {
  int i, rlen = 0;
  int type = CMD_COMMON;
  MSG_HEAD_T head, *head_ptr = NULL;

  type = eng_atdiag_parse(buf, len);

  ENG_LOG("%s: type=%d", __FUNCTION__, type);
  switch (type) {
    case CMD_USER_VER:
      rlen = eng_diag_getver(buf, len, rsp);
      break;
  }

#if 0  // @alvin: FIX ME
    if(type != CMD_COMMON) {
        memset(eng_diag_buf, 0, sizeof(eng_diag_buf));
        memset(eng_atdiag_buf, 0, sizeof(eng_atdiag_buf));
        memcpy((char*)&head,buf,sizeof(MSG_HEAD_T));
        head.len = sizeof(MSG_HEAD_T)+rlen;
        ENG_LOG("%s: head.len=%d\n",__FUNCTION__, head.len);
        memcpy(eng_diag_buf,&head,sizeof(MSG_HEAD_T));
        memcpy(eng_diag_buf+sizeof(MSG_HEAD_T),rsp,rlen);
        rlen = eng_hex2ascii(eng_diag_buf, eng_atdiag_buf, head.len);
        rlen = eng_atdiag_rsp(eng_get_csclient_fd(), eng_atdiag_buf, rlen);
    }
#endif
  return rlen;
}

int eng_diag(char *buf, int len) {
  int ret = 0;
  int type, num;
  int retry_time = 0;
  int ret_val = 0;
  int fd = -1;
  char rsp[512];
  MSG_HEAD_T head, *head_ptr = NULL;

  memset(rsp, 0, sizeof(rsp));
  fd = get_ser_diag_fd();
  g_setuart_ok=0;
  type = eng_diag_parse(buf, len, &num);

  ENG_LOG("%s:write type=%d,num=%d\n", __FUNCTION__, type, num);

  if (type != CMD_COMMON) {
    ret_val = eng_diag_user_handle(type, buf, len - num);
    ENG_LOG("%s:ret_val=%d\n", __FUNCTION__, ret_val);

    if (ret_val && !g_setuart_ok) {
      eng_diag_buf[0] = 0x7e;

      sprintf(rsp, "%s", "\r\nOK\r\n");
      head.len = sizeof(MSG_HEAD_T) + strlen("\r\nOK\r\n");
      ENG_LOG("%s: head.len=%d\n", __FUNCTION__, head.len);
      head.seq_num = 0;
      head.type = 0x9c;
      head.subtype = 0x00;
      memcpy(eng_diag_buf + 1, &head, sizeof(MSG_HEAD_T));
      memcpy(eng_diag_buf + sizeof(MSG_HEAD_T) + 1, rsp, strlen(rsp));
      eng_diag_buf[head.len + 1] = 0x7e;
      eng_diag_len = head.len + 2;

      retry_time = 0;  // reset retry time counter
    write_again:
      ret = eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
      if (ret <= 0) {
        ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
      }
    }

    if (s_cp_ap_proc) {
      ENG_LOG("%s: This command need to send to CP\n", __FUNCTION__);
      s_cp_ap_proc = 0;
      ret = 0;
    } else {
      ret = 1;
    }
  }

  ENG_LOG("%s: ret=%d\n", __FUNCTION__, ret);

  return ret;
}

// send at should from ril interface @allen
// enable this function temporarily @alvin
int eng_diag_getver(unsigned char *buf, int len, char *rsp) {
  int wlen, fd;
  int maxlen = 248;
  int rlen = 0;
  int cmdlen;
  int sipc_fd;
  MSG_HEAD_T head, *head_ptr = NULL;
  char androidver[256];
  char sprdver[256];
  char modemver[512];
  char *ptr, *atrsp;

  // get android version
  memset(androidver, 0, sizeof(androidver));
  property_get(ENG_ANDROID_VER, androidver, "");
  ENG_LOG("%s: Android %s", __FUNCTION__, androidver);

  // get sprd version
  memset(sprdver, 0, sizeof(sprdver));
  property_get(ENG_SPRD_VERS, sprdver, "");
  ENG_LOG("%s: %s", __FUNCTION__, sprdver);

#if 0  // For getting AP version
#ifndef ENG_AT_CHANNEL
    // open at sipc channel
    sipc_fd = eng_open_dev(at_sipc_devname[g_run_mode], O_WRONLY);
    if(sipc_fd < 0) {
        ENG_LOG("%s: can't open sipc: %s\n", __FUNCTION__, at_sipc_devname[g_run_mode]);
        return 0;
    }

    //get modem version
    do {
        memset(modemver, 0, sizeof(modemver));
        strcpy(modemver, "at+cgmr\r");
        cmdlen=strlen(modemver);
        wlen = write(sipc_fd, modemver, cmdlen);
        memset(modemver, 0, sizeof(modemver));
        rlen = read(sipc_fd, modemver, sizeof(modemver));
        ENG_LOG("%s: %s",__FUNCTION__, modemver);
    }while(strstr(modemver, "desterr")!=NULL);

    close(sipc_fd);// close the sipc channel
#else
    // User at channel, modemId = 0 & simId = 0 is temporary.
    atrsp = sendCmd(0, "at+cgmr\r");
    memset(modemver, 0, sizeof(modemver));
    strcpy(modemver, atrsp);
#endif

    ptr = strstr(modemver, "HW");
    *ptr = 0;
#endif
  // ok
  sprintf(rsp, "%s", sprdver);
  rlen = strlen(rsp);

  if (rlen > maxlen) {
    rlen = maxlen;
    rsp[rlen] = 0;
  }
  ENG_LOG("%s:rlen=%d; %s", __FUNCTION__, rlen, rsp);
  return rlen;
}

int eng_diag_bootreset(unsigned char *buf, int len, char *rsp) {
  int rlen = 0;

  sprintf(rsp, "%s", "OK");
  rlen = strlen(rsp);

  ENG_LOG("%s:rlen=%d; %s", __FUNCTION__, rlen, rsp);
  return rlen;
}

int eng_diag_apcmd_hdlr(unsigned char *buf, int len, char *rsp) {
  int rlen = 0, i = 0;
  char *ptr = NULL;

  ptr = buf + 1 + sizeof(MSG_HEAD_T);

  while (*(ptr + i) != 0x7e) {
    i++;
  }

  *(ptr + i - 1) = '\0';

  ENG_LOG("%s: s_cmd_index: %d", __FUNCTION__, s_cmd_index);
  eng_linuxcmd_hdlr(s_cmd_index, ptr, rsp);

  rlen = strlen(rsp);

  ENG_LOG("%s:rlen:%d; %s", __FUNCTION__, rlen, rsp);

  return rlen;
}

#if 0
int eng_diag_getband(char *buf,int len, char *rsp)
{
    char cmdbuf[64];
    int wlen, rlen, cmdlen;

    memset(cmdbuf, 0, sizeof(cmdbuf));
    sprintf(cmdbuf, "%d,%d",ENG_AT_CURRENT_BAND,0);
    cmdlen=strlen(cmdbuf);
    wlen = eng_at_write(eng_get_csclient_fd(), cmdbuf, cmdlen);

    memset(cmdbuf, 0, sizeof(cmdbuf));
    rlen = eng_at_read(eng_get_csclient_fd(),cmdbuf,sizeof(cmdbuf));
    ENG_LOG("%s: rsp=%s\n",__FUNCTION__, cmdbuf);
    sprintf(rsp, "%s", cmdbuf);

    return rlen;
}
#endif

static void eng_diag_char2hex(unsigned char *hexdata, char *chardata) {
  int i, index = 0;
  char *ptr;
  char tmp[4];

  while ((ptr = strchr(chardata, ':')) != NULL) {
    snprintf(tmp, 3, "%s", chardata);
    hexdata[index++] = strtol(tmp, NULL, 16);
    chardata = ptr + 1;
  }

  hexdata[index++] = strtol(chardata, NULL, 16);

  for (i = 0; i < index; i++) {
    ENG_LOG("%s: [%d]=0x%x\n", __FUNCTION__, i, hexdata[i]);
  }
}

int eng_diag_decode7d7e(char *buf, int len) {
  int i, j, m = 0;
  char tmp;
  for (i = 0; i < len; i++) {
    if ((buf[i] == 0x7d) || (buf[i] == 0x7e)) {
      tmp = buf[i + 1] ^ 0x20;
      ENG_LOG("%s: tmp=%x, buf[%d]=%x", __FUNCTION__, tmp, i + 1, buf[i + 1]);
      buf[i] = tmp;
      j = i + 1;
      memcpy(&buf[j], &buf[j + 1], len - j);
      len--;
      m++;
      ENG_LOG("%s AFTER:", __FUNCTION__);
      /*
         for(j=0; j<len; j++) {
         ENG_LOG("%x,",buf[j]);
         }*/
    }
  }
  ENG_LOG("%s: m=%d", __FUNCTION__, m);
  return m;
}

int eng_diag_encode7d7e(char *buf, int len, int *extra_len) {
  int i, j;
  char tmp;

  ENG_LOG("%s: len=%d", __FUNCTION__, len);

  for (i = 0; i < len; i++) {
    if ((buf[i] == 0x7d) || (buf[i] == 0x7e)) {
      tmp = buf[i] ^ 0x20;
      ENG_LOG("%s: tmp=%x, buf[%d]=%x", __FUNCTION__, tmp, i, buf[i]);
      buf[i] = 0x7d;
      for (j = len; j > i + 1; j--) {
        buf[j] = buf[j - 1];
      }
      buf[i + 1] = tmp;
      len++;
      (*extra_len)++;

      ENG_LOG("%s: AFTER:[%d]", __FUNCTION__, len);
      for (j = 0; j < len; j++) {
        ENG_LOG("%x,", buf[j]);
      }
    }
  }

  return len;
}

static int eng_diag_btwifiimei(char *buf, int len, char *rsp, int rsplen) {
  int rlen = 0, i;
  int ret = -1;
  int cmd_mask = 0;
  unsigned short crc = 0;
  unsigned char crc1, crc2, crc3, crc4;
  char tmp;
  char btaddr[32] = {0};
  char wifiaddr[32] = {0};
  char tmprsp[512] = {0};
  char *pBtAddr = NULL, *pWifiAddr = NULL;
  int headlen = sizeof(MSG_HEAD_T);
  REF_NVWriteDirect_T *direct;
  MSG_HEAD_T *head_ptr = NULL;
  head_ptr = (MSG_HEAD_T *)(buf + 1);
  direct = (REF_NVWriteDirect_T *)(buf + DIAG_HEADER_LENGTH + 1);
  cmd_mask = head_ptr->subtype & 0x7f;
  ENG_LOG("Call %s, subtype=%x\n", __FUNCTION__, head_ptr->subtype);

  if ((head_ptr->subtype & DIAG_CMD_READ) == 0) {  // write command
    crc1 = *(buf + DIAG_HEADER_LENGTH + sizeof(REF_NVWriteDirect_T) + 1);
    crc2 = *(buf + DIAG_HEADER_LENGTH + sizeof(REF_NVWriteDirect_T) + 2);
    crc =
        crc16(crc, (const unsigned char *)direct, sizeof(REF_NVWriteDirect_T));
    crc3 = crc & 0xff;
    crc4 = (crc >> 8) & 0xff;
    ENG_LOG("%s: crc [%x,%x], [%x,%x]\n", __func__, crc3, crc4, crc1, crc2);

    if ((crc1 == crc3) && (crc2 == crc4)) {
      if ((cmd_mask & DIAG_CMD_BTBIT) || (cmd_mask & DIAG_CMD_WIFIBIT)) {
        // write bt address
        if ((head_ptr->subtype & DIAG_CMD_BTBIT) > 0) {
          sprintf(btaddr, "%02x:%02x:%02x:%02x:%02x:%02x", direct->btaddr[5],
                  direct->btaddr[4], direct->btaddr[3], direct->btaddr[2],
                  direct->btaddr[1], direct->btaddr[0]);
          pBtAddr = btaddr;
          ENG_LOG("%s: BTADDR:%s\n", __func__, btaddr);
        }

        // write wifi address
        if ((head_ptr->subtype & DIAG_CMD_WIFIBIT) > 0) {
          sprintf(wifiaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
                  direct->wifiaddr[0], direct->wifiaddr[1], direct->wifiaddr[2],
                  direct->wifiaddr[3], direct->wifiaddr[4],
                  direct->wifiaddr[5]);
          pWifiAddr = wifiaddr;
          ENG_LOG("%s: WIFIADDR:%s\n", __func__, wifiaddr);
        }

        ret = eng_btwifimac_write(pBtAddr, pWifiAddr);
      }

      if (g_ap_cali_flag &&
          ((cmd_mask & DIAG_CMD_IMEI1BIT) || (cmd_mask & DIAG_CMD_IMEI2BIT) ||
           (cmd_mask & DIAG_CMD_IMEI3BIT) || (cmd_mask & DIAG_CMD_IMEI4BIT))) {
        int imei[IMEI_NUM] = {DIAG_CMD_IMEI1BIT, DIAG_CMD_IMEI2BIT,
                              DIAG_CMD_IMEI3BIT, DIAG_CMD_IMEI4BIT};
        for (i = 0; i < IMEI_NUM; i++) {
          if (imei[i] & cmd_mask) {
            ret = eng_diag_write_imei(direct, i + 1);
            if (ret <= 0) break;
          }
        }
      }
    }

    if (!s_cp_ap_proc) {
      if (ret <= 0) {  // fail
        head_ptr->subtype = 0x00;
      } else {
        head_ptr->subtype = 0x01;
      }

      memcpy(tmprsp, (unsigned char *)head_ptr, headlen);
      head_ptr = (MSG_HEAD_T *)tmprsp;
      head_ptr->len = headlen + 2;
      rlen = translate_packet(rsp, tmprsp, head_ptr->len);
    }
  } else {  // read command
    direct = (REF_NVWriteDirect_T *)(tmprsp + headlen);

    if ((cmd_mask & DIAG_CMD_BTBIT) || (cmd_mask & DIAG_CMD_WIFIBIT)) {
      // read btaddr
      if ((head_ptr->subtype & DIAG_CMD_BTBIT) > 0) {
        ret = eng_btwifimac_read(btaddr, ENG_BT_MAC);
        ENG_LOG("%s: after BTADDR:%s\n", __func__, btaddr);
        pBtAddr = (char *)(direct->btaddr);
        if (!ret) {
          eng_diag_char2hex((unsigned char *)pBtAddr, btaddr);
          tmp = pBtAddr[0];
          pBtAddr[0] = pBtAddr[5];
          pBtAddr[5] = tmp;  // converge BT address
          tmp = pBtAddr[1];
          pBtAddr[1] = pBtAddr[4];
          pBtAddr[4] = tmp;
          tmp = pBtAddr[2];
          pBtAddr[2] = pBtAddr[3];
          pBtAddr[3] = tmp;
        }
      }
      // read wifiaddr
      if ((head_ptr->subtype & DIAG_CMD_WIFIBIT) > 0) {
        ret = eng_btwifimac_read(wifiaddr, ENG_WIFI_MAC);
        ENG_LOG("%s: after WIFIADDR:%s\n", __func__, wifiaddr);
        pWifiAddr = (char *)(direct->wifiaddr);
        if (!ret) eng_diag_char2hex((unsigned char *)pWifiAddr, wifiaddr);
      }
    }

    if (g_ap_cali_flag &&
        ((cmd_mask & DIAG_CMD_IMEI1BIT) || (cmd_mask & DIAG_CMD_IMEI2BIT) ||
         (cmd_mask & DIAG_CMD_IMEI3BIT) || (cmd_mask & DIAG_CMD_IMEI4BIT))) {
      int imei[IMEI_NUM] = {DIAG_CMD_IMEI1BIT, DIAG_CMD_IMEI2BIT,
                            DIAG_CMD_IMEI3BIT, DIAG_CMD_IMEI4BIT};
      for (i = 0; i < IMEI_NUM; i++) {
        if (imei[i] & cmd_mask) {
          ret = eng_diag_read_imei(direct, i + 1);
          if (ret <= 0) break;
        }
      }
    }
    // response
    head_ptr->subtype = 0x01;
    memcpy(tmprsp, (unsigned char *)head_ptr, headlen);
    head_ptr = (MSG_HEAD_T *)tmprsp;

    rlen = sizeof(REF_NVWriteDirect_T);
    crc =
        crc16(crc, (const unsigned char *)direct, sizeof(REF_NVWriteDirect_T));

    *(tmprsp + headlen + rlen) = crc & 0xff;
    *(tmprsp + headlen + rlen + 1) = (crc >> 8) & 0xff;
    ENG_LOG("%s: read crc = %d, [%x,%x]\n", __func__, crc,
            *(tmprsp + headlen + rlen), *(tmprsp + headlen + rlen + 1));
    rlen += 2;
    head_ptr->len = headlen + rlen;
    rlen = translate_packet(rsp, tmprsp, headlen + rlen);
  }

  // clear BT/WIFI bit in this diag framer
  if (s_cp_ap_proc) {
    head_ptr->subtype &= ~(DIAG_CMD_BTBIT | DIAG_CMD_WIFIBIT);
  }

  ENG_LOG("%s: rlen=%d\n", __func__, rlen);
  return rlen;
}

int eng_diag_factorymode(char *buf, int len, char *rsp) {
  char *pdata = NULL;
  MSG_HEAD_T *head_ptr = NULL;
  char value[PROPERTY_VALUE_MAX];

  head_ptr = (MSG_HEAD_T *)(buf + 1);
  pdata = buf + DIAG_HEADER_LENGTH + 1;  // data content;

  ENG_LOG("%s: operation=%x; end=%x\n", __func__, *pdata, *(pdata + 1));

  switch (*pdata) {
    case 0x00:
      ENG_LOG("%s: should close the vser,gser when next reboot\n",
              __FUNCTION__);
    case 0x01:
      eng_sql_string2int_set(ENG_TESTMODE, *pdata);
      eng_check_factorymode(0);
      head_ptr->subtype = 0x00;
      break;
    default:
      head_ptr->subtype = 0x01;
      break;
  }

  return 0;
}

int ascii2bin(unsigned char *dst, unsigned char *src, unsigned long size) {
  unsigned char h, l;
  unsigned long count = 0;

  if ((NULL == dst) || (NULL == src)) return -1;

  while (count < size) {
    if ((*src >= '0') && (*src <= '9')) {
      h = *src - '0';
    } else {
      h = *src - 'A' + 10;
    }

    src++;

    if ((*src >= '0') && (*src <= '9')) {
      l = *src - '0';
    } else {
      l = *src - 'A' + 10;
    }

    src++;
    count += 2;

    *dst = (unsigned char)(h << 4 | l);
    dst++;
  }

  return 0;
}

int bin2ascii(unsigned char *dst, unsigned char *src, unsigned long size) {
  unsigned char semi_octet;
  unsigned long count = 0;

  if ((NULL == dst) || (NULL == src)) return -1;

  while (count < size) {
    semi_octet = ((*src) & 0xf0) >> 4;
    if (semi_octet <= 9) {
      *dst = semi_octet + '0';
    } else {
      *dst = semi_octet + 'A' - 10;
    }

    dst++;

    semi_octet = ((*src) & 0x0f);
    if (semi_octet <= 9) {
      *dst = semi_octet + '0';
    } else {
      *dst = semi_octet + 'A' - 10;
    }

    dst++;

    src++;
    count++;
  }

  return 0;
}

int at_tok_equel_start(char **p_cur) {
  if (*p_cur == NULL) {
    return -1;
  }

  // skip prefix
  // consume "^[^:]:"

  *p_cur = strchr(*p_cur, '=');

  if (*p_cur == NULL) {
    return -1;
  }

  (*p_cur)++;

  return 0;
}

int is_ap_at_cmd_need_to_handle(char *buf, int len) {
  unsigned int i, ret = 0;
  MSG_HEAD_T *head_ptr = NULL;
  char *ptr = NULL;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  head_ptr = (MSG_HEAD_T *)(buf + 1);
  ptr = buf + 1 + sizeof(MSG_HEAD_T);

  if (-1 != (s_cmd_index = eng_at2linux(ptr))) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief judge if the AT cmd need to handle
 *
 * @param buf diag buf
 * @param len diag len
 *
 * @return 1 if the cmd need to handle,otherwise 0
 */
#if (!defined USE_AUDIO_WHALE_HAL) && (!defined CONFIG_MINIENGPC)
int is_audio_at_cmd_need_to_handle(char *buf, int len) {
  unsigned int i, ret = 0;
  MSG_HEAD_T *head_ptr = NULL;
  char *ptr = NULL;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  head_ptr = (MSG_HEAD_T *)(buf + 1);
  ptr = buf + 1 + sizeof(MSG_HEAD_T);

  if (g_is_data) {
    // if not AT cmd
    ret = strncmp(ptr, "AT", strlen("AT"));
    if ((ret != 0 && isdigit(*ptr)) || (ret != 0 && isupper(*ptr))) {
      return 1;
    }
  }

  // AT+SADM4AP
  ret = strncmp(ptr, at_sadm, strlen(at_sadm));
  if (0 == ret) {
    at_tok_equel_start(&ptr);
    at_tok_nextint(&ptr, &cmd_type);
    ENG_LOG("%s,SADM4AP :value = 0x%02x", __FUNCTION__, cmd_type);
    for (i = 0; i < sizeof(at_sadm_cmd_to_handle) / sizeof(int); i += 1) {
      if (-1 == at_sadm_cmd_to_handle[i]) {
        ENG_LOG("end of at_sadm_cmd_to_handle");
        return 0;
      }
      if (at_sadm_cmd_to_handle[i] == cmd_type) {
        ENG_LOG("at_sadm_cmd_to_handle=%d", at_sadm_cmd_to_handle[i]);
        if (GET_ARM_VOLUME_MODE_COUNT != cmd_type) {
          ENG_LOG("NOT CMD TO GET COUNT");
          g_index = atoi(ptr);
          // g_index -= '0';
          ENG_LOG("index = %d", g_index);
          if (g_index >= adev_get_audiomodenum4eng()) {
            return 0;
          }
        }
        return 1;
      }
    }
  }

  // AT+SPENHA
  ret = strncmp(ptr, at_spenha, strlen(at_spenha));
  if (0 == ret) {
    at_tok_equel_start(&ptr);
    at_tok_nextint(&ptr, &eq_or_tun_type);
    at_tok_nextint(&ptr, &cmd_type);
    ENG_LOG("%s,SPENHA :value = 0x%02x", __FUNCTION__, cmd_type);
    for (i = 0; i < sizeof(at_spenha_cmd_to_handle) / sizeof(int); i += 1) {
      if (-1 == at_spenha_cmd_to_handle[i]) {
        ENG_LOG("end of at_spenha_cmd_to_handle");
        return 0;
      }

      if (at_spenha_cmd_to_handle[i] == cmd_type) {
        ENG_LOG("at_spenha_cmd_to_handle=%d", at_spenha_cmd_to_handle[i]);
        if (GET_AUDIO_ENHA_MODE_COUNT != cmd_type) {
          at_tok_nextint(&ptr, &g_index);
          if ((g_index > adev_get_audiomodenum4eng()) || (g_index <= 0)) {
            return 0;
          }
          g_index--;
          ENG_LOG("BINARY index = %x", g_index);
          if (SET_AUDIO_ENHA_DATA_TO_MEMORY == cmd_type) {
            eq_mode_sel_type = *ptr;
            eq_mode_sel_type -= '0';
            ENG_LOG("BINARY eq_mode_sel_type = %x", eq_mode_sel_type);
          }
        }

        return 1;
      }
    }
  }
  // AT+CALIBR
  ret = strncmp(ptr, at_calibr, strlen(at_calibr));
  if (0 == ret) {
    at_tok_equel_start(&ptr);
    at_tok_nextint(&ptr, &cmd_type);
    ENG_LOG("%s,CALIBR :value = 0x%02x", __FUNCTION__, cmd_type);
    return 1;
  }

  // AT+PEINFO
  ret = strncmp(ptr, at_phoneinfo, strlen(at_phoneinfo));
  if (0 == ret) {
    ENG_LOG("%s,PEINFO", __FUNCTION__);
    return 1;
  }
  // AT+PELOOP
  ENG_LOG("%s,ptr:%s",__FUNCTION__,ptr);
  ret = strncmp(ptr,at_phoneloop,strlen(at_phoneloop));
  if (0 == ret) {
      at_tok_equel_start(&ptr);
      at_tok_nextint(&ptr,&loop_enable);
      at_tok_nextint(&ptr,&loop_route);
      ENG_LOG("%s,PELOOP : enable %d, route:%d",__FUNCTION__,loop_enable,loop_route);
      return 1;
  }
  ENG_LOG("%s,cmd don't need to handle", __FUNCTION__);

  return 0;
}
#endif
int eng_diag_adc(char *buf, int *Irsp) {
  MSG_HEAD_T *head_ptr = NULL;
  unsigned char buf1[8];
  int result;
  if (Irsp != NULL) {
    sprintf(Irsp, "\r\nERROR\r\n");
  } else {
    ENG_LOG("%s,in eng_diag_adc,Irsp is null", __FUNCTION__);
    return 0;
  }

  head_ptr = (MSG_HEAD_T *)(buf + 1);
  memset(buf1, 0, 8);
  sprintf(buf1, "%d", head_ptr->subtype);
  adc_get_result(buf1);
  result = atoi(buf1);
  Irsp[0] = result;
  return strlen(Irsp);
}

// for mobile test tool
void At_cmd_back_sig(void) {
  int fd = -1;
  fd = get_ser_diag_fd();
  eng_diag_buf[0] = 0x7e;

  eng_diag_buf[1] = 0x00;
  eng_diag_buf[2] = 0x00;
  eng_diag_buf[3] = 0x00;
  eng_diag_buf[4] = 0x00;
  eng_diag_buf[5] = 0x08;
  eng_diag_buf[6] = 0x00;
  eng_diag_buf[7] = 0xD5;
  eng_diag_buf[8] = 0x00;

  eng_diag_buf[9] = 0x7e;
  eng_diag_write2pc(eng_diag_buf, eng_diag_len, fd);
  memset(eng_diag_buf, 0, 10);
}
#if (!defined USE_AUDIO_WHALE_HAL) && (!defined CONFIG_MINIENGPC)
static AUDIO_TOTAL_T *eng_regetpara(void) {
  int srcfd;
  char *filename = NULL;
  // eng_getparafromnvflash();
  ALOGW("wangzuo eng_regetpara 1");

  AUDIO_TOTAL_T *aud_params_ptr;
  int len = sizeof(AUDIO_TOTAL_T) * adev_get_audiomodenum4eng();

  aud_params_ptr = calloc(1, len);
  if (!aud_params_ptr) return 0;
  memset(aud_params_ptr, 0, len);
  srcfd = open((char *)(ENG_AUDIO_PARA_DEBUG), O_RDONLY);
  filename = (srcfd < 0) ? (ENG_AUDIO_PARA) : (ENG_AUDIO_PARA_DEBUG);
  if (srcfd >= 0) {
    close(srcfd);
  }

  ALOGW("eng_regetpara %s", filename);  ////done,into
  stringfile2nvstruct(filename, aud_params_ptr, len);

  return aud_params_ptr;
}

static void eng_setpara(AUDIO_TOTAL_T *ptr) {  // to do
  int len = sizeof(AUDIO_TOTAL_T) * adev_get_audiomodenum4eng();

  ALOGW("wangzuo eng_setpara 2");
  nvstruct2stringfile(ENG_AUDIO_PARA_DEBUG, ptr, len);
}
static int eng_notify_mediaserver_updatapara(int ram_ops, int index,
                                             AUDIO_TOTAL_T *aud_params_ptr) {
  int result = 0;
  int fifo_id = -1;
  int receive_fifo_id = -1;
  int ret;
  int length = 0;
  ALOGE("eng_notify_mediaserver_updatapara E,%d:%d!\n", ram_ops, index);
  fifo_id = open(AUDFIFO, O_WRONLY);
  if (fifo_id != -1) {
    int buff = 1;
    ALOGE("eng_notify_mediaserver_updatapara notify OPS!\n");
    result = write(fifo_id, &ram_ops, sizeof(int));
    if (ram_ops & ENG_RAM_OPS) {
      result = write(fifo_id, &index, sizeof(int));
      result = write(fifo_id, aud_params_ptr, sizeof(AUDIO_TOTAL_T));
      ALOGE("eng_notify_mediaserver_updatapara,index:%d,size:%d!\n", index,
            sizeof(AUDIO_TOTAL_T));
    }
    if (ram_ops & ENG_PHONEINFO_OPS) {
      receive_fifo_id = open(AUDFIFO_2, O_RDONLY);
      if (receive_fifo_id != -1) {
        result = read(receive_fifo_id, &length, sizeof(int));
        if (result < 0) {
          goto error;
        }
        sprintf((char *)aud_params_ptr, "%d", length);
        result =
            read(receive_fifo_id, (void *)aud_params_ptr + sizeof(int), length);
        if (result < 0) {
          goto error;
        }
        result += sizeof(int);
        close(receive_fifo_id);
        receive_fifo_id = -1;
        ALOGE("eng_notify_mediaserver_updatapara,result:%d,received:%d!\n",
              result, length);
      } else {
        ALOGE("%s open audio FIFO_2 error %s,fifo_id:%d\n", __FUNCTION__,
              strerror(errno), fifo_id);
      }
    }
    if (ram_ops & ENG_PHONELOOP_OPS) {
        ALOGE("eng_notify_mediaserver_updatapara,loop entry, enable:%d, loop_route:%d!!!\n",loop_enable, loop_route);
        result = write(fifo_id,&loop_enable,sizeof(int));
        result = write(fifo_id,&loop_route,sizeof(int));
    }
    close(fifo_id);
    fifo_id = -1;
  } else {
    ALOGE("%s open audio FIFO error %s,fifo_id:%d\n", __FUNCTION__,
          strerror(errno), fifo_id);
  }
  ALOGE("eng_notify_mediaserver_updatapara X,result:%d,length:%d!\n", result,
        length);
  return result;
error:
  ALOGE("eng_notify_mediaserver_updatapara X,ERROR,result:%d!\n", result);
  if (receive_fifo_id != -1) {
    close(receive_fifo_id);
    receive_fifo_id = -1;
  }

  if (fifo_id != -1) {
    close(fifo_id);
    fifo_id = -1;
  }
  return result;
}

void *eng_getpara(void) {
  int srcfd;
  char *filename = NULL;
  ALOGW("wangzuo eng_getpara 3");  ////done,into
  int audio_fd;
  static int read = 0;
  int len = sizeof(AUDIO_TOTAL_T) * adev_get_audiomodenum4eng();
  if (read) {
    ALOGW("eng_getpara read already.");  ////done,into
    return audio_total;
  } else {
    read = 1;
  }
  memset(audio_total, 0, len);
  srcfd = open((char *)(ENG_AUDIO_PARA_DEBUG), O_RDONLY);
  filename = (srcfd < 0) ? (ENG_AUDIO_PARA) : (ENG_AUDIO_PARA_DEBUG);
  if (srcfd >= 0) {
    close(srcfd);
  }
  ALOGW("wangzuo eng_getpara %s", filename);  ////done,into
  stringfile2nvstruct(filename, audio_total,
                      len);  // get data from audio_hw.txt.
  return audio_total;
}
int eng_diag_audio(char *buf, int len, char *rsp) {
  int fd;
  int wlen, rlen, ret = 0;
  MSG_HEAD_T *head_ptr = NULL;
  char *ptr = NULL;
  AUDIO_TOTAL_T *audio_ptr;
  int audio_fd = -1;
  int audiomode_count = 0;
  int ram_ofs = 0;
  if (rsp != NULL) {
    sprintf(rsp, "\r\nERROR\r\n");
  }

  if ((NULL == buf) || (NULL == rsp)) {
    goto out;
  }

  head_ptr = (MSG_HEAD_T *)(buf + 1);
  ENG_LOG("Call %s, subtype=%x\n", __FUNCTION__, head_ptr->subtype);
  ptr = buf + 1 + sizeof(MSG_HEAD_T);
  ENG_LOG("Call %s, ptr=%s\n", __FUNCTION__, ptr);

  // AT+CALIBR
  ret = strncmp(ptr, at_calibr, strlen(at_calibr));
  if (0 == ret) {
    if (SET_CALIBRATION_ENABLE == cmd_type) {
      enable_calibration();
      sprintf(rsp, "\r\nenable_calibration   \r\n");
    } else if (SET_CALIBRATION_DISABLE == cmd_type) {
      disable_calibration();
      sprintf(rsp, "\r\ndisable_calibration   \r\n");
    }
    At_cmd_back_sig();
    // return rsp != NULL ? strlen(rsp):0;
    return strlen(rsp);
  }
  // AT+PEINFO
  // buffer format like bellow:
  // [+PEINFO:][int][-----------phoneinfo---------------------]
  ret = strncmp(ptr, at_phoneinfo, strlen(at_phoneinfo));
  if (0 == ret) {
    int length = 0;
    char bin_tmp[2 * ENG_DIAG_SIZE];
    memcpy(rsp, "+PEINFO:", sizeof("+PEINFO:"));
    length = eng_notify_mediaserver_updatapara(ENG_PHONEINFO_OPS, 0, bin_tmp);
    if (length > 0) {
      bin2ascii(rsp + strlen("+PEINFO:"), bin_tmp, length);
      ENG_LOG("Call %s, rsp=%s\n", __FUNCTION__, rsp);
      ENG_LOG("Call %s, rsp=%s,%s\n", __FUNCTION__, (rsp + strlen("+PEINFO:")),
              (rsp + strlen("+PEINFO:")));
      ENG_LOG("Call %s, item1=%s,%s\n", __FUNCTION__,
              (rsp + strlen("+PEINFO:") + AUDIO_AT_HARDWARE_NAME_LENGTH),
              (rsp + strlen("+PEINFO:") + AUDIO_AT_HARDWARE_NAME_LENGTH +
               AUDIO_AT_ITEM_NAME_LENGTH));
      ENG_LOG("Call %s, item2=%s,%s\n", __FUNCTION__,
              (rsp + strlen("+PEINFO:") + AUDIO_AT_HARDWARE_NAME_LENGTH +
               AUDIO_AT_ITEM_NAME_LENGTH + AUDIO_AT_ITEM_VALUE_LENGTH),
              rsp + strlen("+PEINFO:") + AUDIO_AT_HARDWARE_NAME_LENGTH +
                  AUDIO_AT_ITEM_NAME_LENGTH + AUDIO_AT_ITEM_VALUE_LENGTH +
                  AUDIO_AT_ITEM_NAME_LENGTH);
      return strlen(rsp + strlen("+PEINFO:")) + strlen("+PEINFO:");
    } else {
      goto out;
    }
  }
  // audio_fd = open(ENG_AUDIO_PARA_DEBUG,O_RDWR);
  if (g_is_data) {
    ENG_LOG("HEY,DATA HAS COME!!!!");
    g_is_data = g_is_data;
    wlen = head_ptr->len - sizeof(MSG_HEAD_T) - 1;
    ENG_LOG("NOTICE:length is %x,%x,%x,%x", wlen, sizeof(AUDIO_TOTAL_T),
            sizeof(AUDIO_NV_ARM_MODE_INFO_T), sizeof(AUDIO_ENHA_EQ_STRUCT_T));

    audio_ptr = (AUDIO_TOTAL_T *)eng_regetpara();  // audio_ptr = (AUDIO_TOTAL_T
    // *)mmap(0,4*sizeof(AUDIO_TOTAL_T),PROT_READ|PROT_WRITE,MAP_SHARED,audio_fd,0);
    if ((AUDIO_TOTAL_T *)(-1) == audio_ptr ||
        (AUDIO_TOTAL_T *)(0) == audio_ptr) {
      ALOGE("mmap failed %s", strerror(errno));
      goto out;
    }

    if (g_is_data & AUDIO_NV_ARM_DATA_MEMORY) {
      ram_ofs |= ENG_RAM_OPS;
      g_is_data &= (~AUDIO_NV_ARM_DATA_MEMORY);
      g_indicator |= AUDIO_NV_ARM_INDI_FLAG;
      ascii2bin(
          (unsigned char *)(&audio_total[g_index]
                                 .audio_nv_arm_mode_info.tAudioNvArmModeStruct),
          (unsigned char *)ptr, wlen);
    }
    if (g_is_data & AUDIO_NV_ARM_DATA_FLASH) {
      ram_ofs |= ENG_FLASH_OPS;
      g_is_data &= (~AUDIO_NV_ARM_DATA_FLASH);
      g_indicator |= AUDIO_NV_ARM_INDI_FLAG;
      ascii2bin(
          (unsigned char *)(&audio_total[g_index]
                                 .audio_nv_arm_mode_info.tAudioNvArmModeStruct),
          (unsigned char *)ptr, wlen);
      audio_ptr[g_index].audio_nv_arm_mode_info.tAudioNvArmModeStruct =
          audio_total[g_index].audio_nv_arm_mode_info.tAudioNvArmModeStruct;
    }
    if (g_is_data & AUDIO_ENHA_DATA_MEMORY) {
      ram_ofs |= ENG_RAM_OPS;
      g_is_data &= (~AUDIO_ENHA_DATA_MEMORY);
      g_indicator |= AUDIO_ENHA_EQ_INDI_FLAG;
      ascii2bin((unsigned char *)(&audio_total[g_index].audio_enha_eq),
                (unsigned char *)ptr, wlen);
    }
    if (g_is_data & AUDIO_ENHA_DATA_FLASH) {
      ram_ofs |= ENG_FLASH_OPS;
      g_is_data &= (~AUDIO_ENHA_DATA_FLASH);
      g_indicator |= AUDIO_ENHA_EQ_INDI_FLAG;
      ascii2bin((unsigned char *)(&audio_total[g_index].audio_enha_eq),
                (unsigned char *)ptr, wlen);
      audio_ptr[g_index].audio_enha_eq = audio_total[g_index].audio_enha_eq;
    }
    if (g_is_data & AUDIO_ENHA_TUN_DATA_MEMORY) {
      ram_ofs |= ENG_RAM_OPS;
      g_is_data &= (~AUDIO_ENHA_TUN_DATA_MEMORY);
      ascii2bin((unsigned char *)tun_data, (unsigned char *)ptr, wlen);
    }

    ENG_LOG("g_indicator = 0x%x,%x\n", g_indicator, AUDIO_DATA_READY_INDI_FLAG);

    if (audio_ptr) {
      if (ram_ofs & ENG_FLASH_OPS) {
        eng_setpara(audio_ptr);
      }

      if (g_indicator) {
        ram_ofs |= ENG_PGA_OPS;
      }
      eng_notify_mediaserver_updatapara(ram_ofs, g_index,
                                        &audio_total[g_index]);
      free(audio_ptr);
    }

    if (g_indicator) {
      ENG_LOG("data is ready!g_indicator = 0x%x,g_index:%d\n", g_indicator,
              g_index);
      g_indicator = 0;
      parse_vb_effect_params((void *)audio_total, adev_get_audiomodenum4eng() *
                                                      sizeof(AUDIO_TOTAL_T));
    }

    sprintf(rsp, "\r\nOK\r\n");
    goto out;
  }

  //if ptr points to "AT+PELOOP" 3026
  ret = strncmp(ptr,at_phoneloop,strlen(at_phoneloop));
  if ( 0 == ret ) {
      int length = 0;
      char bin_tmp[2*ENG_DIAG_SIZE];
      memcpy(rsp,"+PELOOP:",sizeof("+PELOOP:"));
      length = eng_notify_mediaserver_updatapara(ENG_PHONELOOP_OPS,0, bin_tmp);
      if(length > 0)
          sprintf(rsp,"\r\nOK\r\n");
      else
          goto out;
  }

  // if ptr points to "AT+SADM4AP"
  ret = strncmp(ptr, at_sadm, strlen(at_sadm));
  if (0 == ret) {
    switch (cmd_type) {
      case GET_ARM_VOLUME_MODE_COUNT:
        ENG_LOG("%s,GET MODE COUNT:%d\n", __FUNCTION__,
                adev_get_audiomodenum4eng());
        sprintf(rsp, "+SADM4AP: %d", adev_get_audiomodenum4eng());
        ENG_LOG("%s,GET MODE COUNT:%s\n", __FUNCTION__, rsp);
        break;
      case GET_ARM_VOLUME_MODE_NAME:
        ENG_LOG("ARM VOLUME NAME is %s",
                audio_total[g_index].audio_nv_arm_mode_info.ucModeName);
        sprintf(rsp, "+SADM4AP: %d,\"%s\"", g_index,
                audio_total[g_index].audio_nv_arm_mode_info.ucModeName);
        ENG_LOG("%s,GET_ARM_VOLUME_MODE_NAME:%d ---- >%s \n", __FUNCTION__,
                g_index, rsp);
        break;
      case SET_ARM_VOLUME_DATA_TO_RAM:
        ENG_LOG("%s,set arm nv mode data to memory\n", __FUNCTION__);
        g_is_data |= AUDIO_NV_ARM_DATA_MEMORY;
        sprintf(rsp, "\r\n> ");
        break;
      case SET_ARM_VOLUME_DATA_TO_FLASH:
        ENG_LOG("%s,set arm nv mode data to flash\n", __FUNCTION__);
        g_is_data |= AUDIO_NV_ARM_DATA_FLASH;
        sprintf(rsp, "\r\n> ");
        break;
      case GET_ARM_VOLUME_DATA_FROM_FLASH:
        audio_ptr = (AUDIO_TOTAL_T *)eng_regetpara();  //(AUDIO_TOTAL_T
        //*)mmap(0,4*sizeof(AUDIO_TOTAL_T),PROT_READ|PROT_WRITE,MAP_SHARED,audio_fd,0);
        if (((AUDIO_TOTAL_T *)(-1) != audio_ptr) &&
            ((AUDIO_TOTAL_T *)(0) != audio_ptr)) {
          // audio_total[g_index].audio_nv_arm_mode_info.tAudioNvArmModeStruct=audio_ptr[g_index].audio_nv_arm_mode_info.tAudioNvArmModeStruct;
          // munmap((void *)audio_ptr,4*sizeof(AUDIO_TOTAL_T));
          audio_total[g_index].audio_nv_arm_mode_info.tAudioNvArmModeStruct =
              audio_ptr[g_index].audio_nv_arm_mode_info.tAudioNvArmModeStruct;
          free(audio_ptr);
        }
      // there is no break in this case,'cause it will share the code with the
      // following case
      case GET_ARM_VOLUME_DATA_FROM_RAM:
        ENG_LOG("%s,get arm volume data,audio_total:0x%0x,--->%d \n",
                __FUNCTION__, audio_total, g_index);
        sprintf(rsp, "+SADM4AP: 0,\"%s\",",
                audio_total[g_index].audio_nv_arm_mode_info.ucModeName);
        bin2ascii((unsigned char *)(rsp + strlen(rsp)),
                  (unsigned char *)(&audio_total[g_index]
                                         .audio_nv_arm_mode_info
                                         .tAudioNvArmModeStruct),
                  sizeof(AUDIO_NV_ARM_MODE_STRUCT_T));
        break;

      default:
        sprintf(rsp, "\r\nERROR\r\n");
        break;
    } /* -----  end switch  ----- */
  }

  // AT+SPENHA
  ret = strncmp(ptr, at_spenha, strlen(at_spenha));
  if (0 == ret) {
    ENG_LOG("receive AT+SPENHA cmd\n");
    if (1 == eq_or_tun_type) {
      switch (cmd_type) {
        case GET_AUDIO_ENHA_MODE_COUNT:
          sprintf(rsp, "+SPENHA: 1");
          break;
        case SET_AUDIO_ENHA_DATA_TO_MEMORY:
        case SET_AUDIO_ENHA_DATA_TO_FLASH:
          ENG_LOG("%s,set enha tun data to flash\n", __FUNCTION__);
          g_is_data |= AUDIO_ENHA_TUN_DATA_MEMORY;
          sprintf(rsp, "\r\n> ");
          break;
        case GET_AUDIO_ENHA_DATA_FROM_FLASH:
        case GET_AUDIO_ENHA_DATA_FROM_MEMORY:
          ENG_LOG("%s,get audio enha data\n", __FUNCTION__);
          sprintf(rsp, "+SPENHA: %d,", g_index);
          bin2ascii((unsigned char *)(rsp + strlen(rsp)),
                    (unsigned char *)tun_data, sizeof(tun_data));
          break;
        default:
          break;
      }
    } else {
      switch (cmd_type) {
        case GET_AUDIO_ENHA_MODE_COUNT:
          sprintf(rsp, "+SPENHA: %d", adev_get_audiomodenum4eng());
          break;

        case SET_AUDIO_ENHA_DATA_TO_MEMORY:
          ENG_LOG("%s,set enha data to memory\n", __FUNCTION__);
          g_is_data |= AUDIO_ENHA_DATA_MEMORY;
          sprintf(rsp, "\r\n> ");
          break;
        case SET_AUDIO_ENHA_DATA_TO_FLASH:
          ENG_LOG("%s,set enha data to flash\n", __FUNCTION__);
          g_is_data |= AUDIO_ENHA_DATA_FLASH;
          sprintf(rsp, "\r\n> ");
          break;
        case GET_AUDIO_ENHA_DATA_FROM_FLASH:
          audio_ptr = (AUDIO_TOTAL_T *)eng_regetpara();  // (AUDIO_TOTAL_T
          // *)mmap(0,4*sizeof(AUDIO_TOTAL_T),PROT_READ|PROT_WRITE,MAP_SHARED,audio_fd,0);
          if (NULL != audio_ptr) {
            audio_total[g_index].audio_enha_eq =
                audio_ptr[g_index].audio_enha_eq;
            free(audio_ptr);  // munmap((void
                              // *)audio_ptr,4*sizeof(AUDIO_TOTAL_T));
          }
        // there is no break in this case,'cause it will share the code with the
        // following case
        case GET_AUDIO_ENHA_DATA_FROM_MEMORY:
          ENG_LOG("%s,get audio enha data\n", __FUNCTION__);
          sprintf(rsp, "+SPENHA: %d,", g_index);
          bin2ascii((unsigned char *)(rsp + strlen(rsp)),
                    (unsigned char *)(&audio_total[g_index].audio_enha_eq),
                    sizeof(AUDIO_ENHA_EQ_STRUCT_T));
          break;
        default:
          break;
      } /* -----  end switch  ----- */
    }
  }

out:
  // if (audio_fd >=0)
  // close(audio_fd);

  return rsp != NULL ? strlen(rsp) : 0;
}
#endif
static int eng_diag_product_ctrl(char *buf, int len, char *rsp, int rsplen) {
  int offset = 0;
  int data_len = 0;
  int head_len = 0;
  int rsp_len = 0;
  unsigned char *nvdata = NULL;
  NVITEM_ERROR_E nverr = NVERR_NONE;
  MSG_HEAD_T *msg_head = (MSG_HEAD_T *)(buf + 1);

  head_len = sizeof(MSG_HEAD_T) + 2 * sizeof(unsigned short);
  offset = *(unsigned short *)((char *)msg_head + sizeof(MSG_HEAD_T));
  data_len = *(unsigned short *)((char *)msg_head + sizeof(MSG_HEAD_T) +
                                 sizeof(unsigned short));

  ENG_LOG("%s: offset: %d, data_len: %d\n", __FUNCTION__, offset, data_len);

  if (rsplen < (head_len + data_len + 2)) {  // 2:0x7e
    ENG_LOG("%s: Rsp buffer is not enough, need buf: %d\n", __FUNCTION__,
            head_len + data_len);
    return 0;
  }

  // 2: NVITEM_PRODUCT_CTRL_READ
  // 3: NVITEM_PRODUCT_CTRL_WRITE
  ENG_LOG("%s: msg_head->subtype: %d\n", __FUNCTION__, msg_head->subtype);
  switch (msg_head->subtype) {
    case 2: {
      nvdata = (unsigned char *)malloc(data_len + head_len);
      memcpy(nvdata, msg_head, head_len);

      nverr = eng_read_productnvdata(nvdata + head_len, data_len);
      if (NVERR_NONE != nverr) {
        ENG_LOG("%s: Read ERROR: %d\n", __FUNCTION__, nverr);
        data_len = 0;
      }

      ((MSG_HEAD_T *)nvdata)->subtype = nverr;
      ((MSG_HEAD_T *)nvdata)->len = head_len + data_len;

      rsp_len = translate_packet(rsp, nvdata, head_len + data_len);

      free(nvdata);
    } break;
    case 3: {
      nvdata = (unsigned char *)malloc(rsplen);

      nverr = eng_read_productnvdata(nvdata, data_len);
      if (NVERR_NONE != nverr) {
        ENG_LOG("%s: Read before writing ERROR: %d\n", __FUNCTION__, nverr);
      } else {
        memcpy(nvdata + offset, (char *)msg_head + head_len, data_len);
        nverr = eng_write_productnvdata(nvdata, data_len);
        if (NVERR_NONE != nverr) {
          ENG_LOG("%s:Write ERROR: %d\n", __FUNCTION__, nverr);
        }
      }

      free(nvdata);

      msg_head->subtype = nverr;
      msg_head->len = sizeof(MSG_HEAD_T);

      rsp_len =
          translate_packet(rsp, (unsigned char *)msg_head, sizeof(MSG_HEAD_T));
    } break;
    default:
      ENG_LOG("%s: ERROR Oper: %d !\n", __FUNCTION__, msg_head->subtype);
      return 0;
  }

  ENG_LOG("%s: rsp_len : %d\n", __FUNCTION__, rsp_len);

  return rsp_len;
}

static int eng_diag_direct_phschk(char *buf, int len, char *rsp, int rsplen) {
  int crc = 0;
  int data_len = 0;
  int recv_crc = 0;
  int rsp_len = 0;
  unsigned char result;
  unsigned char *nvdata;
  ERR_IMEI_E error;
  uint32_t magic;
  int sn_len = 0;

  NVITEM_ERROR_E nverr = NVERR_NONE;
  MSG_HEAD_T *msg_head = (MSG_HEAD_T *)(buf + 1);

  do {
    recv_crc =
        *(unsigned short *)&(buf[msg_head->len - sizeof(unsigned short) + 1]);
    crc = crc16(0, (unsigned char *)(msg_head + 1),
                msg_head->len - sizeof(MSG_HEAD_T) - sizeof(unsigned short));

    if (recv_crc != crc) {
      ENG_LOG("%s: CRC Error! recv_crc: %d, crc16: %d\n", __FUNCTION__,
              recv_crc, crc);
      msg_head->len = sizeof(MSG_HEAD_T) + 2 * sizeof(unsigned short);
      *(unsigned short *)(msg_head + 1) = IMEI_CRC_ERR;
      break;
    }

    ENG_LOG("%s: Current oper: %d\n", __FUNCTION__,
            (msg_head->subtype & RW_MASK));

    if ((msg_head->subtype & RW_MASK) == WRITE_MODE) {
      if (0 != (msg_head->subtype & RM_VALID_CMD_MASK)) {
        nvdata = (unsigned char *)(msg_head + 1);
        data_len = msg_head->len - sizeof(MSG_HEAD_T) - sizeof(unsigned short);

        ENG_LOG("%s: data_len: %d\n", __FUNCTION__, data_len);

        nverr = eng_write_productnvdata(nvdata, data_len);
        if (NVERR_NONE != nverr) {
          ENG_LOG("%s:Write ERROR: %d\n", __FUNCTION__, nverr);
          error = IMEI_SAVE_ERR;
          result = 0;
        } else {
          error = IMEI_ERR_NONE;
          result = 1;
        }
      } else {
        ENG_LOG("%s: Write error, subtype : %d\n", __FUNCTION__,
                msg_head->subtype);
        error = IMEI_CMD_ERR;
        result = 0;
      }

      if (result) {
        msg_head->len = sizeof(MSG_HEAD_T) + sizeof(unsigned short);
        msg_head->subtype = MSG_ACK;
        *((unsigned short *)((unsigned char *)(msg_head + 1))) = 0;
      } else {
        msg_head->subtype = MSG_NACK;
        *(unsigned short *)(msg_head + 1) = error;
        *((unsigned short *)((unsigned char *)(msg_head + 1) +
                             sizeof(unsigned short))) = 0;
        msg_head->len = sizeof(MSG_HEAD_T) + 2 * sizeof(unsigned short);
      }

      rsp_len = translate_packet(rsp, (unsigned char *)msg_head, msg_head->len);
    } else {  // Read Mode
      ENG_LOG("%s: Read Mode ! \n", __FUNCTION__);
      nvdata = (unsigned char *)malloc(rsplen + sizeof(MSG_HEAD_T) +
                                       sizeof(unsigned short));
      memcpy(nvdata, msg_head, sizeof(MSG_HEAD_T));

      nverr = eng_read_productnvdata(nvdata + sizeof(MSG_HEAD_T), rsplen);
      if (NVERR_NONE != nverr) {
        ENG_LOG("%s:Read ERROR: %d\n", __FUNCTION__, nverr);
        msg_head->len = sizeof(MSG_HEAD_T) + sizeof(unsigned short);
        *((unsigned short *)((unsigned char *)(msg_head + 1))) = 0;
        msg_head->subtype = MSG_NACK;
      } else {
        msg_head = (MSG_HEAD_T *)nvdata;
	magic = *((uint32_t*)(nvdata + sizeof(MSG_HEAD_T)));
	ENG_LOG("%s:phase check magic: %x\n", __FUNCTION__,magic);
	if(magic == SP09_SPPH_MAGIC_NUMBER){
		sn_len = sizeof(SP09_TEST_DATA_INFO_T);
	}else if(magic == SP15_SPPH_MAGIC_NUMBER){
		sn_len = sizeof(SP15_TEST_DATA_INFO_T);
	}
	msg_head->len = sizeof(MSG_HEAD_T) + sn_len +
                        sizeof(unsigned short);

        *((unsigned short *)((unsigned char *)(msg_head + 1) + sn_len)) =
            crc16(0, ((unsigned char *)(msg_head + 1)), sn_len);

        msg_head->subtype = MSG_ACK;
      }

      rsp_len = translate_packet(rsp, (unsigned char *)msg_head, msg_head->len);

      free(nvdata);
    }
  } while (0);

  ENG_LOG("%s: rsp_len : %d\n", __FUNCTION__, rsp_len);

  return rsp_len;
}

int eng_diag_mmicit_read(char *buf, int len, char *rsp, int rsplen) {
  int ret = 0, count;
  int datalen, namelen;
  char *rspdata = NULL;
  char *pCur = NULL;
  eng_str2int_table_sqlresult result;
  MSG_HEAD_T *pHead = (MSG_HEAD_T *)(buf + 1);

  ret = eng_sql_string2int_table_get(&result);
  if (ret) {
    ENG_LOG("%s: ENG read sqlite table failed\n", __FUNCTION__);
    memcpy(rsp, buf, len);  // return a empty diag framer
    return len;
  }

  rspdata = malloc(rsplen - 2);
  if (NULL == rspdata) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rspdata, (char *)pHead, sizeof(MSG_HEAD_T));
  pCur = rspdata + sizeof(MSG_HEAD_T);
  datalen = sizeof(MSG_HEAD_T);

  for (count = 0; count < result.count; count++) {
    namelen = strlen(result.table[count].name);
    ENG_LOG("%s: namelen: %d, name: %s\n", __FUNCTION__, namelen,
            result.table[count].name);
    *(pCur++) = namelen + 2;
    *(pCur++) = result.table[count].groupid;
    *(pCur++) = result.table[count].value;
    sprintf(pCur, "%s", result.table[count].name);
    pCur += namelen;
    datalen += (namelen + 3);
  }

  ENG_LOG("%s: datalen: %d\n", __FUNCTION__, datalen);

  ((MSG_HEAD_T *)rspdata)->len = datalen;
  rsplen = translate_packet(rsp, rspdata, datalen);

  free(rspdata);

  return rsplen;
}

int is_rm_cali_nv_need_to_handle(char *buf, int len) {
  int crc = 0;
  int recv_crc = 0;
  int cmd_mask;
  MSG_HEAD_T *msg_head = (MSG_HEAD_T *)(buf + 1);

  // Check CRC
  recv_crc =
      *(unsigned short *)&(buf[msg_head->len - sizeof(unsigned short) + 1]);
  crc = crc16(0, (unsigned char *)(msg_head + 1),
              msg_head->len - sizeof(MSG_HEAD_T) - sizeof(unsigned short));

  if (recv_crc != crc) {
    ENG_LOG("%s: CRC Error! recv_crc: %d, crc16: %d\n", __FUNCTION__, recv_crc,
            crc);
    return 0;  // send to CP
  }

  if ((msg_head->subtype & RW_MASK) == WRITE_MODE) {
    ENG_LOG("%s: Write mode !\n", __FUNCTION__);
    if ((msg_head->subtype & RM_VALID_CMD_MASK) == 0) {
      ENG_LOG("%s: not valid cmd\n", __FUNCTION__);
      return 0;
    }
  } else {
    ENG_LOG("%s: Read mode !\n", __FUNCTION__);
  }

  if (0 != (cmd_mask = (msg_head->subtype & 0x7f))) {
    ENG_LOG("%s: cmd_mask: %d, subtype: %d\n", __FUNCTION__, cmd_mask,
            msg_head->subtype);

    if ((cmd_mask & DIAG_CMD_BTBIT) || (cmd_mask & DIAG_CMD_WIFIBIT) ||
        (g_ap_cali_flag &&
         ((cmd_mask & DIAG_CMD_IMEI1BIT) || (cmd_mask & DIAG_CMD_IMEI2BIT) ||
          (cmd_mask & DIAG_CMD_IMEI3BIT) || (cmd_mask & DIAG_CMD_IMEI4BIT)))) {
      ENG_LOG("%s: Get BT/WIFI Mac addr req or IMEI req!\n", __FUNCTION__);
      if (cmd_mask = (cmd_mask & (~(DIAG_CMD_BTBIT | DIAG_CMD_WIFIBIT)))) {
        if (g_ap_cali_flag) {
          if (cmd_mask & (~(DIAG_CMD_IMEI1BIT | DIAG_CMD_IMEI2BIT |
                            DIAG_CMD_IMEI3BIT | DIAG_CMD_IMEI4BIT))) {
            ENG_LOG("%s: Have other commands !\n", __FUNCTION__);
            return 2;
          } else {
            ENG_LOG("%s: No other commands !\n", __FUNCTION__);
            return 1;
          }
        }

        ENG_LOG("%s: Have other commands !\n", __FUNCTION__);
        return 2;
      } else {
        ENG_LOG("%s: No other commands !\n", __FUNCTION__);
        return 1;
      }
    }
  }

  return 0;
}

static void eng_diag_reboot(int reset) {
  char name[64] = {0};

  switch (reset) {
    case 1:
      strcpy(name, "cftreboot");
      break;
    case 2:
      strcpy(name, "recovery");
      break;
    default:
      return;
  }

  sync();
  android_reboot(ANDROID_RB_RESTART2, 0, name);

  return;
}

static int eng_diag_deep_sleep(char *buf, int len, char *rsp) {
  char cmd[] = {"echo mem > /sys/power/state"};
  system(cmd);
  return 0;
}

static int eng_diag_ap_req(char *buf, int len) {
  int ret;
  TOOLS_DIAG_AP_CMD_T *apcmd =
      (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T));

  if (DIAG_AP_CMD_FILE_OPER == apcmd->cmd) {
    ret = CMD_USER_FILE_OPER;
  } else if (DIAG_AP_CMD_SWITCH_CP == apcmd->cmd) {
    ret = CMD_USER_CFT_SWITCH;
  } else if (DIAG_AP_CMD_CHANGE == apcmd->cmd) {
    ret = CMD_USER_ENABLE_CHARGE_ONOFF;
  } else if (DIAG_AP_CMD_READ_CURRENT == apcmd->cmd) {
    ret = CMD_USER_GET_CHARGE_CURRENT;
  } else if (DIAG_AP_CMD_GET_MODEM_MODE == apcmd->cmd) {
    ret = CMD_USER_GET_MODEM_MODE;
  }else if(DIAG_AP_CMD_READ_MMI == apcmd->cmd){
    ret = CMD_USER_READ_MMI;
  }else if(DIAG_AP_CMD_WRITE_MMI == apcmd->cmd){
    ret = CMD_USER_WRITE_MMI;
  } else if (DIAG_AP_CMD_BKLIGHT == apcmd->cmd) {
    ret = CMD_USER_BKLIGHT;
  }else if(DIAG_AP_CMD_TSX_DATA == apcmd->cmd){
    ret = CMD_USER_TXDATA;
  } else if (DIAG_AP_CMD_PWMODE == apcmd->cmd) {
    ret = CMD_USER_PWMODE;
  } else if (DIAG_AP_CMD_MODEM_DB_ATTR == apcmd->cmd) {
    ret = CMD_USER_MODEM_DB_ATTR;
  } else if (DIAG_AP_CMD_MODEM_DB_READ == apcmd->cmd) {
    ret = CMD_USER_MODEM_DB_READ;
  } else if (DIAG_AP_CMD_TEE_PRODUCTION == apcmd->cmd) {
    ret = CMD_USER_TEE_PRODUCTION;
  } else {
    ret = CMD_USER_APCALI;
  }

  ENG_LOG("%s: Handle CMD: %d\n", __FUNCTION__, ret);

  return ret;
}

static int eng_get_filesize(unsigned char *filename) {
  struct stat fileattr;

  ENG_LOG("%s: file name: %s\n", __FUNCTION__, filename);

  if (stat(filename, &fileattr) < 0) {
    return 0;
  }

  return fileattr.st_size;
}

static int eng_diag_fileoper_hdlr(char *buf, int len, char *rsp) {
  int ret = 0, fd_ser = -1;
  ;
  TOOLS_DIAG_AP_CMD_T *apcmd =
      (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  TOOLS_DIAG_AP_FILEOPER_REQ_T *fileoper =
      (TOOLS_DIAG_AP_FILEOPER_REQ_T *)(apcmd + 1);

  ENG_LOG("%s: File oper: %d, filename: %s\n", __FUNCTION__, fileoper->file_cmd,
          fileoper->file_name);

  switch (fileoper->file_cmd) {
    case 0:  // Query file status
    {
      unsigned char tmp[128] = {0};
      MSG_HEAD_T *header = (MSG_HEAD_T *)tmp;
      TOOLS_DIAG_AP_CNF_T *aprsp =
          (TOOLS_DIAG_AP_CNF_T *)(tmp + sizeof(MSG_HEAD_T));
      TOOLS_DIAG_AP_FILE_STATUS_T *filests =
          (TOOLS_DIAG_AP_FILE_STATUS_T *)(tmp + sizeof(MSG_HEAD_T) +
                                          sizeof(TOOLS_DIAG_AP_CNF_T));

      memcpy(tmp, buf + 1, sizeof(MSG_HEAD_T));  // copy the diag header
      header->len = sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T) +
                    sizeof(TOOLS_DIAG_AP_FILE_STATUS_T);

      aprsp->status = 0;
      aprsp->length = sizeof(TOOLS_DIAG_AP_FILE_STATUS_T);

      filests->file_size = eng_get_filesize(fileoper->file_name);
      if (0 == filests->file_size) {
        aprsp->status = 1;  // error
        ENG_LOG("%s: get file size error(query)\n", __FUNCTION__);
      }

      ret = translate_packet(rsp, tmp, header->len);
      ENG_LOG("%s: response length: %d\n", __FUNCTION__, ret);
    } break;
    case 1:  // Read file
    {
      int fd = -1;
      int file_size = 0;
      int read_size = 0;
      int total_size = 0;
      int diag_size = MAX_DIAG_TRANSMIT_FILE_LEN + sizeof(MSG_HEAD_T) +
                      sizeof(TOOLS_DIAG_AP_CNF_T) +
                      sizeof(TOOLS_DIAG_AP_FILE_DATA_T);
      unsigned char *tmp, *diag_frame;

      tmp = malloc(diag_size);
      memset(tmp, 0, diag_size);

      diag_frame = malloc(2 * diag_size + 2);

      MSG_HEAD_T *header = (MSG_HEAD_T *)tmp;
      TOOLS_DIAG_AP_CNF_T *aprsp =
          (TOOLS_DIAG_AP_CNF_T *)(tmp + sizeof(MSG_HEAD_T));
      TOOLS_DIAG_AP_FILE_DATA_T *filedata =
          (TOOLS_DIAG_AP_FILE_DATA_T *)(tmp + sizeof(MSG_HEAD_T) +
                                        sizeof(TOOLS_DIAG_AP_CNF_T));

      memcpy(tmp, buf + 1, sizeof(MSG_HEAD_T));  // copy the diag header
      header->len = sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T) +
                    sizeof(TOOLS_DIAG_AP_FILE_DATA_T);
      aprsp->status = 0;
      aprsp->length = sizeof(TOOLS_DIAG_AP_FILE_DATA_T);

      file_size = eng_get_filesize(fileoper->file_name);
      if (0 == file_size) {
        ENG_LOG("%s: get file size error(read): file_size: %d\n", __FUNCTION__,
                file_size);
        aprsp->status = 1;
      } else {
        fd = open(fileoper->file_name, O_RDONLY);
        if (fd >= 0) {
          do {
            read_size = read(fd, filedata->data, MAX_DIAG_TRANSMIT_FILE_LEN);
            total_size += read_size;

            if (total_size != file_size) {
              filedata->status = 1;
            } else {
              filedata->status = 0;
            }
            filedata->data_len = read_size;

            memset(diag_frame, 0, 2 * diag_size + 2);
            ret = translate_packet(diag_frame, tmp, header->len);
            fd_ser = get_ser_diag_fd();
            eng_diag_write2pc(diag_frame, ret, fd_ser);

            ENG_LOG("%s: response length(read): %d\n", __FUNCTION__, ret);
          } while (file_size != total_size);

          close(fd);
          free(diag_frame);
          free(tmp);

          return 0;
        } else {
          ENG_LOG("%s: file open fail\n", __FUNCTION__);
          aprsp->status = 1;
        }
      }

      ret = translate_packet(diag_frame, tmp, header->len);
      free(diag_frame);
      free(tmp);
    } break;
    case 2:  // Write file
    {
      int fd = -1;
      int write_size = 0;
      unsigned char tmp[128] = {0};
      MSG_HEAD_T *header = (MSG_HEAD_T *)tmp;
      TOOLS_DIAG_AP_CNF_T *aprsp =
          (TOOLS_DIAG_AP_CNF_T *)(tmp + sizeof(MSG_HEAD_T));
      TOOLS_DIAG_AP_FILE_DATA_T *filedata =
          (TOOLS_DIAG_AP_FILE_DATA_T *)(fileoper + 1);

      memcpy(tmp, buf + 1, sizeof(MSG_HEAD_T));  // copy the diag header
      header->len = sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T);
      aprsp->status = 0;

      ENG_LOG("%s: Open the file : %s, datalen: %d\n", __FUNCTION__,
              fileoper->file_name, filedata->data_len);

      fd = open(fileoper->file_name, O_WRONLY);
      if (fd >= 0) {
        ENG_LOG("%s: File is open\n", __FUNCTION__);
        if (lseek(fd, s_cur_filepos, SEEK_SET) == -1) {
          aprsp->status = 1;
          ENG_LOG("%s:  lseek error :%s", __FUNCTION__, strerror(errno));
        }
        write_size = write(fd, filedata->data, filedata->data_len);
        if (write_size < 0) {
          aprsp->status = 1;
        }

        if (filedata->status) {
          s_cur_filepos += write_size;
        } else {
          s_cur_filepos = 0;
        }

        ENG_LOG("%s: write_size: %d\n", __FUNCTION__, write_size);
        close(fd);
      } else {
        ENG_LOG("%s, open fail, fd: %d, err: %s\n", __FUNCTION__, fd,
                strerror(errno));
        aprsp->status = 1;  // fail
      }

      ret = translate_packet(rsp, tmp, header->len);
    } break;
    default:
      ENG_LOG("%s: Error operation!\n", __FUNCTION__);
      break;
  }

  return ret;
}

static int eng_diag_write_imei(REF_NVWriteDirect_T *direct, int num) {
  int ret = 0;
  int fd = -1;
  char imei_path[ENG_DEV_PATH_LEN] = {0};
  char imeinv[MAX_IMEI_LENGTH] = {0};
  char imeistr[MAX_IMEI_STR_LENGTH] = {0};

  ENG_LOG("%s: imei num: %d\n", __FUNCTION__, num);

  switch (num) {
    case 1:
      strcpy(imei_path, ENG_IMEI1_CONFIG_FILE);
      memcpy(imeinv, direct->imei1, MAX_IMEI_LENGTH);
      break;
    case 2:
      strcpy(imei_path, ENG_IMEI2_CONFIG_FILE);
      memcpy(imeinv, direct->imei2, MAX_IMEI_LENGTH);
      break;
    case 3:
      strcpy(imei_path, ENG_IMEI3_CONFIG_FILE);
      memcpy(imeinv, direct->imei3, MAX_IMEI_LENGTH);
      break;
    case 4:
      strcpy(imei_path, ENG_IMEI4_CONFIG_FILE);
      memcpy(imeinv, direct->imei4, MAX_IMEI_LENGTH);
      break;
    default:
      return 0;
  }

  fd = open(imei_path, O_WRONLY);
  if (fd >= 0) {
    ImeiConvNV2Str(imeinv, imeistr);
    ret = write(fd, imeistr, MAX_IMEI_STR_LENGTH);
    if (ret > 0) {
      ret = 1;
      fsync(fd);
    } else {
      ret = 0;
    }
    close(fd);
  }

  return ret;
}

static int eng_diag_read_imei(REF_NVWriteDirect_T *direct, int num) {
  int ret = 0;
  int fd = -1;
  char imei_path[ENG_DEV_PATH_LEN] = {0};
  char imeistr[MAX_IMEI_STR_LENGTH] = {0};
  char *imeinv = 0;

  ENG_LOG("%s: imei num: %d\n", __FUNCTION__, num);

  switch (num) {
    case 1:
      strcpy(imei_path, ENG_IMEI1_CONFIG_FILE);
      imeinv = direct->imei1;
      break;
    case 2:
      strcpy(imei_path, ENG_IMEI2_CONFIG_FILE);
      imeinv = direct->imei2;
      break;
    case 3:
      strcpy(imei_path, ENG_IMEI3_CONFIG_FILE);
      imeinv = direct->imei3;
      break;
    case 4:
      strcpy(imei_path, ENG_IMEI4_CONFIG_FILE);
      imeinv = direct->imei4;
      break;
    default:
      return 0;
  }

  fd = open(imei_path, O_RDONLY);
  if (fd >= 0) {
    ret = read(fd, imeistr, MAX_IMEI_STR_LENGTH);
    if (ret > 0) {
      ImeiConvStr2NV(imeistr, imeinv);
      ret = 1;
    } else {
      ret = 0;
    }
    close(fd);
  }

  return ret;
}

static void ImeiConvStr2NV(unsigned char *szImei, unsigned char *nvImei) {
  unsigned char temp1 = (unsigned char)(szImei[0] - '0');
  unsigned char temp2 = 0;
  nvImei[0] = (unsigned char)MAKE1BYTE2BYTES(temp1, (unsigned char)0x0A);
  int i;
  for (i = 1; i < MAX_IMEI_LENGTH; i++) {
    temp1 = (unsigned char)(szImei[2 * i - 0] - '0');
    temp2 = (unsigned char)(szImei[2 * i - 1] - '0');
    nvImei[i] = (unsigned char)MAKE1BYTE2BYTES(temp1, temp2);
  }
}

static void ImeiConvNV2Str(unsigned char *nvImei, unsigned char *szImei) {
  int i;

  for (i = 0; i < MAX_IMEI_LENGTH - 1; i++) {
    szImei[2 * i + 0] = ((nvImei[i] & 0xF0) >> 4) + '0';
    szImei[2 * i + 1] = (nvImei[i + 1] & 0x0F) + '0';
  }

  szImei[14] = ((nvImei[7] & 0xF0) >> 4) + '0';
}

static char MAKE1BYTE2BYTES(unsigned char high4bit, unsigned char low4bit) {
  char temp;
  temp = (high4bit << 4) | low4bit;
  return temp;
}
static void eng_diag_cft_switch_hdlr(char *buf, int len, char *rsp,
                                     int rsplen) {
  int fd = -1;
  char *rsp_ptr;
  unsigned short type = 0;
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return;
  }

  rsplen = sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  ((TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T)))->status = 0x01;

  TOOLS_DIAG_AP_CMD_T *reqcmd =
      (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  TOOLS_DIAG_AP_SWITCH_CP_T *cp_status =
      (TOOLS_DIAG_AP_SWITCH_CP_T *)(reqcmd + 1);

  type = cp_status->cp_no;

  switch (type) {
    case 0:
      property_set("sys.cfg.type", "0");
      ((TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T)))->status = 0x00;
      break;
    case 1:
      property_set("sys.cfg.type", "1");
      ((TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T)))->status = 0x00;
      break;
    default:
      ENG_LOG("%s:ERROR type !!!", __FUNCTION__);
  }

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  fd = get_ser_diag_fd();
  eng_diag_write2pc(rsp, rsplen, fd);
  free(rsp_ptr);
  exit(1);
}
#ifndef CONFIG_MINIENGPC
static int eng_diag_gps_autotest_hdlr(char *buf, int len, char *rsp,
                                      int rsplen) {
  int ret = 0, init_mode = 0, stop_mode = 0;
  char tmpbuf[ENG_DIAG_SIZE] = {0};
  static int init = 0;
  MSG_HEAD_T *msg_head = (MSG_HEAD_T *)(buf + 1);
  MSG_HEAD_T *rsp_head = (MSG_HEAD_T *)tmpbuf;

  memcpy(tmpbuf, buf + 1, sizeof(MSG_HEAD_T));
  sprintf(tmpbuf + sizeof(MSG_HEAD_T), "%s", "\r\nOK\r\n");
  rsp_head->len = sizeof(MSG_HEAD_T) + strlen("\r\nOK\r\n");

  ENG_LOG("%s: msg_head->subtype: %d\n", __FUNCTION__, msg_head->subtype);

  init_mode = get_init_mode();
  if (init_mode == msg_head->subtype) {
    set_pc_mode(1);
    if (0 == init) {
      sem_init(&g_gps_sem, 0, 0);
      if (0 != eng_thread_create(&gps_thread_hdlr, eng_gps_log_thread, 0)) {
        ENG_LOG("gps log thread start error");
      }
      init = 1;
    }
  }

  stop_mode = get_stop_mode();
  if (stop_mode == msg_head->subtype) {
    gps_export_stop();
    g_gps_log_enable = 0;
  }
  ret = set_gps_mode(msg_head->subtype);

  if (ret == 1) {
    ENG_LOG("%s: gps_export_start \n", __FUNCTION__);
    ret = gps_export_start();
    if (!ret) {
      tmpbuf[sizeof(MSG_HEAD_T)] = 0;
      g_gps_log_enable = 1;
      sem_post(&g_gps_sem);
    } else {
      tmpbuf[sizeof(MSG_HEAD_T)] = 1;
    }
    rsp_head->len = sizeof(MSG_HEAD_T) + 1;
  }

  ret = translate_packet(rsp, tmpbuf, rsp_head->len);

  ENG_LOG("%s: ret: %d\n", __FUNCTION__, ret);

  return ret;
}
#endif
static int eng_diag_enable_charge(char *buf, int len, char *rsp, int rsplen) {
  int ret = 0;
  char *rsp_ptr;
  TOOLS_DIAG_AP_CNF_T *aprsp;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  TOOLS_DIAG_AP_CHARGE_T *charge_flag =
      (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T) +
                              sizeof(TOOLS_DIAG_AP_CMD_T));

  rsplen = sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  aprsp = (TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T));
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;
  aprsp->length = 0;
  if (1 == charge_flag->on_off) {
    ret = connect_vbus_charger();
    if (ret > 0) {
      aprsp->status = 0x00;
    } else {
      aprsp->status = 0x01;
      ENG_LOG("%s: enable charge failed !!!", __FUNCTION__);
    }
  } else if (0 == charge_flag->on_off) {
    ret = disconnect_vbus_charger();
    if (ret > 0) {
      aprsp->status = 0x00;
    } else {
      aprsp->status = 0x01;
      ENG_LOG("%s: disable charge failed !!!", __FUNCTION__);
    }
  }

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}
static int eng_diag_get_charge_current(char *buf, int len, char *rsp,
                                       int rsplen) {
  int ret = 0;
  int charging_current = 0, battery_current = 0;
  int charging_read_len = 0, battery_read_len = 0;
  char *rsp_ptr;
  MSG_HEAD_T *msg_head_ptr;
  TOOLS_DIAG_AP_CNF_T *aprsp;
  TOOLS_DIAG_CHARGE_CURRENT_CNF_T *currentdata;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }
  msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  rsplen = sizeof(TOOLS_DIAG_CHARGE_CURRENT_CNF_T) +
           sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  aprsp = (TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T));
  currentdata =
      (TOOLS_DIAG_CHARGE_CURRENT_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T) +
                                          sizeof(TOOLS_DIAG_AP_CNF_T));
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;
  aprsp->length = sizeof(TOOLS_DIAG_CHARGE_CURRENT_CNF_T);

  charging_read_len = get_charging_current(&charging_current);
  if (charging_read_len <= 0) {
    ENG_LOG("%s: read charging current failed !!!%s\n", __FUNCTION__,
            strerror(errno));
    goto out;
  }

  battery_read_len = get_battery_current(&battery_current);
  if (battery_read_len <= 0) {
    ENG_LOG("%s: read battery current failed !!!%s\n", __FUNCTION__,
            strerror(errno));
    goto out;
  }
  currentdata->charging = charging_current;
  currentdata->battery = battery_current;
  aprsp->status = 0x00;

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}
static int get_charging_current(int *value) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;

  fd = open(CHARGING_CURRENT_FILE_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) *value = strtol(buffer, &endptr, 0);
    close(fd);
    ENG_LOG("%s %s value = %d, read_len = %d \n", __FUNCTION__,
            CHARGING_CURRENT_FILE_PATH, *value, read_len);
  } else {
    ENG_LOG("%s open %s failed\n", __FUNCTION__, CHARGING_CURRENT_FILE_PATH);
  }
  return read_len;
}

static int get_battery_current(int *value) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;

  fd = open(BATTERY_CURRENT_FILE_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) *value = strtol(buffer, &endptr, 0);
    close(fd);
    ENG_LOG("%s %s value = %d, read_len = %d \n", __FUNCTION__,
            BATTERY_CURRENT_FILE_PATH, *value, read_len);
  } else {
    ENG_LOG("%s open %s failed\n", __FUNCTION__, BATTERY_CURRENT_FILE_PATH);
  }
  return read_len;
}
#ifndef CONFIG_MINIENGPC
static int eng_diag_read_efuse(char *buf, int len, char *rsp, int rsplen) {
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  unsigned char data_buf[16] = {0};  // FIX ME,adjusted by ronghua interfaces
  unsigned char hash_buf[41] = {0};
  unsigned char uid_buf[17] = {0};
  unsigned char *str = NULL;
  char *endptr;
  int ret = 0;
  char *rsp_ptr, *temp;
  unsigned short block;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  block = *(unsigned short *)(msg_head_ptr + 1);

  rsplen = sizeof(EFUSE_INFO_T_RES) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T) + 2);
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  ((EFUSE_INFO_T_RES *)(rsp_ptr + sizeof(MSG_HEAD_T)))->flag = 0x00;

  do {
    ENG_LOG("%s,block = %d", __FUNCTION__, block);
    if (0 == block || 1 == block) {
      ret = efuse_uid_read(uid_buf, sizeof(uid_buf));
      if (ret < 0) {
        ENG_LOG("%s,efuse_uid_read ret = %d", __FUNCTION__, ret);
        break;
      }
      str = uid_buf;
      str += (block * 8);
    } else {
      ret = efuse_hash_read(hash_buf, sizeof(hash_buf));
      if (ret < 0) {
        ENG_LOG("%s,efuse_hash_read ret = %d", __FUNCTION__, ret);
        break;
      }
      str = hash_buf;
      str += ((block - 2) * 8);
    }

    memcpy(data_buf, str, 8);
    data_buf[8] = '\0';

    rsplen = sizeof(EFUSE_READ_INFO_T_RES) + sizeof(MSG_HEAD_T);
    temp = (char *)malloc(rsplen);
    if (NULL == temp) {
      ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
      return 0;
    }
    free(rsp_ptr);
    rsp_ptr = temp;
    memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T) + 2);
    ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
    ((EFUSE_READ_INFO_T_RES *)(rsp_ptr + sizeof(MSG_HEAD_T)))->data =
        strtoul(data_buf, &endptr, 16);
  } while (0);

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_get_time_sync_info(char *buf,int len,char *rsp, int rsplen)
{
  int ret = 0, n = 0, fd = -1, tz;
  struct pollfd fds;
  char buffer[16] = {0};
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T*)(buf + 1);
  char *rsp_ptr;
  TOOLS_DIAG_AP_CNF_T *aprsp;
  MODEM_TIMESTAMP_T time_stamp;
  int sleep_time = 0;
  int valid = 0;
  TIME_SYNC_T tsync;

  if(NULL == buf){
    ENG_LOG("%s: null pointer",__FUNCTION__);
    return 0;
  }

  rsplen = sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MODEM_TIMESTAMP_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char*)malloc(rsplen);
  memset(rsp_ptr, 0, rsplen);
  if(NULL == rsp_ptr){
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T*)rsp_ptr)->len = rsplen - sizeof(MODEM_TIMESTAMP_T);
  aprsp = rsp_ptr + sizeof(MSG_HEAD_T);
  aprsp->status = 0x01;
  aprsp->length = sizeof(MODEM_TIMESTAMP_T);

  pthread_mutex_lock(&g_time_sync_lock);
  if (g_time_sync.sys_cnt || g_time_sync.uptime) {
    memcpy(&tsync, &g_time_sync, sizeof(TIME_SYNC_T));
    valid = 1;
  }
  pthread_mutex_unlock(&g_time_sync_lock);

  if (valid) {
    current_ap_time_stamp_handle(&tsync, &time_stamp);
    memcpy(rsp_ptr + sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T), &time_stamp, sizeof(MODEM_TIMESTAMP_T));
    ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
    aprsp->status = 0x00;
  } else {
    ENG_LOG("%s: time sync error: g_time_sync={0}\n", __FUNCTION__);
  }


out:
  rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_write_efuse(char *buf, int len, char *rsp, int rsplen) {
  int ret, flag = -1;
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  unsigned char hashstr_buf[HASH_LEN + 1] = {0};
  unsigned char *hashstr = hashstr_buf;
  char *rsp_ptr;
  unsigned int secure_boot_flag = 0;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  rsplen = sizeof(char) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  *(char *)(rsp_ptr + sizeof(MSG_HEAD_T)) = 0x00;

  do {
    flag = eng_parse_hash_cmdline(hashstr);
    if (0 == flag) {
      ENG_LOG("%s,doesn't read the hash value", __FUNCTION__);
      break;
    }

    secure_boot_flag = efuse_secure_is_enabled();
    ret = efuse_hash_write(hashstr, strlen(hashstr), secure_boot_flag);
    if (ret <= 0) {
      ENG_LOG("%s,efuse_hash_write data failed. ret=%d", __FUNCTION__, ret);
      break;
    }
    *(char *)(rsp_ptr + sizeof(MSG_HEAD_T)) = 0x01;
  } while (0);

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}
#endif
static int eng_parse_hash_cmdline(unsigned char *cmdvalue) {
  int fd = 0, ret = 0, i = 0, flag = 0;
  char cmdline[ENG_CMDLINE_LEN] = {0};
  char *hashstr = NULL;

  fd = open("/proc/cmdline", O_RDONLY);
  if (fd < 0) {
    ENG_LOG("%s,/proc/cmdline open failed", __FUNCTION__);
    return 0;
  }

  ret = read(fd, cmdline, sizeof(cmdline));
  if (ret < 0) {
    ENG_LOG("%s,/proc/cmdline read failed", __FUNCTION__);
    close(fd);
    return 0;
  }
  ENG_LOG("%s,cmdline: %s\n", __FUNCTION__, cmdline);
  hashstr = strstr(cmdline, CMD_SECURESTRING);
  if (hashstr != NULL) {
    hashstr += strlen(CMD_SECURESTRING);
    memcpy(cmdvalue, hashstr, HASH_LEN);
    flag = 1;
  }

  ENG_LOG("%s,cmdline: %s\n", __FUNCTION__, cmdvalue);
  return flag;
}

static int eng_parse_publickey_cmdline(char *path, int *pos, int *len) {
  int fd = -1;
  int ret = 0;
  int strlength = 0;
  char cmdline[ENG_CMDLINE_LEN] = {0};
  char *buf = NULL, *ch = NULL;
  char str[10] = {0};

  fd = open("/proc/cmdline", O_RDONLY);
  if (fd < 0) {
    ENG_LOG("%s,/proc/cmdline open failed", __FUNCTION__);
    return 0;
  }

  ret = read(fd, cmdline, sizeof(cmdline));
  if (ret < 0) {
    ENG_LOG("%s,/proc/cmdline read failed", __FUNCTION__);
    close(fd);
    return 0;
  }
  ENG_LOG("%s,cmdline: %s\n", __FUNCTION__, cmdline);

  buf = strstr(cmdline, CMD_PUBLICKEYPATH);
  if (buf != NULL) {
    buf += strlen(CMD_PUBLICKEYPATH);
    ch = strchr(buf, ' ');
    if (ch != NULL) {
      strlength = ch - buf;
      memcpy(path, buf, strlength);
      ENG_LOG("%s,path = %s ", __FUNCTION__, path);
    }
  }

  buf = strstr(cmdline, CMD_PUBLICKEYSTART);
  if (buf != NULL) {
    buf += strlen(CMD_PUBLICKEYSTART);
    ch = strchr(buf, ' ');
    if (ch != NULL) {
      strlength = ch - buf;
      strncpy(str, buf, strlength);
      *pos = atoi(str);
      ENG_LOG("%s,pos = %d ", __FUNCTION__, *pos);
    }
  }

  buf = strstr(cmdline, CMD_PUBLICKEYLEN);
  if (buf != NULL) {
    buf += strlen(CMD_PUBLICKEYLEN);
    ch = strchr(buf, ' ');
    if (ch != NULL) {
      strlength = ch - buf;
      strncpy(str, buf, strlength);
      *len = atoi(str);
      ENG_LOG("%s,len = %d ", __FUNCTION__, *len);
    }
  }
  close(fd);
  return 1;
}

static int eng_diag_read_publickey(char *buf, int len, char *rsp, int rsplen) {
  int fd = -1;
  int ret = 0;
  int publickeypos = 0;
  int publickeylen = 0;
  char data_buf[1024] = {0};
  char *endptr;
  char *rsp_ptr, *temp;
  char *publickeypath = NULL;
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  rsplen = sizeof(char) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  *((char *)(rsp_ptr + sizeof(MSG_HEAD_T))) = 0x00;

  publickeypath = (char *)malloc(1024);
  eng_parse_publickey_cmdline(publickeypath, &publickeypos, &publickeylen);

  do {
    fd = open(publickeypath, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s,%s open failed", publickeypath, __FUNCTION__);
      break;
    }

    if (-1 == lseek(fd, publickeypos, SEEK_SET)) {
      ENG_LOG("%s,/dev/block/mmcblk0boot0 seek failed", __FUNCTION__);
      break;
    }
    ret = read(fd, data_buf, publickeylen);
    if (ret <= 0) {
      ENG_LOG("%s,/dev/block/mmcblk0boot0 read data failed", __FUNCTION__);
      break;
    }

    rsplen = publickeylen + sizeof(MSG_HEAD_T);
    temp = (char *)malloc(rsplen);
    if (NULL == temp) {
      ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
      break;
    }
    free(rsp_ptr);
    rsp_ptr = temp;
    memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
    ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
    memcpy(rsp_ptr + sizeof(MSG_HEAD_T), data_buf, publickeylen);
  } while (0);

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  if (fd >= 0) close(fd);
  free(rsp_ptr);
  free(publickeypath);
  return rsplen;
}
#ifndef CONFIG_MINIENGPC
static int eng_diag_enable_secure(char *buf, int len, char *rsp, int rsplen) {
  int ret = 0;
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  char *rsp_ptr;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  rsplen = sizeof(char) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  *(char *)(rsp_ptr + sizeof(MSG_HEAD_T)) = 0x00;

  ret = efuse_secure_enable();
  if (ret >= 0) *(char *)(rsp_ptr + sizeof(MSG_HEAD_T)) = 0x01;

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_read_enable_secure_bit(char *buf, int len, char *rsp,
                                           int rsplen) {
  int fd = -1;
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  int ret = 0;
  char *rsp_ptr;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  rsplen = sizeof(char) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  *(char *)(rsp_ptr + sizeof(MSG_HEAD_T)) = 0x00;

  ret = efuse_secure_is_enabled();
  if (0 != ret) *(char *)(rsp_ptr + sizeof(MSG_HEAD_T)) = 0x01;

  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}
#endif
static int eng_diag_get_modem_mode(char *buf, int len, char *rsp, int rsplen) {
  int ret = 0;
  char property_info[PROPERTY_VALUE_MAX] = {0};
  char *rsp_ptr;
  MSG_HEAD_T *msg_head_ptr;
  TOOLS_DIAG_AP_CNF_T *aprsp;
  TOOLS_DIAG_AP_MODULE_T *modem_mode;

  if (NULL == buf) {
    ENG_LOG("%s,null pointer", __FUNCTION__);
    return 0;
  }

  msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  rsplen = sizeof(TOOLS_DIAG_AP_MODULE_T) + sizeof(TOOLS_DIAG_AP_CNF_T) +
           sizeof(MSG_HEAD_T);
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  aprsp = (TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T));
  modem_mode = (TOOLS_DIAG_AP_MODULE_T *)(rsp_ptr + sizeof(MSG_HEAD_T) +
                                          sizeof(TOOLS_DIAG_AP_CNF_T));
  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;

  property_get("ro.product.modem.mode", property_info, "not_find");
  if (0 == strcmp(property_info, "not_find")) {
    ENG_LOG("%s: read ro.product.modem.mode failed !!!%s\n", __FUNCTION__,
            strerror(errno));
    goto out;
  }

  aprsp->length = strlen(property_info);
  memcpy(modem_mode, property_info, strlen(property_info));
  aprsp->status = 0x00;

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int read_tsx_data(TOOLS_DIAG_AP_TSX_DATA_T * res)
{
  int rcount;
  int ret = 0;
  FILE * fp = NULL;
  if(NULL == res)
  {
    ENG_LOG("%s: res is NULL!!!",__FUNCTION__);
    ret = -1;
    return ret;
  }
  if(access(ENG_TXDATA_FILE, F_OK) == 0) {
    ENG_LOG("%s: %s exists",__FUNCTION__, ENG_TXDATA_FILE);
    fp = fopen(ENG_TXDATA_FILE, "r");
    if(NULL == fp)
    {
      ENG_LOG("%s: fopen fail errno=%d, strerror(errno)=%s",__FUNCTION__, errno, strerror(errno));
      ret = -1;
      return ret;
    }
    rcount = fread(&res->value[0], sizeof(TSX_DATA_T), 2, fp);
    if(rcount <= 0)
    {
      ret = -1;
    }
    ENG_LOG("%s: fread count %d",__FUNCTION__, rcount);
    fclose(fp);
  }else{
    ret = -1;
    ENG_LOG("%s: %s not exists",__FUNCTION__, ENG_TXDATA_FILE);
  }
  return ret;
}
static int write_tsx_data(TOOLS_DIAG_AP_TSX_DATA_T * req)
{
  int rcount;
  int ret = 0, fd = -1;
  FILE * fp = NULL;
  mode_t old_mask;
  static first_flag = 1;
  if(NULL == req)
  {
    ENG_LOG("%s: req is NULL!!!",__FUNCTION__);
    ret = -1;
    return ret;
  }
  ENG_LOG("%s: %s exists",__FUNCTION__, ENG_TXDATA_FILE);
  if(first_flag) old_mask = umask(0);
  fp = fopen(ENG_TXDATA_FILE, "w+");
  if(first_flag) umask(old_mask);
  if(NULL == fp)
  {
    ENG_LOG("%s: fopen fail errno=%d, strerror(errno)=%s",__FUNCTION__, errno, strerror(errno));
    first_flag = 1;
    ret = -1;
    return ret;
  }
  else
  {
    first_flag = 0;
  }
  rcount = fwrite(&req->value[0], sizeof(TSX_DATA_T), 2, fp);
  ENG_LOG("%s: fread count %d",__FUNCTION__, rcount);
  if(2 != rcount)
  {
    ret = -1;
  }
  else
  {
    fflush(fp);
    fd = fileno(fp);
    if(fd > 0) {
      fsync(fd);
    } else {
      ENG_LOG("%s: fileno() error, strerror(errno)=%s", __FUNCTION__, strerror(errno));
      ret = -1;
    }
  }
  fclose(fp);
  return ret;
}
static int eng_diag_txdata(char *buf, int len, char *rsp, int rsplen)
{
  int ret = 0;
  char *rsp_ptr;
  MSG_HEAD_T* msg_head_ptr;
  TOOLS_DIAG_AP_CNF_T* aprsp;
  TOOLS_DIAG_AP_TSX_DATA_T* src_tsxdata;
  TOOLS_DIAG_AP_TSX_DATA_T* tsxdata;
  if(NULL == buf){
    ENG_LOG("%s,null pointer",__FUNCTION__);
    return 0;
  }
  msg_head_ptr = (MSG_HEAD_T*)(buf + 1);
  rsplen = sizeof(TOOLS_DIAG_AP_TSX_DATA_T)+ sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char*)malloc(rsplen);
  if(NULL == rsp_ptr){
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memset(rsp_ptr, 0x00, rsplen);
  aprsp = (TOOLS_DIAG_AP_CNF_T*)(rsp_ptr + sizeof(MSG_HEAD_T));
  tsxdata = (TOOLS_DIAG_AP_TSX_DATA_T*)(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T));
  memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
  aprsp->status = DIAG_AP_CMD_TSX_DATA;
  aprsp->length= sizeof(TOOLS_DIAG_AP_TSX_DATA_T);
  src_tsxdata = (TOOLS_DIAG_AP_TSX_DATA_T*)(buf + 1 + sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T));
  if(0 == src_tsxdata->cmd)
  {
#if 0
    /*memcpy api defect, trigger crash in androidN*/
    memcpy(tsxdata, src_tsxdata, sizeof(TOOLS_DIAG_AP_TSX_DATA_T));
#else
    for(int y=0; y<sizeof(TOOLS_DIAG_AP_TSX_DATA_T); y++) {
      *((char *)tsxdata + y) = *((char *)src_tsxdata + y);
    }
#endif
    ret = write_tsx_data(src_tsxdata);
    if(0 == ret)
    {
      tsxdata->res_status = 0;
    }else{
      tsxdata->res_status = 1;
    }
  }else if(1 == src_tsxdata->cmd){
    ret = read_tsx_data(tsxdata);
    tsxdata->cmd = 1;
    if(0 == ret)
    {
      tsxdata->res_status = 0;
    }else{
      tsxdata->res_status = 1;
    }
  }else{
    ENG_LOG("%s: tsx_data cmd not read and write !!!\n", __FUNCTION__);
  }
out:
  rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_set_backlight(char *buf, int len, char *rsp, int rsplen) {
  int fd = -1;
  char *rsp_ptr;
  char time[32] = {0};
  TOOLS_DIAG_AP_CNF_T *aprsp;

  TOOLS_DIAG_AP_CMD_T *apcmd =
      (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  unsigned short backlight_time = *(unsigned short *)(apcmd + 1);
  ENG_LOG("%s: backlight_time: %d\n", __FUNCTION__, backlight_time);

  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  rsplen =
      sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(unsigned short);

  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  aprsp = (TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;
  aprsp->length = sizeof(unsigned short);
  *((unsigned short *)(rsp_ptr + sizeof(MSG_HEAD_T) +
                       sizeof(TOOLS_DIAG_AP_CNF_T))) = backlight_time;

  fd = open(ENG_SET_BACKLIGHT, O_WRONLY);
  if (fd < 0) {
    ENG_LOG("%s: open %s failed, err: %s\n", __func__, ENG_SET_BACKLIGHT,
            strerror(errno));
    goto out;
  }

  sprintf(time, "%d", backlight_time);
  len = write(fd, time, strlen(time));
  if (len <= 0) {
    ENG_LOG("%s: write %s failed, err: %s\n", __func__, ENG_SET_BACKLIGHT,
            strerror(errno));
    close(fd);
    goto out;
  }

  aprsp->status = 0x00;
  close(fd);

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_set_powermode(char *buf, int len, char *rsp, int rsplen) {
  int flag = 0, ret = 0;
  char *rsp_ptr;
  TOOLS_DIAG_AP_CNF_T *aprsp;
  char *insmode_flag = NULL;
  char cmd[1024] = {0};
  char time[32] = {0};
  char cmdline[ENG_CMDLINE_LEN] = {0};

  TOOLS_DIAG_AP_CMD_T *apcmd =
      (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  unsigned short powermode = *(unsigned short *)(apcmd + 1);
  ENG_LOG("%s: powermode: %d\n", __FUNCTION__, powermode);

  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  rsplen =
      sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(unsigned short);

  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  aprsp = (TOOLS_DIAG_AP_CNF_T *)(rsp_ptr + sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;
  aprsp->length = sizeof(unsigned short);
  *((unsigned short *)(rsp_ptr + sizeof(MSG_HEAD_T) +
                       sizeof(TOOLS_DIAG_AP_CNF_T))) = powermode;

  eng_open_wifi_switch();

  flag = system("ifconfig wlan0 up");
  ENG_LOG("%s: ifconfig wlan0 up.\n", __FUNCTION__);
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG("%s: ifconfig wlan0 up. flag=%d,err:%s\n", __FUNCTION__, flag,
            strerror(errno));
    goto out;
  }

  if (1 == powermode) {  // save power mode
    flag = system("iwnpi wlan0 lna_on");
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s: flag error\n", __FUNCTION__);
      goto out;
    }
  } else if (0 == powermode) {  // non-save power mode
    flag = system("iwnpi wlan0 lna_off");
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s: flag error\n", __FUNCTION__);
      goto out;
    }
  }
  aprsp->status = 0x00;

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_set_ipconfigure(char *buf, int len, char *rsp, int rsplen) {
  int flag = 0, ret = 0;
  char *rsp_ptr, *insmode_flag = NULL;
  char cmd[ENG_CMDLINE_LEN] = {0};
  char cmdline[ENG_CMDLINE_LEN] = {0};

  WIFI_CONFIGURE_IP_T *apcmd =
      (WIFI_CONFIGURE_IP_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  rsplen = sizeof(MSG_HEAD_T) + sizeof(char);

  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  *((char *)(rsp_ptr + sizeof(MSG_HEAD_T))) = 0;
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;

  if (0 == eng_open_wifi_switch()) {
    ENG_LOG("%s:open wifi function eng_open_wifi_switch failed\n",
            __FUNCTION__);
    goto out;
  }

  flag = system("wpa_cli -iwlan0 IFNAME=wlan0 remove_network all");
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG(
        "%s:wpa_cli -iwlan0 IFNAME=wlan0 remove_network all. flag=%d,err:%s\n",
        __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  flag = system("wpa_cli -iwlan0 IFNAME=wlan0 add_network");
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG("%s:wpa_cli -iwlan0 IFNAME=wlan0 add_network. flag=%d,err:%s\n",
            __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  sprintf(cmd, "wpa_cli -iwlan0 IFNAME=wlan0 set_network 0 ssid /%s/",
          apcmd->szSSID);
  flag = system(cmd);
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG(
        "%s: wpa_cli -iwlan0 IFNAME=wlan0 set_network 0 ssid /CMCC/. "
        "lag=%d,err:%s\n",
        __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  flag = system("wpa_cli -iwlan0 IFNAME=wlan0 set_network 0 key_mgmt NONE");
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG(
        "%s:wpa_cli -iwlan0 IFNAME=wlan0 set_network 0 key_mgmt NONE. "
        "flag=%d,err:%s\n",
        __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  flag = system("wpa_cli -iwlan0 IFNAME=wlan0 select_network 0");
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG(
        "%s:wpa_cli -iwlan0 IFNAME=wlan0 select_network 0. flag=%d,err:%s\n",
        __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  sprintf(cmd, "ifconfig wlan0 %s netmask %s ", apcmd->szIPAddress,
          apcmd->szSubnet);
  flag = system(cmd);
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG("%s: ifconfig wlan0 netmask flag error. flag=%d,err:%s\n",
            __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  sprintf(cmd, "ip route add default via %s ", apcmd->szGateway);
  flag = system(cmd);
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG("%s:ip route add default via. flag error. flag=%d,err:%s\n",
            __FUNCTION__, flag, strerror(errno));
    flag = system("ip route del");
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s: ip route del. flag=%d,err:%s\n", __FUNCTION__, flag,
              strerror(errno));
      goto out;
    }
    flag = system(cmd);
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s:ip route add default via. flag error. flag=%d,err:%s\n",
              __FUNCTION__, flag, strerror(errno));
      goto out;
    }
  }

  flag = system("ndc resolver setdefaultif wlan0");
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG("%s: ndc resolver setdefaultif wlan0. flag=%d,err:%s\n",
            __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  sprintf(cmd,
          "ndc resolver setifdns wlan0 "
          " %s ",
          apcmd->szDNS);
  system(cmd);
  if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
    ENG_LOG("%s:ndc resolver setifdns wlan0 flag erro. flag=%d,err:%s\n",
            __FUNCTION__, flag, strerror(errno));
    goto out;
  }

  *((char *)(rsp_ptr + sizeof(MSG_HEAD_T))) = 1;

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}
static int eng_open_wifi_switch() {
  int flag = 0, ret = 0, fd = -1;
  char *insmode_flag = NULL;
  char cmd[ENG_CMDLINE_LEN] = {0};
  char cmdline[ENG_CMDLINE_LEN] = {0};
  char *process_name = "wpa_supplicant";

#ifdef ENGMODE_EUT_SPRD
  ENG_LOG("%s:sprd chip\n", __FUNCTION__);

  fd = open("/proc/modules", O_RDONLY);
  if (fd < 0) {
    ENG_LOG("%s,/proc/modules open failed", __FUNCTION__);
    return 0;
  }

  ret = read(fd, cmdline, sizeof(cmdline));
  if (ret < 0) {
    ENG_LOG("%s,/proc/modules read failed", __FUNCTION__);
    return 0;
  }

  insmode_flag = strstr(cmdline, WIFI_DRIVER_MODULE_NAME);
  if (insmode_flag == NULL) {
    sprintf(cmd, "insmod %s", WIFI_DRIVER_MODULE_PATH);
    flag = system(cmd);
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s:insmod /system/lib/modules/sprdwl.ko. flag=%d,err:%s\n",
              __FUNCTION__, flag, strerror(errno));
      return 0;
    }
  }
#endif
  if (0 == eng_detect_process(process_name)) {
    flag = system("start wpa_supplicant");
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s: start wpa_supplicant failed. flag=%d ,err:%s\n",
              __FUNCTION__, flag, strerror(errno));
      return 0;
    }
  }

  sleep(5);
  return 1;
}
static int eng_detect_process(char *process_name) {
  FILE *ptr;
  char buff[512], ps[128];
  int count = -1;

  sprintf(ps, "ps |grep -c %s", process_name);
  if ((ptr = popen(ps, "r")) != NULL) {
    if (fgets(buff, 512, ptr) != NULL) {
      count = atoi(buff);
    }
  }

  ENG_LOG("%s,count = %d", __FUNCTION__, count);
  pclose(ptr);

  return count;
}
static int eng_diag_read_register(char *buf, int len, char *rsp, int rsplen) {
  FILE *fd;
  int m = 0;
  char *rsp_ptr, *temp, *end;
  char stemp[9] = {0};
  char AddrCount[64] = {0};
  unsigned int pdata[64] = {0};
  char cmd[ENG_CMDLINE_LEN] = {0};
  char regvalue[ENG_DIAG_SIZE] = {0};
  char *value = regvalue;

  WIFI_REGISTER_REQ_T *apcmd =
      (WIFI_REGISTER_REQ_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  rsplen = sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T) + 1;

  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  *((char *)(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T))) = 0;

  sprintf(cmd, "iwnpi wlan0 get_reg %s %x %d", apcmd->szType, apcmd->nRegAddr,
          apcmd->nCount);
  fd = popen(cmd, "r");
  if (NULL == fd) {
    ENG_LOG("%s: popen error.\n", __FUNCTION__);
    goto out;
  }
  fread(regvalue, sizeof(char), sizeof(regvalue), fd);
  rsplen = sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T) +
           (apcmd->nCount) * 4;  // for every type return 4 bytes data
  temp = (char *)malloc(rsplen);
  if (NULL == temp) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    goto out;
  }

  free(rsp_ptr);
  rsp_ptr = temp;
  memcpy(rsp_ptr, msg_head_ptr,
         sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;

  while (m < apcmd->nCount) {
    if (*value == '0' && *(value + 1) == 'x') {
      value += 2;
      memcpy(stemp, value, 8);
      pdata[m] = (unsigned int)strtoul(stemp, &end, 16);
      value += 8;
      m++;
    } else {
      value += 1;
    }
  }

  memcpy(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T), pdata,
         (apcmd->nCount) * 4);

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  pclose(fd);
  return rsplen;
}

static int eng_diag_write_register(char *buf, int len, char *rsp, int rsplen) {
  int flag;
  unsigned int i, j, ret;
  char *rsp_ptr;
  char RegValue[32] = {0};
  char cmd[1024] = {0};

  WIFI_REGISTER_REQ_T *apcmd =
      (WIFI_REGISTER_REQ_T *)(buf + 1 + sizeof(MSG_HEAD_T));
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T *)(buf + 1);
  char *value = (char *)(apcmd + 1);

  rsplen = sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T) + 1;
  rsp_ptr = (char *)malloc(rsplen);
  if (NULL == rsp_ptr) {
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  memcpy(rsp_ptr, msg_head_ptr,
         sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T));
  ((MSG_HEAD_T *)rsp_ptr)->len = rsplen;
  *((char *)(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T))) = 0;

  for (i = 0; i < apcmd->nCount; i++) {
    sprintf(cmd, "iwnpi wlan0 set_reg %s %x ", apcmd->szType,
            apcmd->nRegAddr + i * apcmd->nUit);
    for (j = 0; j < apcmd->nUit; j++) {
      sprintf(RegValue, "%02x", *value);
      strcat(cmd, RegValue);
      value += 1;
    }

    flag = system(cmd);
    if (!WIFEXITED(flag) || WEXITSTATUS(flag) || -1 == flag) {
      ENG_LOG("%s iwnpi wlan0 set_reg flag =%d\n", __FUNCTION__, flag);
      goto out;
    }
  }
  *((char *)(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(WIFI_REGISTER_REQ_T))) = 1;

out:
  rsplen = translate_packet(rsp, (unsigned char *)rsp_ptr,
                            ((MSG_HEAD_T *)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}

static int eng_diag_read_mmi(char *buf, int len, char *rsp, int rsplen)
{
  int fd = -1;
  int read_len;
  char buffer[256];
  char *rsp_ptr;
  MSG_HEAD_T* msg_head_ptr;
  TOOLS_DIAG_AP_CNF_T* aprsp;
  TOOLS_DIAG_MMI_CIT_T* test_result,*req_ptr;

  if(NULL == buf){
    ENG_LOG("%s,null pointer",__FUNCTION__);
    return 0;
  }

  msg_head_ptr = (MSG_HEAD_T*)(buf + 1);
  req_ptr = (TOOLS_DIAG_MMI_CIT_T*)(buf + 1 + sizeof(MSG_HEAD_T)+sizeof(TOOLS_DIAG_AP_CMD_T));
  rsplen = sizeof(TOOLS_DIAG_AP_CNF_T)+ sizeof(TOOLS_DIAG_MMI_CIT_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char*)malloc(rsplen);
  if(NULL == rsp_ptr){
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  aprsp = (TOOLS_DIAG_AP_CNF_T*)(rsp_ptr + sizeof(MSG_HEAD_T));
  test_result = (TOOLS_DIAG_MMI_CIT_T*)(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T));
  memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;
  aprsp->length= sizeof(TOOLS_DIAG_MMI_CIT_T);

  if( 0 == req_ptr->uType){
    fd = open(WHOLE_PHONE_TEST_FILE_PATH,O_RDONLY); //00: whole phone test; 01: PCBA test ; 02: BBAT test
  }else if(1 == req_ptr->uType){
    fd = open(PCBA_TEST_FILE_PATH,O_RDONLY);
  }else if(2 == req_ptr->uType){
    fd = open(BBAT_TEST_FILE_PATH,O_RDONLY);
  }
  if(fd < 0){
    ENG_LOG("%s open failed,type = %d\n",__FUNCTION__,req_ptr->uType);
    goto out;
  }
  read_len = read(fd,buffer,sizeof(buffer));
  if(read_len < 0){
    ENG_LOG("%s read failed! read_len = %d,type = %d\n",__FUNCTION__,read_len,req_ptr->uType);
    goto out;
  }

  memcpy(test_result->uBuff,buffer,read_len);
  aprsp->status = 0x00;

out:
  rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
  free(rsp_ptr);
  if(fd >= 0 )
    close(fd);
  return rsplen;
}

static int eng_diag_write_mmi(char *buf, int len, char *rsp, int rsplen)
{
  int fd = -1;
  int write_len;
  char *rsp_ptr;
  MSG_HEAD_T* msg_head_ptr;
  TOOLS_DIAG_AP_CNF_T* aprsp;
  TOOLS_DIAG_MMI_CIT_T* req_ptr;

  if(NULL == buf){
    ENG_LOG("%s,null pointer",__FUNCTION__);
    return 0;
  }

  msg_head_ptr = (MSG_HEAD_T*)(buf + 1);
  req_ptr = (TOOLS_DIAG_MMI_CIT_T*)(buf + 1 + sizeof(MSG_HEAD_T)+sizeof(TOOLS_DIAG_AP_CMD_T));

  rsplen = sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MSG_HEAD_T);
  rsp_ptr = (char*)malloc(rsplen);
  if(NULL == rsp_ptr){
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }
  aprsp = (TOOLS_DIAG_AP_CNF_T*)(rsp_ptr + sizeof(MSG_HEAD_T));
  memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
  aprsp->status = 0x01;
  aprsp->length= 0;

  if( 0 == req_ptr->uType){
    fd = open(WHOLE_PHONE_TEST_FILE_PATH,O_WRONLY);  //00: whole phone test; 01: PCBA test ; 02: BBAT test
  }else if(1 == req_ptr->uType){
    fd = open(PCBA_TEST_FILE_PATH,O_WRONLY);
  }else if(2 == req_ptr->uType){
    fd = open(BBAT_TEST_FILE_PATH,O_WRONLY);
  }
  if(fd < 0){
    ENG_LOG("%s open failed,type = %d\n",__FUNCTION__,req_ptr->uType);
    goto out;
  }
  write_len = write(fd,req_ptr->uBuff,sizeof(req_ptr->uBuff));
  if(write_len < 0){
    ENG_LOG("%s write failed!write_len = %d,type = %d\n",__FUNCTION__,write_len,req_ptr->uType);
    goto out;
  } else {
    fsync(fd);
  }

  aprsp->status = 0x00;

out:
  rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
  free(rsp_ptr);
  if(fd >= 0 )
    close(fd);

  return rsplen;
}

int eng_init_test_file(void)
{
  int i,len = 0;
  int fd_bbat = 1,fd_pcba = 1,fd_whole = -1;
  TEST_NEW_RESULT_INFO result[64] = {0};
  parse_config();

  if (0 != access(BBAT_TEST_FILE_PATH,F_OK)){
    fd_bbat = open(BBAT_TEST_FILE_PATH,O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd_bbat < 0 && (errno != EEXIST)) {
      ENG_LOG("%s,creat %s failed.",__FUNCTION__,BBAT_TEST_FILE_PATH);
      return 0;
    }
    //init /productinfo/BBATtest.txt
    for(i = 0;i < 64; i++){
      result[i].type_id = 2;
      result[i].function_id = i;
      result[i].support= bbat_support_result[i].support;
      result[i].status = 0;
    }
    len = write(fd_bbat,result,sizeof(result));
    if(len < 0){
      ENG_LOG("%s %s write_len = %d,type = %d\n",__FUNCTION__,BBAT_TEST_FILE_PATH,len);
    } else {
      fsync(fd_bbat);
    }
  }

  if (0 != access(PCBA_TEST_FILE_PATH,F_OK)){
    fd_pcba = open(PCBA_TEST_FILE_PATH,O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd_pcba < 0 && (errno != EEXIST)) {
      ENG_LOG("%s,creat %s failed.",__FUNCTION__,PCBA_TEST_FILE_PATH);
      return 0;
    }
    //init /productinfo/PCBAtest.txt
    for(i = 0;i < 64; i++){
      result[i].type_id = 1;
      result[i].function_id = i;
      result[i].support= support_result[i].support;
      result[i].status = 0;
    }
    result[0].support= 0;//lcd not support
    len = write(fd_pcba,result,sizeof(result));
    if(len < 0){
      ENG_LOG("%s write %s failed!write_len = %d,type = %d\n",__FUNCTION__,PCBA_TEST_FILE_PATH,len);
    } else {
      fsync(fd_pcba);
    }
  }

  if (0 != access(WHOLE_PHONE_TEST_FILE_PATH,F_OK)){
    fd_whole = open(WHOLE_PHONE_TEST_FILE_PATH,O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd_whole < 0 && (errno != EEXIST)) {
      ENG_LOG("%s,creat %s failed.",__FUNCTION__,WHOLE_PHONE_TEST_FILE_PATH);
      return 0;
    }
    //init /productinfo/wholetest.txt
    for(i = 0;i < 64; i++){
      result[i].type_id = 0;
      result[i].function_id= i;
      result[i].support= support_result[i].support;
      result[i].status = 0;
    }
    len = write(fd_whole,result,sizeof(result));
    if(len < 0){
      ENG_LOG("%s write %s failed! write_len = %d,type = %d\n",__FUNCTION__,WHOLE_PHONE_TEST_FILE_PATH,len);
    } else {
      fsync(fd_whole);
    }
  }
  if(fd_pcba >= 0)
    close(fd_pcba);
  if(fd_bbat >= 0)
    close(fd_bbat);
  if(fd_whole >= 0)
    close(fd_whole);

  return 1;
}

static int parse_string(char * buf, char gap, char* value)
{
  int len = 0;
  char *ch = NULL;
  char str[10] = {0};

  if(buf != NULL && value  != NULL){
    ch = strchr(buf, gap);
    if(ch != NULL){
      len = ch - buf ;
      strncpy(str, buf, len);
      *value = atoi(str);
    }
  }
  return len;
}

static int parse_2_entries(char *type, char* arg1, char* arg2)
{
  int len;
  char *str = type;

  /* sanity check */
  if(str == NULL) {
    ENG_LOG("type is null!");
    return -1;
  }

  if((len = parse_string(str, '\t', arg1)) <= 0)  return -1;
  str += len + 1;
  if(str == NULL) {
    ENG_LOG("mmitest type is null!");
    return -1;
  }
  if((len = parse_string(str, '\t', arg2)) <= 0)  return -1;

  return 0;
}

static int parse_config(void)
{
  FILE *fp;
  int ret = 0, count = 0;
  char id,flag;
  char buffer[MAX_LINE_LEN]={0};

  /*for PCBA*/
  fp = fopen(ENG_PCBA_SUPPORT_CONFIG, "r");
  if(fp == NULL) {
    ENG_LOG("mmitest open %s failed, %s", ENG_PCBA_SUPPORT_CONFIG,strerror(errno));
    return -1;
  }

  /* parse line by line */
  while(fgets(buffer, MAX_LINE_LEN, fp) != NULL) {
    if(buffer[0] == '#')
      continue;
    if((buffer[0]>='0') && (buffer[0]<='9')){
      ret = parse_2_entries(buffer,&id,&flag);
      if(ret != 0) {
        ENG_LOG("mmitest parse %s return %d.  reload", ENG_PCBA_SUPPORT_CONFIG,ret);
        fclose(fp);
        return -1;
      }
      support_result[count].id = id;
      support_result[count++].support= flag;
    }
  }

  fclose(fp);
  if(count < 64) {
    ENG_LOG("mmitest parse PCBA.conf failed");
  }

  /*for BBAT*/
  fp = fopen(ENG_BBAT_SUPPORT_CONFIG, "r");
  if(fp == NULL) {
    ENG_LOG("bbattest open %s failed, %s", ENG_BBAT_SUPPORT_CONFIG,strerror(errno));
    return -1;
  }

  /* parse line by line */
  ret = 0;
  count = 0;
  while(fgets(buffer, MAX_LINE_LEN, fp) != NULL) {
    if(buffer[0] == '#')
      continue;
    if((buffer[0]>='0') && (buffer[0]<='9')){
      ret = parse_2_entries(buffer,&id,&flag);
      if(ret != 0) {
        ENG_LOG("bbattest parse %s return %d.  reload", ENG_BBAT_SUPPORT_CONFIG,ret);
        fclose(fp);
        return -1;
      }
      bbat_support_result[count].id = id;
      bbat_support_result[count++].support= flag;
    }
  }

  fclose(fp);
  if(count < 64) {
    ENG_LOG("bbattest parse BBAT.conf failed");
  }

  return ret;
}

#if (defined TEE_PRODUCTION_CONFIG) && (!defined CONFIG_MINIENGPC)
static int eng_diag_tee_production(char *buf, int len, char *rsp, int rsplen)
{
  uint8_t *tee_msg;
  uint32_t tee_msg_len;
  uint8_t tee_rsp[ENG_TEE_RSP_LEN] = {0};
  uint32_t tee_rsp_len = 0;
  char *rsp_ptr;
  int ret = -1;
  unsigned short status = 0x01;
  TOOLS_DIAG_AP_CNF_T *aprsp;
  MSG_HEAD_T *msg_head_ptr = (MSG_HEAD_T*)(buf + 1);
  TOOLS_DIAG_AP_CMD_T *apbuf = (TOOLS_DIAG_AP_CMD_T *)(buf + 1 + sizeof(MSG_HEAD_T));

  if(NULL == buf){
    ENG_LOG("%s: null pointer",__FUNCTION__);
    return 0;
  }

  tee_msg = (uint8_t*)(buf + 1 + sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CMD_T));
  tee_msg_len = apbuf->length;

  ret =  TEECex_SendMsg_To_TEE(tee_msg, tee_msg_len, tee_rsp, &tee_rsp_len);

  if(0 != ret) {
    ENG_LOG("%s: TEECex_SendMsg_To_TEE() error ret=%d\n", __FUNCTION__, ret);
  } else {
    ENG_LOG("%s: TEECex_SendMsg_To_TEE() success\n", __FUNCTION__);
    status = 0x00;
  }

  rsplen = sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T) + tee_rsp_len;
  rsp_ptr = (char*)malloc(rsplen);
  if(NULL == rsp_ptr){
    ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
    return 0;
  }

  memcpy(rsp_ptr, msg_head_ptr, sizeof(MSG_HEAD_T));
  ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
  aprsp = (TOOLS_DIAG_AP_CNF_T*)(rsp_ptr + sizeof(MSG_HEAD_T));
  aprsp->length = tee_rsp_len;
  aprsp->status = status;
  memcpy(rsp_ptr + sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T), tee_rsp, tee_rsp_len);

  rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
  free(rsp_ptr);
  return rsplen;
}
#endif
