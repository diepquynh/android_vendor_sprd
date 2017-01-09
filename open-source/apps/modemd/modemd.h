#ifndef __MODEMD_H__

#define __MODEMD_H__

#define MODEMD_DEBUG

#ifdef MODEMD_DEBUG
#define MODEMD_LOGD(x...) ALOGD( x )
#define MODEMD_LOGE(x...) ALOGE( x )
#else
#define MODEMD_LOGD(x...) do {} while(0)
#define MODEMD_LOGE(x...) do {} while(0)
#endif

#define MODEM_READY                 0
#define MODEM_ASSERT                1
#define MODEM_RESET                 2

#define MAX_CLIENT_NUM              10
#define min(A,B)                    (((A) < (B)) ? (A) : (B))

/* indicate which modem is enable */
#define TD_MODEM_ENABLE_PROP        "persist.modem.t.enable"
#define W_MODEM_ENABLE_PROP        "persist.modem.w.enable"
#define LTE_MODEM_ENABLE_PROP        "persist.modem.l.enable"
#define LF_MODEM_ENABLE_PROP        "persist.modem.lf.enable"
#define TL_MODEM_ENABLE_PROP        "persist.modem.tl.enable"

// modem is external or not, default is internal
#define MODEM_EXTERNAL_PROP        "ro.modem.external.enable"

/* sim card num property */
char SP_SIM_NUM_PROP[20]; //ro.modem.*.count

/* stty interface property */
char SP_TTY_DEV_PROP[20]; //ro.modem.*.tty

#define TD_PROC_PROP                "ro.modem.t.dev"
#define W_PROC_PROP                 "ro.modem.w.dev"
#define LTE_PROC_PROP               "ro.modem.l.dev"
#define LF_PROC_PROP                "ro.modem.lf.dev"
#define TL_PROC_PROP                "ro.modem.tl.dev"
char SP_PROC_PROP[20]; // ro.modem.*.dev

#define DEFAULT_TD_PROC_DEV         "/proc/cpt/"
#define DEFAULT_W_PROC_DEV          "/proc/cpw/"
#define DEFAULT_LTE_PROC_DEV        "/proc/cpl/"
#define DEFAULT_LF_PROC_DEV         "/proc/cplf/"
#define DEFAULT_TL_PROC_DEV         "/proc/cptl/"

#define MODEM_BANK               "modem"
#define DSP_BANK                 "dsp"
#define TGDSP_BANK               "tgdsp"
#define LDSP_BANK                "ldsp"
#define WARM_BANK                "warm"
#define FIXNV_BANK               "fixnv"
#define RUNNV_BANK               "runnv"
#define MODEM_START              "start"
#define MODEM_STOP               "stop"

/* load modem image interface */
#define TD_MODEM_BANK               "/proc/cpt/modem"
#define TD_DSP_BANK                 "/proc/cpt/dsp"
#define TD_MODEM_START              "/proc/cpt/start"
#define TD_MODEM_STOP               "/proc/cpt/stop"
#define W_MODEM_BANK                "/proc/cpw/modem"
#define W_DSP_BANK                  "/proc/cpw/dsp"
#define W_MODEM_START               "/proc/cpw/start"
#define W_MODEM_STOP                "/proc/cpw/stop"

/* modem/dsp partition */
#define TD_MODEM_SIZE               (10*1024*1024)
#define TD_DSP_SIZE                 (5*1024*1024)
#define W_MODEM_SIZE                (10*1024*1024)
#define W_DSP_SIZE                  (5*1024*1024)

#define L_MODEM_SIZE               (0xbb0000)
#define L_TGDSP_SIZE               (0x2e0000)
#define L_LDSP_SIZE                (0x1c0000)
#define L_WARM_SIZE                (0x280000)

#define LF_MODEM_SIZE               (0x9b0000)
#define LF_TGDSP_SIZE               (0x2e0000)
#define LF_LDSP_SIZE                (0x1c0000)
#define LF_WARM_SIZE                (0x280000)

#define TL_MODEM_SIZE               (0x8b0000)
#define TL_TGDSP_SIZE               (0x2e0000)
#define TL_LDSP_SIZE                (0x1c0000)

