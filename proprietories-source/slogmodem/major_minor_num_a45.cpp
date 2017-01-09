/*
 *  stor_mgr.cpp - storage manager.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-3-22 Zhihang YAN
 *  Initial version.
 */
#include <ctype.h>

#include "cp_log_cmn.h"
#include "parse_utils.h"
#include "stor_mgr.h"

int StorageManager::get_major_minor_in_mount(const uint8_t* mount_info,
    size_t len) {
  int err = -1;

  if (!mount_info || (!len)) {
    return err;
  }

  const uint8_t* minor_str =
      static_cast<uint8_t*>(memchr(mount_info, ':', len));

  if (minor_str && (minor_str > mount_info)) {

    const uint8_t* p_minor = minor_str;
    const uint8_t* major_str = minor_str;

    while ((p_minor > mount_info) && (isdigit(*(-- p_minor)))) {
      -- major_str;
    }

    if (major_str != minor_str) {
      unsigned major_num;
      unsigned minor_num;
      if (!parse_number(major_str,
                        minor_str - major_str, major_num) &&
          !parse_number((minor_str + 1),
                        len - (minor_str - mount_info) -1, minor_num)) {
        m_major_num = major_num;
        m_minor_num = minor_num;

        info_log("major number: %d, minor number: %d.",
                  m_major_num, m_minor_num);
        err = 0;
      }
    }
  }

  return err;
}
