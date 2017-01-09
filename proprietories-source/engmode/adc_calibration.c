#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "engopt.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"
#include "eng_diag.h"
#include "calibration.h"

#ifdef CONFIG_NAND
#include <sys/ioctl.h>
#include <ubi-user.h>
#endif

#define CONFIG_AP_ADC_CALIBRATION
#define ADC_CALIBR_FGU_ON
#ifdef CONFIG_AP_ADC_CALIBRATION

#define PRECISION_1MV (1 << 24)
#define PRECISION_10MV (0)
#define MAX_VOLTAGE (0xFFFFFF)

#define ADC_CAL_TYPE_FILE "/sys/class/power_supply/sprdfgu/fgu_cal_from_type"

static int vbus_charger_disconnect = 0;
static int get_other_ch_adc_value(int channel, int scale);

int disconnect_vbus_charger(void) {
  int fd;
  int ret = -1;

  if (vbus_charger_disconnect == 0) {
    fd = open(CHARGER_STOP_PATH, O_WRONLY);
    if (fd >= 0) {
      ret = write(fd, "1", 2);
      if (ret < 0) {
        ENG_LOG("%s write %s failed! \n", __func__, CHARGER_STOP_PATH);
        close(fd);
        return 0;
      }
      close(fd);
      sleep(1);
      vbus_charger_disconnect = 1;
    } else {
      ENG_LOG("%s open %s failed! \n", __func__, CHARGER_STOP_PATH);
      return 0;
    }
  }
  return 1;
}
int connect_vbus_charger(void) {
  int fd;
  int ret = -1;

  if (vbus_charger_disconnect == 1) {
    fd = open(CHARGER_STOP_PATH, O_WRONLY);
    if (fd >= 0) {
      ret = write(fd, "0", 2);
      if (ret < 0) {
        ENG_LOG("%s write %s failed! \n", __func__, CHARGER_STOP_PATH);
        close(fd);
        return 0;
      }
      close(fd);
      sleep(1);
      vbus_charger_disconnect = 0;
    } else {
      ENG_LOG("%s open %s failed! \n", __func__, CHARGER_STOP_PATH);
      return 0;
    }
  }
  return 1;
}

void initialize_ctrl_file(void) {
  CALI_INFO_DATA_T cali_info;
  int ret;
  unsigned int adc_magic = ADC_MAGIC;

  int fd = open(CALI_CTRL_FILE_PATH, O_RDWR);

  if (fd < 0) {
    ENG_LOG("%s open %s failed\n", __func__, CALI_CTRL_FILE_PATH);
    return;
  }
  sync();
  lseek(fd, ADC_DATA_START + sizeof(unsigned int), SEEK_SET);

  ret = read(fd, &cali_info, sizeof(cali_info));
  if (ret < 0) {
    ENG_LOG(" %s read failed...\n", __func__);
    close(fd);
    return;
  }
  if (cali_info.magic != CALI_MAGIC) {
    memset(&cali_info, 0xff, sizeof(cali_info));
    cali_info.adc_para.reserved[7] = 0;
    cali_info.magic = CALI_MAGIC;
  }

#ifdef ADC_CALIBR_FGU_ON
  cali_info.adc_para.reserved[6] = 2;
#else
  cali_info.adc_para.reserved[6] = 0;
#endif

  lseek(fd, ADC_DATA_START, SEEK_SET);

#ifdef CONFIG_NAND
  __s64 up_sz = sizeof(cali_info) + sizeof(unsigned int);
  ioctl(fd, UBI_IOCVOLUP, &up_sz);
#endif

  write(fd, &adc_magic, sizeof(unsigned int));
  write(fd, &cali_info, sizeof(cali_info));
  close(fd);
}

