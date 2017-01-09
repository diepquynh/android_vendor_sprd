#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <utils/Log.h>
#include <cutils/android_reboot.h>
#include "cutils/properties.h"
#include <hardware_legacy/power.h>

#include "engopt.h"
#include "eng_cmd4linuxhdlr.h"
#include "eng_sqlite.h"
#include "eng_diag.h"
#include "eng_pcclient.h"
#include "eng_at.h"
#include "eng_btwifiaddr.h"
#ifndef CONFIG_MINIENGPC
#ifdef ENG_AT_CHANNEL
#include "atci.h"
#endif
#endif
#include <poll.h>
#include <cutils/sockets.h>

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

#ifdef sp6810a
#define ENG_KEYPAD_PATH "/sys/devices/platform/sprd-keypad6810/emulate"
#elif sp8805ga
#define ENG_KEYPAD_PATH "/sys/devices/platform/sprd-keypad8805ga/emulate"
#elif sprdop
#define ENG_KEYPAD_PATH "/sys/devices/platform/sprd-keypad/emulate"
#else
#define ENG_KEYPAD_PATH "/sys/devices/platform/sprd-keypad/emulate"
#endif
#define DEV_TCARD_BLOCK_NAME "/dev/block/mmcblk1"
#define MAXLENRESP 10
#define SLOG_MODEM_SERVER_SOCK_NAME "slogmodem"
#define ENABLE_WCN_LOG_CMD  "ENABLE_LOG WCN\n"
#define DISABLE_WCN_LOG_CMD  "DISABLE_LOG WCN\n"
#define ENABLE_GNSS_LOG_CMD  "ENABLE_LOG GNSS\n"
#define DISABLE_GNSS_LOG_CMD  "DISABLE_LOG GNSS\n"

//AGDSP LOG, PCM DUMP status
static int g_agdsp_log_status = 0; // 0: Log Disabled, 1: To UART, 2: To AP
static int g_agdsp_pcm_status = 0; // 0: PCM disabled, 1: PCM enabled

extern int g_reset;
extern int g_setuart_ok;
extern eng_dev_info_t *g_dev_info;
extern void set_raw_data_speed(int fd, int speed);
extern int get_ser_diag_fd(void);
extern int translate_packet(char *dest,char *src,int size);
extern int eng_diag_write2pc(char* diag_data, int r_cnt, int ser_fd);
extern int eng_atdiag_hdlr(unsigned char *buf, int len, char *rsp);
#if defined(ENGMODE_EUT_BCM) || defined(ENGMODE_EUT_SPRD)
extern int eng_atdiag_euthdlr(char *buf, int len, char *rsp, int module_index);
#endif
extern int eng_gps_atdiag_euthdlr(char *buf, int len, char *rsp,
                                  int module_index);
extern void eng_check_factorymode(int normal_cali);
extern int turnoff_lcd_backlight(void);
static unsigned char g_buffer[ENG_BUFFER_SIZE];
static int eng_linuxcmd_rpoweron(char *req, char *rsp);
static int eng_linuxcmd_keypad(char *req, char *rsp);
static int eng_linuxcmd_vbat(char *req, char *rsp);
static int eng_linuxcmd_stopchg(char *req, char *rsp);
static int eng_linuxcmd_factoryreset(char *req, char *rsp);
static int eng_linuxcmd_getfactoryreset(char *req, char *rsp);
static int eng_linuxcmd_mmitest(char *req, char *rsp);
static int eng_linuxcmd_bttest(char *req, char *rsp);
static int eng_linuxcmd_getbtaddr(char *req, char *rsp);
static int eng_linuxcmd_setbtaddr(char *req, char *rsp);
static int eng_linuxcmd_gsnr(char *req, char *rsp);
static int eng_linuxcmd_gsnw(char *req, char *rsp);
static int eng_linuxcmd_getwifiaddr(char *req, char *rsp);
static int eng_linuxcmd_setwifiaddr(char *req, char *rsp);
static int eng_linuxcmd_writeimei(char *req, char *rsp);
static int eng_linuxcmd_getich(char *req, char *rsp);
static int eng_linuxcmd_simchk(char *req, char *rsp);
static int eng_linuxcmd_atdiag(char *req, char *rsp);
static int eng_linuxcmd_infactorymode(char *req, char *rsp);
static int eng_linuxcmd_fastdeepsleep(char *req, char *rsp);
static int eng_linuxcmd_chargertest(char *req, char *rsp);
static int eng_linuxcmd_bteutmode(char *req, char *rsp);
static int eng_linuxcmd_bleeutmode(char *req, char *rsp);
static int eng_linuxcmd_wifieutmode(char *req, char *rsp);
static int eng_linuxcmd_gpseutmode(char *req, char *rsp);
static int eng_linuxcmd_batttest(char *req, char *rsp);
static int eng_linuxcmd_temptest(char *req, char *rsp);
static int eng_linuxcmd_logctl(char *req,char *rsp);
static int eng_linuxcmd_rtctest(char *req, char *rsp);
static int eng_linuxcmd_setuartspeed(char* req, char* rsp);
static int eng_linuxcmd_wiqpb(char *req, char *rsp);
static int eng_linuxcmd_property(char *req, char *rsp);
static int eng_linuxcmd_audiologctl(char *req, char *rsp);
static int eng_linuxcmd_checksd(char *req,char *rsp);

