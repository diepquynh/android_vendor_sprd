#ifndef __WCND_H__
#define __WCND_H__


#include "wcnd_config.h"

#define WCND_DEBUG
#define WCND_WIFI_CONFIG_FILE_PATH "/data/misc/wifi/wifimac.txt"
#define WCND_WIFI_FACTORY_CONFIG_FILE_PATH "/productinfo/wifimac.txt"
#define WCND_BT_CONFIG_FILE_PATH "/data/misc/bluedroid/btmac.txt"
#define WCND_BT_FACTORY_CONFIG_FILE_PATH "/productinfo/btmac.txt"
#define MAC_LEN 6

#ifdef WCND_DEBUG
#define WCND_LOGD(x...) ALOGD( x )
#define WCND_LOGE(x...) ALOGE( x )
#else
#define WCND_LOGD(x...) do {} while(0)
#define WCND_LOGE(x...) do {} while(0)
#endif

#ifndef TEMP_FAILURE_RETRY
/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif


#ifdef USE_MARLIN

//the CP2(WIFI/BT/FM) device node
#define WCN_DEV				"NOT-USED"

//when what to read is that has been to write, then it indicate the CP2(WIFI/BT/FM) is OK
#define WCN_LOOP_IFACE		"/proc/mdbg/loopcheck"

//when assert happens, related information will write to this interface
#define WCN_ASSERT_IFACE		"/proc/mdbg/assert"

//The interface to communicate with slog
#define WCN_SLOG_IFACE		"NOT-USED"

//write '1' to this interface, the CP2(WIFI/BT/FM) will stop
#define WCN_STOP_IFACE		"NOT-USED"

//write '1' to this interface, the CP2(WIFI/BT/FM) will start to boot up
#define WCN_START_IFACE		"NOT-USED"

//write image file to this interface, then the image will be download to CP2(WIFI/BT/FM)
#define WCN_DOWNLOAD_IFACE	"NOT-USED"

//read from this interface, the memory infor will be read from CP2(WIFI/BT/FM)
#define WCN_DUMP_IFACE		"NOT-USED"

//when an exception of watdog happens, related infor will write to this interface
#define WCN_WATCHDOG_IFACE	"/proc/mdbg/wdtirq"

//can read the status infor from this interface
#define WCN_STATUS_IFACE		"NOT-USED"


//send at cmd to this interface
#define WCN_ATCMD_IFACE		"/proc/mdbg/at_cmd"


#define LOOP_TEST_STR "at+loopcheck\r"
#define LOOP_TEST_ACK_STR "loopcheck_ack"

#else

//the CP2(WIFI/BT/FM) device node
#define WCN_DEV				"/dev/cpwcn"

//when what to read is that has been to write, then it indicate the CP2(WIFI/BT/FM) is OK
#define WCN_LOOP_IFACE		"/dev/spipe_wcn0"

//when assert happens, related information will write to this interface
#define WCN_ASSERT_IFACE		"/dev/spipe_wcn2"

//The interface to communicate with slog
#define WCN_SLOG_IFACE		"/dev/slog_wcn"

//write '1' to this interface, the CP2(WIFI/BT/FM) will stop
#define WCN_STOP_IFACE		"/proc/cpwcn/stop"

//write '1' to this interface, the CP2(WIFI/BT/FM) will start to boot up
#define WCN_START_IFACE		"/proc/cpwcn/start"

//write image file to this interface, then the image will be download to CP2(WIFI/BT/FM)
#define WCN_DOWNLOAD_IFACE	"/proc/cpwcn/modem"

//read from this interface, the memory infor will be read from CP2(WIFI/BT/FM)
#define WCN_DUMP_IFACE		"/proc/cpwcn/mem"

//when an exception of watdog happens, related infor will write to this interface
#define WCN_WATCHDOG_IFACE	"/proc/cpwcn/wdtirq"

//can read the status infor from this interface
#define WCN_STATUS_IFACE		"/proc/cpwcn/status"


//send at cmd to this interface
#define WCN_ATCMD_IFACE		"/dev/spipe_wcn5"


#define LOOP_TEST_STR "hi"
#define LOOP_TEST_ACK_STR "hi"


#endif





#define USER_DEBUG_VERSION_STR "userdebug"