void disable_calibration(void) {
  CALI_INFO_DATA_T cali_info;
  int ret = 0;

  int fd = open(CALI_CTRL_FILE_PATH, O_RDWR);

  if (fd < 0) {
    ENG_LOG("%s open %s failed\n", __func__, CALI_CTRL_FILE_PATH);
    return;
  }
  lseek(fd, ADC_DATA_START + sizeof(unsigned int), SEEK_SET);

  ret = read(fd, &cali_info, sizeof(cali_info));
  if (ret <= 0) {
    ENG_LOG(" %s read failed...\n", __func__);
    close(fd);
    return;
  }

  cali_info.magic = CALI_MAGIC;
  cali_info.cali_flag = CALI_COMP;

  lseek(fd, ADC_DATA_START + sizeof(unsigned int), SEEK_SET);

#ifdef CONFIG_NAND
  __s64 up_sz = sizeof(cali_info);
  ioctl(fd, UBI_IOCVOLUP, &up_sz);
#endif

  write(fd, &cali_info, sizeof(cali_info));
  close(fd);
}

void enable_calibration(void) {
  CALI_INFO_DATA_T cali_info;

  int fd = open(CALI_CTRL_FILE_PATH, O_RDWR);
  int ret = 0;

  if (fd < 0) {
    ENG_LOG("%s open %s failed\n", __func__, CALI_CTRL_FILE_PATH);
    return;
  }
  lseek(fd, ADC_DATA_START + sizeof(unsigned int), SEEK_SET);

  ret = read(fd, &cali_info, sizeof(cali_info));
  if (ret <= 0) {
    ENG_LOG(" %s read failed...\n", __func__);
    close(fd);
    return;
  }
  cali_info.magic = 0xFFFFFFFF;
  cali_info.cali_flag = 0xFFFFFFFF;

  lseek(fd, ADC_DATA_START + sizeof(unsigned int), SEEK_SET);

#ifdef CONFIG_NAND
  __s64 up_sz = sizeof(cali_info);
  ioctl(fd, UBI_IOCVOLUP, &up_sz);
#endif

  write(fd, &cali_info, sizeof(cali_info));
  close(fd);
}

void adc_get_result(char *chan) {
  int ret = 0;
  int fd = open(ADC_CHAN_FILE_PATH, O_RDWR);
  if (fd < 0) {
    ENG_LOG("%s open %s failed\n", __func__, ADC_CHAN_FILE_PATH);
    return;
  }
  write(fd, chan, strlen(chan));
  lseek(fd, 0, SEEK_SET);
  memset(chan, 0, 8);
  ret = read(fd, chan, 8);
  if (ret <= 0) {
    ENG_LOG(" %s read failed...\n", __func__);
    close(fd);
    return;
  }
  close(fd);
}
static int AccessADCDataFile(unsigned char flag, char *lpBuff, int size) {
  int fd = -1;
  int ret = 0;
  CALI_INFO_DATA_T cali_info;

  fd = open(BATTER_CALI_CONFIG_FILE, O_RDWR);

  if (flag == 1) {
    if (fd < 0) {
      ENG_LOG("%s open %s failed\n", __func__, CALI_CTRL_FILE_PATH);
      return 0;
    }
    lseek(fd, ADC_DATA_START + sizeof(unsigned int),
          SEEK_SET);  // skip SP09_PHASE_CHECK_T and adc magic flag(ADC_MAGIC),
                      // read or write adc data
    ret = read(fd, &cali_info, sizeof(cali_info));

    if (size < sizeof(cali_info.adc_para))
      memcpy(&cali_info.adc_para, lpBuff, size);
    else
      memcpy(&cali_info.adc_para, lpBuff, sizeof(cali_info.adc_para));
    lseek(fd, ADC_DATA_START + sizeof(unsigned int),
          SEEK_SET);  // come back adc data header
    cali_info.magic = CALI_MAGIC;

#ifdef CONFIG_NAND
    __s64 up_sz = sizeof(cali_info);
    ioctl(fd, UBI_IOCVOLUP, &up_sz);
#endif

    ret = write(fd, &cali_info, sizeof(cali_info));
    fsync(fd);
    sleep(1);
  } else {
    if (fd < 0) {
      ENG_LOG("%s open %s failed\n", __func__, CALI_CTRL_FILE_PATH);
      return 0;
    }
    lseek(fd, ADC_DATA_START + sizeof(unsigned int), SEEK_SET);
    ret = read(fd, &cali_info, sizeof(cali_info));

    if (size < sizeof(cali_info.adc_para))
      memcpy(lpBuff, &cali_info.adc_para, size);
    else
      memcpy(lpBuff, &cali_info.adc_para, sizeof(cali_info.adc_para));
  }
  close(fd);
  sync();

  return ret;
}
static int get_battery_voltage(void) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  int value = 0;

  fd = open(BATTERY_VOL_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) value = atoi(buffer);
    close(fd);
  }
  return value;
}
static int get_battery_adc_value(void) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;
  int value = 0;

  fd = open(BATTERY_ADC_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) value = strtol(buffer, &endptr, 0);
    close(fd);
  }
  return value;
}
static int get_fgu_current_adc(int *value) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;

  fd = open(FGU_CURRENT_ADC_FILE_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) *value = strtol(buffer, &endptr, 0);
    close(fd);
    ENG_LOG("%s %s value = %d\n", __func__, FGU_VOL_ADC_FILE_PATH, *value);
  } else {
    ENG_LOG("%s open %s failed\n", __func__, FGU_CURRENT_ADC_FILE_PATH);
  }
  return read_len;
}
static int get_fgu_vol_adc(int *value) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;

  fd = open(FGU_VOL_ADC_FILE_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) *value = strtol(buffer, &endptr, 0);
    close(fd);
    ENG_LOG("%s %s value = %d, read_len = %d \n", __func__,
            FGU_VOL_ADC_FILE_PATH, *value, read_len);
  } else {
    ENG_LOG("%s open %s failed\n", __func__, FGU_VOL_ADC_FILE_PATH);
  }
  return read_len;
}