static struct eng_linuxcmd_str eng_linuxcmd[] = {
    {CMD_SENDKEY, CMD_TO_AP, "AT+SENDKEY", eng_linuxcmd_keypad},
    {CMD_GETICH, CMD_TO_AP, "AT+GETICH?", eng_linuxcmd_getich},
    {CMD_ETSRESET, CMD_TO_AP, "AT+ETSRESET", eng_linuxcmd_factoryreset},
    {CMD_RPOWERON, CMD_TO_AP, "AT+RPOWERON", eng_linuxcmd_rpoweron},
    {CMD_GETVBAT, CMD_TO_AP, "AT+GETVBAT", eng_linuxcmd_vbat},
    {CMD_STOPCHG, CMD_TO_AP, "AT+STOPCHG", eng_linuxcmd_stopchg},
    {CMD_TESTMMI, CMD_TO_AP, "AT+TESTMMI", eng_linuxcmd_mmitest},
    {CMD_BTTESTMODE, CMD_TO_AP, "AT+BTTESTMODE", eng_linuxcmd_bttest},
    {CMD_GETBTADDR, CMD_TO_AP, "AT+GETBTADDR", eng_linuxcmd_getbtaddr},
    {CMD_SETBTADDR, CMD_TO_AP, "AT+SETBTADDR", eng_linuxcmd_setbtaddr},
    {CMD_GSNR, CMD_TO_AP, "AT+GSNR", eng_linuxcmd_gsnr},
    {CMD_GSNW, CMD_TO_AP, "AT+GSNW", eng_linuxcmd_gsnw},
    {CMD_GETWIFIADDR, CMD_TO_AP, "AT+GETWIFIADDR", eng_linuxcmd_getwifiaddr},
    {CMD_SETWIFIADDR, CMD_TO_AP, "AT+SETWIFIADDR", eng_linuxcmd_setwifiaddr},
    {CMD_ETSCHECKRESET, CMD_TO_AP, "AT+ETSCHECKRESET",
     eng_linuxcmd_getfactoryreset},
    {CMD_SIMCHK, CMD_TO_AP, "AT+SIMCHK", eng_linuxcmd_simchk},
    {CMD_INFACTORYMODE, CMD_TO_AP, "AT+FACTORYMODE",
     eng_linuxcmd_infactorymode},
    {CMD_FASTDEEPSLEEP, CMD_TO_APCP, "AT+SYSSLEEP", eng_linuxcmd_fastdeepsleep},
    {CMD_CHARGERTEST, CMD_TO_AP, "AT+CHARGERTEST", eng_linuxcmd_chargertest},
    {CMD_SPBTTEST, CMD_TO_AP, "AT+SPBTTEST", eng_linuxcmd_bteutmode},
    {CMD_SPBTTEST, CMD_TO_AP, "AT+SPBLETEST", eng_linuxcmd_bleeutmode},
    {CMD_SPWIFITEST, CMD_TO_AP, "AT+SPWIFITEST",
     eng_linuxcmd_wifieutmode},  // 21
    {CMD_SPGPSTEST, CMD_TO_AP, "AT+SPGPSTEST", eng_linuxcmd_gpseutmode},
    {CMD_ATDIAG, CMD_TO_AP, "+SPBTWIFICALI", eng_linuxcmd_atdiag},
    {CMD_BATTTEST, CMD_TO_AP, "AT+BATTTEST", eng_linuxcmd_batttest},
    {CMD_TEMPTEST, CMD_TO_AP, "AT+TEMPTEST", eng_linuxcmd_temptest},
    {CMD_LOGCTL, CMD_TO_AP, "AT+LOGCTL", eng_linuxcmd_logctl},
    {CMD_RTCTEST, CMD_TO_AP, "AT+RTCCTEST", eng_linuxcmd_rtctest},
    {CMD_SPWIQ, CMD_TO_AP, "AT+SPWIQ", eng_linuxcmd_wiqpb},
    {CMD_PROP, CMD_TO_AP, "AT+PROP", eng_linuxcmd_property},
    {CMD_SETUARTSPEED, CMD_TO_AP, "AT+SETUARTSPEED", eng_linuxcmd_setuartspeed},
    {CMD_AUDIOLOGCTL, CMD_TO_AP, "AT+SPAUDIOOP", eng_linuxcmd_audiologctl},
    {CMD_SPCHKSD,        CMD_TO_AP,     "AT+SPCHKSD",      eng_linuxcmd_checksd},
};

/** returns 1 if line starts with prefix, 0 if it does not */
static int eng_cmdstartwith(const char *line, const char *prefix) {
  for (; *line != '\0' && *prefix != '\0'; line++, prefix++) {
    if (*line != *prefix) {
      return 0;
    }
  }
  return *prefix == '\0';
}

int eng_at2linux(char *buf) {
  int ret = -1;
  int i;

  for (i = 0; i < (int)NUM_ELEMS(eng_linuxcmd); i++) {
    if (strcasestr(buf, eng_linuxcmd[i].name) != NULL) {
      ENG_LOG("eng_at2linux %s", eng_linuxcmd[i].name);
      if ((strcasestr(buf, "AT+TEMPTEST")) &&
          ((strcasestr(buf, "AT+TEMPTEST=1,0,1")) ||
           (strcasestr(buf, "AT+TEMPTEST=1,1,1")) ||
           (strcasestr(buf, "AT+TEMPTEST=1,0,4")) ||
           (strcasestr(buf, "AT+TEMPTEST=1,1,4"))))
        return -1;
      else
        return i;
    }
  }

  return ret;
}

eng_cmd_type eng_cmd_get_type(int cmd) {
  if (cmd <= NUM_ELEMS(eng_linuxcmd))
    return eng_linuxcmd[cmd].type;
  else
    return CMD_INVALID_TYPE;
}

int eng_linuxcmd_hdlr(int cmd, char *req, char *rsp) {
  if (cmd >= (int)NUM_ELEMS(eng_linuxcmd)) {
    ENG_LOG("%s: no handler for cmd%d", __FUNCTION__, cmd);
    return -1;
  }
  return eng_linuxcmd[cmd].cmd_hdlr(req, rsp);
}

int eng_linuxcmd_rpoweron(char *req, char *rsp) {
  sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
  sync();
  android_reboot(ANDROID_RB_RESTART2, 0, "fastsleep");

  return 0;
}

static int eng_get_device_from_path(const char *path, char *device_name) {
  char device[256];
  char mount_path[256];
  char rest[256];
  FILE *fp;
  char line[1024];

  if (!(fp = fopen("/proc/mounts", "r"))) {
    ENG_LOG("Error opening /proc/mounts (%s)", strerror(errno));
    return 0;
  }

  while (fgets(line, sizeof(line), fp)) {
    line[strlen(line) - 1] = '\0';
    sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
    if (!strcmp(mount_path, path)) {
      strcpy(device_name, device);
      fclose(fp);
      return 1;
    }
  }

  fclose(fp);
  return 0;
}

int eng_linuxcmd_factoryreset(char *req, char *rsp) {
  int ret = 1;
  char cmd[] = "--wipe_data";
  int fd = -1;
  char format_cmd[1024];
  char convert_name[] = "/dev/block/platform/sprd-sdhci.3/by-name/sd";
  static char MKDOSFS_PATH[] = "/system/bin/newfs_msdos";
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  ENG_LOG("Call %s\n", __FUNCTION__);

  /*format internal sd card. code from vold*/
  sprintf(format_cmd, "%s -F 32 -O android %s", MKDOSFS_PATH, convert_name);
  system(format_cmd);
  // delete files in ENG_RECOVERYDIR
  system("rm -r /cache/recovery");

  // mkdir ENG_RECOVERYDIR
  if (mkdir(ENG_RECOVERYDIR, mode) == -1) {
    ret = 0;
    ENG_LOG("%s: mkdir %s fail [%s]\n", __FUNCTION__, ENG_RECOVERYDIR,
            strerror(errno));
    goto out;
  }

  // create ENG_RECOVERYCMD
  fd = open(ENG_RECOVERYCMD, O_CREAT | O_RDWR, 0666);

  if (fd < 0) {
    ret = 0;
    ENG_LOG("%s: open %s fail [%s]\n", __FUNCTION__, ENG_RECOVERYCMD,
            strerror(errno));
    goto out;
  }
  if (write(fd, cmd, strlen(cmd)) < 0) {
    ret = 0;
    ENG_LOG("%s: write %s fail [%s]\n", __FUNCTION__, ENG_RECOVERYCMD,
            strerror(errno));
    goto out;
  }
  if (eng_sql_string2string_set("factoryrst", "DONE") == -1) {
    ret = 0;
    ENG_LOG("%s: set factoryrst fail\n", __FUNCTION__);
    goto out;
  }

  g_reset = 2;
// sync();
//__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
//        LINUX_REBOOT_CMD_RESTART2, "recovery");
out:
  if (ret == 1)
    sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
  else
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);

  if (fd >= 0) close(fd);

  return 0;
}