//the key used by property_get() to get partition path
#define PARTITION_PATH_PROP_KEY	"ro.product.partitionpath"

//the key used to get the build type
#define BUILD_TYPE_PROP_KEY "ro.build.type"

#define WCND_MODEM_ENABLE_PROP_KEY "ro.modem.wcn.enable"

#define WCND_RESET_PROP_KEY "persist.sys.sprd.wcnreset"
#define WCND_SLOG_ENABLE_PROP_KEY "persist.sys.sprd.wcnlog"
#define WCND_SLOG_RESULT_PROP_KEY "persist.sys.sprd.wcnlog.result"
#define WCND_ENGCTRL_PROP_KEY "persist.sys.engpc.disable"

#define WCND_CP2_STATE_PROP_KEY "persist.sys.wcn.status"



//the image file
#define WCN_IMAGE_NAME	"wcnmodem"

#define WCN_IMAGE_SIZE	(1*1024*1024)	//max size is 1M

#define WCND_SOCKET_NAME	"wcnd"
#define WCND_ENG_SOCKET_NAME	"wcnd_eng"



#define WCND_CP2_EXCEPTION_STRING "WCN-CP2-EXCEPTION"
#define WCND_CP2_RESET_START_STRING "WCN-CP2-RESET-START"
#define WCND_CP2_RESET_END_STRING "WCN-CP2-RESET-END"
#define WCND_CP2_ALIVE_STRING "WCN-CP2-ALIVE"
#define WCND_CP2_CLOSED_STRING "WCN-CP2-CLOSED"


#define WCND_CP2_DEFAULT_CP2_VERSION_INFO "Fail: UNKNOW VERSION"

#define WCND_ENG_AUTOTEST_STRING "engtest autotest=1"


//the marlin2 indentity string
#define MARLIN2_IDNENTITY "marlin2"

#define MARLIN_CHIP_TYPE_1 (0) //for marlin
#define MARLIN_CHIP_TYPE_2 (1) //for marlin2

#define MARLIN2_POWEROFF_IDNENTITY "poweroff"


typedef struct structWcndWorker {
	int (*handler)(void *ctx);
	void *ctx;
	void *data;
	int replyto_fd; //fd to replay message
	struct structWcndWorker *next;
} WcndWorker;


typedef struct structWcndMessage{
	int event;
	int replyto_fd; //fd to replay message
}WcndMessage;

typedef struct structWcndClient{
	int sockfd;
	int type;//to identify if it is a socket for sending cmds or just for listening event
}WcndClient;

typedef int (*cmd_handler)(int client_fd, int argc, char* argv[]);

typedef struct structWcnCmdExecuter{
	char* name;
	int (*runcommand)(int client_fd, int argc, char* argv[]);
}WcnCmdExecuter;

#define WCND_MAX_CLIENT_NUM	(10)

#define WCND_MAX_IFACE_NAME_SIZE		(32)

#define WCND_MAX_CMD_EXECUTER_NUM		(10)

