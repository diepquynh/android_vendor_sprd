#include <errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <utils/Log.h>
#include <android/log.h>

#include "nlnpi.h"
#include "iwnpi.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "IWNPI_CMD"

char ret_result_buf[WLNPI_RES_BUF_LEN + 1] = {0x00};

/* This is used to caculate the cmd section size */
SECTION(get);
SECTION(set);

/* ----------------SET CMD WITHOUT ARG -------------------- */
static int print_reply_status(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_STATUS]) {
    if (nla_len(tb_msg[NLNPI_ATTR_REPLY_STATUS]) == 4)
      printf("ret: status %d :end\n",
             *(int *)nla_data(tb_msg[NLNPI_ATTR_REPLY_STATUS]));
    else
      printf("ret: Invild len %d :end\n",
             nla_len(tb_msg[NLNPI_ATTR_REPLY_STATUS]));
  } else {
    printf("ret: status Failed! :end\n");
  }

  return NL_SKIP;
}

static int npi_set_noarg_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                             struct nl_msg *msg, int argc, char **argv) {
  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_reply_status, NULL);
  return 0;
}

TOPLEVEL(start, NULL, NLNPI_CMD_START, 0, npi_set_noarg_cmd,
         "Start npi thread in CP2.");

TOPLEVEL(stop, NULL, NLNPI_CMD_STOP, 0, npi_set_noarg_cmd,
         "Stop npi thread in CP2.");

TOPLEVEL(tx_start, NULL, NLNPI_CMD_TX_START, 0, npi_set_noarg_cmd, "Start tx.");

TOPLEVEL(tx_stop, NULL, NLNPI_CMD_TX_STOP, 0, npi_set_noarg_cmd, "Stop tx.");

TOPLEVEL(rx_start, NULL, NLNPI_CMD_RX_START, 0, npi_set_noarg_cmd, "Start rx.");

TOPLEVEL(rx_stop, NULL, NLNPI_CMD_RX_STOP, 0, npi_set_noarg_cmd, "Stop rx.");

TOPLEVEL(sin_wave, NULL, NLNPI_CMD_SIN_WAVE, 0, npi_set_noarg_cmd, "Sin wave.");

/* ----------------SET CMD WITH ARG -------------------- */
#define NPI_SET_ARGINT_CMD(name, nl_attr)                                  \
  static int npi_##name##_cmd(struct nlnpi_state *state, struct nl_cb *cb, \
                              struct nl_msg *msg, int argc, char **argv) { \
    int data;                                                              \
    char **err = NULL;                                                     \
                                                                           \
    if (argc != 1 || !argv) return 1;                                      \
                                                                           \
    data = strtol(argv[0], err, 10);                                       \
    if (err) {                                                             \
      fprintf(stderr, "Invild data format\n");                             \
      return 2;                                                            \
    }                                                                      \
    NLA_PUT(msg, nl_attr, sizeof(data), &data);                            \
                                                                           \
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_reply_status, NULL);    \
                                                                           \
    return 0;                                                              \
  nla_put_failure:                                                         \
    return -ENOBUFS;                                                       \
  }

NPI_SET_ARGINT_CMD(set_rate, NLNPI_ATTR_SET_RATE)
NPI_SET_ARGINT_CMD(set_channel, NLNPI_ATTR_SET_CHANNEL)
NPI_SET_ARGINT_CMD(set_tx_power, NLNPI_ATTR_SET_TX_POWER)

NPI_SET_ARGINT_CMD(set_rx_count, NLNPI_ATTR_SET_RX_COUNT)
NPI_SET_ARGINT_CMD(set_tx_mode, NLNPI_ATTR_TX_MODE)

NPI_SET_ARGINT_CMD(set_tx_count, NLNPI_ATTR_TX_COUNT)
NPI_SET_ARGINT_CMD(set_band, NLNPI_ATTR_SET_BAND)
NPI_SET_ARGINT_CMD(set_pkt_length, NLNPI_ATTR_SET_PKTLEN)

TOPLEVEL(set_band, "<NUM>", NLNPI_CMD_SET_BAND, 0, npi_set_band_cmd,
         "Set band.");
TOPLEVEL(set_pkt_length, "<NUM>", NLNPI_CMD_SET_PKTLEN, 0,
         npi_set_pkt_length_cmd, "Set band.");
TOPLEVEL(set_rate, "<NUM>", NLNPI_CMD_SET_RATE, 0, npi_set_rate_cmd,
         "Set rate.");