int eng_linuxcmd_getfactoryreset(char *req, char *rsp) {
  sprintf(rsp, "%s%s%s%s", eng_sql_string2string_get("factoryrst"), ENG_STREND,
          SPRDENG_OK, ENG_STREND);
  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);

  return 0;
}

int eng_linuxcmd_keypad(char *req, char *rsp) {
  int ret = 0, fd;
  char *keycode;
  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);

  keycode = strchr(req, '=');

  if (keycode == NULL) {
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
    ret = -1;
  } else {
    keycode++;
    ENG_LOG("%s: keycode = %s\n", __FUNCTION__, keycode);
    fd = open(ENG_KEYPAD_PATH, O_RDWR);
    if (fd >= 0) {
      ENG_LOG("%s: send keycode to emulator\n", __FUNCTION__);
      write(fd, keycode, strlen(keycode));
    } else {
      ENG_LOG("%s: open %s fail [%s]\n", __FUNCTION__, ENG_KEYPAD_PATH,
              strerror(errno));
    }
    if (fd >= 0) close(fd);
    sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
    ret = 0;
  }

  return ret;
}

int eng_linuxcmd_vbat(char *req, char *rsp) {
  int fd, ret = 1, len = 0;
  int voltage;
  float vol;
  char buffer[16];

  fd = open(ENG_BATVOL, O_RDONLY);
  if (fd < 0) {
    ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL, strerror(errno));
    ret = 0;
  }

  if (ret == 1) {
    memset(buffer, 0, sizeof(buffer));
    len = read(fd, buffer, sizeof(buffer));
    if (len > 0) {
      voltage = atoi(buffer);
      ENG_LOG("%s: buffer=%s; voltage=%d\n", __FUNCTION__, buffer, voltage);
      vol = ((float)voltage) * 0.001;
      sprintf(rsp, "%.3g%s%s%s", vol, ENG_STREND, SPRDENG_OK, ENG_STREND);
    } else {
      sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
    }
  } else {
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  }

  if (fd >= 0) close(fd);

  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);
  return 0;
}

int eng_linuxcmd_batttest(char *req, char *rsp) {
  char ptr_parm1[1];
  char ptr_parm2[1];
  float vol = 0;
  int fd, voltage = 0, ret = 1, chg_sts = 0;
  char buffer[16];
  int len = 0;

  req = strchr(req, '=');
  req++;
  ptr_parm1[0] = *req;
  req = strchr(req, ',');
  req++;
  ptr_parm2[0] = *req;

  if ((ptr_parm1[0] == '1') && (ptr_parm2[0] == '2')) {
    fd = open(ENG_BATTVOL_NOW, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL,
              strerror(errno));
      ret = 0;
    }

    if (ret == 1) {
      memset(buffer, 0, sizeof(buffer));
      len = read(fd, buffer, sizeof(buffer));
      if (len > 0) {
        voltage = atoi(buffer);
        ENG_LOG("%s: buffer=%s; voltage=%d\n", __FUNCTION__, buffer, voltage);
        vol = ((float)voltage) * 0.001 / 1000;
      } else {
        sprintf(rsp, "+BATTTEST:1,2 NA");
        if (fd >= 0) close(fd);
        return 0;
      }
    } else {
      sprintf(rsp, "+BATTTEST:1,2 NA");
      if (fd >= 0) close(fd);
      return 0;
    }

    if (fd >= 0) close(fd);
    sprintf(rsp, "+BATTTEST:1,%.3g", vol);
    return 0;
  } else if ((ptr_parm1[0] == '1') && (ptr_parm2[0] == '1')) {
    fd = open(ENG_BATTCHG_STS, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL,
              strerror(errno));
      ret = 0;
    }

    if (ret == 1) {
      memset(buffer, 0, sizeof(buffer));
      len = read(fd, buffer, sizeof(buffer));
      if (len > 0) {
        chg_sts = atoi(buffer);
        ENG_LOG("%s: buffer=%s; chg_sts=%d\n", __FUNCTION__, buffer, chg_sts);
      } else {
        if (chg_sts == 1) sprintf(rsp, "+BATTTEST:1,CHAR");
        if (fd >= 0) close(fd);
        return 0;
      }
    } else {
      if (chg_sts == 1) sprintf(rsp, "+BATTTEST:1,CHAR");
      if (fd >= 0) close(fd);
      return 0;
    }

    if (fd >= 0) close(fd);
    sprintf(rsp, "+BATTTEST:1,%d", chg_sts);
    return 0;
  } else {
    sprintf(rsp, "NA");
  }

  return 0;
}

int eng_linuxcmd_stopchg(char *req, char *rsp) {
  int fd;
  int ret = 1;
  char ok[] = "1";
  fd = open(ENG_STOPCHG, O_WRONLY);

  if (fd < 0) {
    ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL, strerror(errno));
    ret = 0;
  }

  if (ret == 1) {
    ret = write(fd, ok, strlen(ok));
    if (ret < 0)
      sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
    else
      sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
  } else {
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  }

  if (fd >= 0) close(fd);
  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);
  return 0;
}

int eng_linuxcmd_mmitest(char *req, char *rsp) {
  int rc;

  rc = eng_sql_string2int_get("result");

  if (rc == ENG_SQLSTR2INT_ERR)
    sprintf(rsp, "%d%s%s%s", rc, ENG_STREND, SPRDENG_ERROR, ENG_STREND);
  else
    sprintf(rsp, "%d%s%s%s", rc, ENG_STREND, SPRDENG_OK, ENG_STREND);

  return 0;
}

int eng_linuxcmd_bttest(char *req, char *rsp) {
  system("chown system system /dev/ttyS0");
  system("bccmd -t bcsp -d /dev/ttyS0 psload -r /etc/ps.psr");
  system("hciattach -s 921600 /dev/ttyS0 bcsp 921600");
  system("hciconfig hci0 up");
  system("hcitool cmd 0x03 0x0005 0x02 0x00 0x02");
  system("hcitool cmd 0x03 0x001A 0x03");
  system("hcitool cmd 0x06 0x0003");

  sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
  return 0;
}

int eng_linuxcmd_setbtaddr(char *req, char *rsp) {
  char address[64];
  char wifiaddr[32] = {0};
  char *ptr = NULL;
  char *endptr = NULL;
  int length;

  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);

  ptr = strchr(req, '"');
  if (ptr == NULL) {
    ENG_LOG("%s: req %s ERROR no start \"\n", __FUNCTION__, req);
    return -1;
  }

  ptr++;

  endptr = strchr(ptr, '"');
  if (endptr == NULL) {
    ENG_LOG("%s: req %s ERROR no end \"\n", __FUNCTION__, req);
    return -1;
  }

  length = endptr - ptr + 1;

  memset(address, 0, sizeof(address));
  snprintf(address, length, "%s", ptr);

  ENG_LOG("%s: bt address is %s; length=%d\n", __FUNCTION__, address, length);

  eng_btwifimac_read(wifiaddr, ENG_WIFI_MAC);
  eng_btwifimac_write(address, wifiaddr);
  sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);

  return 0;
}