static void ap_get_fgu_current_adc(MSG_AP_ADC_CNF *pMsgADC) {
  int current_adc = 0;
  int read_len = 0;

  read_len = get_fgu_current_adc(&current_adc);
  if (read_len > 0) {
    pMsgADC->ap_adc_req.parameters[0] = current_adc;
    pMsgADC->diag_ap_cnf.status = 0;
  } else {
    pMsgADC->diag_ap_cnf.status = 1;
  }
}

static void ap_get_fgu_vol_adc(MSG_AP_ADC_CNF *pMsgADC) {
  int vol_adc = 0;
  int read_len = 0;
  read_len = get_fgu_vol_adc(&vol_adc);

  if (read_len > 0) {
    pMsgADC->ap_adc_req.parameters[0] = vol_adc;
    pMsgADC->diag_ap_cnf.status = 0;
  } else {
    pMsgADC->diag_ap_cnf.status = 1;
  }
}

static int get_fgu_current_real(unsigned int *value) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;

  fd = open(FGU_CURRENT_FILE_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) *value = strtol(buffer, &endptr, 0);
    close(fd);
    ENG_LOG("%s %s value = %d\n", __func__, FGU_CURRENT_FILE_PATH, *value);
  } else {
    ENG_LOG("%s open %s failed\n", __func__, FGU_CURRENT_FILE_PATH);
  }
  return read_len;
}

static int get_fgu_vol_real(int *value) {
  int fd = -1;
  int read_len = 0;
  char buffer[16] = {0};
  char *endptr;

  fd = open(FGU_VOL_FILE_PATH, O_RDONLY);

  if (fd >= 0) {
    read_len = read(fd, buffer, sizeof(buffer));
    if (read_len > 0) *value = strtol(buffer, &endptr, 0);
    close(fd);
    ENG_LOG("%s %s value = %d, read_len = %d \n", __func__, FGU_VOL_FILE_PATH,
            *value, read_len);
  } else {
    ENG_LOG("%s open %s failed\n", __func__, FGU_VOL_FILE_PATH);
  }
  return read_len;
}

