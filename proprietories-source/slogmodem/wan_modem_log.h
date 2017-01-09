/*
 *  wan_modem_log.h - The WAN MODEM log and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-13 Zhang Ziyi
 *  Initial version.
 */
#ifndef WAN_MODEM_LOG_H_
#define WAN_MODEM_LOG_H_

#include "log_pipe_hdl.h"
#include "stor_mgr.h"
#include "wcdma_iq_mgr.h"
#include "wan_modem_time_sync_hdl.h"

class LogFile;

class WanModemLogHandler : public LogPipeHandler {
 public:
  WanModemLogHandler(LogController* ctrl, Multiplexer* multi,
                     const LogConfig::ConfigEntry* conf,
                     StorageManager& stor_mgr, const char* dump_path);
  ~WanModemLogHandler();

  // Override base class function
  void process_alive();

  /*  pause - pause logging and I/Q before log clearing.
   *
   *  This function assume m_enable is true.
   *
   *  Return Value:
   *    0
   */
  int pause() override;
   /*  resume - resume logging and I/Q after log clearing.
   *
   *  This function assume m_enable is true.
   *
   *  Return Value:
   *    0
   */
  int resume() override;
  /*  enable_wcdma_iq - enable WCDMA I/Q saving.
   *
   *  Return LCR_SUCCESS on success, LCR_xxx on error.
   */
  int enable_wcdma_iq();
  /*  disable_wcdma_iq - disable WCDMA I/Q saving.
   *
   *  Return LCR_SUCCESS on success, LCR_xxx on error.
   */
  int disable_wcdma_iq();

 private:
  // number of bytes predicated for modem version when not available
  static const size_t PRE_MODEM_VERSION_LEN = 200;

  enum ModemVerState {
    MVS_NOT_BEGIN,
    MVS_QUERY,
    MVS_GOT,
    MVS_FAILED
  };

  enum ModemVerStoreState {
    MVSS_NOT_SAVED,
    MVSS_SAVED
  };

  const char* m_dump_path;
  // WCDMA I/Q manager
  WcdmaIqManager m_wcdma_iq_mgr;
  // Save WCDMA I/Q
  bool m_save_wcdma_iq;
  WanModemTimeSync* m_time_sync_notifier;
  bool m_timestamp_miss;
  ModemVerState m_ver_state;
  ModemVerStoreState m_ver_stor_state;
  LogString m_modem_ver;
  size_t m_reserved_version_len;

  // Override base class function
  int create_storage() override;

  // Override base class function
  void stop_transaction(CpWorkState state) override;

  void on_new_log_file(LogFile* f) override;

  /*  start_dump - override the virtual function to implement WAN
   *               MODEM memory dump.
   *  Return Value:
   *    Return 0 if the dump transaction is started successfully,
   *    return -1 if the dump transaction can not be started,
   *    return 1 if the dump transaction is finished.
   */
  int start_dump(const struct tm& lt);

  /*  start - override the virtual function for starting WAN log
   *
   *  Return Value:
   *    Return 0 if WAN log start successfully,
   */
  int start();

  /*
   *    stop - Stop logging.
   *
   *    This function put the LogPipeHandler in an adminitrative
   *    disable state and close the log device.
   *
   *    Return Value:
   *      0
   */
  int stop();

  int start_ver_query();
  void correct_ver_info();
  /* frame_noversion_and_reserve - frame the unavailable information
   *                               and reserve spaces for the the modem
   *                               version to come.
   * @length - reserved length to be written to log file
   *
   * Return pointer to the framed info.
   */
  uint8_t* frame_noversion_and_reserve(size_t& length);
  /* frame_version_info - frame the version info
   * @payload - payload of version info
   * @pl_len - payload length
   * @len_reserved - spaces reserved for the resulted frame if not equal 0
   * @frame_len - length of resulted frame
   *
   * Return pointer to the framed info.
   */
  uint8_t* frame_version_info(const uint8_t* payload, size_t pl_len,
                              size_t len_reserved, size_t& frame_len);
  /*  save_log_parsing_lib - save the MODEM log parsing lib to current
   *                         media (under the modem_log directory).
   *  Return Value:
   *    Return 0 on success, -1 otherwise.
   *
   */
  int save_log_parsing_lib(MediaStorage& ms);

  int save_log_lib(const LogString& lib_name, const LogString& sha1_name,
                   int modem_part, size_t offset, size_t len,
                   const uint8_t* sha1);

  bool save_timestamp(LogFile* f);
  /*
   *    save_version - Save version.
   */
  bool save_version();

  void process_assert(bool save_md = true);

  /*  get_image_path - get the device file path of the MODEM image partition.
   *  @ct: the WAN MODEM type
   *  @img_path: the buffer to hold the device file path.
   *  @len: length of the buffer.
   *
   *  Return Value:
   *    Return 0 on success, -1 otherwise.
   *
   */
  static int get_image_path(CpType ct, char* img_path, size_t len);

  static int open_modem_img();

  static int get_modem_img(int img_fd, uint8_t* new_sha1, size_t& lib_offset,
                           size_t& lib_len);

  static void transaction_modem_ver_notify(void* client,
                                           DataConsumer::LogProcResult res);

  /*  diag_transaction_notify - Diagnosis port transaction result
   *                            notification function.
   *  @client: client parameter. It's the WanModemLogHandler* pointer.
   *  @result: the transaction result.
   *
   *  This function is called by current DataConsumer object.
   */
  static void diag_transaction_notify(void* client,
                                      DataConsumer::LogProcResult res);

  /* calculate_modem_timestamp - calculate wan modem timestamp with the
   *                             system count and uptime received from refnotify.
   * @ts: time sync info consist of cp boot system count and uptime
   * @modem_timestamp: value result
   */
  static void calculate_modem_timestamp(const time_sync& ts,
                                        modem_timestamp& mp);

  /*  notify_media_change - callback function to trigger the copy of
   *                        modem log library from internal storage to
   *                        external storage.
   *  @client: the log pipe handler client(wan, wcn or gnss).
   *  @type: internal storage or external storage.
   */
  static void notify_media_change(void* client, StorageManager::MediaType type);

  static void notify_modem_time_update(void* client, const time_sync& ts);

  static int check_cur_parsing_lib(const LogString& log_name,
                                   const LogString& sha1_name,
                                   uint8_t* old_sha1, size_t& old_len);

  static void fill_smp_header(uint8_t* buf, size_t len,
                              unsigned lcn, unsigned type);

  static void fill_diag_header(uint8_t* buf, uint32_t sn, unsigned len,
                               unsigned cmd, unsigned subcmd);
};

#endif  // !WAN_MODEM_LOG_H_