int eng_linuxcmd_getbtaddr(char *req, char *rsp) {
  char btaddr[32] = {0};
  int ret = 0;

  ret = eng_btwifimac_read(btaddr, ENG_BT_MAC);
  if (ret < 0) {
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  } else {
    sprintf(rsp, "%s%s%s%s", btaddr, ENG_STREND, SPRDENG_OK, ENG_STREND);
  }

  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);

  return 0;
}

int eng_linuxcmd_gsnr(char *req, char *rsp) {
  sprintf(rsp, "%s%s%s%s", eng_sql_string2string_get("gsn"), ENG_STREND,
          SPRDENG_OK, ENG_STREND);
  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);
  return 0;
}

int eng_linuxcmd_gsnw(char *req, char *rsp) {
  char address[64];
  char *ptr = NULL;
  char *endptr = NULL;
  int length;

  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);

  ptr = strchr(req, '"');
  if (ptr == NULL) {
    ENG_LOG("%s: req %s ERROR no start \"\n", __FUNCTION__, req);
    return -1;
  }

  ptr++;

  endptr = strchr(ptr, '"');
  if (endptr == NULL) {
    ENG_LOG("%s: req %s ERROR no end \"\n", __FUNCTION__, req);
    return -1;
  }

  length = endptr - ptr + 1;

  memset(address, 0, sizeof(address));
  snprintf(address, length, "%s", ptr);

  ENG_LOG("%s: GSN is %s; length=%d\n", __FUNCTION__, address, length);

  eng_sql_string2string_set("gsn", address);
  sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
  return 0;
}

int eng_linuxcmd_getwifiaddr(char *req, char *rsp) {
  char wifiaddr[32] = {0};
  int ret = 0;

  ret = eng_btwifimac_read(wifiaddr, ENG_WIFI_MAC);
  if (ret < 0) {
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  } else {
    sprintf(rsp, "%s%s%s%s", wifiaddr, ENG_STREND, SPRDENG_OK, ENG_STREND);
  }

  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);

  return 0;
}

int eng_linuxcmd_setwifiaddr(char *req, char *rsp) {
  char address[64];
  char btaddr[32] = {0};
  char *ptr = NULL;
  char *endptr = NULL;
  int length;

  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);

  ptr = strchr(req, '"');
  if (ptr == NULL) {
    ENG_LOG("%s: req %s ERROR no start \"\n", __FUNCTION__, req);
    return -1;
  }

  ptr++;

  endptr = strchr(ptr, '"');
  if (endptr == NULL) {
    ENG_LOG("%s: req %s ERROR no end \"\n", __FUNCTION__, req);
    return -1;
  }

  length = endptr - ptr + 1;

  memset(address, 0, sizeof(address));
  snprintf(address, length, "%s", ptr);

  ENG_LOG("%s: wifi address is %s; length=%d\n", __FUNCTION__, address, length);

  eng_btwifimac_read(btaddr, ENG_BT_MAC);
  eng_btwifimac_write(btaddr, address);

  sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);

  return 0;
}

int eng_linuxcmd_writeimei(char *req, char *rsp) {
  return eng_diag_writeimei(req, rsp);
}

int eng_linuxcmd_getich(char *req, char *rsp) {
  int fd, ret = 1;
  int current;
  char buffer[16];
  int len = 0;

  fd = open(ENG_CURRENT, O_RDONLY);
  if (fd < 0) {
    ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL, strerror(errno));
    ret = 0;
  }

  if (ret == 1) {
    memset(buffer, 0, sizeof(buffer));
    len = read(fd, buffer, sizeof(buffer));
    if (len > 0) {
      current = atoi(buffer);
      ENG_LOG("%s: buffer=%s; current=%d\n", __FUNCTION__, buffer, current);
      sprintf(rsp, "%dmA%s%s%s", current, ENG_STREND, SPRDENG_OK, ENG_STREND);
    } else {
      ENG_LOG("%s: ERROR\n", __FUNCTION__);
      sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
    }
  } else {
    ENG_LOG("%s: ERROR\n", __FUNCTION__);
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  }

  if (fd >= 0) close(fd);

  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);
  return 0;
}

static int eng_simtest_checksim_euicc(int type) {
  int fd, ret = 0;
  char simstatus;
  char cmd[32];
  char *atchannel, *atrsp;
  int len = 0;
  ENG_LOG("%s: type=%d\n", __FUNCTION__, type);

#ifndef ENG_AT_CHANNEL
  // @alvin: FIX ME: which channel should we use ?
  atchannel = eng_diag_at_channel();
  fd = eng_open_dev(atchannel, O_WRONLY);
  if (fd < 0) {
    ENG_LOG("%s: can't open at channel: %s", __FUNCTION__, atchannel);
  }

  do {
    memset(cmd, 0, sizeof(cmd));
    strcpy(cmd, "at+euicc?\r");
    len = write(fd, cmd, strlen(cmd));
    if (len <= 0) {
      continue;
    }
    memset(cmd, 0, sizeof(cmd));
    len = read(fd, cmd, sizeof(cmd));
    if (len <= 0) {
      continue;
    }
    ENG_LOG("%s: response=%s\n", __FUNCTION__, cmd);
  } while (strstr(cmd, "err") != NULL);

  close(fd);
#else  // use at channel: modemid=0 & simid=0 is temporary
#ifndef CONFIG_MINIENGPC
  atrsp = sendCmd(0, "at_euicc?\r");
  memset(cmd, 0, sizeof(cmd));
  strcpy(cmd, atrsp);
#endif
#endif
  simstatus = cmd[0];

  if (simstatus == '2') {
    ret = 0;
  } else if ((simstatus == '0') || (simstatus == '1')) {
    ret = 1;
  }

  return ret;
}

int eng_linuxcmd_simchk(char *req, char *rsp) {
  int sim1 = 0;

  sim1 = eng_simtest_checksim_euicc(0);

  sprintf(rsp, "%d%s%s%s", sim1, ENG_STREND, SPRDENG_OK, ENG_STREND);

  return 0;
}