TOPLEVEL(set_channel, "<NUM>", NLNPI_CMD_SET_CHANNEL, 0, npi_set_channel_cmd,
         "Set channal num.");
TOPLEVEL(set_tx_power, "<NUM>", NLNPI_CMD_SET_TX_POWER, 0, npi_set_tx_power_cmd,
         "Set tx power.");

TOPLEVEL(set_rx_count, "<NUM>", NLNPI_CMD_CLEAR_RX_COUNT, 0,
         npi_set_rx_count_cmd, "Clear rx count.");
TOPLEVEL(set_tx_mode, "<NUM>", NLNPI_CMD_SET_TX_MODE, 0, npi_set_tx_mode_cmd,
         "Set tx mode.");

TOPLEVEL(set_tx_count, "<NUM>", NLNPI_CMD_SET_TX_COUNT, 0, npi_set_tx_count_cmd,
         "Set tx count. (0:tx forever)");

static int npi_set_mac_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                           struct nl_msg *msg, int argc, char **argv) {
  unsigned char addr[6];

  if (argc != 1 || !argv) return 1;

  if (mac_addr_a2n(addr, argv[0]) == 0) {
    NLA_PUT(msg, NLNPI_ATTR_SET_MAC, 6, addr);
  } else {
    fprintf(stderr, "ret: invalid mac address :end\n");
    return 2;
  }

  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_reply_status, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(set_mac, "<xx:xx:xx:xx:xx:xx>", NLNPI_CMD_SET_MAC, 0, npi_set_mac_cmd,
         "Set mac addr like 00:22:33:44:55:66.");

#define REG_MAC 0
#define REG_PHY0 1
#define REG_PHY1 2
#define REG_RF 3
static int npi_set_reg_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                           struct nl_msg *msg, int argc, char **argv) {
  unsigned int start_addr;
  unsigned char data[9];
  unsigned int value;
  char **err = NULL;

  if (argc != 3 || !argv) return 1;

  if (strncmp(argv[0], "mac", 3) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    if (start_addr & 0x3) {
      printf("ret: Invild addr 0x%x, should 4 bytes align :end\n", start_addr);
      return 2;
    }
    data[0] = REG_MAC;
  } else if (strncmp(argv[0], "rf", 2) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    data[0] = REG_RF;
  } else if (strncmp(argv[0], "phy0", 4) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    data[0] = REG_PHY0;
  } else if (strncmp(argv[0], "phy1", 4) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    data[0] = REG_PHY1;
  } else {
    printf("ret: Invild reg name %s :end\n", argv[0]);
    return 2;
  }

  value = strtol(argv[2], err, 16);
  if (err) {
    fprintf(stderr, "Invild value format\n");
    return 2;
  }
  memcpy(&(data[1]), &start_addr, 4);
  memcpy(&(data[5]), &value, 4);

  NLA_PUT(msg, NLNPI_ATTR_SET_REG, 9, data);

  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_reply_status, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(set_reg, "<mac/phy0/phy1/rf> <addr_offset> <value>", NLNPI_CMD_SET_REG,
         0, npi_set_reg_cmd,
         "<addr_offset> and <value> should be hex type like(0x1234).");

/* data format is len(2 bytes) + name + value */
static int npi_set_debug_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                             struct nl_msg *msg, int argc, char **argv) {
  unsigned short len;
  unsigned char data[38];
  unsigned int value;
  char **err = NULL;

  if (argc != 2 || !argv) return 1;

  len = strlen(argv[0]);
  if (len > 32) {
    fprintf(stderr, "Invild debug name length(%d)\n", len);
    return 2;
  }
  memcpy(data, &len, sizeof(len));
  sscanf(argv[0], "%s", &(data[2]));
  value = strtol(argv[1], err, 16);
  if (err) {
    fprintf(stderr, "Invild value format\n");
    return 2;
  }
  memcpy(&(data[2 + len]), &value, sizeof(value));
  NLA_PUT(msg, NLNPI_ATTR_SET_DEBUG, (sizeof(len) + len + sizeof(value)), data);

  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_reply_status, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(set_debug, "<value_name> <value>", NLNPI_CMD_SET_DEBUG, 0,
         npi_set_debug_cmd, "value should be like 0x12345678 hex type.");

/* data format is len(4 bytes) + count(4 bytes) */
static int npi_set_get_sblock_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                                  struct nl_msg *msg, int argc, char **argv) {
  unsigned int len;
  unsigned int count;
  unsigned char data[8];
  char **err = NULL;

  if (argc > 2 || !argv) return 1;

  if (argc == 1) count = 0;
  if (argc == 2) {
    count = strtol(argv[1], err, 10);
    if (err) {
      fprintf(stderr, "Invild count format\n");
      return 2;
    }
  }

  memcpy(&(data[4]), &count, sizeof(count));
  len = strtol(argv[0], err, 10);
  if (err) {
    fprintf(stderr, "Invild length format\n");
    return 2;
  }
  memcpy(data, &len, sizeof(len));
  NLA_PUT(msg, NLNPI_ATTR_SBLOCK_ARG, sizeof(data), data);

  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_reply_status, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(set_sblock, "<send_len> [count]", NLNPI_CMD_SET_SBLOCK, 0,
         npi_set_get_sblock_cmd, "count 0 or no count means send forever.");
TOPLEVEL(get_sblock, "<send_len> [count]", NLNPI_CMD_GET_SBLOCK, 0,
         npi_set_get_sblock_cmd, "count 0 or no count means send forever.");
/* ----------------GET CMD WITH INT ARG REPLY-------------------- */
static int print_reply_rx_count_data(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
  unsigned int count[3];

  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_DATA]) {
    if (nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]) == 12) {
      memcpy(count, nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]), sizeof(count));
      snprintf(ret_result_buf, WLNPI_RES_BUF_LEN,
               "ret: reg value: rx_end_count=%d rx_err_end_count=%d "
               "fcs_fail_count=%d :end\n",
               count[0], count[1], count[2]);

    } else {
      snprintf(ret_result_buf, WLNPI_RES_BUF_LEN, "ret: Invild len %d :end\n",
               nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]));
    }
    printf("%s", ret_result_buf);
    ALOGD("%s", ret_result_buf);
  } else {
    printf("ret: Failed to get result! :end\n");
    ALOGD("ret: Failed to get result! :end");
  }

  return NL_SKIP;
}

