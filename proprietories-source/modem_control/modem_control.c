/**
 * modem_control.c ---
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>
#include <fcntl.h>
#include "modem_control.h"
#include "packet.h"
#include "nv_read.h"
#ifdef SECURE_BOOT_ENABLE
#include "modem_verify.h"
#endif

#define BM_DEV "/dev/sprd_bm"
#define DMC_MPU "/dev/dmc_mpu"

#define MODEM_MAGIC "SCI1"
#define MODEM_HDR_SIZE 12  // size of a block
#define SCI_TYPE_MODEM_BIN 1
#define SCI_TYPE_PARSING_LIB 2
#define MODEM_LAST_HDR 0x100
#define MODEM_SHA1_HDR 0x400
#define MODEM_SHA1_SIZE 20

typedef struct {
  uint32_t type_flags;
  uint32_t offset;
  uint32_t length;
} data_block_header_t;

enum sci_bm_cmd_index {
  BM_STATE = 0x0,
  BM_CHANNELS,
  BM_AXI_DEBUG_SET,
  BM_AHB_DEBUG_SET,
  BM_PERFORM_SET,
  BM_PERFORM_UNSET,
  BM_OCCUR,
  BM_CONTINUE_SET,
  BM_CONTINUE_UNSET,
  BM_DFS_SET,
  BM_DFS_UNSET,
  BM_PANIC_SET,
  BM_PANIC_UNSET,
  BM_BW_CNT_START,
  BM_BW_CNT_STOP,
  BM_BW_CNT_RESUME,
  BM_BW_CNT,
  BM_BW_CNT_CLR,
  BM_DBG_INT_CLR,
  BM_DBG_INT_SET,
  BM_CMD_MAX,
};

static int assert_fd = 0;
int server_modem_ctrl = -1;
int client_modemd = -1;
sem_t sem_first_boot;
static bool g_b_wake_locking = false;
extern char g_modem[PROPERTY_VALUE_MAX];
static LOAD_VALUE_S load_value;

#ifdef SECURE_BOOT_ENABLE
uint8_t s_modem_puk[PUBKEY_LEN] = {0};

int HexToBin(const char *hex_ptr, int length, char *bin_ptr) {
  char *dest_ptr = bin_ptr;
  int i;
  char ch;

  if (hex_ptr == NULL || bin_ptr == NULL) {
    return -1;
  }

  for (i = 0; i < length; i += 2) {
    ch = hex_ptr[i];
    if (ch >= '0' && ch <= '9')
      *dest_ptr = (char)((ch - '0') << 4);
    else if (ch >= 'a' && ch <= 'f')
      *dest_ptr = (char)((ch - 'a' + 10) << 4);
    else if (ch >= 'A' && ch <= 'F')
      *dest_ptr = (char)((ch - 'A' + 10) << 4);
    else
      return -1;

    ch = hex_ptr[i + 1];
    if (ch >= '0' && ch <= '9')
      *dest_ptr |= (char)(ch - '0');
    else if (ch >= 'a' && ch <= 'f')
      *dest_ptr |= (char)(ch - 'a' + 10);
    else if (ch >= 'A' && ch <= 'F')
      *dest_ptr |= (char)(ch - 'A' + 10);
    else
      return -1;

    dest_ptr++;
  }
  return 0;
}

static int modem_parse_puk_cmdline(uint8_t *puk_ptr) {
  int fd = 0, ret = 0, i = 0, flag = 0;
  char cmdline[CMDLINE_LENGTH] = {0};
  char puk_str[PUBKEY_LEN * 2 + 1] = {0};
  char *p_str = NULL;

  /* Read PUK from cmdline */
  fd = open("/proc/cmdline", O_RDONLY);
  if (fd < 0) {
    MODEM_LOGD("[secure]%s, /proc/cmdline open failed", __FUNCTION__);
    return 0;
  }
  ret = read(fd, cmdline, sizeof(cmdline));
  if (ret < 0) {
    MODEM_LOGD("[secure]%s,/proc/cmdline read failed", __FUNCTION__);
    close(fd);
    return 0;
  }
  MODEM_LOGD("[secure]%s,cmdline: %s\n", __FUNCTION__, cmdline);
  p_str = strstr(cmdline, CMD_PUKSTRING);
  if (p_str != NULL) {
    p_str += strlen(CMD_PUKSTRING);
    memcpy(puk_str, p_str, PUBKEY_LEN * 2);
    MODEM_LOGD("[secure]%s, puk_str = %s\n", __FUNCTION__, puk_str);
    HexToBin(puk_str, PUBKEY_LEN * 2, puk_ptr);
    flag = 1;
  } else {
    MODEM_LOGD("[secure]%s, parse puk failed", __FUNCTION__);
  }
  return flag;
}

