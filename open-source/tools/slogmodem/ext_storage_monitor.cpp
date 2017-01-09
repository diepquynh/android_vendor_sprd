/*
 *  ext_storage_monitor.cpp - kernel will send a netlink socket to notify the sdcard's status.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-11-23 Yan Zhihang
 *  Initial version.
 */
#include <cutils/sockets.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "cp_log_cmn.h"
#include "ext_storage_monitor.h"
#include "parse_utils.h"
#include "stor_mgr.h"

ExtStorageMonitor::ExtStorageMonitor(StorageManager* stor_mgr,
                                     LogController* ctrl,
                                     Multiplexer* multi)
    : FdHandler(-1, ctrl, multi), m_stor_mgr(stor_mgr) {}

ExtStorageMonitor::~ExtStorageMonitor() {
  clear_ptr_container(m_event_clients);
}

int ExtStorageMonitor::init() {
  struct sockaddr_nl nls;

  memset(&nls, 0, sizeof(struct sockaddr_nl));
  nls.nl_family = AF_NETLINK;
  nls.nl_pid = getpid();
  nls.nl_groups = -1;
  m_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);

  if ((m_fd = socket(PF_NETLINK, SOCK_RAW,
                     NETLINK_KOBJECT_UEVENT)) < 0) {
    err_log("Unable to create uevent socket.");
    return -1;
  }

  long flags = fcntl(m_fd, F_GETFL);
  flags |= O_NONBLOCK;
  int err = fcntl(m_fd, F_SETFL, flags);
  if (-1 == err) {
    err_log("set netlink socket to O_NONBLOCK error");
    ::close(m_fd);
    m_fd = -1;
    return -1;
  }

  if (bind(m_fd, (const sockaddr*)&nls, sizeof(struct sockaddr_nl))) {
    err_log("netlink bind err");
    ::close(m_fd);
    m_fd = -1;
    return -1;
  } else {
    multiplexer()->register_fd(this, POLLIN);
  }

  return 0;
}

void ExtStorageMonitor::subscribe_ext_storage_evt(void* client_ptr,
    ext_storage_evt_callback_t cb) {
  EventClient* c = new EventClient;
  c->client_ptr = client_ptr;
  c->cb = cb;
  m_event_clients.push_back(c);
}

void ExtStorageMonitor::unsubscribe_ext_storage_evt(void* client_ptr) {
  for (auto it = m_event_clients.begin(); it != m_event_clients.end(); ++it) {
    EventClient* client = *it;
    if (client_ptr == client->client_ptr) {
      m_event_clients.erase(it);
      delete client;
      break;
    }
  }
}

void ExtStorageMonitor::notify_ext_stor_evt(
    StorageManager::ExtStorEvent ext_stor_evt,
    unsigned major_num, unsigned minor_num) {
  for (auto it = m_event_clients.begin(); it != m_event_clients.end(); ++it) {
    EventClient* client = *it;
    client->cb(client->client_ptr, ext_stor_evt, major_num, minor_num);
  }
}

void ExtStorageMonitor::process(int events) {
  uint8_t buf[2048];
  uint8_t* pbuf = buf;
  uint8_t* pnext = 0;
  struct sockaddr_nl sa;
  struct iovec iov = { buf, sizeof(buf) };
  struct msghdr msg = { &sa, sizeof sa, &iov, 1, NULL, 0, 0 };

  unsigned evt_major_num = UINT_MAX;
  unsigned evt_minor_num = UINT_MAX;

  bool evt_block_subsystem = false;
  StorageManager::ExtStorEvent evt_action =
      StorageManager::ExtStorEvent::EVT_NONE;

  ssize_t len = recvmsg(m_fd, &msg, 0);

  if (-1 == len) {
    return;
  }
  size_t remain_len = len;
  uint8_t* end_ptr = buf + len;
  // netlink message format:
  //   (add/remove)@(device node path)\0... ...
  //   ACTION=(add/remove)\0
  //   MAJOR=xxx\0
  //   MINOR=xxx\0... ...
  //   SUBSYSTEM=block\0
  //   ... ...\0\0
  while (remain_len) {
    if ((remain_len >= 7) &&
        (!strncmp(reinterpret_cast<char*>(pbuf), "ACTION=", 7))) {
      remain_len -= 7;
      pbuf += 7;
      const uint8_t* tok;
      size_t tok_len;
      tok = get_token(pbuf, remain_len, tok_len);
      if (tok) {
        if ((3 == tok_len) && !memcmp(tok, "add", 3)) {
          evt_action = StorageManager::ExtStorEvent::EVT_ADD;
        } else if ((6 == tok_len) && (!memcmp(tok, "remove", 6))) {
          evt_action = StorageManager::ExtStorEvent::EVT_RMD;
        } else {
          // Action is neither add nor remove
          break;
        }

        pbuf = const_cast<uint8_t*>(tok) + tok_len;
        remain_len = end_ptr - pbuf;
      } else {
        // No token after ACTION=
        break;
      }
   } else if ((remain_len >= 6) &&
              (!strncmp(reinterpret_cast<char*>(pbuf), "MAJOR=", 6))) {
      remain_len -= 6;
      pbuf += 6;
      pnext = static_cast<uint8_t*>(memchr(pbuf, '\0', remain_len));
      if ((pnext <= pbuf) ||
          (parse_number(pbuf,
                        static_cast<size_t>(pnext - pbuf), evt_major_num))) {
        break;
      }
    } else if ((remain_len >= 6) &&
               (!strncmp(reinterpret_cast<char*>(pbuf), "MINOR=", 6))) {
      remain_len -= 6;
      pbuf += 6;
      pnext = static_cast<uint8_t*>(memchr(pbuf, '\0', remain_len));
      if ((pnext <= pbuf) ||
          (parse_number(reinterpret_cast<uint8_t*>(pbuf),
                        static_cast<size_t>(pnext - pbuf), evt_minor_num))) {
        break;
      }
    } else if ((remain_len >= 10) &&
               (!strncmp(reinterpret_cast<char*>(pbuf), "SUBSYSTEM=", 10))) {
      remain_len -= 10;
      pbuf += 10;
      const uint8_t* tok;
      size_t tok_len;
      tok = get_token(pbuf, remain_len, tok_len);
      if (tok) {
        if ((5 == tok_len) && !memcmp(tok, "block", 5)) {
          // SUBSYSTEM=block (10+5)
          evt_block_subsystem = true;
        } else {
          // SUBSYSTEM != block
          break;
        }
        pbuf = const_cast<uint8_t*>(tok) + tok_len;
        remain_len = end_ptr - pbuf;
      } else {
        // No token after SUBSYSTEM=
        break;
      }
    }

    pnext = static_cast<uint8_t*>(memchr(pbuf, '\0', remain_len));

    if (pnext) {
      pbuf = pnext + 1;
      remain_len = end_ptr - pbuf;
    } else {
      // No '\0' in remaining data
      break;
    }
  }

  if (evt_block_subsystem &&
      (evt_action != StorageManager::ExtStorEvent::EVT_NONE) &&
      (UINT_MAX != evt_major_num) && (UINT_MAX != evt_minor_num)) {
    info_log("Block device:%u,%u %s event message.",
             evt_major_num, evt_minor_num,
             (evt_action == StorageManager::ExtStorEvent::EVT_RMD)
                 ? "remove" : "add");

    notify_ext_stor_evt(evt_action, evt_major_num, evt_minor_num);
  }
}