static int print_reply_int_data(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

  ALOGD("print_reply_int_data\n");
  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_DATA]) {
    if (nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]) == 4)
      snprintf(ret_result_buf, WLNPI_RES_BUF_LEN, "ret: %d :end\n",
               *(unsigned int *)nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]));
    else
      snprintf(ret_result_buf, WLNPI_RES_BUF_LEN, "ret: Invild len %d :end\n",
               *(unsigned int *)nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]));
    printf("%s", ret_result_buf);
    ALOGD("%s", ret_result_buf);
  } else {
    printf("ret: Failed to get result! :end\n");
    ALOGD("ret: Failed to get result! :end");
  }

  return NL_SKIP;
}

#define NPI_GET_ARGINT_CMD(name, nl_attr, print_function, get_count)       \
  static int npi_##name##_cmd(struct nlnpi_state *state, struct nl_cb *cb, \
                              struct nl_msg *msg, int argc, char **argv) { \
    unsigned int get_len = get_count;                                      \
                                                                           \
    if (argc) return 1;                                                    \
                                                                           \
    NLA_PUT_U32(msg, nl_attr, get_len);                                    \
                                                                           \
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_function, NULL);        \
    return 0;                                                              \
  nla_put_failure:                                                         \
    return -ENOBUFS;                                                       \
  }

NPI_GET_ARGINT_CMD(get_channel, NLNPI_ATTR_GET_CHANNEL, print_reply_int_data, 4)
NPI_GET_ARGINT_CMD(get_rx_err, NLNPI_ATTR_GET_RX_COUNT, print_reply_int_data, 4)
NPI_GET_ARGINT_CMD(get_rssi, NLNPI_ATTR_RSSI, print_reply_int_data, 4)

NPI_GET_ARGINT_CMD(get_band, NLNPI_ATTR_GET_BAND, print_reply_int_data, 4)
NPI_GET_ARGINT_CMD(get_payload, NLNPI_ATTR_GET_PAYLOAD, print_reply_int_data, 4)
NPI_GET_ARGINT_CMD(get_bandwidth, NLNPI_ATTR_GET_BANDWIDTH,
                   print_reply_int_data, 4)
NPI_GET_ARGINT_CMD(get_pkt_length, NLNPI_ATTR_GET_PKTLEN, print_reply_int_data,
                   4)
NPI_GET_ARGINT_CMD(get_tx_mode, NLNPI_ATTR_GET_TX_MODE, print_reply_int_data, 4)

NPI_GET_ARGINT_CMD(get_rx_ok, NLNPI_ATTR_GET_RX_COUNT,
                   print_reply_rx_count_data, 12)

