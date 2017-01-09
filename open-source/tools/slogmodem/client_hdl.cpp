/*
 *  client_hdl.cpp - The base class implementation for file descriptor
 *                   handler.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 *
 *  2015-6-5 Zhang Ziyi
 *  CP dump notification added.
 *
 *  2015-7-22 Zhang Ziyi
 *  I/Q commands added.
 */
#include <cstring>
#include <poll.h>
#include <unistd.h>

#include "client_hdl.h"
#include "client_mgr.h"
#include "client_req.h"
#include "log_ctrl.h"
#include "log_pipe_hdl.h"
#include "parse_utils.h"
#include "req_err.h"

ClientHandler::ClientHandler(int sock, LogController* ctrl,
                             Multiplexer* multiplexer, ClientManager* mgr)
    : DataProcessHandler(sock, ctrl, multiplexer, CLIENT_BUF_SIZE),
      m_mgr(mgr),
      m_trans_type{CTT_UNKNOWN},
      m_state{CTS_IDLE},
      m_cp{0},
      m_cp_dump_notify{false} {}

ClientHandler::~ClientHandler() {
  if (CTS_IDLE != m_state) {
    m_cp->cancel_trans_result_notify(this);
  }
}

const uint8_t* ClientHandler::search_end(const uint8_t* req, size_t len) {
  const uint8_t* endp = req + len;

  while (req < endp) {
    if ('\0' == *req || '\n' == *req) {
      break;
    }
    ++req;
  }

  if (req == endp) {
    req = 0;
  }

  return req;
}

int ClientHandler::process_data() {
  const uint8_t* start = m_buffer.buffer;
  const uint8_t* end = m_buffer.buffer + m_buffer.data_len;
  size_t rlen = m_buffer.data_len;

  while (start < end) {
    const uint8_t* p1 = search_end(start, rlen);
    if (!p1) {  // Not complete request
      break;
    }
    process_req(start, p1 - start);
    start = p1 + 1;
    rlen = end - start;
  }

  if (rlen && start != m_buffer.buffer) {
    memmove(m_buffer.buffer, start, rlen);
  }
  m_buffer.data_len = rlen;

  return 0;
}

