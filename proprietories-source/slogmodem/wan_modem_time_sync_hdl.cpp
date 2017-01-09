/*
 * wan_modem_time_sync_hdl.cpp - The receiver of wan modem time
 *                               synchronisation and handler of updating
 *                               wan modem log file time information.
 *
 * History:
 * 2016-04-11 Zhihang YAN
 * Initial version.
 */

#ifdef HOST_TEST_
  #include "sock_test.h"
#else
  #include <cutils/sockets.h>
#endif
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cp_log_cmn.h"
#include "def_config.h"
#include "multiplexer.h"
#include "wan_modem_time_sync_hdl.h"

WanModemTimeSync::WanModemTimeSync(LogController* ctrl,
                                   Multiplexer* multi)
    : DataProcessHandler(-1, ctrl, multi, WAN_MODEM_TIME_SYNC_BUF_SIZE),
      m_ts{0, 0},
      m_timer{0} {}

WanModemTimeSync::~WanModemTimeSync() {
  clear_ptr_container(m_event_clients);
  if (m_timer) {
    multiplexer()->timer_mgr().del_timer(m_timer);
    m_timer = 0;
  }
}

int WanModemTimeSync::init() {
  m_fd = socket_local_client(CP_TIME_SYNC_SERVER_SOCKET,
                             ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

  if (-1 == m_fd) {
    m_timer = multiplexer()->timer_mgr().create_timer(500,
                                                      connect_server, this);
  } else {
    long flags = fcntl(m_fd, F_GETFL);
    flags |= O_NONBLOCK;
    int err = fcntl(m_fd, F_SETFL, flags);
    if (-1 == err) {
      close();
      err_log("set time sync socket O_NONBLOCK fail.");
    } else {
      multiplexer()->register_fd(this, POLLIN);
    }
  }

  return m_fd;
}

bool WanModemTimeSync::active_get_time_sync_info(time_sync& ts) {
  bool ret = false;

  if (0 != m_ts.uptime) {
    ts = m_ts;
    ret = true;
  }

  return ret;
}

void WanModemTimeSync::connect_server(void* param) {
  WanModemTimeSync* p = static_cast<WanModemTimeSync*>(param);

  p->m_timer = 0;
  p->init();
}

void WanModemTimeSync::subscribe_cp_time_update_evt(void* client_ptr,
    cp_time_update_callback_t cb) {
  EventClient* c = new EventClient;
  c->client_ptr = client_ptr;
  c->cb = cb;
  m_event_clients.push_back(c);
}

void WanModemTimeSync::unsubscribe_cp_time_update_evt(void* client_ptr) {
  for (auto it = m_event_clients.begin(); it != m_event_clients.end(); ++it) {
    EventClient* client = *it;
    if (client_ptr == client->client_ptr) {
      m_event_clients.erase(it);
      delete client;
      break;
    }
  }
}

void WanModemTimeSync::notify_cp_time_update(const time_sync& ts) const {
  for (auto it = m_event_clients.begin(); it != m_event_clients.end(); ++it) {
    EventClient* client = *it;
    client->cb(client->client_ptr, ts);
  }
}

int WanModemTimeSync::process_data() {
  if (2 * sizeof(uint32_t) <= m_buffer.data_len) {
    memcpy(&m_ts, m_buffer.buffer, 2 * sizeof(uint32_t));
    info_log("MODEM time info: sys_cnt = %u, up time = %u",
             static_cast<unsigned>(m_ts.sys_cnt),
             static_cast<unsigned>(m_ts.uptime));
    m_buffer.data_len = 0;
    notify_cp_time_update(m_ts);
  } else {
    err_log("modem time synchronisation information length %d is not correct.",
            static_cast<int>(m_buffer.data_len));
  }

  return 0;
}

void WanModemTimeSync::process_conn_closed() {
  multiplexer()->unregister_fd(this);
  close();
  init();
}

void WanModemTimeSync::process_conn_error(int /*err*/) {
  process_conn_closed();
}