static int modem_verify_image(char *fin, int offsetin, int size) {
  int ret = 0;
  int fdin = -1, readsize = 0, imagesize = 0;
  uint8_t *buf = NULL;

  MODEM_LOGD("[secure]%s: enter", __FUNCTION__);
  MODEM_LOGD("[secure]%s: fin = %s, size = %d", __FUNCTION__, fin, size);

  /* Read image */
  fdin = open(fin, O_RDONLY, 0);
  if (fdin < 0) {
    MODEM_LOGE("[secure]%s: Failed to open %s", __FUNCTION__, fin);
    return -1;
  }
  if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
    MODEM_LOGE("[secure]failed to lseek %d in %s", offsetin, fin);
    ret = -1;
    goto leave;
  }

   imagesize = size;

  MODEM_LOGD("[secure]%s: imagesize = %d", __FUNCTION__, imagesize);
  buf = malloc(imagesize);
  if (buf == 0) {
    MODEM_LOGE("[secure]%s: Malloc failed!!", __FUNCTION__);
    ret = -1;
    goto leave;
  }
  memset(buf, 0, imagesize);
  readsize = read(fdin, buf, imagesize);
  MODEM_LOGD("[secure]%s: buf readsize = %d", __FUNCTION__, readsize);
  if (readsize <= 0) {
    MODEM_LOGE("[secure]failed to read %s", fin);
    ret = -1;
    goto leave;
  }

  /* Start verify */
  secure_verify(buf, s_modem_puk);
  ret = 0;
leave:
  close(fdin);
  free(buf);
  return ret;
}

#endif

static uint32_t get_boot_offset(IMAGE_LOAD_S *table, uint32_t secure_offset) {
  int fdin = -1;
  uint32_t offset = 0, hdr_offset = 0;
  uint8_t hdr_buf[MODEM_HDR_SIZE << 3] = {0};
  uint8_t read_len;
  uint8_t result = 0;  // 0:OK, 1:not find, 2:some error occur
  data_block_header_t *hdr_ptr = NULL;

  if (strstr(table->path_r, "modem")) {
    fdin = open(table->path_r, O_RDONLY, 0);
    if (fdin < 0) {
      return offset;
    }

    do {
      if (lseek(fdin, hdr_offset + secure_offset, SEEK_SET) !=
          (hdr_offset + secure_offset)) {
        result = 2;
        break;
      }

      read_len = read(fdin, hdr_buf, sizeof(hdr_buf));
      if (read_len !=
          sizeof(hdr_buf)) {  // image size should be > (MODEM_HDR_SIZE << 3)
        result = 2;
        break;
      }

      if (!hdr_offset) {
        if (memcmp(hdr_buf, MODEM_MAGIC, sizeof(MODEM_MAGIC))) {
          result = 2;
          MODEM_LOGD("old image format!\n");
          break;
        }

        hdr_ptr = (data_block_header_t *)hdr_buf + 1;
        hdr_offset = MODEM_HDR_SIZE;
      } else {
        hdr_ptr = (data_block_header_t *)hdr_buf;
      }

      data_block_header_t* endp
          = (data_block_header_t*)(hdr_buf + sizeof hdr_buf);
      int found = 0;
      while (hdr_ptr < endp) {
        uint32_t type = (hdr_ptr->type_flags & 0xff);
        if (SCI_TYPE_MODEM_BIN == type) {
          found = 1;
          break;
        }

        /*  There is a bug (622472) in MODEM image generator.
         *  To recognize wrong SCI headers and correct SCI headers,
         *  we devise the workaround.
         *  When the MODEM image generator is fixed, remove #if 0.
         */
#if 0
        if (hdr_ptr->type_flags & MODEM_LAST_HDR) {
          result = 2;
          MODEM_LOGE("no modem image, error image header!!!\n");
          break;
        }
#endif
        hdr_ptr++;
      }
      if (!found) {
        result = 2;
        MODEM_LOGE("no MODEM exe found in SCI header!");
      }

      if (result != 1) {
        break;
      }
    } while (1);

    if (!result) {
      offset = hdr_ptr->offset;
      if (hdr_ptr->type_flags & MODEM_SHA1_HDR) {
        offset += MODEM_SHA1_SIZE;
      }
    }

    close(fdin);
  }

  return offset;
}
int write_proc_file(char *file, int offset, char *string) {
  int fd, stringsize, res = -1, retry = 0;

  MODEM_LOGD("%s: file is %s, string is %s!\n", __FUNCTION__, file, string);

  do {
    fd = open(file, O_RDWR);
    if (fd < 0) {
      MODEM_LOGE("fd =%d: open file %s, error: %s", fd, file, strerror(errno));
      usleep(200000);
      retry++;
    }
  } while (fd < 0 && retry < 6);

  if (lseek(fd, offset, SEEK_SET) != offset) {
    MODEM_LOGE("Cant lseek file %s, error :%s", file, strerror(errno));
    goto leave;
  }

  stringsize = strlen(string);
  if (write(fd, string, stringsize) != stringsize) {
    MODEM_LOGE("Could not write %s in %s, error :%s", string, file,
               strerror(errno));
    goto leave;
  }

  res = 0;
leave:
  close(fd);

  return res;
}

void modemd_enable_busmonitor(bool bEnable) {
  int fd;
  int param;
  int cmd;

  fd = open(BM_DEV, O_RDWR);
  if (fd < 0) {
    MODEM_LOGIF("%s: %s failed, error: %s", __FUNCTION__, BM_DEV,
                strerror(errno));
    return;
  }

  cmd = bEnable ? BM_DBG_INT_SET : BM_DBG_INT_CLR;
  ioctl(fd, cmd, &param);

  MODEM_LOGIF("%s: bEnable = %d, cmd = %d", __FUNCTION__, bEnable, cmd);
  close(fd);
}