static void ap_get_fgu_current_real(MSG_AP_ADC_CNF *pMsgADC) {
  unsigned int real_current = 0;
  int read_len = 0;

  read_len = get_fgu_current_real(&real_current);
  if (read_len > 0) {
    pMsgADC->ap_adc_req.parameters[0] = real_current;
    pMsgADC->diag_ap_cnf.status = 0;
  } else {
    pMsgADC->diag_ap_cnf.status = 1;
  }
}

static void ap_get_fgu_vol_real(MSG_AP_ADC_CNF *pMsgADC) {
  unsigned int real_vol = 0;
  int read_len = 0;
  read_len = get_fgu_vol_real(&real_vol);

  if (read_len > 0) {
    pMsgADC->ap_adc_req.parameters[0] = real_vol;
    pMsgADC->diag_ap_cnf.status = 0;
  } else {
    pMsgADC->diag_ap_cnf.status = 1;
  }
}

/*copy from packet.c and modify*/
static int untranslate_packet_header(char *dest, char *src, int size,
                                     int unpackSize) {
  int i;
  int translated_size = 0;
  int status = 0;
  int flag = 0;
  for (i = 0; i < size; i++) {
    switch (status) {
      case 0:
        if (src[i] == 0x7e) status = 1;
        break;
      case 1:
        if (src[i] != 0x7e) {
          status = 2;
          dest[translated_size++] = src[i];
        }
        break;
      case 2:
        if (src[i] == 0x7E) {
          // unsigned short crc;
          // crc = crc_16_l_calc((char const *)dest,translated_size-2);
          return translated_size;
        } else {
          if ((dest[translated_size - 1] == 0x7D) && (!flag)) {
            flag = 1;
            if (src[i] == 0x5E)
              dest[translated_size - 1] = 0x7E;
            else if (src[i] == 0x5D)
              dest[translated_size - 1] = 0x7D;
          } else {
            flag = 0;
            dest[translated_size++] = src[i];
          }

          if (translated_size >= unpackSize + 1 && unpackSize != -1)
            return translated_size;
        }
        break;
    }
  }

  return translated_size;
}

int translate_packet(char *dest, char *src, int size) {
  int i;
  int translated_size = 0;

  dest[translated_size++] = 0x7E;

  for (i = 0; i < size; i++) {
    if (src[i] == 0x7E) {
      dest[translated_size++] = 0x7D;
      dest[translated_size++] = 0x5E;
    } else if (src[i] == 0x7D) {
      dest[translated_size++] = 0x7D;
      dest[translated_size++] = 0x5D;
    } else
      dest[translated_size++] = src[i];
  }
  dest[translated_size++] = 0x7E;
  return translated_size;
}

static int is_adc_calibration(char *dest, int destSize, char *src,
                              int srcSize) {
  int translated_size = 0;

  memset(dest, 0, destSize);
  if (srcSize > destSize)
    memcpy(dest, (src + 1), destSize);
  else
    memcpy(dest, (src + 1), (srcSize - 1));
  MSG_HEAD_T *lpHeader = (MSG_HEAD_T *)dest;
  if (DIAG_CMD_APCALI == lpHeader->type) {
    TOOLS_DIAG_AP_CMD_T *lpAPCmd = (TOOLS_DIAG_AP_CMD_T *)(lpHeader + 1);
    switch (lpAPCmd->cmd) {
      case DIAG_AP_CMD_ADC: {
        TOOLS_AP_ADC_REQ_T *lpAPADCReq = (TOOLS_AP_ADC_REQ_T *)(lpAPCmd + 1);
        if (lpAPADCReq->operate == 0)
          return AP_ADC_CALIB;
        else if (lpAPADCReq->operate == 1)
          return AP_ADC_LOAD;
        else if (lpAPADCReq->operate == 2)
          return AP_ADC_SAVE;
        else if (lpAPADCReq->operate == 3)
          return AP_GET_FGU_VOL_ADC;
        else if (lpAPADCReq->operate == 4)
          return AP_GET_FGU_CURRENT_ADC;
        else if (lpAPADCReq->operate == 5)
          return AP_GET_FGU_TYPE;
        else if (lpAPADCReq->operate == 6)
          return AP_GET_FGU_VOL_REAL;
        else if (lpAPADCReq->operate == 7)
          return AP_GET_FGU_CURRENT_REAL;
        else
          return 0;
      } break;
      case DIAG_AP_CMD_LOOP:
        return AP_DIAG_LOOP;
        break;

      default:
        break;
    }
  } else if (DIAG_CMD_GETVOLTAGE == lpHeader->type) {
    return AP_GET_VOLT;
  }
  return 0;
}

