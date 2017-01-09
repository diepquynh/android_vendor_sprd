/*
 *  cp_log_cmn.h - Common functions declarations for the CP log and dump
 *                 program.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef _CP_LOG_CMN_H_
#define _CP_LOG_CMN_H_

#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef HOST_TEST_
#include <list>
#include <vector>
#include <string>

#define LogList std::list
#define LogVector std::vector
#define LogString std::string

inline const char* ls2cstring(const LogString& str) { return str.c_str(); }

inline void str_assign(LogString& str, const char* s, size_t n) {
  str.assign(s, n);
}

inline bool str_empty(const LogString& s) { return s.empty(); }

template <typename C>
void remove_at(C& c, size_t index) {
  typename C::iterator it = c.begin();
  size_t i = 0;

  while (i < index) {
    ++it;
    ++i;
  }
  c.erase(it);
}
template <typename C, typename V>
void ll_remove(C& c, const V& v) {
  c.remove(v);
}

template <typename T>
T rm_last(LogList<T>& c) {
  T t = c.back();
  c.pop_back();
  return t;
}

#else  // !HOST_TEST_

#include <utils/List.h>
#include <utils/Vector.h>
#include <utils/String8.h>

#define LogList android::List
#define LogVector android::Vector
#define LogString android::String8

inline const char* ls2cstring(const LogString& str) { return str.string(); }

inline void str_assign(LogString& str, const char* s, size_t n) {
  str.setTo(s, n);
}

inline bool str_empty(const LogString& s) { return s.isEmpty(); }

template <typename C>
void remove_at(C& c, size_t index) {
  c.removeAt(index);
}

template <typename C, typename V>
void ll_remove(C& c, const V& v) {
  typename C::iterator it;
  for (it = c.begin(); it != c.end(); ++it) {
    if (*it == v) {
      c.erase(it);
      break;
    }
  }
}

template <typename T>
T rm_last(LogList<T>& c) {
  typename LogList<T>::iterator it1;
  typename LogList<T>::iterator it2;
  T e;

  for (it1 = c.begin(); it1 != c.end(); ++it1) {
    it2 = it1;
  }
  e = *it2;
  c.erase(it2);
  return e;
}
#endif

inline bool str_starts_with(const LogString& s1, const char* s2, size_t len) {
  return s1.length() >= len && !memcmp(ls2cstring(s1), s2, len);
}

inline bool str_ends_with(const LogString& s1, const char* s2, size_t len) {
  return s1.length() >= len &&
         !memcmp(ls2cstring(s1) + s1.length() - len, s2, len);
}

template <typename C>
void clear_ptr_container(C& c) {
  typename C::iterator it;

  for (it = c.begin(); it != c.end(); ++it) {
    delete (*it);
  }
  c.clear();
}

#include <errno.h>
#ifdef HOST_TEST_
#include <cstdio>
#define ALOGE printf
#define ALOGD printf
#else
#include <cutils/log.h>
#endif

#define LOG_LEVEL 3

#if LOG_LEVEL >= 1
#define err_log(fmt, arg...) \
  ALOGE("%s: " fmt " [%d(%s)]\n", __func__, ##arg, errno, strerror(errno))
#else
#define err_log(fmt, arg...)
#endif

#if (LOG_LEVEL >= 2)
#define warn_log(fmt, arg...) ALOGE("%s: " fmt "\n", __func__, ##arg)
#else
#define warn_log(fmt, arg...)
#endif

#if (LOG_LEVEL >= 3)
#define info_log(fmt, arg...) ALOGE("%s: " fmt "\n", __func__, ##arg)
#else
#define info_log(fmt, arg...)
#endif

enum CpType {
  CT_UNKNOWN = -1,
  CT_WCDMA,
  CT_TD,
  CT_3MODE,
  CT_4MODE,
  CT_5MODE,
  CT_WCN,
  CT_GNSS,
  CT_AGDSP,
  CT_NUMBER,
  CT_WANMODEM
};

enum IqType { IT_UNKNOWN = -1, IT_GSM, IT_WCDMA, IT_ALL };

struct modem_timestamp {
  uint32_t magic_number; /* magic number for verify the structure */
  uint32_t tv_sec;       /* clock time, seconds since 1970.01.01 */
  uint32_t tv_usec;      /* clock time, microeseconds part */
  uint32_t sys_cnt;      /* modem's time */
};

int copy_file(int src_fd, int dest_fd);
int get_timezone();
int copy_file_seg(int src_fd, int dest_fd, size_t m);
int copy_file(const char* src, const char* dest);
int set_nonblock(int fd);
void data2HexString(uint8_t* out_buf, const uint8_t* data, size_t len);

#endif  // !_CP_LOG_CMN_H_