void modemd_enable_dmc_mpu(bool bEnable) {
  int fd;
  int param;
  int cmd;

  fd = open(DMC_MPU, O_RDWR);
  if (fd < 0) {
    MODEM_LOGD("%s: %s failed, error: %s", __FUNCTION__, DMC_MPU,
               strerror(errno));
    return;
  }

  cmd = bEnable;
  ioctl(fd, cmd, &param);

  MODEM_LOGD("%s: bEnable = %d, cmd = %d", __FUNCTION__, bEnable, cmd);
  close(fd);
}

int load_cp_cmdline(char *fin, char *fout)
{
  int fdin, fdout, wsize;
  char *str;
  char cmdline[CPCMDLINE_SIZE] = {0};

  MODEM_LOGD("%s (%s ==> %s)\n",__func__,
      fin, fout);
  modemd_enable_busmonitor(false);

  fdin = open(fin, O_RDONLY, 0);
  if (fdin < 0) {
    MODEM_LOGE("failed to open %s", fin);
    modemd_enable_busmonitor(true);
    return -1;
  }

  str = cmdline;
  if (read(fdin, cmdline, sizeof(cmdline)-1)> 0) {
    str = strstr(cmdline, "modem=");
    if (str != NULL){
      str = strchr(str,' ');
      str += 1;
    } else {
      MODEM_LOGD("cmdline 'modem=' is not exist\n");
      str = cmdline;
    }
  }

  fdout = open(fout, O_WRONLY, 0);
  if (fdout < 0) {
    close(fdin);
    MODEM_LOGE("failed to open %s, error: %s", fout, strerror(errno));
    modemd_enable_busmonitor(true);
    return -1;
  }

  /* some cp cmdline only 0x400, we just write all cp cmdline,
     and end with 0, so size is  strlen(str)*sizeof(char) + 1 */
  wsize = write(fdout, str, (strlen(str) + 1)*sizeof(char));
  MODEM_LOGD("write cmdline [wsize = %d]", wsize);

  if (wsize <= 0) {
    MODEM_LOGE("failed to write %s [wsize = %d]",
        fout, wsize);
  }

  modemd_enable_busmonitor(true);
  close(fdin);
  close(fdout);
  return wsize;
}

int load_sipc_image(char *fin, int offsetin, char *fout, int offsetout,
                    uint size) {
  int res = -1, fdin, fdout, bufsize, i, rsize, rrsize, wsize;
  char buf[8192];

  MODEM_LOGD("%s: (%s(%d) ==> %s(%d) size=0x%x)\n", __FUNCTION__, fin, offsetin,
             fout, offsetout, size);
  modemd_enable_busmonitor(false);
  modemd_enable_dmc_mpu(false);

  fdin = open(fin, O_RDONLY, 0);
  if (fdin < 0) {
    MODEM_LOGE("failed to open %s", fin);
    modemd_enable_busmonitor(true);
    modemd_enable_dmc_mpu(true);
    return -1;
  }

  fdout = open(fout, O_WRONLY, 0);
  if (fdout < 0) {
    close(fdin);
    MODEM_LOGE("failed to open %s, error: %s", fout, strerror(errno));
    modemd_enable_busmonitor(true);
    modemd_enable_dmc_mpu(true);
    return -1;
  }

  if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
    MODEM_LOGE("failed to lseek %d in %s", offsetin, fin);
    goto leave;
  }

  if (lseek(fdout, offsetout, SEEK_SET) != offsetout) {
    MODEM_LOGE("failed to lseek %d in %s", offsetout, fout);
    goto leave;
  }

  do {
    rsize = min(size, sizeof(buf));
    rrsize = read(fdin, buf, rsize);
    if (rrsize == 0) goto leave;
    if (rrsize < 0) {
      MODEM_LOGE("failed to read %s %s", fin, strerror(errno));
      goto leave;
    }
    wsize = write(fdout, buf, rrsize);

    MODEM_LOGIF("write %s [wsize=%d, rsize=%d, remain=%d]", fout, wsize, rsize,
                size);

    if (wsize <= 0) {
      MODEM_LOGE("failed to write %s [wsize=%d, rsize=%d, remain=%d]", fout,
                 wsize, rsize, size);
      goto leave;
    }
    size -= rrsize;
  } while (size > 0);
  res = 0;
leave:
  modemd_enable_busmonitor(true);
  modemd_enable_dmc_mpu(true);
  close(fdin);
  close(fdout);
  return res;
}

