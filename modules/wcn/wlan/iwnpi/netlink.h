#ifndef __WLNPI_H__
#define __WLNPI_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#ifndef TIZEN_EXT
//#include <netlink-types.h>
#endif
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/netlink.h>
#include <endian.h>
#include <linux/types.h>
#include "common.h"

#define ETH_ALEN 6
enum wlan_nl_commands
{
	WLAN_NL_CMD_UNSPEC,
	WLAN_NL_CMD_NPI,
	WLAN_NL_CMD_GET_INFO,
	WLAN_NL_CMD_MAX,
};
enum wlan_nl_attrs
{
	WLAN_NL_ATTR_UNSPEC,
	WLAN_NL_ATTR_DEVINDEX,
	WLAN_NL_ATTR_COMMON_USR_TO_DRV,
	WLAN_NL_ATTR_COMMON_DRV_TO_USR,
	WLAN_NL_ATTR_MAX,
};
enum WLNPI_CMD_LIST
{
    WLNPI_CMD_START,
    WLNPI_CMD_STOP,
    WLNPI_CMD_SET_MAC,
    WLNPI_CMD_GET_MAC,
    WLNPI_CMD_SET_MAC_FILTER,
    WLNPI_CMD_GET_MAC_FILTER,  // 5
    WLNPI_CMD_SET_CHANNEL,
    WLNPI_CMD_GET_CHANNEL,
    WLNPI_CMD_GET_RSSI,
    WLNPI_CMD_SET_TX_MODE,
    WLNPI_CMD_GET_TX_MODE,  // 10
    WLNPI_CMD_SET_RATE,
    WLNPI_CMD_GET_RATE,
    WLNPI_CMD_SET_BAND,
    WLNPI_CMD_GET_BAND,
    WLNPI_CMD_SET_BW,  // 15
    WLNPI_CMD_GET_BW,
    WLNPI_CMD_SET_PKTLEN,
    WLNPI_CMD_GET_PKTLEN,
    WLNPI_CMD_SET_PREAMBLE,
    WLNPI_CMD_SET_GUARD_INTERVAL,  // 20
    WLNPI_CMD_GET_GUARD_INTERVAL,
    WLNPI_CMD_SET_BURST_INTERVAL,
    WLNPI_CMD_GET_BURST_INTERVAL,
    WLNPI_CMD_SET_PAYLOAD,
    WLNPI_CMD_GET_PAYLOAD,  // 25
    WLNPI_CMD_SET_TX_POWER,
    WLNPI_CMD_GET_TX_POWER,
    WLNPI_CMD_SET_TX_COUNT,
    WLNPI_CMD_GET_RX_OK_COUNT,
    WLNPI_CMD_TX_START,  // 30
    WLNPI_CMD_TX_STOP,
    WLNPI_CMD_RX_START,
    WLNPI_CMD_RX_STOP,
    WLNPI_CMD_GET_REG,
    WLNPI_CMD_SET_REG,  // 35
    WLNPI_CMD_SIN_WAVE,
    WLNPI_CMD_LNA_ON,
    WLNPI_CMD_LNA_OFF,
    WLNPI_CMD_GET_LNA_STATUS,
	WLNPI_CMD_SET_WLAN_CAP,
	WLNPI_CMD_GET_WLAN_CAP,
    WLNPI_CMD_MAX, // 42
};

struct wlan_nl_sock_state
{
	struct nl_sock *sock;
	int nl_id;
	int nl_cmd_id;
};

typedef struct
{
	struct nl_sock *sock;
	unsigned int devindex;
	int nl_id;
	int nl_cmd_id;
	unsigned char mac[6];
}wlnpi_t;

struct buf_info {
	unsigned char *addr;
	unsigned int *len;
};

typedef struct wlnpi_cmd_t wlnpi_cmd_t___;
typedef  int (*P_FUNC_1)(int, char **,  unsigned char *, int * );
typedef  int (*P_FUNC_2)(struct wlnpi_cmd_t *, unsigned char *, int);
struct wlnpi_cmd_t
{
	char    *name;
	char    *help;
	P_FUNC_1 parse;
	P_FUNC_2 show;
	char     id;
	struct ss_cmd *priv;
};

typedef struct
{
	unsigned char  type;
	unsigned char  subtype;
	unsigned short len;
}WLNPI_CMD_HDR_T;

enum WLNPI_CMD_TYPE
{
	HOST_TO_MARLIN_CMD = 1,
	MARLIN_TO_HOST_REPLY  ,
};

extern wlnpi_t g_wlnpi;
extern struct wlnpi_cmd_t *match_cmd_table(char *name);
extern int __handle_ss_cmd(int argc, char **argv,void *priv);
#endif