static int ap_adc_calibration(MSG_AP_ADC_CNF *pMsgADC) {
  int channel = pMsgADC->ap_adc_req.parameters[0] - 1;
  int adc_value = 0;
  int adc_result = 0;
  int i = 0;

  pMsgADC->diag_ap_cnf.status = 0;
  adc_result = get_other_ch_adc_value(channel, 0);  // small scale
  if (adc_result >= 0) {
    pMsgADC->ap_adc_req.parameters[0] = (unsigned short)(adc_result & 0xFFFF);
    if (channel != 5) {
      adc_result = get_other_ch_adc_value(channel, 1);  // large scale
      if (adc_result >= 0)
        pMsgADC->ap_adc_req.parameters[1] =
            (unsigned short)(adc_result & 0xFFFF);
      else
        pMsgADC->diag_ap_cnf.status = 1;
    }
  } else {
    pMsgADC->diag_ap_cnf.status = 1;
  }

  return adc_result;
}

static int ap_adc_save(TOOLS_AP_ADC_REQ_T *pADCReq, MSG_AP_ADC_CNF *pMsgADC) {
  AP_ADC_T adcValue;
  int ret = 0;

  memset(&adcValue, 0, sizeof(adcValue));
  ret = AccessADCDataFile(1, (char *)pADCReq->parameters,
                          sizeof(pADCReq->parameters));
  if (ret > 0)
    pMsgADC->diag_ap_cnf.status = 0;
  else
    pMsgADC->diag_ap_cnf.status = 1;

  return ret;
}
static int ap_adc_load(MSG_AP_ADC_CNF *pMsgADC) {
  int ret = AccessADCDataFile(0, (char *)pMsgADC->ap_adc_req.parameters,
                              sizeof(pMsgADC->ap_adc_req.parameters));
  if (ret > 0)
    pMsgADC->diag_ap_cnf.status = 0;
  else
    pMsgADC->diag_ap_cnf.status = 1;

  return ret;
}
static int ap_get_voltage(MSG_AP_ADC_CNF *pMsgADC) {
  int voltage = 0;
  int *para = NULL;
  int i = 0;
  MSG_HEAD_T *Msg = (MSG_HEAD_T *)pMsgADC;

  for (; i < 16; i++) voltage += get_battery_voltage();
  voltage >>= 4;

  para = (int *)(Msg + 1);
  //        *para = (voltage/10);
  if (voltage > MAX_VOLTAGE) {
    *para = (PRECISION_10MV | ((voltage / 10) & MAX_VOLTAGE));
  } else {
    *para = (PRECISION_1MV | (voltage & MAX_VOLTAGE));
  }
  pMsgADC->msg_head.len = 12;

  return voltage;
}

