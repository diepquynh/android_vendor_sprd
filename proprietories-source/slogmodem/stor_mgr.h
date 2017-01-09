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

#include "ext_storage_monitor.h"
#include "media_stor.h"

class CpStorage;
class FileWatcher;
class LogController;
class LogPipeHandler;
class Multiplexer;

class StorageManager {
 public:
  enum MediaType { MT_NONE, MT_INTERNAL, MT_SD_CARD };

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
  int init(const LogString& extern_stor_pos, LogController* ctrl,
           Multiplexer* multiplexer);

  void set_use_ext_stor_fuse(bool use_fuse = true) {
    m_use_ext_stor_fuse = use_fuse;
  }
  void set_overwrite(bool ow);
  bool overwrite() const { return m_overwrite; }
  void set_file_size_limit(size_t sz);
  void set_data_part_limit(uint64_t sz);
  void set_sd_limit(uint64_t sz);

  const LogString& ext_stor_mount_point() const {
    return m_mount_point;
  }

  /*  create_storage - create CP storage handle.
   *  @cp_log: the LogPipeHandler object reference.
   *  @max_buf: maximum size of one buffer
   *  @max_num: maximum number of buffers
   *  @cmt_size: threshold of data commit size
   *
   *  Return the CpStorage pointer on success, NULL on failure.
   */
  CpStorage* create_storage(LogPipeHandler& cp_log,
                            size_t max_buf,
                            size_t max_num,
                            size_t cmt_size);
  void delete_storage(CpStorage* stor);

  /* get_media_stor - get media storage (internal or external) according
   *                  to media type.
   * @MediaType: MT_INTERNAL or MT_SD_CARD
   * Return the media storage
   */
  MediaStorage* get_media_stor(MediaType t);

  MediaStorage* get_media_stor() { return get_media_stor(m_cur_type); }
  /*  get_media_stor - get MediaStorage according to its mount point.
   *  @mnt_point: mount point of the storage.
   *
   */
  MediaStorage* get_media_stor(const LogString& mnt_point);

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
   *                          client when external storage is umounted.
   */
  void notify_ext_stor_umount();

  /*  set_cp_log_in_ap_log - if cp log save in ap log directory:
   *                                      ./storage(/data)
   *                                          - sdcard
   *                                              - ylog(slog)
   *                                                 - yy-mm-dd-hh-mm-ss
   *                                                 - modem
   */
  void set_cp_log_in_ap_log_dir(const char* ap_log_dir) {
    m_save_in_ap_log = true;
    m_set_in_ap_log_dir = ap_log_dir;
  }
  /*  is_modem_log_in_ap_log - return if cp log will be saved in ap log
   *                           directory or not.
   */
  bool is_modem_log_in_ap_log() const { return m_save_in_ap_log; }

  /*  process_unmount - process the administrative unmount event (from vold).
   *  @ms: the media that is to be unmounted.
   *
   */
  void process_unmount(MediaStorage* ms);

 private:
  enum ExtStorDetectMethod { SDM_AUTO, SDM_MANUAL };
  enum MediaMountState {
    MMS_NOT_MOUNTED,        // Not mounted
    MMS_MEDIA_INSERTED,     // Got media insert event
    MMS_MOUNTED,            // File system mounted and accessible
    MMS_MEDIA_REMOVED,      // Got media removal event
    MMS_START_UNMOUNT,      // File system unmount notification from storage subsystem
    MMS_FS_MOUNTED          // File system mount notification from storage subsystem
  };

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
  // ap log directory
  const char* m_set_in_ap_log_dir;
  // if cp log save in ap log directory, true
  bool m_save_in_ap_log;
  // External storage mount point detection method
  ExtStorDetectMethod m_ext_stor_detect_method;
  // Use external storage on FUSE? (Only valid for auto detection)
  bool m_use_ext_stor_fuse;
  // Media mount state
  MediaMountState m_ext_stor_mount_state;
  // External storage mount point
  LogString m_mount_point;
  // External storage uevent monitor
  ExtStorageMonitor* m_ext_stor_monitor;
  // Major number of external storage device
  unsigned m_major_num;
  // Minor number of external storage device
  unsigned m_minor_num;

  void stop_all_cps();
  /*  get_sd_state - check whether the external storage is mounted.
   *
   *  Return Value:
   *    Return true if mounted, false otherwise.
   */
  bool get_sd_state();
  bool check_vfat_mount(LogString& sd_root);
  /*  try_use_sd_card - try to init and create CP set directory on SD.
   *
   *  If successful, return 0; otherwise return -1.
   */
  int try_use_sd_card();
  // Initialisation of external storage
  void init_ext_stor();
  // get major and minor number from mount file for external storage device
  int get_major_minor_in_mount(const uint8_t* mount_info, size_t len);
  /*  clear_ext_sd - clear the CP log on external SD.
   *
   *  This function is called when current media is not external SD.
   */
  void clear_ext_sd();

  static void notify_ext_storage_event(void* client,
                                       ExtStorageMonitor::StorageEvent event,
                                       unsigned major_num,
                                       unsigned minor_num);
};

#endif  // !_STOR_MGR_H_