char RO_SP_MODEM_FIXNVSIZE[50]; //ro.modem.*.fixnv_size
char RO_SP_MODEM_RUNNVSIZE[50]; //ro.modem.*.runnv_size

/* detect assert/hangup interface */
char SP_ASSERT_PROP[25]; // ro.modem.*.assert
char SP_LOOP_PROP[20]; // ro.modem.*.loop

/* default value for detect assert/hangup interface */
#define DEFAULT_TD_ASSERT_DEV         "/dev/spipe_td2"
#define DEFAULT_W_ASSERT_DEV         "/dev/spipe_w2"

/*two spipe_lte2?? */
#define DEFAULT_TL_ASSERT_DEV         "/dev/spipe_lte2"
#define DEFAULT_LF_ASSERT_DEV         "/dev/spipe_lte2"
#define DEFAULT_L_ASSERT_DEV         "/dev/spipe_lte2"

#define DEFAULT_TD_LOOP_DEV         "/dev/spipe_td0"
#define DEFAULT_W_LOOP_DEV         "/dev/spipe_w0"
/*two spipe_lte0?? */
#define DEFAULT_TL_LOOP_DEV         "/dev/spipe_lte0"
#define DEFAULT_LF_LOOP_DEV         "/dev/spipe_lte0"
#define DEFAULT_L_LOOP_DEV         "/dev/spipe_l0"

#define TD_WATCHDOG_DEV              "/proc/cpt/wdtirq"
#define W_WATCHDOG_DEV               "/proc/cpw/wdtirq"
#define TL_WATCHDOG_DEV              "/proc/cptl/wdtirq"
#define LF_WATCHDOG_DEV               "/proc/cptl/wdtirq"
#define L_WATCHDOG_DEV               "/proc/cptl/wdtirq"

#define TTY_DEV_PROP             "persist.ttydev"
#define PHONE_APP_PROP             "sys.phone.app"
#define MODEMRESET_PROPERTY          "persist.sys.sprd.modemreset"
#define MODEM_RESET_PROP             MODEMRESET_PROPERTY
#define LTE_MODEM_START_PROP         "ril.service.l.enable"
#define SSDA_MODE_PROP               "persist.radio.ssda.mode"
#define SSDA_TESTMODE_PROP           "persist.radio.ssda.testmode"
#define SVLTE_MODE                   "svlte"

#define PHONE_APP             "com.android.phone"
#define MODEM_SOCKET_NAME         "modemd"
char PHS_SOCKET_NAME[10]; // phst/phsw/phsl/phstl/phslf
#define MODEM_ENGCTRL_NAME           "modemd_engpc"
#define MODEM_ENGCTRL_PRO            "persist.sys.engpc.disable"

#define ENGPC_REQUSETY_OPEN          "0"
#define ENGPC_REQUSETY_CLOSE         "1"
#define SSDA_TEST_MODE_SVLTE          0

#define IMEI1_CONFIG_FILE    "/productinfo/imei1.txt"
#define IMEI2_CONFIG_FILE    "/productinfo/imei2.txt"
#define IMEI3_CONFIG_FILE    "/productinfo/imei3.txt"
#define IMEI4_CONFIG_FILE    "/productinfo/imei4.txt"
#define MAX_IMEI_STR_LENGTH  15
#define IMEI_NUM             4
#define MINIDUMP             1
#define SEARCH_NETWORK       2

#define PERSIST_MODEM_CHAR        "persist.modem."

static pthread_mutex_t aliveMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t aliveCond = PTHREAD_COND_INITIALIZER;
static int modem_alive =0;
static int control_fd = -1;

typedef enum {
    LTE_MODEM_DEFAULT = -1,
    LTE_MODEM_OFF,
    LTE_MODEM_ON
}LTE_MODEM_START_E;


extern int  client_fd[];

int stop_service(char* modem, int is_vlx);
int start_service(char* modem, int is_vlx, int restart);

int write_proc_file(char *file, int offset, char *string);
int vlx_reboot_init(void);
int detect_vlx_modem(char *modem);
void start_modem(char *param);

void* detect_modem_blocked(void *param);

extern void exit_modemd(void);
int open_modem_dev(char *path);
int wait_for_alive(int modem, int is_assert);
int detect_clients_dispose_state(int cause);

#endif
