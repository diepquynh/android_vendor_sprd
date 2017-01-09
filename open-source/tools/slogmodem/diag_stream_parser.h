/*
 *  diag_stream_parser.h - diag frame parser
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-15 Cao Xiaoyin
 *  Initial version.
 */

#ifndef _DIAG_STREAM_PARSER_H_
#define _DIAG_STREAM_PARSER_H_

#include "diag_cmd_def.h"

class DiagStreamParser {
 public:
  DiagStreamParser();
  ~DiagStreamParser();

  bool unescape(uint8_t *src_ptr, size_t src_len, uint8_t **dst_ptr,
                size_t *dst_len, size_t *used_len);

  void frame(uint8_t type, uint8_t subtype, uint8_t *pl, size_t pl_len,
             uint8_t **out_buf, size_t *out_len);

  uint8_t get_type(uint8_t *frame_head) {
    return ((struct diag_cmd_head *)frame_head)->type;
  }

  uint8_t get_subytpe(uint8_t *frame_head) {
    return ((struct diag_cmd_head *)frame_head)->subtype;
  }

  uint8_t *get_payload(uint8_t *frame_head) {
    return frame_head + sizeof(struct diag_cmd_head);
  }

  uint8_t get_head_size() { return sizeof(struct diag_cmd_head); }

 private:
  DiagParseState m_state;
  uint8_t m_pool[64 * 1024];
  size_t m_len;

  size_t escape(uint8_t *src_ptr, size_t src_len, uint8_t *dst_ptr);
};

#endif
