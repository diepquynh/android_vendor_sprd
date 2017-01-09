/*
 * Driver interaction with extended Linux CFG8021
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
#include "includes.h"
#include <sys/types.h>
#include <fcntl.h>
#include <net/if.h>

#include "common.h"
#include "linux_ioctl.h"
#include "driver_nl80211.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#ifdef ANDROID
#include "android_drv.h"
#endif

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#include "hostapd.h"
#include "sta_info.h"
//<-- Add for SoftAp Advance Feature END

typedef struct android_wifi_priv_cmd {
//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
  u64 buf;
#else
  char *buf;     // pointer to struct driver_cmd_msg
#endif
//<-- Add for SoftAp Advance Feature END
  int used_len;  // length of driver_cmd_msg including data
  int total_len;
} android_wifi_priv_cmd;

struct driver_cmd_msg {
  uint16_t msg_type;  // CMD_SUBTYPE_XX
  uint16_t msg_len;   // msg_data length
  char msg_data[];
};

#define CMD_SUBTYPE_MAX_NUM_STA (4)
#define CMD_SUBTYPE_MIRACAST_MODE (5)

#define SPRDWL_WNM_BTM (1 << 0)
#define SPRDWL_WNM_PARP (1 << 1)
#define SPRDWL_WNM_MIPM (1 << 2)
#define SPRDWL_WNM_DMS (1 << 3)
#define SPRDWL_WNM_SLEEP (1 << 4)
#define SPRDWL_WNM_TFS (1 << 5)

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
static int android_priv_cmd(struct i802_bss *bss, const char *cmd)
{
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct ifreq ifr;
    android_wifi_priv_cmd priv_cmd;
    char buf[MAX_DRV_CMD_SIZE];
    int ret;

    os_memset(&ifr, 0, sizeof(ifr));
    os_memset(&priv_cmd, 0, sizeof(priv_cmd));
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

    os_memset(buf, 0, sizeof(buf));
    os_strlcpy(buf, cmd, sizeof(buf));

#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)buf;
#else
    priv_cmd.buf = buf;
#endif
    priv_cmd.used_len = sizeof(buf);
    priv_cmd.total_len = sizeof(buf);
    ifr.ifr_data = &priv_cmd;

    ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr);
    if (ret < 0) {
        wpa_printf(MSG_ERROR, "%s: failed to issue private commands",
                 __func__);
        return ret;
    }

    return 0;
}

#define HOSTAP_BLOCK_LIST_FILE "/data/misc/wifi/hostapd.blocklist"
#define HOSTAP_MAX_BLOCK_NUM 8


static int hostapd_read_mac_addr_list(const char *file_path, u8 mac_addr_list[][ETH_ALEN])
{
  FILE *f;
  int mac_addr_list_len = 0;
  f = fopen(file_path, "r");
  if (f == NULL) return 0;
  mac_addr_list_len = fread(mac_addr_list, ETH_ALEN, HOSTAP_MAX_BLOCK_NUM, f);
  fclose(f);
  return mac_addr_list_len;
}

static int hostapd_write_mac_addr_list(const char *file_path, u8 mac_addr_list[][ETH_ALEN], int mac_addr_list_len)
{
  FILE *f;
  int ret = 0;
  f = fopen(file_path, "w");
  if (f == NULL) return -1;
  if(fwrite(mac_addr_list, ETH_ALEN, mac_addr_list_len, f) != mac_addr_list_len) {
      wpa_printf(MSG_ERROR, "Failed to write mac address list");
      ret = -1;
  }
  fclose(f);
  return ret;
}

int hostapd_init_block_list(void *priv)
{
  u8 mac_addr_list[HOSTAP_MAX_BLOCK_NUM][ETH_ALEN];
  int mac_addr_list_len;
  char cmd[MAX_DRV_CMD_SIZE];
  int i;

  mac_addr_list_len = hostapd_read_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list);
  for(i=0; i<mac_addr_list_len; i++) {
      snprintf(cmd, sizeof(cmd),"BLOCK %02x:%02x:%02x:%02x:%02x:%02x", mac_addr_list[i][0], mac_addr_list[i][1], mac_addr_list[i][2], mac_addr_list[i][3], mac_addr_list[i][4], mac_addr_list[i][5]);
      if(android_priv_cmd(priv, cmd) < 0)
      return -1;
  }
  return 0;
}

int hostapd_add_station_to_mac_addr_list(const u8 *mac_addr)
{
  u8 mac_addr_list[HOSTAP_MAX_BLOCK_NUM][ETH_ALEN];
  int mac_addr_list_len;
  int i;
  mac_addr_list_len = hostapd_read_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list);
  for(i=0; i<mac_addr_list_len; i++) {
      if(os_memcmp(mac_addr_list[i], mac_addr, ETH_ALEN) == 0) {
          return 0;
      }
  }
  if(mac_addr_list_len == HOSTAP_MAX_BLOCK_NUM) return -1;
  os_memcpy(mac_addr_list[mac_addr_list_len], mac_addr, ETH_ALEN);
  mac_addr_list_len++;
  return hostapd_write_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list, mac_addr_list_len);
}

int hostapd_del_station_from_mac_addr_list(const u8 *mac_addr)
{
  u8 mac_addr_list[HOSTAP_MAX_BLOCK_NUM][ETH_ALEN];
  int mac_addr_list_len;
  int i;
  mac_addr_list_len = hostapd_read_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list);
  for(i=0; i<mac_addr_list_len; i++) {
      if(os_memcmp(mac_addr_list[i], mac_addr, ETH_ALEN) == 0) {
          break;
      }
  }
  if(i == mac_addr_list_len) {
      return 0;
  } else {
      for( ; i<mac_addr_list_len-1; i++) {
          os_memcpy(mac_addr_list[i], mac_addr_list[i+1], ETH_ALEN);
      }
      mac_addr_list_len--;
  }
  return hostapd_write_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list, mac_addr_list_len);
}
//<-- Add for SoftAp Advance Feature END

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
                                  size_t buf_len) {
  struct i802_bss *bss = priv;
  struct wpa_driver_nl80211_data *drv = bss->drv;
  struct ifreq ifr;
  android_wifi_priv_cmd priv_cmd;
  int ret = 0;
  union wpa_event_data event_data;
  //NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
  struct hostapd_data *hostapd = bss->ctx;
  struct mac_acl_entry *acl, *newacl;
  char cmd_buf[MAX_DRV_CMD_SIZE];
  int i, add;
  //<-- Add for SoftAp Advance Feature END

  wpa_printf(MSG_DEBUG, "%s: Driver cmd: %s\n", __func__, cmd);
  memset(&ifr, 0, sizeof(ifr));
  memset(&priv_cmd, 0, sizeof(priv_cmd));
  memset(&event_data, 0, sizeof(event_data));

  if (os_strcasecmp(cmd, "STOP") == 0) {
    linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 0);
    wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
  } else if (os_strcasecmp(cmd, "START") == 0) {
    linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 1);
    wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
  } else if (os_strcasecmp(cmd, "MACADDR") == 0) {
    u8 macaddr[ETH_ALEN] = {};
    ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
    if (!ret)
      ret =
          os_snprintf(buf, buf_len, "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
  } else if (os_strncasecmp(cmd, "MAX_STA", 7) == 0) {
    int hdrlen = 2 * sizeof(unsigned short);
    char max_sta_num = *buf;
    wpa_printf(MSG_INFO, "max_sta_num: %d, buf_len: %zu.", max_sta_num, buf_len);
    memset(buf, 0, buf_len);
    *(unsigned short *)buf = CMD_SUBTYPE_MAX_NUM_STA;
    *(unsigned short *)(buf + hdrlen / 2) = sizeof(max_sta_num);
    *(buf + hdrlen) = max_sta_num;
    buf_len = hdrlen + sizeof(max_sta_num);

    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);
    wpa_printf(MSG_DEBUG, "buf_len:%zu.", buf_len);
//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)buf;
#else
    priv_cmd.buf = buf;
#endif
//<-- Add for SoftAp Advance Feature END
    priv_cmd.used_len = buf_len;
    priv_cmd.total_len = buf_len;
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 2, &ifr)) < 0) {
      wpa_printf(MSG_ERROR, "%s: failed to issue private commands\n", __func__);
      ret = -1;
    } else {
      wpa_printf(MSG_INFO, "%s %s len = %d, %d", __func__, buf, ret,
                 strlen(buf));
    }

  } else if (os_strncasecmp(cmd, "MIRACAST", 8) == 0) {
    struct driver_cmd_msg *miracast;
    int value = atoi(cmd + 8);

    memset(buf, 0, buf_len);
    miracast = (struct driver_cmd_msg *)buf;
    miracast->msg_type = CMD_SUBTYPE_MIRACAST_MODE;
    miracast->msg_len = sizeof(int);
    memcpy(miracast->msg_data, &value, sizeof(value));

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)miracast;
#else
    priv_cmd.buf = (char *)miracast;
#endif
//<-- Add for SoftAp Advance Feature END
    priv_cmd.used_len = sizeof(*miracast) + miracast->msg_len;
    priv_cmd.total_len = priv_cmd.used_len;

    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 2, &ifr)) < 0) {
      wpa_printf(
          MSG_ERROR,
          "%s: failed to issue private commands(%s), total_len=%d, value=%d\n",
          __func__, strerror(errno), priv_cmd.total_len, value);
      ret = -1;
    }

    memset(buf, 0, buf_len);
  } else if (os_strncasecmp(cmd, "SETSUSPENDMODE", 14) == 0) {
    os_memcpy(buf, cmd, strlen(cmd) + 1);
    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)buf;
#else
    priv_cmd.buf = buf;
#endif
//<-- Add for SoftAp Advance Feature END
    priv_cmd.used_len = buf_len;
    priv_cmd.total_len = buf_len;
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 4, &ifr)) < 0) {
      wpa_printf(MSG_ERROR, "%s: failed to issue private commands(%s)\n",
                 __func__, strerror(errno));
      ret = -1;
    }
  } else if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
    os_memcpy(buf, cmd, strlen(cmd) + 1);
    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)buf;
#else
    priv_cmd.buf = buf;
#endif
//<-- Add for SoftAp Advance Feature END
    priv_cmd.used_len = buf_len;
    priv_cmd.total_len = buf_len;
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 5, &ifr)) < 0) {
      wpa_printf(MSG_ERROR, "%s: failed to issue private commands(%s)\n",
                 __func__, strerror(errno));
      ret = -1;
    } else {
      wpa_supplicant_event(drv->ctx, EVENT_CHANNEL_LIST_CHANGED, &event_data);
      wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret,
                 strlen(buf));
    }
  } else if (os_strncasecmp(cmd, "11VCFG_GET", 10) == 0) {
    os_memcpy(buf, cmd, strlen(cmd) + 1);
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)buf;
#else
    priv_cmd.buf = buf;
#endif
//<-- Add for SoftAp Advance Feature END
    priv_cmd.used_len = buf_len;
    priv_cmd.total_len = buf_len;
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
      wpa_printf(MSG_ERROR, "%s: failed to issue private commands\n", __func__);
    } else {
      wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret,
                 strlen(buf));
      wpa_printf(MSG_DEBUG, "11V_CFGGET: 0x%x ", buf[11]);
    }

  } else if (os_strncasecmp(cmd, "11VCFG_SET", 10) == 0) {
    os_memcpy(buf, cmd, 11);
    int tmp;
    tmp = atoi(&cmd[11]);
    buf[11] = tmp;
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
#ifdef USE_64_BIT_IPC
    priv_cmd.buf = (u64)(uintptr_t)buf;
#else
    priv_cmd.buf = buf;
#endif
//<-- Add for SoftAp Advance Feature END
    priv_cmd.used_len = buf_len;
    priv_cmd.total_len = buf_len;
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
      wpa_printf(MSG_ERROR, "%s: failed to issue private commands\n", __func__);
    } else {
      wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret,
                 strlen(buf));
      wpa_printf(MSG_DEBUG, "11V_CFGSET: 0x%x ", buf[11]);
    }
//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
    } else if (os_strncasecmp(cmd, "WHITE_ADD_ONCE", 14) == 0) {
      wpa_printf(MSG_INFO, "%s", cmd);
      os_memset(cmd_buf, 0, sizeof(cmd_buf));
      strncpy(cmd_buf, cmd, 9);
      cmd_buf[9] = ' ';
      os_snprintf(cmd_buf+10, 18, MACSTR, MAC2STR(&cmd[15]));
      wpa_printf(MSG_INFO, "%s", cmd_buf);
      if (android_priv_cmd(priv, cmd_buf) < 0)
          return -1;
      ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "WHITE_EN_ONCE", 13) == 0) {
      wpa_printf(MSG_INFO, "%s", cmd);
      if (android_priv_cmd(priv, "WHITE_EN \0") < 0)
          return -1;
      ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "WHITE_ADD", 9) == 0) {
      wpa_printf(MSG_INFO, "%s", cmd);
      char addr[ETH_ALEN];
      hwaddr_aton(cmd+10, addr);
      add = 1;
      i = 0;
      acl = hostapd->conf->accept_mac;
      while (i < hostapd->conf->num_accept_mac) {
          if (os_memcmp(acl[i].addr, addr, ETH_ALEN) == 0) {
              add = 0;
              wpa_printf(MSG_INFO, "This mac has been added.");
              break;
          }
          i++;
      }

      if (add) {
          if (android_priv_cmd(priv, cmd) < 0)
              return -1;

          newacl = os_realloc_array(acl, hostapd->conf->num_accept_mac+ 1, sizeof(*acl));
          if (newacl == NULL) {
              wpa_printf(MSG_ERROR, "MAC list reallocation failed");
              return -1;
          }

          hostapd->conf->accept_mac = newacl;
          os_memcpy(newacl[hostapd->conf->num_accept_mac].addr, addr, ETH_ALEN);
          hostapd->conf->num_accept_mac++;
      }
      ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "WHITE_DEL", 9) == 0 ) {
      wpa_printf(MSG_INFO, "%s", cmd);
      char addr[ETH_ALEN];
      hwaddr_aton(cmd+10, addr);
      i = 0;
      acl = hostapd->conf->accept_mac;
      if (android_priv_cmd(priv, cmd) < 0)
          return -1;

      while (i < hostapd->conf->num_accept_mac) {
          if (os_memcmp(acl[i].addr, addr, ETH_ALEN) == 0) {
              os_remove_in_array(acl, hostapd->conf->num_accept_mac, sizeof(*acl), i);
              hostapd->conf->num_accept_mac--;
              break;
          }
          i++;
      }
      ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "WHITE_EN", 8) == 0) {
        struct sta_info *sta;
        int sum = 0;
        os_memset(cmd_buf, 0, sizeof(cmd_buf));
        strncpy(cmd_buf, cmd, 8);
        cmd_buf[8] = ' ';
        acl = hostapd->conf->accept_mac;
        for (sta = hostapd->sta_list; sta; sta = sta->next) {
            i = 0; add = 1;
            while (i < hostapd->conf->num_accept_mac) {
                wpa_printf(MSG_INFO, "WHITE_EN accept " MACSTR, MAC2STR(acl[i].addr));
                if (os_memcmp(acl[i].addr, sta->addr, ETH_ALEN) == 0) {
                    add = 0;
                    break;
                }
                i++;
            }
            if (add) {
                wpa_printf(MSG_INFO, "WHITE_EN add " MACSTR, MAC2STR(sta->addr));
                os_snprintf(&cmd_buf[10+sum*18], 18, MACSTR, MAC2STR(sta->addr));
                cmd_buf[10+sum*18+17] = ' ';
                sum++;
            }
        }
        cmd_buf[9] = sum;
        wpa_printf(MSG_INFO, "WHITE_EN %d.", cmd_buf[9]);
        wpa_printf(MSG_INFO, "%s", cmd_buf);
        if (android_priv_cmd(priv, cmd_buf) < 0)
            return -1;
        ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "WHITE_DIS", 9) == 0) {
        u8 mac_addr_list[HOSTAP_MAX_BLOCK_NUM][ETH_ALEN];
        int mac_addr_list_len;
        struct sta_info *sta;
        int sum = 0;
        os_memset(cmd_buf, 0, sizeof(cmd_buf));
        strncpy(cmd_buf, cmd, 9);
        cmd_buf[9] = ' ';
        mac_addr_list_len = hostapd_read_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list);
        acl = hostapd->conf->accept_mac;
        for (sta = hostapd->sta_list; sta; sta = sta->next) {
            i = 0; add = 0;
            while(i < mac_addr_list_len){
                if(os_memcmp(mac_addr_list[i], sta->addr, ETH_ALEN) == 0){
                    add = 1;
                    break;
                 }
                 i++;
            }
            if(add){
                  wpa_printf(MSG_INFO, "WHITE_DIS add " MACSTR, MAC2STR(sta->addr));
                  os_snprintf(&cmd_buf[11+sum*18], 18, MACSTR, MAC2STR(sta->addr));
                  cmd_buf[11+sum*18+17] = ' ';
                  sum++;
            }
        }
        cmd_buf[10] = sum;
        wpa_printf(MSG_INFO, "WHITE_DIS %d.", cmd_buf[10]);
        wpa_printf(MSG_INFO, "%s", cmd_buf);
        if (android_priv_cmd(priv, cmd_buf) < 0)
            return -1;
        ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "BLOCK ", 6) == 0) {
        u8 mac_addr[ETH_ALEN];
        if(sscanf(cmd+6, MACSTR, &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) != 6)
            return -1;
        if(android_priv_cmd(priv, cmd) < 0)
            return -1;
        if(hostapd_add_station_to_mac_addr_list(mac_addr) < 0)
            return -1;
        ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "UNBLOCK ", 8) == 0) {
        u8 mac_addr[ETH_ALEN];
        if(sscanf(cmd+8, MACSTR, &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) != 6)
            return -1;
        if(android_priv_cmd(priv, cmd) < 0)
            return -1;
        if(hostapd_del_station_from_mac_addr_list(mac_addr) < 0)
            return -1;
        ret = os_snprintf(buf, buf_len, "OK");
    } else if (os_strncasecmp(cmd, "BLOCK_LIST", 10) == 0) {
        u8 mac_addr_list[HOSTAP_MAX_BLOCK_NUM][ETH_ALEN];
        int mac_addr_list_len;
        mac_addr_list_len = hostapd_read_mac_addr_list(HOSTAP_BLOCK_LIST_FILE, mac_addr_list);
        for(i=0; i<mac_addr_list_len; i++) {
            ret = os_snprintf(buf + i * 18, buf_len - i * 18, MACSTR " ", MAC2STR(mac_addr_list[i]));
        }
        ret = mac_addr_list_len * 18;
//<-- Add for SoftAp Advance Feature END
  } else if (os_strncasecmp(cmd, "OFFLOAD_ROAMING", 15) == 0) {
        if(android_priv_cmd(priv, cmd) < 0)
            return -1;
  } else { /* Use private command */
  }
  return ret;
}