static void ap_get_fgu_type(MSG_AP_ADC_CNF *pMsgADC) {
  int fd = -1;
  int r_cnt = 0;
  char fgu_type[2] = {0};
  MSG_HEAD_T *Msg = (MSG_HEAD_T *)pMsgADC;

  fd = open(ADC_CAL_TYPE_FILE, O_RDONLY);
  if (fd >= 0) {
    r_cnt = read(fd, fgu_type, 2);
    if (r_cnt > 0) {
      ENG_LOG("%s: fgu_type:%s\n", __FUNCTION__, fgu_type);
      pMsgADC->ap_adc_req.parameters[0] = atoi(fgu_type);
      pMsgADC->diag_ap_cnf.status = 0;
    } else {
      ENG_LOG("%s: read fgu cali file failed,read:%d\n", __FUNCTION__, r_cnt);
      pMsgADC->diag_ap_cnf.status = 1;
    }

    close(fd);
  } else {
    ENG_LOG("%s: open fgu cali file failed, err: %s\n", __FUNCTION__,
            strerror(errno));
    pMsgADC->diag_ap_cnf.status = 1;
  }
}

int ap_adc_process(int adc_cmd, char *src, int size, MSG_AP_ADC_CNF *pMsgADC) {
  MSG_HEAD_T *lpHeader = (MSG_HEAD_T *)src;
  TOOLS_DIAG_AP_CMD_T *lpAPCmd = (TOOLS_DIAG_AP_CMD_T *)(lpHeader + 1);
  TOOLS_AP_ADC_REQ_T *lpApADCReq = (TOOLS_AP_ADC_REQ_T *)(lpAPCmd + 1);
  memcpy(&(pMsgADC->msg_head), lpHeader, sizeof(MSG_HEAD_T));
  pMsgADC->msg_head.len = sizeof(TOOLS_DIAG_AP_CNF_T) +
                          sizeof(TOOLS_AP_ADC_REQ_T) + sizeof(MSG_HEAD_T);
  pMsgADC->diag_ap_cnf.length = sizeof(TOOLS_AP_ADC_REQ_T);
  memcpy(&(pMsgADC->ap_adc_req), lpApADCReq, sizeof(TOOLS_AP_ADC_REQ_T));

  switch (adc_cmd) {
    case AP_ADC_CALIB:
      ap_adc_calibration(pMsgADC);
      break;
    case AP_ADC_LOAD:
      ap_adc_load(pMsgADC);
      break;
    case AP_ADC_SAVE:
      ap_adc_save(lpApADCReq, pMsgADC);
      break;
    case AP_GET_VOLT:
      ap_get_voltage(pMsgADC);
      break;
    case AP_GET_FGU_VOL_ADC:
      ap_get_fgu_vol_adc(pMsgADC);
      break;
    case AP_GET_FGU_CURRENT_ADC:
      ap_get_fgu_current_adc(pMsgADC);
      break;
    case AP_GET_FGU_TYPE:
      ap_get_fgu_type(pMsgADC);
      break;
    case AP_GET_FGU_VOL_REAL:
      ap_get_fgu_vol_real(pMsgADC);
      break;
    case AP_GET_FGU_CURRENT_REAL:
      ap_get_fgu_current_real(pMsgADC);
    default:
      return 0;
  }

  return 1;
}
#endif
int write_adc_calibration_data(char *data, int size) {
  int ret = 0;
#ifdef CONFIG_AP_ADC_CALIBRATION
  ret = AccessADCDataFile(1, data, size);
#endif
  return ret;
}
int read_adc_calibration_data(char *buffer, int size) {
#ifdef CONFIG_AP_ADC_CALIBRATION
  int ret;
  if (size > 48) size = 48;
  ret = AccessADCDataFile(0, buffer, size);
  if (ret > 0) return size;
#endif
  return 0;
}
int eng_battery_calibration(char *data, int count, char *out_msg, int out_len) {
#ifdef CONFIG_AP_ADC_CALIBRATION
  int adc_cmd = 0;
  int ret = 0;
  MSG_AP_ADC_CNF adcMsg;

  if (count > 0) {
    adc_cmd = is_adc_calibration(out_msg, out_len, data, count);

    if (adc_cmd != 0) {
      disconnect_vbus_charger();
      switch (adc_cmd) {
        case AP_DIAG_LOOP:
          if (out_len > count) {
            ret = count;
          } else {
            ret = out_len;
          }
          memcpy(out_msg, data, out_len);
          break;
        default:
          memset(&adcMsg, 0, sizeof(adcMsg));
          ap_adc_process(adc_cmd, out_msg, count, &adcMsg);
          ret = translate_packet(out_msg, (char *)&adcMsg, adcMsg.msg_head.len);
          break;
      }
    }
  }
#endif
  return ret;
}