void ClientHandler::process_req(const uint8_t* req, size_t len) {
  size_t tok_len;
  const uint8_t* endp = req + len;
  const uint8_t* token = get_token(req, len, tok_len);

  if (!token) {
    // Empty line: ignore it.
    return;
  }

  // What request?
  bool known_req = false;

  req = token + tok_len;
  len = endp - req;
  switch (tok_len) {
    case 5:
      if (!memcmp(token, "FLUSH", 5)) {
        proc_flush(req, len);
        known_req = true;
      }
      break;
    case 7:
      if (!memcmp(token, "slogctl", 7)) {
        proc_slogctl(req, len);
        known_req = true;
      }
      break;
    case 9:
      if (!memcmp(token, "ENABLE_IQ", 9)) {
        proc_enable_iq(req, len);
        known_req = true;
      } else if (!memcmp(token, "ENABLE_MD", 9)) {
        proc_enable_md(req, len);
        known_req = true;
      } else if (!memcmp(token, "MINI_DUMP", 9)) {
        proc_mini_dump(req, len);
        known_req = true;
      } else if (!memcmp(token, "SUBSCRIBE", 9)) {
        proc_subscribe(req, len);
        known_req = true;
      }
      break;
    case 10:
      if (!memcmp(token, "DISABLE_IQ", 10)) {
        proc_disable_iq(req, len);
        known_req = true;
      } else if (!memcmp(token, "ENABLE_LOG", 10)) {
        proc_enable_log(req, len);
        known_req = true;
      } else if (!memcmp(token, "DISABLE_MD", 10)) {
        proc_disable_md(req, len);
        known_req = true;
      }
      break;
    case 11:
      if (!memcmp(token, "DISABLE_LOG", 11)) {
        proc_disable_log(req, len);
        known_req = true;
      } else if (!memcmp(token, "UNSUBSCRIBE", 11)) {
        proc_unsubscribe(req, len);
        known_req = true;
      }
      break;
    case 12:
      if (!memcmp(token, "SAVE_RINGBUF", 12)) {
        proc_ringbuf(req, len);
        known_req = true;
      }
      break;
    case 13:
      if (!memcmp(token, "GET_LOG_STATE", 13)) {
        proc_get_log_state(req, len);
        known_req = true;
      }
      break;
    case 14:
      if (!memcmp(token, "SAVE_SLEEP_LOG", 14)) {
        proc_sleep_log(req, len);
        known_req = true;
      }
      break;
    case 15:
      if (!memcmp(token, "GET_SD_MAX_SIZE", 15)) {
        proc_get_sd_size(req, len);
        known_req = true;
      } else if (!memcmp(token, "SET_SD_MAX_SIZE", 15)) {
        proc_set_sd_size(req, len);
        known_req = true;
      } else if (!memcmp(token, "GET_MD_STOR_POS", 15)) {
        proc_get_md_pos(req, len);
        known_req = true;
      } else if (!memcmp(token, "SET_MD_STOR_POS", 15)) {
        proc_set_md_pos(req, len);
	known_req = true;
      }
      break;
    case 17:
      if (!memcmp(token, "GET_LOG_OVERWRITE", 17)) {
        proc_get_log_overwrite(req, len);
        known_req = true;
      } else if (!memcmp(token, "GET_LOG_FILE_SIZE", 17)) {
        proc_get_log_file_size(req, len);
        known_req = true;
      } else if (!memcmp(token, "SET_LOG_FILE_SIZE", 17)) {
        proc_set_log_file_size(req, len);
        known_req = true;
      } else if (!memcmp(token, "GET_DATA_MAX_SIZE", 17)) {
        proc_get_data_part_size(req, len);
        known_req = true;
      } else if (!memcmp(token, "SET_DATA_MAX_SIZE", 17)) {
        proc_set_data_part_size(req, len);
        known_req = true;
      }
      break;
    case 20:
      if (!memcmp(token, "ENABLE_LOG_OVERWRITE", 20)) {
        proc_enable_overwrite(req, len);
        known_req = true;
      }
      break;
    case 21:
      if (!memcmp(token, "DISABLE_LOG_OVERWRITE", 21)) {
        proc_disable_overwrite(req, len);
        known_req = true;
      }
      break;
    default:
      break;
  }

  if (!known_req) {
    err_log("unknown request");

    send_response(m_fd, REC_UNKNOWN_REQ);
  }
}

void ClientHandler::proc_flush(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    send_response(m_fd, REC_UNKNOWN_REQ);
    return;
  }

  int ret = controller()->flush();

  ResponseErrorCode code;
  if (!ret) {
    code = REC_SUCCESS;
  } else {
    code = REC_FAILURE;
  }
  send_response(m_fd, code);
}

void ClientHandler::proc_slogctl(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    send_response(m_fd, REC_UNKNOWN_REQ);
    return;
  }

  if (5 == tlen && !memcmp(tok, "clear", 5)) {
    info_log("remove CP log");
    // Delete all logs
    int ret = controller()->clear_log();
    ResponseErrorCode err;
    switch (ret) {
      case LCR_SUCCESS:
        err = REC_SUCCESS;
        break;
      case LCR_IN_TRANSACTION:
        err = REC_IN_TRANSACTION;
        break;
      default:
        err = REC_FAILURE;
        break;
    }
    send_response(m_fd, err);
  } else if (6 == tlen && !memcmp(tok, "reload", 6)) {
    info_log("reload slog.conf");
    // Reload slog.conf and update CP log and log file size
    int ret = controller()->reload_slog_conf();
    ResponseErrorCode code;
    if (!ret) {
      code = REC_SUCCESS;
    } else {
      code = REC_FAILURE;
    }
    send_response(m_fd, code);
  } else {
    send_response(m_fd, REC_UNKNOWN_REQ);
  }
}

