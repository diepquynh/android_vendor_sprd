#ifndef _SS_CMD_H
#define _SS_CMD_H

#ifdef TIZEN_EXT
#include <linux/types.h>
#include <sys/ioctl.h>
#include <dlog.h>
#else
#include <utils/Log.h>
#endif
#include <netlink/socket.h>
#include <netlink/types.h>
#include <netlink/handlers.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/socket.h>
#include <net/if.h>
#include <endian.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG     "sprdrftlib"

#define WLAN_NL_GENERAL_SOCK_ID     (1022)
#define WLNPI_WLAN0_NAME            ("wlan0")
#ifdef TIZEN_EXT
#define ENG_LOG  LOGD
#else
#define ENG_LOG  ALOGD
#endif
#define SS_SUCCESS	(0x0) //return success
#define SS_FAILED	(-0x1)//return failed
#define RET_ERR		(0x0) //samsung test lib success
#define RET_SUC		(0x1) //samsung test lib failed

#define FSM_IDLE	(0x96)
#define FSM_WORK	(0x69)
#define DEV_UP		(0x01)
#define DEV_DOWN	(0x0)

#define ERRSIZE		(1024)

#ifdef TIZEN_EXT
#define ALOGD	LOGD
#define ALOGE	LOGE
#endif

#define pr_err(fmt,arg...)							\
{													\
			extern struct ss_cmd ss_cmd_pri;		\
			snprintf(ss_cmd_pri.errbuf, ERRSIZE,	\
					 "ERROR:"fmt, ## arg);		\
            ALOGE(fmt,## arg);						\
}

#ifndef SS_CMD_DEBUG
#define SSPRINT(fmt,arg...)	\
            ALOGD(fmt,## arg)

#else
#define SSPRINT(arg...)
#endif

#define INSMOD_WLAN			("insmod /lib/modules/sprdwl_ng.ko")
#define RMMOD_WLAN			("rmmod /lib/modules/sprdwl_ng.ko")
#define IFNAME				("wlan0")

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#define CHANNEL(x) (1 << (x))

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define WPA_SCAN_QUAL_INVALID		BIT(0)
#define WPA_SCAN_NOISE_INVALID		BIT(1)
#define WPA_SCAN_LEVEL_INVALID		BIT(2)
#define WPA_SCAN_LEVEL_DBM			BIT(3)
#define WPA_SCAN_AUTHENTICATED		BIT(4)
#define WPA_SCAN_ASSOCIATED			BIT(5)

#define CMD2S(x)	(#x)
#define construct_cmd(cmd_c,cmd_v,cmd_name,org_param) 	\
{														\
	int index = 2;										\
														\
	cmd_v[0] = CMD2S(wlan0);							\
	cmd_v[1] = cmd_name;								\
														\
	for (; index < cmd_c; index++) {					\
		if (org_param == (void *)0) { 					\
			SSPRINT("error:cmd param is null\n");		\
		} else {										\
			cmd_v[index] = *(org_param + index - 2);	\
		}												\
	}													\
}

/*join ssid information define*/
#define WPAS_MAX_SCAN_SSIDS 	16
#define MAX_REPORT_FREQS 	50
#define ETH_ALEN		6
/* Information Element IDs */
#define WLAN_EID_SSID 0
#define WLAN_EID_SUPP_RATES 1
#define WLAN_EID_FH_PARAMS 2
#define WLAN_EID_DS_PARAMS 3
#define WLAN_EID_CF_PARAMS 4
#define WLAN_EID_TIM 5
#define WLAN_EID_IBSS_PARAMS 6
#define WLAN_EID_COUNTRY 7
#define WLAN_EID_CHALLENGE 16

#ifdef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */
#define nl_handle nl_sock
#define nl80211_handle_alloc nl_socket_alloc_cb
#define nl80211_handle_destroy nl_socket_free
#endif /* CONFIG_LIBNL20 */
#define MAX_DRV_CMD_SIZE 248
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;
struct family_data {
	const char *group;
	int id;
};
struct nl80211_global{
	struct nl_handle *nl;
	struct nl_handle *nl_event;
	struct nl_cb *nl_cb;
	int nl80211_id;
	int ifindex;

	char ssid[33];

	unsigned char ssid_len;
	int freq;
	u8 bssid[6];
	bool find;
	bool scan_done;
	bool connected;
	bool connect_finish;
	int ioctl_sock;
	unsigned int chan_bitmap;
};
typedef long os_time_t;
struct os_time {
	os_time_t sec;
	os_time_t usec;
};
struct wpa_scan_res {
	unsigned int flags;
	u8 bssid[ETH_ALEN];
	int freq;
	u16 beacon_int;
	u16 caps;
	int qual;
	int noise;
	int level;
	u64 tsf;
	unsigned int age;
	size_t ie_len;
	size_t beacon_ie_len;
	/*
	 * Followed by ie_len octets of IEs from Probe Response frame (or if
	 * the driver does not indicate source of IEs, these may also be from
	 * Beacon frame). After the first set of IEs, another set of IEs may
	 * follow (with beacon_ie_len octets of data) if the driver provides
	 * both IE sets.
	 */
};
/**
 * struct wpa_scan_results - Scan results
 * @res: Array of pointers to allocated variable length scan result entries
 * @num: Number of entries in the scan result array
 * @fetch_time: Time when the results were fetched from the driver
 */
struct wpa_scan_results {
	struct wpa_scan_res **res;
	size_t num;
	struct os_time fetch_time;
};
struct nl80211_bss_info_arg {
	struct nl80211_global *global;
	struct wpa_scan_results *res;
	unsigned int assoc_freq;
	u8 assoc_bssid[ETH_ALEN];
};
typedef enum _DataRate {
	DATA_RATE_UNSUPPORT, //for index 0
	DATA_RATE_1M,
	DATA_RATE_2M,
	DATA_RATE_5_5M,
	DATA_RATE_6M,
	DATA_RATE_9M,
	DATA_RATE_11M,
	DATA_RATE_12M,
	DATA_RATE_18M,
	DATA_RATE_22M,
	DATA_RATE_24M,
	DATA_RATE_33M,
	DATA_RATE_36M,
	DATA_RATE_48M,
	DATA_RATE_54M,
	DATA_RATE_MCS0,
	DATA_RATE_MCS1,
	DATA_RATE_MCS2,
	DATA_RATE_MCS3,
	DATA_RATE_MCS4,
	DATA_RATE_MCS5,
	DATA_RATE_MCS6,
	DATA_RATE_MCS7,
	DATA_RATE_MCS8,
	DATA_RATE_MCS9,
	DATA_RATE_MCS10,
	DATA_RATE_MCS11,
	DATA_RATE_MCS12,
	DATA_RATE_MCS13,
	DATA_RATE_MCS14,
	DATA_RATE_MCS15,
} DataRate;

typedef enum _AntennaType {
	ALL,
	ANT_1,
	ANT_2,
} AntennaType;

typedef enum ss_cmd_id{
	OpenDUT_ID = 0,
	CloseDUT_ID,
	SetChannel_ID,
	SetDataRate_ID,
	SetLongPreamble_ID,
	SetShortGuardInterval_ID,
	TxGain_ID,
	SetBurstInterval_ID,
	SetPayload_ID,
	SetBand_ID,
	SetBandWidth_ID,
	TxStartWithMod_ID,
	TxStartWoMod_ID,
	TxStop_ID,
	RxStart_ID,
	RxStop_ID,
	GetGoodFrame_ID,
	GetErrorFrame_ID,
	JoinWithSSID_ID,
	JoinWithSSID2G_ID,
	JoinWithSSID5G_ID,
	SetAntenna_ID,
	GetErrorString_ID,
	GetRSSI_ID,
	SetTxCount_ID,
	StartStaMode_ID,
	StopStaMode_ID,
	StartNpiMode_ID,
	StopNpiMode_ID,
	TriggerScan_ID,
	SetCountryCode_ID,
	GetChannelist_ID,
	AddInterface_ID,
	DelInterface_ID,
	SetScanTime_ID,
	SetWlanCap_ID,
	SetLnaOn_ID,
	SetLnaOff_ID,
	/*samsung cmd end*/
	CMD_ID_MAX,
}SS_CMD_ID;
typedef enum{
	FSM_UNDEFINE,
	FSM_NPI_MODE,
	FSM_STA_MODE,
}SS_FSM;
struct ss_cmd {
	unsigned int good_rx_frame;
	unsigned int err_rx_frame;
	unsigned int fcs_fail_frame;
	int rssi;
	DataRate rate;
	int error_id;
	int long_preamble;
	SS_FSM state;
	bool open;
	char errbuf[ERRSIZE];
};
enum nl80211_iftype {
	NL80211_IFTYPE_UNSPECIFIED,
	NL80211_IFTYPE_ADHOC,
	NL80211_IFTYPE_STATION,
	NL80211_IFTYPE_AP,
	NL80211_IFTYPE_AP_VLAN,
	NL80211_IFTYPE_WDS,
	NL80211_IFTYPE_MONITOR,
	NL80211_IFTYPE_MESH_POINT,
	NL80211_IFTYPE_P2P_CLIENT,
	NL80211_IFTYPE_P2P_GO,
	NL80211_IFTYPE_P2P_DEVICE,

	/* keep last */
	NUM_NL80211_IFTYPES,
	NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};

int RFT_OpenDUT(void);
int RFT_CloseDUT(void);
int RFT_SetChannel(int channel);
int RFT_SetDataRate(DataRate rate);
int RFT_SetLongPreamble(int enable);
int RFT_SetShortGuardInterval(int enable);
int RFT_TxGain(int txpwr);
int RFT_SetBurstInterval(int burstinterval);
int RFT_SetPayload(int size);
int RFT_SetBand(int band);
int RFT_SetBandWidth(int width);
int RFT_TxStartWithMod(void);
int RFT_TxStartWoMod(void);
int RFT_TxStop(void);
int RFT_RxStart(void);
int RFT_RxStop(void);
int RFT_GetGoodFrame(void);
int RFT_GetErrorFrame(void);
int RFT_JoinWithSSID(const char* ssid);
int RFT_JoinWithSSID2G(const char* ssid);
int RFT_JoinWithSSID5G(const char* ssid);
int RFT_SetAntenna(AntennaType ant);
const char *RFT_GetErrorString(void);
int RFT_GetRSSI(void);
int RFT_SetTxCount(unsigned int size);
int StartStaMode(void);
int StopStaMode(void);
int StartNpiMode(void);
int StopNpiMode(void);
int TriggerScan(void);
int SetCountryCode(const char *s);
int GetChannelist(void);
int AddInterface(const char *s);
int DelInterface(const char *s);
int SetScanTime(const char *s);
int SetWlanCap(unsigned int cap);
int SetLnaOn(void);
int SetLnaOff(void);
int RFT_SetPsCap(void);
/*cmd end*/
int iw_nl80211_init(struct nl80211_global *global);
void nl80211_iw_cleanup(struct nl80211_global *global);
int nl80211_join_ssid(struct nl80211_global *global);
int nl80211_set_interface(struct nl80211_global *global, enum nl80211_iftype mode);
int process_global_event(struct nl_msg *msg,void *arg);
void set_join_ssid(struct nl80211_global *global ,const char *ssid);
int wpa_driver_nl80211_scan(struct nl80211_global *global);
int nl80211_trigger_scan(struct nl80211_global *global);
int select_nl80211(struct nl80211_global *global,int time, bool *state,bool expect);
int linux_set_iface_flags(int sock, const char *ifname, int dev_up);
int wpa_driver_nl80211_set_country(struct nl80211_global *global, const char *alpha2_arg);
int nl80211_send_get_reg(struct nl80211_global *global);
int nl80211_create_iface_once(struct nl80211_global *global,
				     const char *ifname,
				     enum nl80211_iftype iftype,
				     const u8 *addr, int wds,
				     int (*handler)(struct nl_msg *, void *),
				     void *arg);
int nl80211_remove_iface(struct nl80211_global *global, const char *ifname);
int android_priv_cmd(struct nl80211_global *global, const char *cmd);
enum nl80211_commands {
/* don't change the order or add anything between, this is ABI! */
	NL80211_CMD_UNSPEC,

	NL80211_CMD_GET_WIPHY,		/* can dump */
	NL80211_CMD_SET_WIPHY,
	NL80211_CMD_NEW_WIPHY,
	NL80211_CMD_DEL_WIPHY,

	NL80211_CMD_GET_INTERFACE,	/* can dump */
	NL80211_CMD_SET_INTERFACE,
	NL80211_CMD_NEW_INTERFACE,
	NL80211_CMD_DEL_INTERFACE,

	NL80211_CMD_GET_KEY,
	NL80211_CMD_SET_KEY,
	NL80211_CMD_NEW_KEY,
	NL80211_CMD_DEL_KEY,

	NL80211_CMD_GET_BEACON,
	NL80211_CMD_SET_BEACON,
	NL80211_CMD_START_AP,
	NL80211_CMD_NEW_BEACON = NL80211_CMD_START_AP,
	NL80211_CMD_STOP_AP,
	NL80211_CMD_DEL_BEACON = NL80211_CMD_STOP_AP,

	NL80211_CMD_GET_STATION,
	NL80211_CMD_SET_STATION,
	NL80211_CMD_NEW_STATION,
	NL80211_CMD_DEL_STATION,

	NL80211_CMD_GET_MPATH,
	NL80211_CMD_SET_MPATH,
	NL80211_CMD_NEW_MPATH,
	NL80211_CMD_DEL_MPATH,

	NL80211_CMD_SET_BSS,

	NL80211_CMD_SET_REG,
	NL80211_CMD_REQ_SET_REG,

	NL80211_CMD_GET_MESH_CONFIG,
	NL80211_CMD_SET_MESH_CONFIG,

	NL80211_CMD_SET_MGMT_EXTRA_IE /* reserved; not used */,

	NL80211_CMD_GET_REG,

	NL80211_CMD_GET_SCAN,
	NL80211_CMD_TRIGGER_SCAN,
	NL80211_CMD_NEW_SCAN_RESULTS,
	NL80211_CMD_SCAN_ABORTED,

	NL80211_CMD_REG_CHANGE,

	NL80211_CMD_AUTHENTICATE,
	NL80211_CMD_ASSOCIATE,
	NL80211_CMD_DEAUTHENTICATE,
	NL80211_CMD_DISASSOCIATE,

	NL80211_CMD_MICHAEL_MIC_FAILURE,

	NL80211_CMD_REG_BEACON_HINT,

	NL80211_CMD_JOIN_IBSS,
	NL80211_CMD_LEAVE_IBSS,

	NL80211_CMD_TESTMODE,

	NL80211_CMD_CONNECT,
	NL80211_CMD_ROAM,
	NL80211_CMD_DISCONNECT,

	NL80211_CMD_SET_WIPHY_NETNS,

	NL80211_CMD_GET_SURVEY,
	NL80211_CMD_NEW_SURVEY_RESULTS,

	NL80211_CMD_SET_PMKSA,
	NL80211_CMD_DEL_PMKSA,
	NL80211_CMD_FLUSH_PMKSA,

	NL80211_CMD_REMAIN_ON_CHANNEL,
	NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL,

	NL80211_CMD_SET_TX_BITRATE_MASK,

	NL80211_CMD_REGISTER_FRAME,
	NL80211_CMD_REGISTER_ACTION = NL80211_CMD_REGISTER_FRAME,
	NL80211_CMD_FRAME,
	NL80211_CMD_ACTION = NL80211_CMD_FRAME,
	NL80211_CMD_FRAME_TX_STATUS,
	NL80211_CMD_ACTION_TX_STATUS = NL80211_CMD_FRAME_TX_STATUS,

	NL80211_CMD_SET_POWER_SAVE,
	NL80211_CMD_GET_POWER_SAVE,

	NL80211_CMD_SET_CQM,
	NL80211_CMD_NOTIFY_CQM,

	NL80211_CMD_SET_CHANNEL,
	NL80211_CMD_SET_WDS_PEER,

	NL80211_CMD_FRAME_WAIT_CANCEL,

	NL80211_CMD_JOIN_MESH,
	NL80211_CMD_LEAVE_MESH,

	NL80211_CMD_UNPROT_DEAUTHENTICATE,
	NL80211_CMD_UNPROT_DISASSOCIATE,

	NL80211_CMD_NEW_PEER_CANDIDATE,

	NL80211_CMD_GET_WOWLAN,
	NL80211_CMD_SET_WOWLAN,

	NL80211_CMD_START_SCHED_SCAN,
	NL80211_CMD_STOP_SCHED_SCAN,
	NL80211_CMD_SCHED_SCAN_RESULTS,
	NL80211_CMD_SCHED_SCAN_STOPPED,

	NL80211_CMD_SET_REKEY_OFFLOAD,

	NL80211_CMD_PMKSA_CANDIDATE,

	NL80211_CMD_TDLS_OPER,
	NL80211_CMD_TDLS_MGMT,

	NL80211_CMD_UNEXPECTED_FRAME,

	NL80211_CMD_PROBE_CLIENT,

	NL80211_CMD_REGISTER_BEACONS,

	NL80211_CMD_UNEXPECTED_4ADDR_FRAME,

	NL80211_CMD_SET_NOACK_MAP,

	NL80211_CMD_CH_SWITCH_NOTIFY,

	NL80211_CMD_START_P2P_DEVICE,
	NL80211_CMD_STOP_P2P_DEVICE,

	NL80211_CMD_CONN_FAILED,

	NL80211_CMD_SET_MCAST_RATE,

	NL80211_CMD_SET_MAC_ACL,

	NL80211_CMD_RADAR_DETECT,

	NL80211_CMD_GET_PROTOCOL_FEATURES,

	NL80211_CMD_UPDATE_FT_IES,
	NL80211_CMD_FT_EVENT,

	NL80211_CMD_CRIT_PROTOCOL_START,
	NL80211_CMD_CRIT_PROTOCOL_STOP,

	/* add new commands above here */

	/* used to define NL80211_CMD_MAX below */
	__NL80211_CMD_AFTER_LAST,
	NL80211_CMD_MAX = __NL80211_CMD_AFTER_LAST - 1
};
enum nl80211_attrs {
/* don't change the order or add anything between, this is ABI! */
	NL80211_ATTR_UNSPEC,

	NL80211_ATTR_WIPHY,
	NL80211_ATTR_WIPHY_NAME,

	NL80211_ATTR_IFINDEX,
	NL80211_ATTR_IFNAME,
	NL80211_ATTR_IFTYPE,

	NL80211_ATTR_MAC,

	NL80211_ATTR_KEY_DATA,
	NL80211_ATTR_KEY_IDX,
	NL80211_ATTR_KEY_CIPHER,
	NL80211_ATTR_KEY_SEQ,
	NL80211_ATTR_KEY_DEFAULT,

	NL80211_ATTR_BEACON_INTERVAL,
	NL80211_ATTR_DTIM_PERIOD,
	NL80211_ATTR_BEACON_HEAD,
	NL80211_ATTR_BEACON_TAIL,

	NL80211_ATTR_STA_AID,
	NL80211_ATTR_STA_FLAGS,
	NL80211_ATTR_STA_LISTEN_INTERVAL,
	NL80211_ATTR_STA_SUPPORTED_RATES,
	NL80211_ATTR_STA_VLAN,
	NL80211_ATTR_STA_INFO,

	NL80211_ATTR_WIPHY_BANDS,

	NL80211_ATTR_MNTR_FLAGS,

	NL80211_ATTR_MESH_ID,
	NL80211_ATTR_STA_PLINK_ACTION,
	NL80211_ATTR_MPATH_NEXT_HOP,
	NL80211_ATTR_MPATH_INFO,

	NL80211_ATTR_BSS_CTS_PROT,
	NL80211_ATTR_BSS_SHORT_PREAMBLE,
	NL80211_ATTR_BSS_SHORT_SLOT_TIME,

	NL80211_ATTR_HT_CAPABILITY,

	NL80211_ATTR_SUPPORTED_IFTYPES,

	NL80211_ATTR_REG_ALPHA2,
	NL80211_ATTR_REG_RULES,

	NL80211_ATTR_MESH_CONFIG,

	NL80211_ATTR_BSS_BASIC_RATES,

	NL80211_ATTR_WIPHY_TXQ_PARAMS,
	NL80211_ATTR_WIPHY_FREQ,
	NL80211_ATTR_WIPHY_CHANNEL_TYPE,

	NL80211_ATTR_KEY_DEFAULT_MGMT,

	NL80211_ATTR_MGMT_SUBTYPE,
	NL80211_ATTR_IE,

	NL80211_ATTR_MAX_NUM_SCAN_SSIDS,

	NL80211_ATTR_SCAN_FREQUENCIES,
	NL80211_ATTR_SCAN_SSIDS,
	NL80211_ATTR_GENERATION, /* replaces old SCAN_GENERATION */
	NL80211_ATTR_BSS,

	NL80211_ATTR_REG_INITIATOR,
	NL80211_ATTR_REG_TYPE,

	NL80211_ATTR_SUPPORTED_COMMANDS,

	NL80211_ATTR_FRAME,
	NL80211_ATTR_SSID,
	NL80211_ATTR_AUTH_TYPE,
	NL80211_ATTR_REASON_CODE,

	NL80211_ATTR_KEY_TYPE,

	NL80211_ATTR_MAX_SCAN_IE_LEN,
	NL80211_ATTR_CIPHER_SUITES,

	NL80211_ATTR_FREQ_BEFORE,
	NL80211_ATTR_FREQ_AFTER,

	NL80211_ATTR_FREQ_FIXED,


	NL80211_ATTR_WIPHY_RETRY_SHORT,
	NL80211_ATTR_WIPHY_RETRY_LONG,
	NL80211_ATTR_WIPHY_FRAG_THRESHOLD,
	NL80211_ATTR_WIPHY_RTS_THRESHOLD,

	NL80211_ATTR_TIMED_OUT,

	NL80211_ATTR_USE_MFP,

	NL80211_ATTR_STA_FLAGS2,

	NL80211_ATTR_CONTROL_PORT,

	NL80211_ATTR_TESTDATA,

	NL80211_ATTR_PRIVACY,

	NL80211_ATTR_DISCONNECTED_BY_AP,
	NL80211_ATTR_STATUS_CODE,

	NL80211_ATTR_CIPHER_SUITES_PAIRWISE,
	NL80211_ATTR_CIPHER_SUITE_GROUP,
	NL80211_ATTR_WPA_VERSIONS,
	NL80211_ATTR_AKM_SUITES,

	NL80211_ATTR_REQ_IE,
	NL80211_ATTR_RESP_IE,

	NL80211_ATTR_PREV_BSSID,

	NL80211_ATTR_KEY,
	NL80211_ATTR_KEYS,

	NL80211_ATTR_PID,

	NL80211_ATTR_4ADDR,

	NL80211_ATTR_SURVEY_INFO,

	NL80211_ATTR_PMKID,
	NL80211_ATTR_MAX_NUM_PMKIDS,

	NL80211_ATTR_DURATION,

	NL80211_ATTR_COOKIE,

	NL80211_ATTR_WIPHY_COVERAGE_CLASS,

	NL80211_ATTR_TX_RATES,

	NL80211_ATTR_FRAME_MATCH,

	NL80211_ATTR_ACK,

	NL80211_ATTR_PS_STATE,

	NL80211_ATTR_CQM,

	NL80211_ATTR_LOCAL_STATE_CHANGE,

	NL80211_ATTR_AP_ISOLATE,

	NL80211_ATTR_WIPHY_TX_POWER_SETTING,
	NL80211_ATTR_WIPHY_TX_POWER_LEVEL,

	NL80211_ATTR_TX_FRAME_TYPES,
	NL80211_ATTR_RX_FRAME_TYPES,
	NL80211_ATTR_FRAME_TYPE,

	NL80211_ATTR_CONTROL_PORT_ETHERTYPE,
	NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT,

	NL80211_ATTR_SUPPORT_IBSS_RSN,

	NL80211_ATTR_WIPHY_ANTENNA_TX,
	NL80211_ATTR_WIPHY_ANTENNA_RX,

	NL80211_ATTR_MCAST_RATE,

	NL80211_ATTR_OFFCHANNEL_TX_OK,

	NL80211_ATTR_BSS_HT_OPMODE,

	NL80211_ATTR_KEY_DEFAULT_TYPES,

	NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION,

	NL80211_ATTR_MESH_SETUP,

	NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX,
	NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX,

	NL80211_ATTR_SUPPORT_MESH_AUTH,
	NL80211_ATTR_STA_PLINK_STATE,

	NL80211_ATTR_WOWLAN_TRIGGERS,
	NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED,

	NL80211_ATTR_SCHED_SCAN_INTERVAL,

	NL80211_ATTR_INTERFACE_COMBINATIONS,
	NL80211_ATTR_SOFTWARE_IFTYPES,

	NL80211_ATTR_REKEY_DATA,

	NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS,
	NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN,

	NL80211_ATTR_SCAN_SUPP_RATES,

	NL80211_ATTR_HIDDEN_SSID,

	NL80211_ATTR_IE_PROBE_RESP,
	NL80211_ATTR_IE_ASSOC_RESP,

	NL80211_ATTR_STA_WME,
	NL80211_ATTR_SUPPORT_AP_UAPSD,

	NL80211_ATTR_ROAM_SUPPORT,

	NL80211_ATTR_SCHED_SCAN_MATCH,
	NL80211_ATTR_MAX_MATCH_SETS,

	NL80211_ATTR_PMKSA_CANDIDATE,

	NL80211_ATTR_TX_NO_CCK_RATE,

	NL80211_ATTR_TDLS_ACTION,
	NL80211_ATTR_TDLS_DIALOG_TOKEN,
	NL80211_ATTR_TDLS_OPERATION,
	NL80211_ATTR_TDLS_SUPPORT,
	NL80211_ATTR_TDLS_EXTERNAL_SETUP,

	NL80211_ATTR_DEVICE_AP_SME,

	NL80211_ATTR_DONT_WAIT_FOR_ACK,

	NL80211_ATTR_FEATURE_FLAGS,

	NL80211_ATTR_PROBE_RESP_OFFLOAD,

	NL80211_ATTR_PROBE_RESP,

	NL80211_ATTR_DFS_REGION,

	NL80211_ATTR_DISABLE_HT,
	NL80211_ATTR_HT_CAPABILITY_MASK,

	NL80211_ATTR_NOACK_MAP,

	NL80211_ATTR_INACTIVITY_TIMEOUT,

	NL80211_ATTR_RX_SIGNAL_DBM,

	NL80211_ATTR_BG_SCAN_PERIOD,

	NL80211_ATTR_WDEV,

	NL80211_ATTR_USER_REG_HINT_TYPE,

	NL80211_ATTR_CONN_FAILED_REASON,

	NL80211_ATTR_SAE_DATA,

	NL80211_ATTR_VHT_CAPABILITY,

	NL80211_ATTR_SCAN_FLAGS,

	NL80211_ATTR_CHANNEL_WIDTH,
	NL80211_ATTR_CENTER_FREQ1,
	NL80211_ATTR_CENTER_FREQ2,

	NL80211_ATTR_P2P_CTWINDOW,
	NL80211_ATTR_P2P_OPPPS,

	NL80211_ATTR_LOCAL_MESH_POWER_MODE,

	NL80211_ATTR_ACL_POLICY,

	NL80211_ATTR_MAC_ADDRS,

	NL80211_ATTR_MAC_ACL_MAX,

	NL80211_ATTR_RADAR_EVENT,

	NL80211_ATTR_EXT_CAPA,
	NL80211_ATTR_EXT_CAPA_MASK,

	NL80211_ATTR_STA_CAPABILITY,
	NL80211_ATTR_STA_EXT_CAPABILITY,

	NL80211_ATTR_PROTOCOL_FEATURES,
	NL80211_ATTR_SPLIT_WIPHY_DUMP,

	NL80211_ATTR_DISABLE_VHT,
	NL80211_ATTR_VHT_CAPABILITY_MASK,

	NL80211_ATTR_MDID,
	NL80211_ATTR_IE_RIC,

	NL80211_ATTR_CRIT_PROT_ID,
	NL80211_ATTR_MAX_CRIT_PROT_DURATION,

	NL80211_ATTR_PEER_AID,

	/* add attributes here, update the policy in nl80211.c */

	__NL80211_ATTR_AFTER_LAST,
	NL80211_ATTR_MAX = __NL80211_ATTR_AFTER_LAST - 1
};
enum nl80211_bss {
	__NL80211_BSS_INVALID,
	NL80211_BSS_BSSID,
	NL80211_BSS_FREQUENCY,
	NL80211_BSS_TSF,
	NL80211_BSS_BEACON_INTERVAL,
	NL80211_BSS_CAPABILITY,
	NL80211_BSS_INFORMATION_ELEMENTS,
	NL80211_BSS_SIGNAL_MBM,
	NL80211_BSS_SIGNAL_UNSPEC,
	NL80211_BSS_STATUS,
	NL80211_BSS_SEEN_MS_AGO,
	NL80211_BSS_BEACON_IES,

	/* keep last */
	__NL80211_BSS_AFTER_LAST,
	NL80211_BSS_MAX = __NL80211_BSS_AFTER_LAST - 1
};
enum nl80211_bss_status {
	NL80211_BSS_STATUS_AUTHENTICATED,
	NL80211_BSS_STATUS_ASSOCIATED,
	NL80211_BSS_STATUS_IBSS_JOINED,
};
enum nl80211_reg_rule_attr {
	__NL80211_REG_RULE_ATTR_INVALID,
	NL80211_ATTR_REG_RULE_FLAGS,

	NL80211_ATTR_FREQ_RANGE_START,
	NL80211_ATTR_FREQ_RANGE_END,
	NL80211_ATTR_FREQ_RANGE_MAX_BW,

	NL80211_ATTR_POWER_RULE_MAX_ANT_GAIN,
	NL80211_ATTR_POWER_RULE_MAX_EIRP,

	/* keep last */
	__NL80211_REG_RULE_ATTR_AFTER_LAST,
	NL80211_REG_RULE_ATTR_MAX = __NL80211_REG_RULE_ATTR_AFTER_LAST - 1
};
enum nl80211_frequency_attr {
	__NL80211_FREQUENCY_ATTR_INVALID,
	NL80211_FREQUENCY_ATTR_FREQ,
	NL80211_FREQUENCY_ATTR_DISABLED,
	NL80211_FREQUENCY_ATTR_PASSIVE_SCAN,
	NL80211_FREQUENCY_ATTR_NO_IBSS,
	NL80211_FREQUENCY_ATTR_RADAR,
	NL80211_FREQUENCY_ATTR_MAX_TX_POWER,
	NL80211_FREQUENCY_ATTR_DFS_STATE,
	NL80211_FREQUENCY_ATTR_DFS_TIME,
	NL80211_FREQUENCY_ATTR_NO_HT40_MINUS,
	NL80211_FREQUENCY_ATTR_NO_HT40_PLUS,
	NL80211_FREQUENCY_ATTR_NO_80MHZ,
	NL80211_FREQUENCY_ATTR_NO_160MHZ,

	/* keep last */
	__NL80211_FREQUENCY_ATTR_AFTER_LAST,
	NL80211_FREQUENCY_ATTR_MAX = __NL80211_FREQUENCY_ATTR_AFTER_LAST - 1
};
enum nl80211_mntr_flags {
	__NL80211_MNTR_FLAG_INVALID,
	NL80211_MNTR_FLAG_FCSFAIL,
	NL80211_MNTR_FLAG_PLCPFAIL,
	NL80211_MNTR_FLAG_CONTROL,
	NL80211_MNTR_FLAG_OTHER_BSS,
	NL80211_MNTR_FLAG_COOK_FRAMES,

	/* keep last */
	__NL80211_MNTR_FLAG_AFTER_LAST,
	NL80211_MNTR_FLAG_MAX = __NL80211_MNTR_FLAG_AFTER_LAST - 1
};
#endif/*_SS_CMD_H*/
