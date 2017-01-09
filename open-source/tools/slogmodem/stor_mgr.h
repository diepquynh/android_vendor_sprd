/*
 *  stor_mgr.h - storage manager.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */
#ifndef _STOR_MGR_H_
#define _STOR_MGR_H_

#include "media_stor.h"

class ExtStorageMonitor;
class LogController;
class Multiplexer;
class LogPipeHandler;
class CpStorage;
class FileWatcher;

class StorageManager {
 public:
  enum MediaType { MT_NONE, MT_INTERNAL, MT_SD_CARD };
  enum ExtStorEvent { EVT_NONE, EVT_ADD, EVT_RMD };

  typedef void (*stor_event_callback_t)(void* client_ptr, MediaType type);
  typedef void (*ext_stor_umount_callback_t)(void* client_ptr);
  StorageManager();
  ~StorageManager();

  /*  init - initialize the storage manager.
   *  @sd_top_dir: top directory of SD card.
   *  @ctrl: LogController object
   *  @multiplexer: Multiplexer object
   *
   *  Return 0 on success, -1 on failure.
   */
  int init(LogString extern_stor_pos, LogController* ctrl,
           Multiplexer* multiplexer);

  void set_overwrite(bool ow);
  bool overwrite() const { return m_overwrite; }
  void set_file_size_limit(size_t sz);
  void set_data_part_limit(uint64_t sz);
  void set_sd_limit(uint64_t sz);

  /*  create_storage - create CP storage handle.
   *  @cp_log: the LogPipeHandler object reference.
   *
   *  Return the CpStorage pointer on success, NULL on failure.
   */
  CpStorage* create_storage(LogPipeHandler& cp_log);
  void delete_storage(CpStorage* stor);

  /* get_media_stor - get media storage (internal or external) according
   *                  to media type.
   * @MediaType: MT_INTERNAL or MT_SD_CARD
   * Return the media storage
   */
  MediaStorage* get_media_stor(MediaType t);

  MediaStorage* get_media_stor() { return get_media_stor(m_cur_type); }

  MediaType get_media_type() const { return m_cur_type; }
  /*  check_media_change - check the media change and create the
   *                       CP set directory.
   *
   *  Return true if the media changed, false otherwise.
   */
  bool check_media_change();

  /*  clear - delete all logs.
   */
  void clear();

  /*  proc_working_dir_removed - process the removal of working directory.
   */
  void proc_working_dir_removed(MediaStorage* ms);

  const MediaStorage& get_internal_store() const { return m_data_part; }

  const MediaStorage& get_ext_store() const { return m_sd_card; }

  /*  subscribe_media_change_evt - subscribe the service of media change
   *                               from internal storage to external storage.
   *
   *  @client_ptr - log pipe handler client(wan, wcn or gnss)
   *  @cb - callback how the client handle the event of media change
   */
  void subscribe_media_change_evt(void* client_ptr, stor_event_callback_t cb);

  /*  unsubscribe_media_change_evt - unsubscribe the service of media change
   *                                 from internal storage to external storage.
   *
   *  @client_ptr - log pipe handler client(wan, wcn or gnss)
   */
  void unsubscribe_media_change_evt(void* client_ptr);

  /*  subscribe_ext_stor_umount_evt - subscribe the service of external storage
   *                               umount event.
   *
   *  @client_ptr - non log file clients:
   *                sleep log/ringbuffer/minidump/memory dump
   *  @cb - callback how the client handle the event of umount
   */
  void subscribe_ext_stor_umount_evt(void* client_ptr,
                                     ext_stor_umount_callback_t cb);

  /*  unsubscribe_ext_stor_umount_evt - unsubscribe the service of external
   *                                    storage umount.
   *  @client_ptr - non log file clients:
   *                sleep log/ringbuffer/minidump/memory dump
   */
  void unsubscribe_ext_stor_umount_evt(void* client_ptr);

  /*  notify_media_change - storage manager notify the subscribed handler
   *                        client when storage media changes.
   */
  void notify_media_change();

  /* notify_ext_stor_umount - storage manager notify the subscribed handler
   *                        client when external storage is umounted.
   */
  void notify_ext_stor_umount();

  /*  set_modem_log_in_slog - when save_in_slog_dir is enabled in
   * slog_modem.conf,
   *                          in case of save log, the modem log will be saved
   * like:
   *                                      ./storage
   *                                          - sdcard
   *                                              - slog
   *                                                 - yy-mm-dd-hh-mm-ss
   *                                                 - modem
   */
  void set_modem_log_in_slog() { m_save_in_slog = true; }
  /*  is_modem_log_in_slog - return if modem log will be saved in slog firectory
   *                         or not.
   */
  bool is_modem_log_in_slog() const { return m_save_in_slog; }

 private:
  struct EventClient {
    void* client_ptr;
    union {
      stor_event_callback_t cb;
      ext_stor_umount_callback_t cb_umnt;
    };
  };

  // Current storage media
  MediaType m_cur_type;
  MediaStorage m_data_part;
  MediaStorage m_sd_card;
  // Overwrite?
  bool m_overwrite;
  // CP storage handle
  LogList<CpStorage*> m_cp_handles;
  // File watcher
  FileWatcher* m_file_watcher;
  // Event clients
  LogList<EventClient*> m_event_clients;
  // Umount event clients
  LogList<EventClient*> m_umt_event_clients;
  // Save modem log in slog directory or not
  bool m_save_in_slog;
  // External storage mount point
  LogString m_mount_point;
  // External storage add/remove event
  ExtStorEvent m_ext_stor_event;
  // External storage uevent monitor
  ExtStorageMonitor* m_ext_stor_monitor;
  // Major number of external storage device
  unsigned m_major_num;
  // Minor number of external storage device
  unsigned m_minor_num;

  void stop_all_cps();
  bool get_sd_state();
  bool check_vfat_mount(LogString& sd_root);
  // Initialisation of external storage
  void init_ext_stor();
  // get major and minor number from mount file for external storage device
  int get_major_minor_in_mount(const uint8_t* mount_info, size_t len);

  static void notify_ext_storage_change(void* client, ExtStorEvent event,
                                        unsigned major_num, unsigned minor_num);
};

#endif  // !_STOR_MGR_H_