static int boot_from_table(LOAD_TABLE_S *tables, uint load_max) {
  IMAGE_LOAD_S *tmp_table = &(tables->modem);
  uint load_cnt = 0;
#ifdef SECURE_BOOT_ENABLE
  int secure_offset = BOOT_INFO_SIZE + VLR_INFO_SIZ;
#endif
  uint32_t normal_boot_offset = 0;

  for (; load_cnt < load_max; load_cnt++) {
    if (tmp_table->size != 0) {
#ifdef SECURE_BOOT_ENABLE
      MODEM_LOGD("[secure]%s: table cnt = %d is_secured = %d", __FUNCTION__,
                 load_cnt, tmp_table->is_secured);
      if (1 == tmp_table->is_secured) {
        normal_boot_offset = get_boot_offset(tmp_table, secure_offset);
          MODEM_LOGD("[secure]verify start");
          modem_verify_image(tmp_table->path_r, 0, tmp_table->size);
          MODEM_LOGD("[secure]verify done.");

        MODEM_LOGD("[secure]secure_offset = %d", secure_offset);
        load_sipc_image(tmp_table->path_r, secure_offset + normal_boot_offset,
                        tmp_table->path_w, 0, tmp_table->size);
      } else if (tmp_table->is_cmdline) {
        load_cp_cmdline(tmp_table->path_r,tmp_table->path_w);
      } else {
        normal_boot_offset = get_boot_offset(tmp_table, 0);
        load_sipc_image(tmp_table->path_r, normal_boot_offset,
                        tmp_table->path_w, 0, tmp_table->size);
      }
#else
      if (tmp_table->is_cmdline) {
        load_cp_cmdline(tmp_table->path_r,tmp_table->path_w);
      } else {
        normal_boot_offset = get_boot_offset(tmp_table, 0);
        load_sipc_image(tmp_table->path_r, normal_boot_offset, tmp_table->path_w,
            0, tmp_table->size);
      }
#endif
    } else {
      MODEM_LOGD("%s: image[%d].size=0\n", __FUNCTION__, load_cnt);
    }

    tmp_table++;
  }
  return 0;
}

