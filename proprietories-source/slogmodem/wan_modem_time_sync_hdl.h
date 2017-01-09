/*
 * wan_modem_time_sync_hdl.h - The receiver of wan modem time synchronisation
 *                             and handler of updating wan modem log file
 *                             time information.
 *
 * History:
 * 2016-04-11 Zhihang YAN
 * intial version.
 */

#ifndef WAN_MODEM_TIME_SYNC_HDL_H_
#define WAN_MODEM_TIME_SYNC_HDL_H_

#include "data_proc_hdl.h"
#include "timer_mgr.h"

class LogController;
class Multiplexer;

class WanModemTimeSync : public DataProcessHandler {
 public:
  typedef void (*cp_time_update_callback_t)(void* client, const time_sync& ts);

  WanModemTimeSync(LogController* ctrl, Multiplexer* multi);
  ~WanModemTimeSync();

  int init();

  bool active_get_time_sync_info(time_sync& ts);

  void clear_time_sync_info() {
    if (0 != m_ts.uptime) {
      memset(&m_ts, 0, sizeof m_ts);
    }
  }

  /* subscribe_cp_time_update_evt - Subscribe modem time update event.
   * @client_ptr - client
   * @cb - client callback of this event notification.
   */
  void subscribe_cp_time_update_evt(void* client_ptr,
                                    cp_time_update_callback_t cb);

  void unsubscribe_cp_time_update_evt(void* client_ptr);

 protected:
  static const size_t WAN_MODEM_TIME_SYNC_BUF_SIZE = 64;

 private:
  struct EventClient {
    void* client_ptr;
    cp_time_update_callback_t cb;
  };

  LogList<EventClient*> m_event_clients;
  time_sync m_ts;
  TimerManager::Timer* m_timer;

  int process_data();
  void process_conn_closed();
  void process_conn_error(int err);

  /* notify_cp_time_update - Notification of new cp time update.
   * @modem_time - modem sys count infomation received from refnotify.
   */
  void notify_cp_time_update(const time_sync& ts) const;

  static void connect_server(void* param);
};

#endif // !WAN_MODEM_TIME_SYNC_HDL_H_
