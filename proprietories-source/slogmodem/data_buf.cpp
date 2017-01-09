/*
 *  data_buf.cpp - data buffer functions.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-12 Zhang Ziyi
 *  Initial version.
 */

#include "data_buf.h"

DataBuffer* alloc_data_buf(size_t buf_size) {
  DataBuffer* buf = new DataBuffer;

  buf->buffer = new uint8_t[buf_size];
  buf->buf_size = buf_size;
  buf->data_start = buf->data_len = 0;
  buf->dst_offset = -1;

  return buf;
}

DataBuffer::~DataBuffer() {
  delete [] buffer;
}