int load_sipc_modem_img(int modem, int need_load_pmcp) {
  char path_read[MAX_PATH_LEN] = {0};
  char path_write[MAX_PATH_LEN] = {0};
  char cproc_prop[PROPERTY_VALUE_MAX] = {0};
  char prop_nvp[PROPERTY_VALUE_MAX] = {0};
  char nvp_name[PROPERTY_VALUE_MAX] = {0};

  char alive_info[20] = {0};
  int i, ret;
  bool cppmic_is_ok = 0;

  LOAD_TABLE_S *table_ptr = &(load_value.load_table);

  /* lock wake */
  system("echo load_modem_img >/sys/power/wake_lock");
  g_b_wake_locking = true;

  memset(&load_value, 0, sizeof(load_value));

  /* get read path */
  property_get(PERSIST_MODEM_PATH, path_read, "not_find");
  MODEM_LOGD("%s: path_read is %s\n", __FUNCTION__, path_read);

  /* get nvp prop */
  snprintf(nvp_name, sizeof(nvp_name), PERSIST_MODEM_PROP, g_modem);
  property_get(nvp_name, prop_nvp, "not_find");
  MODEM_LOGD("%s: nvp_name is %s, prop_nvp is %s\n", __FUNCTION__, nvp_name,
             prop_nvp);

  /*get write path */
  snprintf(cproc_prop, sizeof(cproc_prop), PROC_DEV_PROP, g_modem);
  property_get(cproc_prop, path_write, "not_find");
  MODEM_LOGD("%s: cproc_prop is %s, path_write is %s\n", __FUNCTION__,
             cproc_prop, path_write);

  /* init cmd line info */
  strncpy(table_ptr->cmdline.path_r, "/proc/cmdline", MAX_PATH_LEN);
  table_ptr->cmdline.size = CPCMDLINE_SIZE;
  table_ptr->cmdline.is_cmdline = 1;

  /* init pm cp info */
  table_ptr->pmcp.size = 0;
  if (access("/proc/pmic", F_OK) == 0) {
    cppmic_is_ok = 1;
    strncpy(table_ptr->pmcp.path_w, "/proc/pmic/pm_sys", MAX_PATH_LEN);
    mstrncat2(table_ptr->pmcp.path_r, path_read, "pm_sys");
    MODEM_LOGD("%s: pmcp path_read is %s\n", __FUNCTION__, path_read);

    /* size 0 means no load */
    table_ptr->pmcp.size = need_load_pmcp ? PMCP_SIZE : 0;
    strncpy(load_value.pmcp_start, "/proc/pmic/start", MAX_PATH_LEN);
    strncpy(load_value.pmcp_stop, "/proc/pmic/stop", MAX_PATH_LEN);
  }

  /* init cp info */
  switch (modem) {
    case TD_MODEM:
    case W_MODEM: {
      table_ptr->modem.size = MODEM_SIZE;
      table_ptr->dsp.size = TGDSP_SIZE;
      mstrncat2(table_ptr->dsp.path_w, path_write, DSP_BANK);
      mstrncat3(table_ptr->dsp.path_r, path_read, prop_nvp, DSP_BANK);
    } break;

    case LTE_MODEM: {
      table_ptr->modem.size = MODEM_SIZE;
      table_ptr->deltanv.size = DELTANV_SIZE;
      table_ptr->gdsp.size = TGDSP_SIZE;
      table_ptr->ldsp.size = LDSP_SIZE;
      table_ptr->warm.size = WARM_SIZE;
      mstrncat2(table_ptr->gdsp.path_w, path_write, TGDSP_BANK);
      mstrncat2(table_ptr->ldsp.path_w, path_write, LDSP_BANK);
      mstrncat2(table_ptr->warm.path_w, path_write, WARM_BANK);
      mstrncat3(table_ptr->gdsp.path_r, path_read, prop_nvp, TGDSP_BANK);
      if (access(table_ptr->gdsp.path_r, F_OK))
        mstrncat3(table_ptr->gdsp.path_r, path_read, prop_nvp, GDSP_BANK);
      mstrncat3(table_ptr->ldsp.path_r, path_read, prop_nvp, LDSP_BANK);
      mstrncat3(table_ptr->warm.path_r, path_read, prop_nvp, WARM_BANK);
    } break;
  }

  mstrncat2(load_value.cp_start, path_write, MODEM_START);
  mstrncat2(load_value.cp_stop, path_write, MODEM_STOP);
  mstrncat2(table_ptr->modem.path_w, path_write, MODEM_BANK);
  mstrncat2(table_ptr->deltanv.path_w, path_write, DELTANV_BANK);
  mstrncat2(table_ptr->cmdline.path_w, path_write, CMDLINE_BANK);
  mstrncat3(table_ptr->modem.path_r, path_read, prop_nvp, MODEM_BANK);
  mstrncat3(table_ptr->deltanv.path_r, path_read, prop_nvp, DELTANV_BANK);

/* init secure boot info */
#ifdef SECURE_BOOT_ENABLE
  MODEM_LOGD("[secure]%s: modem type = 0x%x", __FUNCTION__, modem);
  table_ptr->modem.is_secured = 1;
  table_ptr->dsp.is_secured = 1;
  table_ptr->gdsp.is_secured = 1;
  table_ptr->ldsp.is_secured = 1;
  table_ptr->warm.is_secured = 1;
  table_ptr->pmcp.is_secured = 1;
  MODEM_LOGD("[secure]%s: is_shutdown = 0x%x", __FUNCTION__, need_load_pmcp);

  memset(s_modem_puk, 0, sizeof(s_modem_puk));
  ret = modem_parse_puk_cmdline(s_modem_puk);
  if (ret != 1) {
    MODEM_LOGD("[secure]%s: modem_parse_puk_cmdline failed!!!", __FUNCTION__);
    return -1;
  }
#endif

  MODEM_LOGD("cp_stop= %s", load_value.cp_stop);
  /* write 1 to stop cp */
  if (cppmic_is_ok && need_load_pmcp) {
    MODEM_LOGD("pmcp_stop= %s", load_value.pmcp_stop);
    write_proc_file(load_value.pmcp_stop, 0, "1");
  }
  write_proc_file(load_value.cp_stop, 0, "1");

  /* load nv image*/
  mstrncat3(load_value.nv1_read, path_read, prop_nvp, FIXNV1_BANK);
  mstrncat3(load_value.nv2_read, path_read, prop_nvp, FIXNV2_BANK);
  mstrncat2(load_value.nv_write, path_write, FIXNV_BANK);
  MODEM_LOGIF("loading FIXNV: path=%s, bak_path=%s, out=%s\n",
              load_value.nv1_read, load_value.nv2_read, load_value.nv_write);
  read_nv_partition(load_value.nv1_read, load_value.nv2_read,
                    load_value.nv_write);

  mstrncat3(load_value.nv1_read, path_read, prop_nvp, RUNNV1_BANK);
  mstrncat3(load_value.nv2_read, path_read, prop_nvp, RUNNV2_BANK);
  mstrncat2(load_value.nv_write, path_write, RUNNV_BANK);
  MODEM_LOGIF("loading RuntimeNV: path=%s, bak_path=%s, out=%s\n",
              load_value.nv1_read, load_value.nv2_read, load_value.nv_write);
  read_nv_partition(load_value.nv1_read, load_value.nv2_read,
                    load_value.nv_write);

  /* load other cp image */
  boot_from_table(table_ptr, sizeof(LOAD_TABLE_S) / sizeof(IMAGE_LOAD_S));

  /* write 1 to start cp */
  if (cppmic_is_ok && need_load_pmcp) {
    MODEM_LOGD("pmcp_start= %s", load_value.pmcp_start);
    write_proc_file(load_value.pmcp_start, 0, "1");
  }
  MODEM_LOGD("cp_start= %s", load_value.cp_start);
  write_proc_file(load_value.cp_start, 0, "1");

  /* unlock wake */
  system("echo load_modem_img >/sys/power/wake_unlock");
  g_b_wake_locking = false;

  return 0;
}

int modem_ctrl_parse_cmdline(char *cmdvalue) {
  int fd = 0, ret = 0;
  char cmdline[CPCMDLINE_SIZE] = {0};
  char *str = NULL;
  int val;

  if (cmdvalue == NULL) {
    MODEM_LOGD("cmd_value = NULL\n");
    return -1;
  }
  fd = open("/proc/cmdline", O_RDONLY);
  if (fd >= 0) {
    if ((ret = read(fd, cmdline, sizeof(cmdline) - 1)) > 0) {
      cmdline[ret] = '\0';
      MODEM_LOGD("modem_ctrl: cmdline %s\n", cmdline);
      str = strstr(cmdline, "modem=");
      if (str != NULL) {
        str += strlen("modem=");
        *(strchr(str, ' ')) = '\0';
      } else {
        MODEM_LOGE("cmdline 'modem=' is not exist\n");
        goto ERROR;
      }
      MODEM_LOGD("cmdline len = %d, str=%s\n", strlen(str), str);
      if (!strcmp(cmdvalue, str))
        val = 1;
      else
        val = 0;
      close(fd);
      return val;
    } else {
      MODEM_LOGE("cmdline modem=NULL");
      goto ERROR;
    }
  } else {
    MODEM_LOGE("/proc/cmdline open error:%s\n", strerror(errno));
    return 0;
  }
ERROR:
  MODEM_LOGD("modem_ctrl: exit parse!");
  close(fd);
  return 0;
}

