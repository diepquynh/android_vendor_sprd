/*
 *  diag_stream_parser.cpp - diag frame parser
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-15 Cao Xiaoyin
 *  Initial version.
 */
#include "diag_cmd_def.h"
#include "diag_stream_parser.h"

DiagStreamParser::DiagStreamParser() : m_state{PPP_DOWN}, m_len{0} {}

DiagStreamParser::~DiagStreamParser() {}

bool DiagStreamParser::unescape(uint8_t *src_ptr, size_t src_len,
                                uint8_t **dst_ptr, size_t *dst_len,
                                size_t *used_len) {
  size_t ori_len = src_len;

  *dst_ptr = NULL;
  *dst_len = 0;

  while (src_len) {
    switch (m_state) {
      case PPP_DOWN:
        if (*src_ptr == FLAG_BYTE) {
          m_state = PPP_READY;
        }
        src_ptr++;
        src_len--;
        break;
      case PPP_HALT:
        m_state = PPP_UP;
        m_pool[m_len++] = *src_ptr ^ COMPLEMENT_BYTE;
        src_ptr++;
        src_len--;
        break;
      case PPP_READY:
        if (*src_ptr == FLAG_BYTE) {
          src_ptr++;
          src_len--;
          break;
        }
        m_state = PPP_UP;
      case PPP_UP:
        if (*src_ptr == FLAG_BYTE) {
          m_state = PPP_DOWN;
          src_ptr++;
          src_len--;
          *dst_ptr = m_pool;
          *dst_len = m_len;
          *used_len = ori_len - src_len;
          m_len = 0;
          return true;
        } else if (*src_ptr == ESCAPE_BYTE) {
          if (src_len > 1) {
            m_pool[m_len++] = *(src_ptr + 1) ^ COMPLEMENT_BYTE;
            src_ptr += 2;
            src_len -= 2;
          } else {
            m_state = PPP_HALT;
            src_ptr++;
            src_len--;
          }
        } else {
          m_pool[m_len++] = *src_ptr++;
          src_len--;
        }
        break;
      default:
        break;
    }
  }
  *used_len = ori_len;
  return false;
}

size_t DiagStreamParser::escape(uint8_t *src_ptr, size_t src_len,
                                uint8_t *dst_ptr) {
  size_t tmp_len = 0;

  while (src_len) {
    switch (*src_ptr) {
      case FLAG_BYTE:
      case ESCAPE_BYTE:
        dst_ptr[tmp_len++] = ESCAPE_BYTE;
        dst_ptr[tmp_len++] = *src_ptr ^ COMPLEMENT_BYTE;
        break;
      default:
        dst_ptr[tmp_len++] = *src_ptr;
    }
    src_len--;
    src_ptr++;
  }

  return tmp_len;
}

void DiagStreamParser::frame(uint8_t type, uint8_t subtype, uint8_t *pl,
                             size_t pl_len, uint8_t **out_buf,
                             size_t *out_len) {
  struct diag_cmd_head head_ptr;
  uint8_t *buf =
      new uint8_t[((pl_len + sizeof(struct diag_cmd_head)) << 1) + 2];
  size_t len = 0;

  buf[len++] = FLAG_BYTE;
  head_ptr.seq_num = 0;  // diag-frame has no sequence number
  head_ptr.data_len = sizeof(struct diag_cmd_head) + pl_len;
  head_ptr.type = type;
  head_ptr.subtype = subtype;
  len += escape(reinterpret_cast<uint8_t *>(&head_ptr),
                sizeof(struct diag_cmd_head),
                &buf[len]);
  if (pl && pl_len) {
    len += escape(pl, pl_len, &buf[len]);
  }
  buf[len++] = FLAG_BYTE;
  *out_buf = buf;
  *out_len = len;
}