NPI_GET_ARGINT_CMD(get_rate, NLNPI_ATTR_GET_RATE, print_reply_int_data, 4)

TOPLEVEL(get_pkt_length, NULL, NLNPI_CMD_GET_PKTLEN, 0, npi_get_pkt_length_cmd,
         "Get packet length.");

TOPLEVEL(get_channel, NULL, NLNPI_CMD_GET_CHANNEL, 0, npi_get_channel_cmd,
         "Get channal num.");
TOPLEVEL(get_band, NULL, NLNPI_CMD_GET_BAND, 0, npi_get_band_cmd, "Get band.");

TOPLEVEL(get_payload, NULL, NLNPI_CMD_GET_PAYLOAD, 0, npi_get_payload_cmd,
         "Get payload.");

TOPLEVEL(get_bandwidth, NULL, NLNPI_CMD_GET_BANDWIDTH, 0, npi_get_bandwidth_cmd,
         "Get bandwidth.");

TOPLEVEL(get_rx_ok, NULL, NLNPI_CMD_GET_RX_OK_COUNT, 0, npi_get_rx_ok_cmd,
         "Get rx right count.");

TOPLEVEL(get_tx_mode, NULL, NLNPI_CMD_GET_TX_MODE, 0, npi_get_tx_mode_cmd,
         "Get tx mode.");
/*
TOPLEVEL(get_rx_err, NULL,
         NLNPI_CMD_GET_RX_ERR_COUNT, 0, npi_get_rx_err_cmd,
         "Get rx error count.");
*/
TOPLEVEL(get_rssi, NULL, NLNPI_CMD_GET_RSSI, 0, npi_get_rssi_cmd, "Get rssi.");
TOPLEVEL(get_rate, NULL, NLNPI_CMD_GET_RATE, 0, npi_get_rate_cmd,
         "Get tx rate.");

/* ----------------GET CMD WITH INT USER DEFINED REPLY-------------------- */
static int print_get_tx_power_result(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
  unsigned int data;
  unsigned short lev_a, lev_b;

  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_DATA]) {
    if (nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]) == 4) {
      data = *(unsigned int *)nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]);
      lev_a = data & 0x0000ffff;
      lev_b = (data & 0xffff0000) >> 16;
      printf("ret: level_a:%d,level_b:%d :end\n", lev_a, lev_b);
    } else {
      printf("ret: Invild len %d :end\n",
             nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]));
    }
  } else {
    printf("ret: Failed to get result! :end\n");
  }

  return NL_SKIP;
}

static int npi_get_tx_power_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                                struct nl_msg *msg, int argc, char **argv) {
  unsigned int get_len = 4;

  if (argc) return 1;

  NLA_PUT_U32(msg, NLNPI_ATTR_GET_TX_POWER, get_len);
  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_get_tx_power_result, NULL);
  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(get_tx_power, NULL, NLNPI_CMD_GET_TX_POWER, 0, npi_get_tx_power_cmd,
         "Get tx power.");

static int print_get_mac_result(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
  char addr[18];

  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_DATA]) {
    if (nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]) == 6) {
      mac_addr_n2a(addr, nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]));
      printf("ret: mac: %s :end\n", addr);
    } else {
      printf("ret: Invild len %d :end\n",
             nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]));
    }
  } else {
    printf("ret: Failed to get result! :end\n");
  }

  return NL_SKIP;
}

static int npi_get_mac_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                           struct nl_msg *msg, int argc, char **argv) {
  unsigned int get_len = 6;
  if (argc) return 1;
  NLA_PUT_U32(msg, NLNPI_ATTR_GET_MAC, get_len);
  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_get_mac_result, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(get_mac, NULL, NLNPI_CMD_GET_MAC, 0, npi_get_mac_cmd, "Get mac addr.");

static int print_get_reg_result(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
  unsigned int data[256];
  unsigned int count, i;

  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_DATA]) {
    count = nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]);
    if ((count < 1024) || !(count % 4)) {
      memcpy(data, nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]), count);
      printf("reg values is :");
      for (i = 0; i < count / 4; i++) {
        if (!(i % 4)) printf("\n");
        printf("0x%08x\t", data[i]);
      }
      printf("\n");
    } else {
      printf("ret: Invild len %d :end\n", count);
    }
  } else {
    printf("ret: Failed to get result! :end\n");
  }

  return NL_SKIP;
}