int open_modem_dev(char *path) {
  int fd = -1;

retry:
  fd = open(path, O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    if (errno == EINTR || errno == EAGAIN)
      goto retry;
    else
      return -1;
  }
  return fd;
}

int wait_for_modem_alive(void) {
  char buf[256] = {0};
  int ret = 0, suc = 0;
  int count = 0;
  int fd = 0;
  char tty_dev[256] = {0};
  struct timeval timeout;
  fd_set rfds;
  char prop_value[PROPERTY_VALUE_MAX] = {0};
  char prop_name[PROPERTY_VALUE_MAX] = {0};
  int try_cnt = 60;
  int is_reset;

  property_get(MODEM_RESET_PROP, prop_value, "0");
  is_reset = atoi(prop_value);

  /* get stty dev prop */
  memset(prop_value, 0, sizeof(prop_value));
  snprintf(prop_name, sizeof(prop_name), TTY_DEV_PROP, g_modem);
  property_get(prop_name, prop_value, "not_find");
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, prop_name, prop_value);

  snprintf(tty_dev, sizeof(tty_dev), "%s0", prop_value);
  do {
    fd = open_modem_dev(tty_dev);
    if (fd > 0) {
      break;
    }
    MODEM_LOGD("%s: times:%d, failed to open tty dev:  %s, fd = %d",
               __FUNCTION__, try_cnt, tty_dev, fd);
    usleep(1000 * 1000);
    try_cnt--;
  } while ((fd <= 0) && try_cnt > 0);

  if (0 == try_cnt) {
      MODEM_LOGE("%s: wait alive open timeout,exit!", __FUNCTION__);
	  if (is_reset)
          system("echo \"c\" > /proc/sysrq-trigger");

    return suc;
  }

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  for (;;) {
    MODEM_LOGD("%s: wait for alive info from : %d", __FUNCTION__, fd);

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    do {
      ret = select(fd + 1, &rfds, NULL, NULL, &timeout);
    } while (ret == -1 && errno == EINTR);
    if (ret < 0) {
      MODEM_LOGE("%s: select error: %s", __FUNCTION__, strerror(errno));
      continue;
    } else if (ret == 0) {
      MODEM_LOGE("%s: wait alive select timeout,exit!", __FUNCTION__);
	  if (is_reset)
          system("echo \"c\" > /proc/sysrq-trigger");

      close(fd);
      return ret;
    } else {
      count = read(fd, buf, sizeof(buf));
      if (count <= 0) {
        MODEM_LOGE("%s: read %d return %d, error: %s", __FUNCTION__, fd, count,
                   strerror(errno));
        continue;
      }
      MODEM_LOGD("%s: read response %s from %d", __FUNCTION__, buf, fd);
      if (strstr(buf, "Alive")) {
        suc = 1;
        break;
      }
    }
  }

  close(fd);
  return suc;
}

static void modemd_read_empty_log(int fd) {
  char buf[2048] = {0};
  int ret = 0;
  int count = 0;
  struct timeval timeout;
  fd_set rfds;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  for (;;) {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    do {
      ret = select(fd + 1, &rfds, NULL, NULL, &timeout);
    } while (ret == -1 && errno == EINTR);
    if (ret < 0) {
      MODEM_LOGIF("select error: %s", strerror(errno));
      break;
    } else if (ret == 0) {
      MODEM_LOGD(" time out, read log over!");
      break;
    } else {
      MODEM_LOGIF("one time read log start");
      do {
        count = read(fd, buf, sizeof(buf));
        MODEM_LOGIF("read log count = %d", count);
      } while (count > 0);
      if (count < 0) break;
    }
  }
}

static void *prepare_reset_modem(void) {
  int w_cnt, log_fd, diag_fd;
  char prop_name[PROPERTY_VALUE_MAX] = {0};
  char diag_chan[PROPERTY_VALUE_MAX] = {0};
  char log_chan[PROPERTY_VALUE_MAX] = {0};
  char s_reset_cmd[2] = {0x7a, 0x0a};
  int count;
  char buf[2048];

  /* get diag dev prop */
  snprintf(prop_name, sizeof(prop_name), DIAG_DEV_PROP, g_modem);
  property_get(prop_name, diag_chan, "not_find");
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, prop_name, diag_chan);

  /* get diag dev prop */
  snprintf(prop_name, sizeof(prop_name), LOG_DEV_PROP, g_modem);
  property_get(prop_name, log_chan, "not_find");
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, prop_name, log_chan);

  log_fd = open(log_chan, O_RDWR | O_NONBLOCK);
  if (log_fd < 0) {
    log_fd = open(diag_chan, O_RDWR | O_NONBLOCK);
    MODEM_LOGD("%s: log chanel open failed, use diag chanel, %s\n",
               __FUNCTION__, strerror(errno));
  }

  if (log_fd >= 0) {
    modemd_read_empty_log(log_fd);
    MODEM_LOGD("%s: read log over %s!\n", __FUNCTION__, log_chan);
    close(log_fd);
  } else {
    MODEM_LOGE("%s: MODEM cannot open log chanel, %s\n", __FUNCTION__,
               strerror(errno));
  }

  /* than write 'z' to cp */
  diag_fd = open(diag_chan, O_RDWR | O_NONBLOCK);
  if (diag_fd < 0) {
    MODEM_LOGE("%s: MODEM cannot open %s, %s\n", __FUNCTION__, diag_chan,
               strerror(errno));
    return NULL;
  } else {
    /* read empty diag first */
    do
    {
      count = read(diag_fd, buf, sizeof(buf));
      //MODEM_LOGD("read diag count = %d", count);
    }while(count>0);

    MODEM_LOGD("%s: ready write diag cmd = %s!", __FUNCTION__, s_reset_cmd);
    w_cnt = write(diag_fd, s_reset_cmd, sizeof(s_reset_cmd));
    MODEM_LOGD("%s: MODEM write diag_chan:%d ,%s\n", __FUNCTION__, w_cnt,
               strerror(errno));
    close(diag_fd);
    return NULL;
  }
}

