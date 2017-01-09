/*
 *  data_buf.h - data buffer definitions.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-11 Zhang Ziyi
 *  Initial version.
 */
#ifndef DATA_BUF_H_
#define DATA_BUF_H_

#include <cstddef>
#include <cstdint>

struct DataBuffer {
  uint8_t* buffer;
  size_t buf_size;
  size_t data_start;
  size_t data_len;
  int dst_offset;

  ~DataBuffer();
};

DataBuffer* alloc_data_buf(size_t buf_size);

#endif  //!DATA_BUF_H_