int eng_ascii2hex(char *inputdata, unsigned char *outputdata,
                  int inputdatasize) {
  int i, j, tmp_len;
  unsigned char tmpData;
  char *ascii_data = inputdata;
  unsigned char *hex_data = outputdata;
  for (i = 0; i < inputdatasize; i++) {
    if ((ascii_data[i] >= '0') && (ascii_data[i] <= '9')) {
      tmpData = ascii_data[i] - '0';
    } else if ((ascii_data[i] >= 'A') && (ascii_data[i] <= 'F')) {  // A....F
      tmpData = ascii_data[i] - 'A' + 10;
    } else if ((ascii_data[i] >= 'a') && (ascii_data[i] <= 'f')) {  // a....f
      tmpData = ascii_data[i] - 'a' + 10;
    } else {
      break;
    }
    ascii_data[i] = tmpData;
  }

  for (tmp_len = 0, j = 0; j < i; j += 2) {
    outputdata[tmp_len++] = (ascii_data[j] << 4) | ascii_data[j + 1];
  }

  ENG_LOG("%s: length=%d", __FUNCTION__, tmp_len);
  for (i = 0; i < tmp_len; i++)
    ENG_LOG("%s [%d]: 0x%x", __FUNCTION__, i, hex_data[i]);

  return tmp_len;
}
int eng_linuxcmd_atdiag(char *req, char *rsp) {
  int len, ret;
  char tmp[3];
  char *data = req;
  char *ptr = NULL;

  ENG_LOG("Call %s", __FUNCTION__);

  if (strstr(req, "DEVT"))
    ret = test_dev(req, rsp);
  else {
    memset(g_buffer, 0, sizeof(g_buffer));
    at_tok_start(&data);
    at_tok_nextstr(&data, &ptr);

    len = eng_ascii2hex(ptr, g_buffer, strlen(ptr));
    ret = eng_atdiag_hdlr(g_buffer, len, rsp);
  }
  return ret;
}

int eng_linuxcmd_bteutmode(char *req, char *rsp) {
  int ret = -1;
  int len = 0;

  ENG_LOG("%s(), cmd = %s", __func__, req);
#if defined(ENGMODE_EUT_BCM) || defined(ENGMODE_EUT_SPRD)
  ret = eng_atdiag_euthdlr(req, len, rsp, BT_MODULE_INDEX);
#endif
  return ret;
}

static int eng_linuxcmd_bleeutmode(char *req, char *rsp) {
  int ret = -1;
  int len = 0;

  ENG_LOG("%s(), cmd = %s", __func__, req);
#if defined(ENGMODE_EUT_BCM) || defined(ENGMODE_EUT_SPRD)
  ret = eng_atdiag_euthdlr(req, len, rsp, BLE_MODULE_INDEX);
#endif
  return ret;
}

int eng_linuxcmd_wifieutmode(char *req, char *rsp) {
  int ret = -1;
  int len = 0;

  ENG_LOG("%s(), cmd = %s", __func__, req);
#if defined(ENGMODE_EUT_BCM) || defined(ENGMODE_EUT_SPRD)
  ret = eng_atdiag_euthdlr(req, len, rsp, WIFI_MODULE_INDEX);
#endif
  return ret;
}

int eng_linuxcmd_gpseutmode(char *req, char *rsp) {
  int ret = -1;
  int len = 0;

  ENG_LOG("Call %s     Command is  %s", __FUNCTION__, req);

  // #if defined(ENGMODE_EUT_BCM)
  // ret = eng_atdiag_euthdlr(req, len, rsp, GPS_MODULE_INDEX);
  // #else
  ret = eng_gps_atdiag_euthdlr(req, len, rsp, GPS_MODULE_INDEX);
  // #endif

  return ret;
}

int eng_linuxcmd_infactorymode(char *req, char *rsp) {
  char *ptr;
  int status;
  int length = strlen(req);
  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);
  if ((ptr = strchr(req, '?')) != NULL) {
    status = eng_sql_string2int_get(ENG_TESTMODE);
    if (status == ENG_SQLSTR2INT_ERR) status = 1;
    sprintf(rsp, "%d%s%s%s", status, ENG_STREND, SPRDENG_OK, ENG_STREND);
  } else if ((ptr = strchr(req, '=')) != NULL) {
    ptr++;
    if (ptr <= (req + length)) {
      status = atoi(ptr);
      ENG_LOG("%s: status=%d\n", __FUNCTION__, status);
      if (status == 0 || status == 1) {
        eng_sql_string2int_set(ENG_TESTMODE, status);
        eng_check_factorymode(0);
        sprintf(rsp, "%s\r\n", SPRDENG_OK);
      } else {
        sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
      }
    } else {
      sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
    }
  } else {
    sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
  }
  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);

  return 0;
}

#define DEEP_WAIT_TIME 15
void *thread_fastsleep(void *para) {
  ALOGE("##: delay 2 seconds to wait AT command has been sent to modem...\n");
  sleep(2);
  ALOGE("##: Going to sleep mode!\n");
  turnoff_lcd_backlight();
  set_screen_state(0);
  ALOGE("##: Waiting for a while...\n");
  char cmd[] = {"echo mem > /sys/power/state"};
  system(cmd);
#if 0
  int fd, stringsize;

  fd = open("proc/syssleep", O_RDWR);
  if (fd < 0) {
    ALOGE("Unknown error: %s", strerror(errno));
    return NULL;
  }

  if (lseek(fd, 0, SEEK_SET) != 0) {
    ALOGE("Cant lseek error :%s", strerror(errno));
    goto leave;
  }

  stringsize = strlen("on");
  if (write(fd, "on", stringsize) != stringsize) {
    ALOGE("Could not write error :%s", strerror(errno));
    goto leave;
  }
leave:
  close(fd);
#endif
  return NULL;
}

int eng_linuxcmd_fastdeepsleep(char *req, char *rsp) {
  pthread_t thread_id;
  int ret;

#if 1
  ret = pthread_create(&thread_id, NULL, thread_fastsleep, NULL);
  if (0 != ret) {
    ALOGE("##: Can't create thread[thread_fastsleep]!\n");
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  } else {
    ALOGE("##: Ccreate thread[thread_fastsleep] sucessfully!\n");
    sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);
  }
#endif
  return 0;
}

int eng_linuxcmd_chargertest(char *req, char *rsp) {
  char *ptr;
  int status, fd;
  int length = strlen(req);

  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);
  if ((ptr = strchr(req, '=')) != NULL) {
    ptr++;
    if (ptr <= (req + length)) {
      status = atoi(ptr);
      ENG_LOG("%s: status=%d\n", __FUNCTION__, status);
      if (status == 1) {
        ENG_LOG("%s: Create %s", __FUNCTION__, ENG_CHARGERTEST_FILE);
        fd = open(ENG_CHARGERTEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0) close(fd);
        sprintf(rsp, "%s\r\n", SPRDENG_OK);

      } else if (status == 0) {
        ENG_LOG("%s: Delete %s", __FUNCTION__, ENG_CHARGERTEST_FILE);
        if (remove(ENG_CHARGERTEST_FILE) == 0) {
          sprintf(rsp, "%s\r\n", SPRDENG_OK);
        } else {
          sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
        }
      } else {
        sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
      }
    } else {
      sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
    }
  } else {
    sprintf(rsp, "%s\r\n", SPRDENG_ERROR);
  }

  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);

  return 0;
}