static void *modem_ctrl_listen_thread(void *param) {
  char buf[MAX_ASSERT_INFO_LEN] = {0};
  int cnt = 0, i;
  int modem = 0;

  MODEM_LOGD("enter %s ......\n", __FUNCTION__);

  if (0 == strcmp("t", g_modem)) {
    modem = TD_MODEM;
  } else if (0 == strcmp("w", g_modem)) {
    modem = W_MODEM;
  } else if (0 == strcmp("l", g_modem)) {
    modem = LTE_MODEM;
  } else {
    MODEM_LOGD("%s: modem type(%s) error!", __FUNCTION__, g_modem);
    return NULL;
  }

  if (modem_ctrl_parse_cmdline("shutdown")) {
    load_sipc_modem_img(modem, 1);
    if (!wait_for_modem_alive()) return NULL;
  }

  MODEM_LOGD("%s: send sem_first_boot\n", __FUNCTION__);
  sem_post(&sem_first_boot);

  while (client_modemd < 0) {
    if (++cnt < 100) {
      usleep(100 * 1000);
      MODEM_LOGD("%s: wait modemd connect\n", __FUNCTION__);
    } else {
      MODEM_LOGE("%s: modemd not start,exit!", __FUNCTION__);
      return NULL;
    }
  }

  MODEM_LOGD("%s: start listen modemd...\n", __FUNCTION__);
  while (1) {
    cnt = read(client_modemd, buf, sizeof(buf));
    if (cnt <= 0) {
      MODEM_LOGE("%s: read cnt errno:[%d] %s\n", __FUNCTION__, errno,
                 strerror(errno));
      break;
    }
    MODEM_LOGD("%s: read cnt= %d, str= %s", __FUNCTION__, cnt, buf);

    if (!strcmp(buf, MODEM_RESET)) {
      load_sipc_modem_img(modem, 0);
      if (!wait_for_modem_alive()) return NULL;

      cnt = write(client_modemd, MODEM_ALIVE, strlen(MODEM_ALIVE));
      if (cnt <= 0) {
        MODEM_LOGE("%s: write modem_alive errno:[%d] %s\n", __FUNCTION__, errno,
                   strerror(errno));
        return NULL;
      } else {
        MODEM_LOGD("%s: write to modemd len = %d,str=Modem Alive\n",
                   __FUNCTION__, cnt);
      }
    }
    if (!strcmp(buf, PREPARE_RESET)) prepare_reset_modem();
  }

  return NULL;
}