static int get_other_ch_adc_value(int channel, int scale) {
  int adc_value = 0;
  int adc_result = 0;
  int i = 0, len = 0, fd = -1;
  char data_buf[16] = {0};
  char ch[8] = {0};
  char path[128] = {0};
  char buf[128] = {0};
  char *start_ptr;
  char *end_ptr;
  int adc_ver_flag = 0;

  if(0 == access(ADC_CHANNEL_PATH, F_OK)) {
    sprintf(path, "%s", ADC_CHANNEL_PATH);
  } else {
    sprintf(path, "%s", ADC_CHANNEL_PATH_NEW);
  }

  fd = open(path, O_WRONLY);
  if (fd < 0) {
    ENG_LOG("%s: open %s failed, err: %s\n", __func__, path,
            strerror(errno));
    return -1;
  }

  sprintf(ch, "%d", channel);
  len = write(fd, ch, strlen(ch));
  if (len <= 0) {
    ENG_LOG("%s: write %s failed, err: %s\n", __func__, path,
            strerror(errno));
    close(fd);
    return -1;
  }
  close(fd);

  if(0 == access(ADC_SCALE_PATH, F_OK)) {
    sprintf(path, "%s", ADC_SCALE_PATH);
  } else {
    sprintf(path, "%s", ADC_SCALE_PATH_NEW);
  }

  fd = open(path, O_WRONLY);
  if (fd < 0) {
    ENG_LOG("%s: open %s failed, err: %s\n", __func__, path,
            strerror(errno));
    return -1;
  }

  sprintf(ch, "%d", scale);
  len = write(fd, ch, strlen(ch));
  if (len <= 0) {
    ENG_LOG("%s: write %s failed, err: %s\n", __func__, path,
            strerror(errno));
    close(fd);
    return -1;
  }
  close(fd);

  if(0 == access(ADC_DATA_RAW_PATH, F_OK)) {
    sprintf(path, "%s", ADC_DATA_RAW_PATH);
    adc_ver_flag = 0;
  } else {
    sprintf(path, "%s", ADC_DATA_RAW_PATH_NEW);
    adc_ver_flag = 1;
  }

  for (i = 0; i < 16; i++) {
    fd = open(path, O_RDONLY);
    if (fd < 0) {
      ENG_LOG("%s: open %s failed, err: %s\n", __func__, path,
              strerror(errno));
      return -1;
    }

    if(adc_ver_flag) {
      len = read(fd, buf, sizeof(buf));
      start_ptr = strstr(buf, "adc_data = ");
      if(NULL == start_ptr) {
        ENG_LOG("%s: strstr() [adc_data = ], not found\n", __func__);
        close(fd);
        return -1;
      }
      end_ptr = strstr(start_ptr, ",");
      if(NULL == end_ptr) {
          ENG_LOG("%s: strstr() [,], not found\n", __func__);
          close(fd);
          return -1;
      }
      memcpy(data_buf, start_ptr + strlen("adc_data = "), end_ptr - start_ptr - strlen("adc_data = "));
    } else {
      len = read(fd, data_buf, sizeof(data_buf));
    }
    if (len < 0) {
      ENG_LOG("%s: read %s failed, err: %s\n", __func__, path,
              strerror(errno));
      close(fd);
      return -1;
    }

    adc_value = atoi(data_buf);
    adc_result += adc_value;
    close(fd);
  }

  adc_result >>= 4;
  return adc_result;
}
