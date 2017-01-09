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

class WanModemLogHandler : public LogPipeHandler {
 public:
  WanModemLogHandler(LogController* ctrl, Multiplexer* multi,
                     const LogConfig::ConfigEntry* conf,
                     StorageManager& stor_mgr, const char* dump_path);

 private:
  const char* m_dump_path;

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

  /*  diag_transaction_notify - Diagnosis port transaction result
   *                            notification function.
   *  @client: client parameter. It's the WanModemLogHandler* pointer.
   *  @result: the transaction result.
   *
   *  This function is called by current DataConsumer object.
   */
  static void diag_transaction_notify(void* client,
                                      DataConsumer::LogProcResult res);

  /*  notify_media_change - callback function to trigger the copy of
   *                        modem log library from internal storage to
   *                        external storage.
   *  @client: the log pipe handler client(wan, wcn or gnss).
   *  @type: internal storage or external storage.
   */
  static void notify_media_change(void* client, StorageManager::MediaType type);

  static int check_cur_parsing_lib(const LogString& log_name,
                                   const LogString& sha1_name,
                                   uint8_t* old_sha1, size_t& old_len);
};

#endif  // !WAN_MODEM_LOG_H_