/* loop detect sipc modem state */
void *detect_sipc_modem(void *param) {
  char assert_dev[PROPERTY_VALUE_MAX] = {0};
  char watchdog_dev[PROPERTY_VALUE_MAX] = {0};
  char prop_name[PROPERTY_VALUE_MAX] = {0};
  int cnt, i, ret, watchdog_fd, max_fd, fd = -1;
  fd_set rfds;
  char buf[256];
  int numRead, numWrite;
  int is_assert = 0;
  pthread_t t1;

  MODEM_LOGD("Enter %s !", __FUNCTION__);
  sem_init(&sem_first_boot, 0, 0);

  /* get diag assert prop */
  snprintf(prop_name, sizeof(prop_name), ASSERT_DEV_PROP, g_modem);
  property_get(prop_name, assert_dev, "not_find");
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, prop_name, assert_dev);

  /* get watchdog dev */
  snprintf(prop_name, sizeof(prop_name), PROC_DEV_PROP, g_modem);
  property_get(prop_name, watchdog_dev, "not_find");
  strncat(watchdog_dev, WDOG_BANK, sizeof(watchdog_dev));
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, watchdog_dev, watchdog_dev);

  /* get diag assert prop */
  snprintf(prop_name, sizeof(prop_name), ASSERT_DEV_PROP, g_modem);
  property_get(prop_name, assert_dev, "not_find");
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, prop_name, assert_dev);

  /* get watchdog dev */
  snprintf(prop_name, sizeof(prop_name), PROC_DEV_PROP, g_modem);
  property_get(prop_name, watchdog_dev, "not_find");
  strncat(watchdog_dev, WDOG_BANK, sizeof(watchdog_dev));
  MODEM_LOGD("%s: %s = %s\n", __FUNCTION__, watchdog_dev, watchdog_dev);

  /*set up socket connection to modemd*/
  server_modem_ctrl =
      socket_local_server(SOCKET_NAME_MODEM_CTL, 0,
                          /*ANDROID_SOCKET_NAMESPACE_RESERVED,*/ SOCK_STREAM);
  if (server_modem_ctrl < 0) {
    MODEM_LOGE("%s: cannot create local socket server\n", __FUNCTION__);
    return NULL;
  }
  if (0 != pthread_create(&t1, NULL, modem_ctrl_listen_thread, g_modem)) {
    MODEM_LOGE(" %s: create error!\n", __FUNCTION__);
  }

  /*wait modemd connect*/
  MODEM_LOGD("%s: wait modemd accept...\n", __FUNCTION__);
  client_modemd = accept(server_modem_ctrl, NULL, NULL);
  if (client_modemd < 0) {
    MODEM_LOGE("Error on accept() errno:[%d] %s\n", errno, strerror(errno));
    sleep(1);
    return NULL;
  }
  MODEM_LOGD("%s: accept client_modemd =%d\n", __FUNCTION__, client_modemd);

  /*wait first time modem load OK*/
  MODEM_LOGD("%s: wait sem_first_boot!\n", __FUNCTION__);
  sem_wait(&sem_first_boot);

  /* open assert_dev and watchdog_dev */
  while ((assert_fd = open(assert_dev, O_RDWR)) < 0) {
    MODEM_LOGE("%s: open %s failed, error: %s", __FUNCTION__, assert_dev,
               strerror(errno));
    sleep(1);
  }
  MODEM_LOGD("%s: open assert dev: %s, fd = %d", __FUNCTION__, assert_dev,
             assert_fd);
  watchdog_fd = open(watchdog_dev, O_RDONLY);
  MODEM_LOGD("%s: open watchdog dev: %s, fd = %d", __FUNCTION__, watchdog_dev,
             watchdog_fd);
  if (watchdog_fd < 0) {
    /* if  watchdog_fd can't be open, we also continue */
    MODEM_LOGE("open %s failed, error: %s", watchdog_dev, strerror(errno));
  }

  /*inform modemd cp is running*/
  MODEM_LOGD("%s: write modem alive to modemd!\n", __FUNCTION__);

  cnt = write(client_modemd, MODEM_ALIVE, strlen(MODEM_ALIVE));
  while (cnt <= 0) {
    MODEM_LOGE("%s: write modem_alive errno:[%d] %s\n", __FUNCTION__, errno,
               strerror(errno));
    sleep(1);
  }
  MODEM_LOGD("%s: write to modemd len = %d\n", __FUNCTION__, cnt);
  max_fd = watchdog_fd > assert_fd ? watchdog_fd : assert_fd;

  FD_ZERO(&rfds);
  FD_SET(assert_fd, &rfds);
  if (watchdog_fd > 0) {
    FD_SET(watchdog_fd, &rfds);
  }

  /*listen assert and WDG event*/
  for (;;) {
    MODEM_LOGD("%s: wait for modem assert/hangup event ...", __FUNCTION__);
    do {
      ret = select(max_fd + 1, &rfds, NULL, NULL, 0);
    } while (ret == -1 && errno == EINTR);
    if (ret > 0) {
      if (FD_ISSET(assert_fd, &rfds)) {
        fd = assert_fd;
      } else if (FD_ISSET(watchdog_fd, &rfds)) {
        fd = watchdog_fd;
      } else {
        MODEM_LOGE("%s: none of assert and watchdog fd is readalbe",
                   __FUNCTION__);
        sleep(1);
        continue;
      }

      system("echo load_modem_img >/sys/power/wake_lock");
      g_b_wake_locking = true;
      memset(buf, 0, sizeof(buf));
      MODEM_LOGD("%s: enter read ...", __FUNCTION__);
      numRead = read(fd, buf, sizeof(buf));
      if (numRead <= 0) {
        MODEM_LOGE("%s: read %d return %d, errno = %s", __FUNCTION__, fd,
                   numRead, strerror(errno));
        sleep(1);
        continue;
      }

      MODEM_LOGD("%s: buf=%s", __FUNCTION__, buf);

      /*if cp or dsp hung, if mode reset open, will reboot system */
      if (strstr(buf, "HUNG")) {
		  char prop[PROPERTY_VALUE_MAX];
		  int is_reset;

		  memset(prop, 0, sizeof(prop));
		  property_get(MODEM_RESET_PROP, prop, "0");
		  is_reset = atoi(prop);
		  if (is_reset)
			  system("echo \"c\" > /proc/sysrq-trigger");
      }

      if ((numWrite = write(client_modemd, buf, numRead)) <= 0)
        MODEM_LOGE("%s: write %d return %d, errno = %s", __FUNCTION__, fd,
                   numWrite, strerror(errno));
      else
        MODEM_LOGD("%s: write to modemd len = %d\n", __FUNCTION__, numWrite);

      system("echo load_modem_img >/sys/power/wake_unlock");
      g_b_wake_locking = false;
    }
  }
  return NULL;
}
