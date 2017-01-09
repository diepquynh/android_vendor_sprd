/**
 * packet.h ---
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#undef LOG_TAG
#ifdef GNSS_DOWNLOAD
#define LOG_TAG "GNSS_DOWNLOAD"
#else
#define LOG_TAG "DOWNLOAD"
#endif

#define DOWNLOAD_DEBUG
#ifdef DOWNLOAD_DEBUG
#define DOWNLOAD_LOGD ALOGD
#define DOWNLOAD_LOGE ALOGE
#else
#define DOWNLOAD_LOGD(x...)  // printf
#define DOWNLOAD_LOGE(x...)  // printf
#endif

#define cpu2be16(wValue) (((wValue & 0xFF) << 8) | ((wValue >> 8) & 0xFF))
#define cpu2be32(dwValue)                                       \
  (((dwValue & 0xFF) << 24) | (((dwValue >> 8) & 0xFF) << 16) | \
  (((dwValue >> 16) & 0xFF) << 8) | (dwValue >> 24))

extern int send_connect_message(int fd, int flag);
extern int send_start_message(int fd, int size, unsigned long addr, int flag);
extern int send_end_message(int fd, int flag);
extern int send_data_message(int fd, char *buffer, int data_size, int flag,
                             int image_size, int image_fd);
extern int send_exec_message(int fd, unsigned long addr, int flag);
extern int send_dump_mem_message(int fd, int size, unsigned long addr,
                                 int flag);
int send_uart_speed_message(int fd, unsigned long speed, int flag);
extern unsigned short frm_chk(const unsigned short *src, int len);
extern unsigned short boot_checksum(const unsigned char *src, int len);
unsigned short calculate_crc(unsigned short crc, char const *buffer, int len);
void set_raw_data_speed(int fd, int speed);
unsigned int send_getchipid_message(int fd, int flag);
#endif  // PACKET_H_