int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration) {
  wpa_printf(MSG_DEBUG, "%s: NOT IMPLETE", __func__);

  return 0;
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len) {
  wpa_printf(MSG_DEBUG, "%s: NOT IMPLETE", __func__);

  /* Return 0 till we handle p2p_presence request completely in the driver */
  return 0;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow) {
  wpa_printf(MSG_DEBUG, "%s: NOT IMPLETE", __func__);

  return 0;
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
                                 const struct wpabuf *proberesp,
                                 const struct wpabuf *assocresp) {
  wpa_printf(MSG_DEBUG, "%s: NOT IMPLETE", __func__);

  return 0;
}

int wpa_driver_nl80211_driver_cmd_wnm(void *priv, enum wnm_oper oper,
                                      const u8 *peer, u8 *buf, u16 *buf_len) {
  struct i802_bss *bss = priv;
  struct wpa_driver_nl80211_data *drv = bss->drv;
  struct ifreq ifr;
  android_wifi_priv_cmd priv_cmd;
  int ret = 0;
  char buff[MAX_DRV_CMD_SIZE];

  wpa_printf(MSG_DEBUG, "wnm_oper: %d\n", oper);
  memset(buff, 0, sizeof(buff));
  memset(&ifr, 0, sizeof(ifr));
  memset(&priv_cmd, 0, sizeof(priv_cmd));

  os_memcpy(buf, "WNM_SLEEP", 9);
  buf[9] = ' ';
  buf[10] = oper;
  os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

  priv_cmd.buf = buff;
  priv_cmd.used_len = sizeof(buff);
  priv_cmd.total_len = sizeof(buff);
  ifr.ifr_data = &priv_cmd;

  if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
    wpa_printf(MSG_ERROR, "%s: failed to issue private commands\n", __func__);
  } else {
    wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret,
               strlen(buf));
  }

  return ret;
}