int eng_linuxcmd_temptest(char *req, char *rsp) {
  char ptr_parm1[1];
  char ptr_parm2[1];
  char ptr_parm3[1];
  float vol = 0;
  int fd, temp_val = 0, ret = 1, len = 0;
  char buffer[16];

  req = strchr(req, '=');
  req++;
  ptr_parm1[0] = *req;
  req = strchr(req, ',');
  req++;
  ptr_parm2[0] = *req;
  req = strchr(req, ',');
  req++;
  ptr_parm3[0] = *req;

  if ((ptr_parm1[0] == '1') && (ptr_parm2[0] == '0') && (ptr_parm3[0] == '1')) {
    fd = open(ENG_WPATEMP, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_WPATEMP,
              strerror(errno));
      ret = 0;
    }

    if (ret == 1) {
      memset(buffer, 0, sizeof(buffer));
      len = read(fd, buffer, sizeof(buffer));
      if (len > 0) {
        temp_val = atoi(buffer);
        ENG_LOG("%s: buffer=%s; temp_val=%d\n", __FUNCTION__, buffer, temp_val);
      } else {
        sprintf(rsp, "+CME Error:NG");
        if (fd >= 0) close(fd);
        return 0;
      }
    } else {
      sprintf(rsp, "+CME Error:NG");
      if (fd >= 0) close(fd);
      return 0;
    }

    if (fd >= 0) close(fd);
    sprintf(rsp, "+TEMPTEST:1,%d", temp_val);
    return 0;
  }
  if ((ptr_parm1[0] == '1') && (ptr_parm2[0] == '1') && (ptr_parm3[0] == '1')) {
    fd = open(ENG_DCXOTEMP, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_DCXOTEMP,
              strerror(errno));
      ret = 0;
    }

    if (ret == 1) {
      memset(buffer, 0, sizeof(buffer));
      len = read(fd, buffer, sizeof(buffer));
      if (len > 0) {
        temp_val = atoi(buffer);
        ENG_LOG("%s: buffer=%s; temp_val=%d\n", __FUNCTION__, buffer, temp_val);
      } else {
        sprintf(rsp, "+CME Error:NG");
        if (fd >= 0) close(fd);
        return 0;
      }
    } else {
      sprintf(rsp, "+CME Error:NG");
      if (fd >= 0) close(fd);
      return 0;
    }

    if (fd >= 0) close(fd);
    sprintf(rsp, "+TEMPTEST:1,%d", temp_val);
    return 0;
  }
  if ((ptr_parm1[0] == '1') && (ptr_parm2[0] == '0')) {
    fd = open(ENG_BATTTEMP, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL,
              strerror(errno));
      ret = 0;
    }

    if (ret == 1) {
      memset(buffer, 0, sizeof(buffer));
      len = read(fd, buffer, sizeof(buffer));
      if (len > 0) {
        temp_val = atoi(buffer);
        ENG_LOG("%s: buffer=%s; temp_val=%d\n", __FUNCTION__, buffer, temp_val);
      } else {
        sprintf(rsp, "+CME Error:NG");
        if (fd >= 0) close(fd);
        return 0;
      }
    } else {
      sprintf(rsp, "+CME Error:NG");
      if (fd >= 0) close(fd);
      return 0;
    }

    if (fd >= 0) close(fd);
    sprintf(rsp, "+TEMPTEST:1,%d", temp_val);
    return 0;
  } else if ((ptr_parm1[0] == '1') && (ptr_parm2[0] == '1')) {
    fd = open(ENG_BATTTEMP_ADC, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s fail [%s]", __FUNCTION__, ENG_BATVOL,
              strerror(errno));
      ret = 0;
    }

    if (ret == 1) {
      memset(buffer, 0, sizeof(buffer));
      len = read(fd, buffer, sizeof(buffer));
      if (len > 0) {
        temp_val = atoi(buffer);
        ENG_LOG("%s: buffer=%s; temp_val=%d\n", __FUNCTION__, buffer, temp_val);
      } else {
        sprintf(rsp, "+CME Error:NG");
        if (fd >= 0) close(fd);
        return 0;
      }
    } else {
      sprintf(rsp, "+CME Error:NG");
      if (fd >= 0) close(fd);
      return 0;
    }

    if (fd >= 0) close(fd);
    sprintf(rsp, "+TEMPTEST:1,%d", temp_val);
    return 0;
  }

  return 0;
}

static int get_response(int fd, size_t timeout) {
  int ret = -1;
  char resp[MAXLENRESP] = {0};
  struct pollfd r_pollfd;

  r_pollfd.fd = fd;
  r_pollfd.events = POLLIN;

  ret = poll(&r_pollfd, 1, timeout);
  if(ret < 0) {
    ENG_LOG("poll slogmodem fail\n");
  } else {
    if (r_pollfd.revents & POLLIN) {
      if ((read(fd, resp, MAXLENRESP)) < 3 || memcmp(resp, "OK\n", 3)) {
        ENG_LOG("err response from slogmodem\n");
      } else {
        ret = 1;
      }
    }
  }
  return ret;
}