static int npi_get_reg_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                           struct nl_msg *msg, int argc, char **argv) {
  unsigned int get_len;
  unsigned int count = 1;
  unsigned int start_addr;
  unsigned char data[6];
  char **err = NULL;

  if (argc < 2 || argc > 3 || !argv) return 1;
  if (argc == 3) {
    count = strtol(argv[2], err, 10);
    if (err) {
      fprintf(stderr, "Invild count format\n");
      return 2;
    }
    if (count > 255) {
      printf("ret: Invild count %d, too large :end\n", count);
      return 2;
    }
  }

  if (strncmp(argv[0], "mac", 3) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    if (start_addr & 0x3) {
      printf("ret: Invild addr 0x%x, should 4 bytes align :end\n", start_addr);
      return 2;
    }
    data[0] = REG_MAC;
  } else if (strncmp(argv[0], "rf", 2) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    data[0] = REG_RF;
  } else if (strncmp(argv[0], "phy0", 4) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    data[0] = REG_PHY0;
  } else if (strncmp(argv[0], "phy1", 4) == 0) {
    start_addr = strtol(argv[1], err, 16);
    if (err) {
      fprintf(stderr, "Invild start_addr format\n");
      return 2;
    }
    data[0] = REG_PHY1;
  } else {
    printf("ret: Invild reg name %s :end\n", argv[0]);
    return 1;
  }

  memcpy(&(data[1]), &start_addr, 4);
  data[5] = (unsigned char)count;
  get_len = count * 4;
  NLA_PUT_U32(msg, NLNPI_ATTR_GET_REG, get_len);

  NLA_PUT(msg, NLNPI_ATTR_GET_REG_ARG, 6, data);
  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_get_reg_result, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}

TOPLEVEL(get_reg, "<mac/phy0/phy1/rf> <addr_offset> [count]", NLNPI_CMD_GET_REG,
         0, npi_get_reg_cmd,
         "<addr_offset>: should be hex type like(0x1234)\n[count]: if not set "
         "default is 1.");

static int print_get_debug_result(struct nl_msg *msg, void *arg) {
  struct nlattr *tb_msg[NLNPI_ATTR_MAX + 1];
  struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

  nla_parse(tb_msg, NLNPI_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

  if (tb_msg[NLNPI_ATTR_REPLY_DATA]) {
    if (nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]) == 4)
      printf("ret: 0x%08x :end\n",
             *(unsigned int *)nla_data(tb_msg[NLNPI_ATTR_REPLY_DATA]));
    else
      printf("ret: Invild len %d :end\n",
             nla_len(tb_msg[NLNPI_ATTR_REPLY_DATA]));
  } else {
    printf("ret: Failed to get result! :end\n");
  }

  return NL_SKIP;
}

static int npi_get_debug_cmd(struct nlnpi_state *state, struct nl_cb *cb,
                             struct nl_msg *msg, int argc, char **argv) {
  unsigned int get_len;
  unsigned char name[32];
  unsigned int name_len;

  if (argc != 1 || !argv) return 1;

  name_len = strlen(argv[0]);
  if (name_len > 32) {
    fprintf(stderr, "Invild debug name length(%d)\n", name_len);
    return 2;
  }

  sscanf(argv[0], "%s", name);
  get_len = 4;
  NLA_PUT_U32(msg, NLNPI_ATTR_GET_DEBUG, get_len);

  NLA_PUT(msg, NLNPI_ATTR_GET_DEBUG_ARG, name_len, name);
  nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_get_debug_result, NULL);

  return 0;
nla_put_failure:
  return -ENOBUFS;
}
TOPLEVEL(get_debug, "<value_name>", NLNPI_CMD_GET_DEBUG, 0, npi_get_debug_cmd,
         NULL);

NPI_GET_ARGINT_CMD(lna_status, NLNPI_ATTR_GET_LNA_STATUS, print_reply_int_data,
                   4)

TOPLEVEL(lna_on, NULL, NLNPI_CMD_LNA_ON, 0, npi_set_noarg_cmd, "lna no");
TOPLEVEL(lna_off, NULL, NLNPI_CMD_LNA_OFF, 0, npi_set_noarg_cmd, "lna off");
TOPLEVEL(lna_status, NULL, NLNPI_CMD_GET_LNA_STATUS, 0, npi_lna_status_cmd,
         "Get lna status.");
TOPLEVEL(speed_up, NULL, NLNPI_CMD_SPEED_UP, 0, npi_set_noarg_cmd, "speed up");
TOPLEVEL(speed_down, NULL, NLNPI_CMD_SPEED_DOWN, 0, npi_set_noarg_cmd,
         "speed down");