//=============================================================================
// add by sprd start
//=============================================================================
//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
int driver_init_mac_acl(struct hostapd_data *hapd)
{
    struct hostapd_config *conf = hapd->iconf;
    int err, i = 0;
    char buf[25];
    char reply[10];
    os_memset(buf, 0, 25);
    strncpy(buf, "WHITE_ADD_ONCE ", 15);
    while(i < conf->bss[0]->num_accept_mac){
        wpa_printf(MSG_INFO, "accept mac:" MACSTR, MAC2STR(conf->bss[0]->accept_mac[i].addr));
        os_memcpy(buf+15, conf->bss[0]->accept_mac[i].addr, ETH_ALEN);
        i++;
        err = wpa_driver_nl80211_driver_cmd(hapd->drv_priv, buf, reply, 10);
        if(err < 0){
            wpa_printf(MSG_ERROR, "WHITE_ADD_ONCE failed.");
            return err;
        }
    }
    if (conf->bss[0]->macaddr_acl == DENY_UNLESS_ACCEPTED) {
        os_memset(buf, 0, 25);
        strncpy(buf, "WHITE_EN_ONCE ", 14);
        err = wpa_driver_nl80211_driver_cmd(hapd->drv_priv, buf, reply, 10);
        if (err < 0){
            wpa_printf(MSG_ERROR, "WHITE_EN_ONCE failed.");
            return err;
        }
    }
    return 0;
}

int driver_init_max_sta_num(struct i802_bss *bss)
{
    char buf[15];
    struct hostapd_data *hostapd = bss->ctx;
    *buf = hostapd->conf->max_num_sta;
    if(wpa_driver_nl80211_driver_cmd(bss, "MAX_STA", buf, sizeof(buf)))
        wpa_printf(MSG_ERROR, "set max_num_sta to driver fail");
    return 0;
}
//=============================================================================
// add by sprd end
//=============================================================================
