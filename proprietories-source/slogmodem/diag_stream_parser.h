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

  /*  reset - reset the parser state.
   */
  void reset();
  /*  unescape - parse the data stream for frames
   *  @src_ptr: input data pointer
   *  @src_len: input data length in byte
   *  @dst_ptr: if a frame is finished, return the frame
   *            pointer in *dst_ptr. The buffer is managed by
   *            DiagStreamParser and the client shall not free it.
   *  @dst_len: if a frame is finished, return the frame
   *            length in *dst_len
   *  @used_len: return the length of the parsed data in *used_len
   *
   *  Return Value:
   *    If a frame is finished, return true; otherwise return
   *    false.
   */
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

  /*  frame - form a diagnosis frame.
   *  @sn: serial number
   *  @cmd: CMD
   *  @sub_cmd: SubCMD
   *  @pl: pointer to the frame payload
   *  @pl_len: payload length
   *  @frame_len: the diagnosis frame length
   *
   *  Return Value:
   *    The pointer to the frame. The buffer is allocated using new, and
   *    the client shall delete it when it's not needed.
   */
  static uint8_t* frame(uint32_t sn, int cmd, int sub_cmd,
                        const uint8_t* pl, size_t pl_len,
                        size_t& frame_len);

 private:
  DiagParseState m_state;
  uint8_t m_pool[64 * 1024];
  size_t m_len;

  static size_t escape(const uint8_t *src_ptr, size_t src_len,
                       uint8_t *dst_ptr);
};

#endif  //!_DIAG_STREAM_PARSER_H_