static int notice_slogmodem(char *cmd)
{
  int ret = 0;
  int fd = socket_local_client(SLOG_MODEM_SERVER_SOCK_NAME,
      ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  if (fd < 0) {
    ENG_LOG("can't connect to slogmodem server, ERROR:%s\n", strerror(errno));
    return -1;
  }

  int flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  int err = fcntl(fd, F_SETFL, flags);
  if (-1 == err) {
    ENG_LOG("set slogmodem socket to O_NONBLOCK error\n");
    ret = -1;
    goto out;
  }

  int len = write(fd, cmd, strlen(cmd));
  if (strlen(cmd) != len) {
    ENG_LOG("FLUSH command write error, len=%d, ERROR:%s\n", len, strerror(errno));
    ret = -1;
    goto out;
  }

  // Wait for the response for a while before failure.
  int result = get_response(fd, 3000);
  if (1 != result) {
    ENG_LOG("ERROR: get response %d\n", result);
    ret = -1;
    goto out;
  }

out:
  close(fd);
  return ret;
}

static int eng_linuxcmd_logctl(char *req, char *rsp)
{
  char *type;
  char ptr_cmd[1];
  int fd = -1;
  int ret = -1;
  int cali_flag = g_dev_info->host_int.cali_flag;

  req = strchr(req, '=');
  if(NULL == req) {
    ENG_LOG("%s: ERROR: invalid cmmond\n", __FUNCTION__);
    goto out;
  }
  req++;
  type = req;
  req = strchr(req, ',');
  if(NULL == req) {
    ENG_LOG("%s: ERROR: invalid cmmond\n", __FUNCTION__);
    goto out;
  }
  *req = '\0';
  req++;
  ptr_cmd[0] = *req;

  if(0 == strncasecmp(type, "WCN", strlen("WCN")) && 1 == cali_flag) {

    if(ptr_cmd[0] == '1') {
      //notice slogmodem
      if(notice_slogmodem(ENABLE_WCN_LOG_CMD) < 0) {
        goto out;
      }
      //enable wcn armlog
      system("wcnd_cli wcn poweron");
      system("wcnd_cli wcn at+armlog=1");
    } else if(ptr_cmd[0] == '0') {
      //notice slogmodem
      if(notice_slogmodem(DISABLE_WCN_LOG_CMD) < 0) {
        goto out;
      }
      //disable wcn armlog
      system("wcnd_cli wcn at+armlog=0");
    } else {
      ENG_LOG("%s: ERROR: invalid cmmond\n", __FUNCTION__);
      goto out;
    }
    sprintf(rsp, "+LOGCTL:%s%s", SPRDENG_OK, ENG_STREND);
    return 0;

  } else if (0 == strncasecmp(type, "GNSS", strlen("GNSS")) && 1 == cali_flag) {

    if(ptr_cmd[0] == '1') {
      if(notice_slogmodem(ENABLE_GNSS_LOG_CMD) < 0) {
        goto out;
      }
    } else if(ptr_cmd[0] == '0') {
      if(notice_slogmodem(DISABLE_GNSS_LOG_CMD) < 0) {
        goto out;
      }
    } else {
      ENG_LOG("%s: ERROR: invalid cmmond\n", __FUNCTION__);
      goto out;
    }
    sprintf(rsp, "+LOGCTL:%s%s", SPRDENG_OK, ENG_STREND);
    return 0;

  }

out:
  sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  return -1;
}

int eng_linuxcmd_rtctest(char *req, char *rsp) {
  char ptr_parm1[1];
  time_t t;
  struct tm tm;
  time_t timer;
  struct timeval tv;

  req = strchr(req, '=');
  req++;
  ptr_parm1[0] = *req;

  memset(&t, 0, sizeof(t));
  memset(&tm, 0, sizeof(tm));
  memset(&timer, 0, sizeof(timer));
  memset(&tv, 0, sizeof(tv));
  if ((ptr_parm1[0] == '1')) {
    t = time(NULL);
    localtime_r(&t, &tm);
    tm.tm_year = tm.tm_year + 1900;
    tm.tm_mon = tm.tm_mon + 1;
    sprintf(rsp, "1,%04d%02d%02d%02d%02d%02d%01d", tm.tm_year, tm.tm_mon,
            tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday);
  } else {
    char ptr_param[10];
    int value[7];
    int i = 0;
    int count = 0;

    memset(value, 0, sizeof(value));
    for (i = 0; i < 7; i++) {
      if (0 == i) {
        count = 4;
      } else if (6 == i) {
        count = 1;
      } else {
        count = 2;
      }
      req = strchr(req, ',');
      req++;
      memset(ptr_param, 0, sizeof(ptr_param));
      strncpy(ptr_param, req, count);
      value[i] = atoi(ptr_param);
      req += count;
    }

    tm.tm_year = value[0] - 1900;
    tm.tm_mon = value[1] - 1;
    tm.tm_mday = value[2];
    tm.tm_hour = value[3];
    tm.tm_min = value[4];
    tm.tm_sec = value[5];
    tm.tm_wday = value[6];

    timer = mktime(&tm);
    if (timer < 0) {
      sprintf(rsp, "error timer < 0");
      return -1;
    }
    tv.tv_sec = timer;
    tv.tv_usec = 0;
    if (settimeofday(&tv, NULL) < 0) {
      ALOGE("Set timer error \n");
      sprintf(rsp, "error,%04d%02d%02d%02d%02d%02d%01d", tm.tm_year, tm.tm_mon,
              tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday);
      return -1;
    }
    sprintf(rsp, "+RTCCTEST:2");
  }
  return 0;
}

static int eng_diag_enter_iq_pb_playback(void) {
  int fd = -1;
  fd = open(IQMODE_FLAG_PATH, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd < 0) {
    ENG_LOG("%s: create %s fail, %s\n", __FUNCTION__, IQMODE_FLAG_PATH,
            strerror(errno));
    return -1;
  }
  close(fd);

  // android_reboot(ANDROID_RB_RESTART2, 0, "iqmode");
  property_set(ANDROID_RB_PROPERTY, "reboot,iqmode");
  return 0;
}

static int eng_diag_iq_pb_normal(void) {
  int fd = -1;

  // android_reboot(ANDROID_RB_RESTART, 0, 0);//will trigger framework
  // systemdump
  property_set(ANDROID_RB_PROPERTY, "reboot");
  return 0;
}

static int eng_diag_iq_pb_capture(void) {
  int fd = -1;

  property_set(ANDROID_RB_PROPERTY, "reboot,iqmode");
  return 0;
}

int eng_linuxcmd_wiqpb(char *req, char *rsp) {
  char ptr_parm1[1];
  req = strchr(req, '=');
  req++;
  ptr_parm1[0] = *req;
  ENG_LOG("%s: AT+SPWIQ=%c\n", __FUNCTION__, ptr_parm1[0]);
  if (ptr_parm1[0] == '0') {
    eng_diag_iq_pb_normal(); // normal mode
  } else if (ptr_parm1[0] == '1') {
    eng_diag_iq_pb_capture(); // iq capture
  } else if (ptr_parm1[0] == '2') {
    eng_diag_enter_iq_pb_playback(); // iq playback
  } else {
    ENG_LOG("%s: AT+SPWIQ=%c is invaild value\n", __FUNCTION__, ptr_parm1[0]);
  }
  return 0;
}

int eng_linuxcmd_property(char *req, char *rsp) {
  char ptr_cmd[1];
  char *ptr_key, *ptr_val;
  char buf[PROPERTY_VALUE_MAX] = {0};

  req = strchr(req, '=');
  req++;
  ptr_cmd[0] = *req;
  req = strchr(req, ',');
  req++;
  if (*req != '[') {
    return -1;
  }
  req++;
  ptr_key = req;
  req = strchr(req, ']');
  *req = '\0';

  if (ptr_cmd[0] == '0') {
    property_get(ptr_key, buf, "NOT FOUND");
    if (!strncmp(buf, "NOT FOUND", sizeof("NOT FOUND") - 1)) {
      sprintf(rsp, "%s%s%s", ENG_STREND, buf, ENG_STREND);
    } else {
      sprintf(rsp, "%s%s%s%s%s", ENG_STREND, "[", buf, "]", ENG_STREND);
    }
  } else if (ptr_cmd[0] == '1') {
    req++;
    if (*req != '[') {
      return -1;
    }
    req++;
    ptr_val = req;
    req = strchr(req, ']');
    *req = '\0';
    property_set(ptr_key, ptr_val);
    sprintf(rsp, "%s%s%s%s%s%s%s", ENG_STREND, "[", ptr_key, "][", ptr_val, "]",
            ENG_STREND);
  }
  return 0;
}

int send_rsp2pc(char *buf, int buf_len, int fd)
{
  int ret = -1;
  char tmp[128] = {0};
  int tmp_len;
  char rsp[256] = {0};
  int rsp_len;
  MSG_HEAD_T head;
  head.len = sizeof(MSG_HEAD_T) + buf_len;
  ENG_LOG("%s: head.len=%d\n",__FUNCTION__, head.len);
  head.seq_num = 0;
  head.type = 0x9c;
  head.subtype = 0x00;
  memcpy(tmp, &head, sizeof(MSG_HEAD_T));
  memcpy(tmp + sizeof(MSG_HEAD_T), buf , buf_len);
  rsp_len = translate_packet(rsp, (unsigned char*)tmp, head.len);
  ret = eng_diag_write2pc(rsp, rsp_len, fd);
  if (ret <= 0) {
    ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
    return -1;
  }
  return 0;
}

int eng_linuxcmd_setuartspeed(char *req,char *rsp)
{
  int speed;
  char *ptr = NULL;
  char buf[64] = {0};

  if (NULL == g_dev_info) {
    ENG_LOG("%s: g_dev_info is NULL", __FUNCTION__);
    return -1;
  }

  ptr = strchr(req, '=');
  if (NULL == ptr) {
    ENG_LOG("%s: format error\n", __FUNCTION__);
    return -1;
  }
  ptr++;
  speed = atoi(ptr);
  ENG_LOG("%s: dev_diag=%s\n", __FUNCTION__, g_dev_info->host_int.dev_diag);

  if (g_dev_info->host_int.dev_type == CONNECT_UART) {
    int ser_fd = get_ser_diag_fd();

    sprintf(buf, "%s%s%s", ENG_STREND, SPRDENG_OK, ENG_STREND);
    if (send_rsp2pc(buf, strlen(buf), ser_fd) <0) {
      return -1;
    } else {
      usleep(200*1000);
    }

    set_raw_data_speed(ser_fd, speed);
    ENG_LOG("%s: set speed=%d success\n", __FUNCTION__, speed);

    g_setuart_ok = 1;
    return 0;
  }
  ENG_LOG("%s: set speed=%d fail\n", __FUNCTION__, speed);
  return -1;
}

int eng_linuxcmd_audiologctl(char *req, char *rsp) {
  /* AT+SPAUDIOOP=<log_dest>[,<pcm_enable>]
   * log_dest     : 0 - disable AGDSP log
   *                1 - enable AGDSP log to UART
   *                2 - enable AGDSP log to AP
   *
   * pcm_enable   : 0 - disable pcm dump,
   *                    if not specified, 0 is by default.
   *                1 - <log_dest> must be non-zero to
   *                    enable pcm dump
   *
   * if <log_dest> = 0, <pcm_enable> = 0.
   */
  const char* preq = req;
  while (' ' == *preq) {
    ++preq;
  }

  preq += 12;

  char ag_log_chan[ENG_DEV_PATH_LEN] = {0};
  char ag_pcm_chan[ENG_DEV_PATH_LEN] = {0};
  int log_fd = -1;
  int pcm_fd = -1;
  int ret = -1;
  int len = 0;

  property_get("ro.modem.ag.log", ag_log_chan, "");
  if (!ag_log_chan[0]) {
    ENG_LOG("%s fail to get agdsp log device file.", __FUNCTION__);
    goto out;
  }

  property_get("ro.modem.ag.pcm", ag_pcm_chan, "");
  if (!ag_pcm_chan[0]) {
    ENG_LOG("%s fail to get agdsp pcm device file.", __FUNCTION__);
    goto out;
  }

  log_fd = open(ag_log_chan, O_WRONLY);
  if (-1 == log_fd) {
    ENG_LOG("fail to open agdsp log device.");
    goto out;
  }

  pcm_fd = open(ag_pcm_chan, O_WRONLY);
  if (-1 == pcm_fd) {
    ENG_LOG("fail to open agdsp pcm device.");
    goto out;
  }

  if ('=' == *preq) {
    ++preq;
    if ('?' == *preq) {
      ++preq;
      if ('\0' == *preq) {
        len = sprintf(rsp, "%s+SPAUDIOOP: (0,1,2),(0,1)%s",
                      ENG_STREND, ENG_STREND);
        ret = 0;
      }
    } else {
      switch (*preq) {
        case '0':
          if (',' == (*(++preq))) {
            // +SPAUDIOOP=0,0
            ++preq;
            if ('0' != *preq) {
              break;
            }

            if ('\0' != *(++preq)) {
              break;
            }
          } else if ('\0' != (*preq)) {
            // +SPAUDIOOP=0
            break;
          }

          if (!ioctl(log_fd, DSPLOG_CMD_LOG_ENABLE, 0)) {
            g_agdsp_log_status = 0;
            g_agdsp_pcm_status = 0;
            ret = 0;
          } else {
            ENG_LOG("AG-DSP log ioctl(DSPLOG_CMD_LOG_ENABLE, %d) error.", 0);
          }
          break;
        case '1':  // +SPAUDIOOP=(1,2)[,(0,1)]
        case '2':;
          int enable_log_status = *preq - '0';
          int set_pcm_status = -1;

          if (',' == (*(++preq))) {
            set_pcm_status = *(++preq) - '0';

            if ((0 != set_pcm_status) &&
                (1 != set_pcm_status)) {
              break;
            }

            if ('\0' != (*(++preq))) {
              break;
            }
          } else if ('\0' == *preq) {
            // +SPAUDIOOP=(1,2)
            set_pcm_status = 0;
          } else {
            break;
          }

          if (!ioctl(log_fd, DSPLOG_CMD_LOG_ENABLE, (void*)enable_log_status)) {
            g_agdsp_log_status = enable_log_status;

            if (!ioctl(pcm_fd, DSPLOG_CMD_PCM_ENABLE, (void*)set_pcm_status)) {
              g_agdsp_pcm_status = set_pcm_status;
              ret = 0;
            } else {
              ENG_LOG("AG-DSP pcm ioctl(DSPLOG_CMD_PCM_ENABLE, %d) error.",
                      set_pcm_status);
            }
          } else {
            ENG_LOG("AG-DSP log ioctl(DSPLOG_CMD_LOG_ENABLE, %d) error.",
                    enable_log_status);
          }
          break;
        default:
          break;
      }
    }
  } else if ('?' == *preq) {
    if ('\0' == *(++preq)) {
      len = sprintf(rsp, "%s+SPAUDIOOP: %d,%d%s",
                    ENG_STREND, g_agdsp_log_status,
                    g_agdsp_pcm_status, ENG_STREND);
      ret = 0;
    }
  }

out:
  if (-1 != log_fd) {
    close(log_fd);
  }

  if (-1 != pcm_fd) {
    close(pcm_fd);
  }

  if (-1 == ret) {
    len = sprintf(rsp, "%s%s%s", ENG_STREND, SPRDENG_ERROR, ENG_STREND);
  } else if (0 == ret) {
    len += sprintf(rsp + len, "%s%s%s", ENG_STREND, SPRDENG_OK, ENG_STREND);
  }

  return len;
}

static int eng_linuxcmd_checksd(char *req, char *rsp)
{
	if (0 == access(DEV_TCARD_BLOCK_NAME , F_OK)){
		ENG_LOG("SD card exist\n");
		sprintf(rsp, "%s%s", "1", ENG_STREND);
	}else{
		ENG_LOG("SD card no exist\n");
		sprintf(rsp, "%s%s", "0", ENG_STREND);
	}
	return 0;
}