void ClientHandler::proc_enable_log(const uint8_t* req, size_t len) {
  LogString sd;

  str_assign(sd, reinterpret_cast<const char*>(req), len);
  info_log("ENABLE_LOG %s", ls2cstring(sd));

  // Parse MODEM types
  ModemSet ms;
  int err = parse_modem_set(req, len, ms);

  if (err || !ms.num) {
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  err = controller()->enable_log(ms.modems, ms.num);
  ResponseErrorCode ec = REC_SUCCESS;
  if (err) {
    ec = REC_FAILURE;
  }

  send_response(m_fd, ec);
}

void ClientHandler::proc_disable_log(const uint8_t* req, size_t len) {
  LogString sd;

  str_assign(sd, reinterpret_cast<const char*>(req), len);
  info_log("DISABLE_LOG %s", ls2cstring(sd));

  // Parse MODEM types
  ModemSet ms;
  int err = parse_modem_set(req, len, ms);

  if (err || !ms.num) {
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  err = controller()->disable_log(ms.modems, ms.num);
  ResponseErrorCode ec = REC_SUCCESS;
  if (err) {
    ec = REC_FAILURE;
  }

  send_response(m_fd, ec);
}

void ClientHandler::proc_enable_md(const uint8_t* req, size_t len) {
  size_t i;

  for (i = 0; i < len; ++i) {
    uint8_t c = req[i];
    if (' ' != c && '\t' != c) {
      break;
    }
  }
  if (i < len) {
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  int err = controller()->enable_md();
  ResponseErrorCode ec = REC_SUCCESS;
  if (err) {
    ec = REC_FAILURE;
  }

  send_response(m_fd, ec);
}

void ClientHandler::proc_disable_md(const uint8_t* req, size_t len) {
  size_t i;

  for (i = 0; i < len; ++i) {
    uint8_t c = req[i];
    if (' ' != c && '\t' != c) {
      break;
    }
  }
  if (i < len) {
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  int err = controller()->disable_md();
  ResponseErrorCode ec = REC_SUCCESS;
  if (err) {
    ec = REC_FAILURE;
  }

  send_response(m_fd, ec);
}

void ClientHandler::proc_mini_dump(const uint8_t* req, size_t len) {
  size_t i;

  for (i = 0; i < len; ++i) {
    uint8_t c = req[i];
    if (' ' != c && '\t' != c) {
      break;
    }
  }
  if (i < len) {
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  int err = controller()->mini_dump();
  ResponseErrorCode ec = REC_SUCCESS;
  if (err) {
    ec = REC_FAILURE;
  }

  send_response(m_fd, ec);
}

void ClientHandler::process_conn_closed() {
  // Clear current transaction result notification
  if (CTS_IDLE != m_state) {
    m_cp->cancel_trans_result_notify(this);
    m_state = CTS_IDLE;
    m_cp = 0;
  }

  // Inform ClientManager the connection is closed
  m_mgr->process_client_disconn(this);
}

void ClientHandler::process_conn_error(int /*err*/) {
  ClientHandler::process_conn_closed();
}

void ClientHandler::proc_set_log_file_size(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SET_LOG_FILE_SIZE invalid parameter");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  unsigned val;

  if (parse_number(tok, tlen, val)) {
    err_log("SET_LOG_FILE_SIZE invalid size");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  info_log("SET_LOG_FILE_SIZE %u", val);

  req = tok + tlen;
  len = endp - req;
  if (len && get_token(req, len, tlen)) {
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  controller()->set_log_file_size(val);
  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_enable_overwrite(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("ENABLE_LOG_OVERWRITE invalid param");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  info_log("ENABLE_LOG_OVERWRITE");

  controller()->set_log_overwrite();
  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_disable_overwrite(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("DISABLE_LOG_OVERWRITE invalid param");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  info_log("DISABLE_LOG_OVERWRITE");

  controller()->set_log_overwrite(false);
  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_set_md_pos(const uint8_t* req, size_t len)
{
  const uint8_t* tok;
  size_t tlen;
  bool md_int_stor = false;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SET_MD_STOR_POS invalid param");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  if (!memcmp(tok, "INTERNAL", 8)) {
    md_int_stor = true;
  } else if (!memcmp(tok, "EXTERNAL", 8)) {
    md_int_stor = false;
  }

  info_log("SET_MD_STOR_POS: %s.", md_int_stor ? "INTERNAL" : "EXTERNAL" );
  controller()->set_md_int_stor(md_int_stor);
  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_set_data_part_size(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SET_DATA_MAX_SIZE no param");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  unsigned val;

  if (parse_number(tok, tlen, val)) {
    err_log("SET_DATA_MAX_SIZE invalid param");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  if (len && get_token(req, len, tlen)) {
    err_log("SET_DATA_MAX_SIZE invalid param");

    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  info_log("SET_DATA_MAX_SIZE %u", val);

  controller()->set_data_part_size(val);
  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_set_sd_size(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SET_SD_MAX_SIZE no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  unsigned val;

  if (parse_number(tok, tlen, val)) {
    err_log("SET_SD_MAX_SIZE invalid size");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  if (len && get_token(req, len, tlen)) {
    err_log("SET_SD_MAX_SIZE invalid param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  info_log("SET_SD_MAX_SIZE %u", val);
  controller()->set_sd_size(val);
  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_get_log_file_size(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("GET_LOG_FILE_SIZE invalid param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  size_t sz = controller()->get_log_file_size();
  char rsp[64];
  int rsp_len = snprintf(rsp, sizeof rsp, "OK %u\n", static_cast<unsigned>(sz));
  write(m_fd, rsp, rsp_len);

  info_log("GET_LOG_FILE_SIZE %u", static_cast<unsigned>(sz));
}

void ClientHandler::proc_get_data_part_size(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("GET_DATA_MAX_SIZE invalid param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  size_t sz = controller()->get_data_part_size();
  char rsp[64];
  int rsp_len = snprintf(rsp, sizeof rsp, "OK %u\n", static_cast<unsigned>(sz));
  write(m_fd, rsp, rsp_len);
  info_log("GET_DATA_MAX_SIZE %u", static_cast<unsigned>(sz));
}

void ClientHandler::proc_get_sd_size(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("GET_SD_MAX_SIZE invalid param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  size_t sz = controller()->get_sd_size();
  char rsp[64];
  int rsp_len = snprintf(rsp, sizeof rsp, "OK %u\n", static_cast<unsigned>(sz));
  write(m_fd, rsp, rsp_len);
  info_log("GET_SD_MAX_SIZE %u", static_cast<unsigned>(sz));
}

void ClientHandler::proc_get_log_overwrite(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("GET_LOG_OVERWRITE invalid param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  bool ow = controller()->get_log_overwrite();
  char rsp[64];
  int rsp_len = snprintf(rsp, sizeof rsp, "OK %s\n", ow ? "ENABLE" : "DISABLE");
  write(m_fd, rsp, rsp_len);
  info_log("GET_LOG_OVERWRITE %s", ow ? "ENABLE" : "DISABLE");
}

void ClientHandler::proc_get_md_pos(const uint8_t* req, size_t len)
{
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("GET_MD_STOR_POS invalid param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  bool md_int_stor = controller()->get_md_int_stor();
  char rsp[64];
  int rsp_len = snprintf(rsp, sizeof rsp, "OK %s\n",
                         md_int_stor ? "INTERNAL" : "EXTERNAL");
  write(m_fd, rsp, rsp_len);
  info_log("GET_MD_STOR_POS %s.", md_int_stor ? "INTERNAL" : "EXTERNAL");
}

void ClientHandler::proc_subscribe(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SUBSCRIBE no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType cpt = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == cpt) {
    err_log("SUBSCRIBE invalid CP type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SUBSCRIBE no <event>");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  if (4 != tlen || memcmp(tok, "DUMP", 4)) {
    err_log("SUBSCRIBE invalid <event>");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  if (len && get_token(req, len, tlen)) {
    err_log("SUBSCRIBE more params than expected");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  m_cp_dump_notify[cpt] = true;

  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::notify_cp_dump(CpType cpt, CpEvent evt) {
  if (cpt > CT_UNKNOWN && cpt < CT_NUMBER && CE_DUMP_START <= evt &&
      CE_DUMP_END >= evt && m_cp_dump_notify[cpt]) {
    send_dump_notify(m_fd, cpt, evt);
  }
}

int ClientHandler::send_dump_notify(int fd, CpType cpt, CpEvent evt) {
  uint8_t buf[128];
  size_t len;

  if (CE_DUMP_START == evt) {
    memcpy(buf, "CP_DUMP_START ", 14);
    len = 14;
  } else {
    memcpy(buf, "CP_DUMP_END ", 12);
    len = 12;
  }

  size_t rlen = sizeof buf - len;
  size_t tlen;
  int ret = -1;
  if (!put_cp_type(buf + len, rlen, cpt, tlen)) {
    len += tlen;
    rlen -= tlen;
    if (rlen) {
      buf[len] = '\n';
      ++len;
      ssize_t wlen = write(fd, buf, len);
      if (static_cast<size_t>(wlen) == len) {
        ret = 0;
      }
    }
  }

  return ret;
}

void ClientHandler::proc_unsubscribe(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("UNSUBSCRIBE no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType cpt = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == cpt) {
    err_log("UNSUBSCRIBE invalid CP type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("UNSUBSCRIBE no <event>");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  if (4 != tlen || memcmp(tok, "DUMP", 4)) {
    err_log("UNSUBSCRIBE invalid <event>");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  if (len && get_token(req, len, tlen)) {
    err_log("UNSUBSCRIBE more params than expected");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  m_cp_dump_notify[cpt] = false;

  send_response(m_fd, REC_SUCCESS);
}

void ClientHandler::proc_sleep_log(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  info_log("SAVE_SLEEP_LOG");

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SAVE_SLEEP_LOG no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType cpt = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == cpt) {
    err_log("SAVE_SLEEP_LOG invalid CP type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("SAVE_SLEEP_LOG extra parameter");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  // Start the sleep log transaction
  LogPipeHandler* cp = controller()->get_cp(cpt);
  if (!cp) {
    err_log("SAVE_SLEEP_LOG unknown CP");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  int err = cp->save_sleep_log(this);
  if (LCR_IN_PROGRESS == err) {
    // Pause data receiving
    del_events(POLLIN);
    m_trans_type = CTT_SAVE_SLEEP_LOG;
    m_state = CTS_EXECUTING;
    m_cp = cp;
  } else {
    err_log("Save sleep log error %d", err);
    send_response(m_fd, trans_result_to_req_result(err));
  }
}

void ClientHandler::proc_ringbuf(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  info_log("SAVE_RINGBUF");

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("SAVE_RINGBUF no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType cpt = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == cpt) {
    err_log("SAVE_RINGBUF invalid CP type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  tok = get_token(req, len, tlen);
  if (tok) {
    err_log("SAVE_RINGBUF extra parameter");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  // Start the sleep log transaction
  LogPipeHandler* cp = controller()->get_cp(cpt);
  if (!cp) {
    err_log("SAVE_RINGBUF unknown CP");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  int err = cp->save_ringbuf(this);
  if (LCR_IN_PROGRESS == err) {
    // Pause data receiving
    del_events(POLLIN);
    m_trans_type = CTT_SAVE_RINGBUF;
    m_state = CTS_EXECUTING;
    m_cp = cp;
  } else {
    err_log("Save ringbuf error %d", err);
    send_response(m_fd, trans_result_to_req_result(err));
  }
}

void ClientHandler::notify_trans_result(int result) {
  if (CTS_EXECUTING == m_state) {
    switch (m_trans_type) {
      case CTT_SAVE_SLEEP_LOG:
        proc_sleep_log_result(result);
        break;
      case CTT_SAVE_RINGBUF:
        proc_ringbuf_result(result);
        break;
      default:
        err_log("Unknown transaction %d result %d", m_trans_type, result);
        break;
    }
  } else {
    err_log("Non CTS_EXECUTING state notification %d", result);
  }
}

void ClientHandler::proc_sleep_log_result(int result) {
  info_log("SAVE_SLEEP_LOG result %d", result);

  m_state = CTS_IDLE;
  m_cp = 0;

  add_events(POLLIN);
  send_response(m_fd, trans_result_to_req_result(result));
}

void ClientHandler::proc_ringbuf_result(int result) {
  info_log("SAVE_RINGBUF result %d", result);

  m_state = CTS_IDLE;
  m_cp = 0;

  add_events(POLLIN);
  send_response(m_fd, trans_result_to_req_result(result));
}

ResponseErrorCode ClientHandler::trans_result_to_req_result(int result) {
  ResponseErrorCode res;

  switch (result) {
    case LCR_SUCCESS:
      res = REC_SUCCESS;
      break;
    case LCR_IN_TRANSACTION:
      res = REC_IN_TRANSACTION;
      break;
    case LCR_LOG_DISABLED:
      res = REC_LOG_DISABLED;
      break;
    case LCR_SLEEP_LOG_NOT_SUPPORTED:
      res = REC_SLEEP_LOG_NOT_SUPPORTED;
      break;
    case LCR_RINGBUF_NOT_SUPPORTED:
      res = REC_RINGBUF_NOT_SUPPORTED;
      break;
    case LCR_CP_ASSERTED:
      res = REC_CP_ASSERTED;
      break;
    default:
      res = REC_FAILURE;
      break;
  }

  return res;
}

void ClientHandler::proc_enable_iq(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("ENABLE_IQ no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType cpt = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == cpt) {
    err_log("ENABLE_IQ invalid CP type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  req = tok + tlen;
  len = endp - req;
  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("ENABLE_IQ no I/Q type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  IqType it;

  it = LogConfig::get_iq_type(tok, tlen);
  if (IT_WCDMA != it) {
    err_log("ENABLE_IQ unknown I/Q type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  // Enable I/Q saving
  int err = controller()->enable_wcdma_iq(cpt);
  if (LCR_SUCCESS != err) {
    err_log("enable_iq error %d", err);
  }
  send_response(m_fd, trans_result_to_req_result(err));
}

void ClientHandler::proc_disable_iq(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  const uint8_t* endp = req + len;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("DISABLE_IQ no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType cpt = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == cpt) {
    err_log("DISABLE_IQ invalid CP type");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  IqType it;

  req = tok + tlen;
  len = endp - req;
  tok = get_token(req, len, tlen);
  if (tok) {
    it = LogConfig::get_iq_type(tok, tlen);
    if (IT_WCDMA != it) {
      err_log("DISABLE_IQ unknown I/Q type");
      send_response(m_fd, REC_INVAL_PARAM);
      return;
    }
  } else {
    it = IT_ALL;
  }

  // Disable I/Q log
  int err = controller()->disable_wcdma_iq(cpt);
  if (LCR_SUCCESS != err) {
    err_log("disable_iq error %d", err);
  }
  send_response(m_fd, trans_result_to_req_result(err));
}

void ClientHandler::proc_get_log_state(const uint8_t* req, size_t len) {
  const uint8_t* tok;
  size_t tlen;

  tok = get_token(req, len, tlen);
  if (!tok) {
    err_log("GET_LOG_STATE no param");
    send_response(m_fd, REC_INVAL_PARAM);
    return;
  }

  CpType ct = get_cp_type(tok, tlen);
  if (CT_UNKNOWN == ct) {
    err_log("GET_LOG_STATE invalid CP type");
    send_response(m_fd, REC_UNKNOWN_CP_TYPE);
    return;
  }

  bool enabled = controller()->get_log_state(ct);
  send_log_state_response(m_fd, enabled);
}
