/*
 *  ext_storage_monitor.h - kernel will send a netlink socket to notify the sdcard's status.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-03-21 Yan Zhihang
 *  Initial version.
 */
#ifndef EXT_STORAGE_MONITOR_H_
#define EXT_STORAGE_MONITOR_H_

#include "multiplexer.h"
#include "stor_mgr.h"

class LogController;
class Multiplexer;

class ExtStorageMonitor : public FdHandler {
 public:
  typedef void (*ext_storage_evt_callback_t)(void* client_ptr,
      StorageManager::ExtStorEvent ext_stor_evt,
      unsigned major_num, unsigned minor_num);

  ExtStorageMonitor(StorageManager* stor_mgr, LogController* ctrl,
                    Multiplexer* multi);
  ~ExtStorageMonitor();

  int init();
  void process(int events);

  /* subscribe_ext_storage_evt - Subscribe external storage sdcard change event.
   *
   * @client_ptr - client: storage manager.
   * @cb - client callback of this event notification.
   *       cb should not be null.
   */
  void subscribe_ext_storage_evt(void* client_ptr,
                                 ext_storage_evt_callback_t cb);
  void unsubscribe_ext_storage_evt(void* client_ptr);

 private:
  struct EventClient {
    void* client_ptr;
    ext_storage_evt_callback_t cb;
  };

  StorageManager* m_stor_mgr;
  LogList<EventClient*> m_event_clients;

  /* notify_ext_stor_evt - Notification of external storage umount event.
   *
   * @ ext_stor_evt - true, if external storage is umounted.
   * @ major_num    - device major number
   * @ minor_num    - device minor number
   */
  void notify_ext_stor_evt(StorageManager::ExtStorEvent ext_stor_evt,
                           unsigned major_num, unsigned minor_num);
};
#endif