typedef struct structWcndManager
{
	//identify if the struct is initialized or not.
	int inited;

	pthread_mutex_t clients_lock;

	//to store the sockets that connect from the clients
	WcndClient clients[WCND_MAX_CLIENT_NUM];

	//the server socket to listen for client to connect
	int listen_fd;

	char wcn_assert_iface_name[WCND_MAX_IFACE_NAME_SIZE];
	char wcn_loop_iface_name[WCND_MAX_IFACE_NAME_SIZE];
	char wcn_watchdog_iface_name[WCND_MAX_IFACE_NAME_SIZE];

	char wcn_stop_iface_name[WCND_MAX_IFACE_NAME_SIZE];
	char wcn_start_iface_name[WCND_MAX_IFACE_NAME_SIZE];
	char wcn_download_iface_name[WCND_MAX_IFACE_NAME_SIZE];
	char wcn_image_file_name[WCND_MAX_IFACE_NAME_SIZE];
	char wcn_atcmd_iface_name[WCND_MAX_IFACE_NAME_SIZE];

	int wcn_image_file_size;

	/*
	* count the notify information send, at the same time use as
	* a notify_id of the notify information send every time.
	* In order to identy the reponse of the notify information from
	* client, that means that the client must send back the notity_id in
	* the reponse of the notify information.
	*/
	int notify_count;

	pthread_mutex_t cmdexecuter_list_lock;

	/**
	* Cmd executer to execute the cmd send from clients
	*/
	const WcnCmdExecuter* cmdexecuter_list[WCND_MAX_CMD_EXECUTER_NUM];

	//to identify if a reset process is going on
	int doing_reset;
	//to identify if CP2 exception happened
	int is_cp2_error;
	//to identify if in the userdebug version
	int is_in_userdebug;

	//wcnd state
	int state;

	//bt/wifi state
	int btwifi_state;

	//saved bt/wifi state when assert happens
	int saved_btwifi_state;

	//pending events that will be handled in next state
	int pending_events;

	//self cmd socket pair to send cmd to self
	int selfcmd_sockets[2];

	//identify if enable to send notify
	int notify_enabled;

	//save the cp2 version info
	char cp2_version_info[256];
	int store_cp2_versin_done;


	pthread_mutex_t worker_lock;
	pthread_cond_t worker_cond;

	//engineer mode cmds queue
	WcndWorker *eng_cmd_queue;

	//to identify if the wcn modem (CP2) is enabled or not.
	//to check property "ro.modem.wcn.enable"
	int is_wcn_modem_enabled;

	//to identify if only for engineer mode, if yes, then the CP2 listen/loop check will be disabled
	int is_eng_mode_only;

	//to saved the cp2 log state
	int is_cp2log_opened;

	//to wait wifi driver unloaded before reset.
	int wait_wifi_driver_unloaded;

	//to identify if in the engpc autotest mode
	int eng_autotest;

	//dump mem in user mode??
	int dumpmem_on;

	//to identity marlin chip
	int marlin_type;
}WcndManager;


//CP2 AT Cmds
#define WCND_ATCMD_CP2_SLEEP "at+cp2sleep\r"
#define WCND_ATCMD_CP2_DISABLE_LOG "AT+ARMLOG=0\r"
#define WCND_ATCMD_CP2_ENABLE_LOG "AT+ARMLOG=1\r"
#define WCND_ATCMD_CP2_GET_VERSION "at+spatgetcp2info\r"
#define WCND_ATCMD_CP2_ENTER_USER "at+cp2_enter_user=1\r"
#define WCND_ATCMD_CP2_EXIT_USER "at+cp2_enter_user=0\r"


//cmd id to identify the cmds that the wcnd care about
#define WCND_ATCMD_CMDID_OTHERS (0)
#define WCND_ATCMD_CMDID_GET_VERSION (1)
#define WCND_ATCMD_CMDID_SLEEP (2)

//export API
WcndManager* wcnd_get_default_manager(void);
int wcnd_register_cmdexecuter(WcndManager *pWcndManger, const WcnCmdExecuter *pCmdExecuter);
int wcnd_send_back_cmd_result(int client_fd, char *str, int isOK);
int wcnd_send_notify_to_client(WcndManager *pWcndManger, char *info_str, int notify_type);
int wcnd_do_wcn_reset_process(WcndManager *pWcndManger);
int wcnd_runcommand(int client_fd, int argc, char* argv[]);
int wcnd_process_atcmd(int client_fd, char *atcmd_str, WcndManager *pWcndManger);
int wcnd_send_selfcmd(WcndManager *pWcndManger, char *cmd);


int wcnd_reboot_cp2(WcndManager *pWcndManger);
int wcnd_stop_cp2(WcndManager *pWcndManger);
int wcnd_start_cp2(WcndManager *pWcndManger);
int wcnd_before_reset(WcndManager *pWcndManger);
int wcnd_dump_cp2_mem(WcndManager *pWcndManger);


int wcnd_open_cp2(WcndManager *pWcndManger);
int wcnd_close_cp2(WcndManager *pWcndManger);
int wcnd_config_cp2_bootup(WcndManager *pWcndManger);
int wcnd_dump_cp2(WcndManager *pWcndManger);


int wcnd_worker_init(WcndManager *pWcndManger);
int wcnd_worker_dispatch(WcndManager *pWcndManger, int (*handler)(void *), char *data, int fd, int type);
int wcnd_woker_handle(void *worker);

//for marlin2
int wcnd_set_marlin2_poweron(WcndManager *pWcndManger, int poweron);

#endif
