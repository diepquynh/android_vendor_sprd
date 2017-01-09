/*
 *  log_config.h - The configuration class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef LOG_CONFIG_H_
#define LOG_CONFIG_H_

#include <cstdint>

#include "def_config.h"
#include "cp_log_cmn.h"

class LogConfig {
 public:
  struct ConfigEntry {
    LogString modem_name;
    CpType type;
    bool enable;
    size_t size;
    int level;

    ConfigEntry(const char* modem, size_t len, CpType t, bool en, size_t sz,
                int lvl)
        : modem_name(modem, len), type{t}, enable{en}, size{sz}, level{lvl} {}
  };

  typedef LogList<ConfigEntry*> ConfigList;
  typedef LogList<ConfigEntry*>::iterator ConfigIter;
  typedef LogList<ConfigEntry*>::const_iterator ConstConfigIter;

  explicit LogConfig(const char* tmp_config);
  ~LogConfig();

  const LogString& get_mount_point() const { return m_mount_point; }

  /*
   * read_config - read config from specifed file.
   *
   * Return Value:
   *   If the function succeeds, return 0; otherwise return -1.
   */
  int read_config(int argc, char** argv);

  bool dirty() const { return m_dirty; }

  /*
   * save - save config to the config file.
   *
   * Return Value:
   *   If the function succeeds, return 0; otherwise return -1.
   */
  int save();

  const ConfigList& get_conf() const { return m_config; }

  bool md_enabled() const { return m_enable_md; }

  bool md_save_to_int() const {
    return m_md_save_to_int;
  }

  void set_md_int_stor(bool md_int_stor) {
    if (m_md_save_to_int != md_int_stor) {
      m_md_save_to_int = md_int_stor;
      m_dirty = true;
    }
  }

  bool store_in_slog_dir() const { return m_save_in_slog_dir; }

  void enable_log(CpType cp, bool en = true);
  void enable_md(bool en = true);

  size_t max_log_file() const { return m_log_file_size; }
  void set_log_file_size(size_t sz) {
    if (m_log_file_size != sz) {
      m_log_file_size = sz;
      m_dirty = true;
    }
  }
  size_t max_data_part_size() const { return m_data_size; }
  void set_data_part_size(size_t sz) {
    if (sz != m_data_size) {
      m_data_size = sz;
      m_dirty = true;
    }
  }
  size_t max_sd_size() const { return m_sd_size; }
  void set_sd_size(size_t sz) {
    if (m_sd_size != sz) {
      m_sd_size = sz;
      m_dirty = true;
    }
  }
  bool overwrite_old_log() const { return m_overwrite_old_log; }
  void set_overwrite(bool en = true) {
    if (m_overwrite_old_log != en) {
      m_overwrite_old_log = en;
      m_dirty = true;
    }
  }

  void enable_iq(CpType cpt, IqType it);
  /*  disable_iq - disable I/Q saving.
   *  @cpt: CP type
   *  @it: I/Q type. IT_ALL for all I/Q types.
   *
   */
  void disable_iq(CpType cpt, IqType it);

  bool wcdma_iq_enabled(CpType cpt) const;

  static CpType get_wan_modem_type() { return s_wan_modem_type; }

  static IqType get_iq_type(const uint8_t* iq, size_t len);
  static int parse_enable_disable(const uint8_t* buf, bool& en);
  static int parse_number(const uint8_t* buf, size_t& num);

 private:
  struct IqConfig {
    CpType cp_type;
    bool gsm_iq;
    bool wcdma_iq;
  };

  bool m_dirty;
  // External storage's mount point
  LogString m_mount_point;
  LogString m_config_file;
  ConfigList m_config;
  bool m_enable_md;
  // true, if minidump storage position is internal
  bool m_md_save_to_int;
  bool m_save_in_slog_dir;
  // Maximum size of a single log file in MB
  size_t m_log_file_size;
  // Maximum log size on /data partition in MB
  size_t m_data_size;
  // Maximum log size on SD card in MB
  size_t m_sd_size;
  // Overwrite on log size full?
  bool m_overwrite_old_log;
  // I/Q config
  LogList<IqConfig*> m_iq_config;

  int parse_cmd_line(int argc, char** argv);

  int parse_line(const uint8_t* buf);
  int parse_stream_line(const uint8_t* buf);
  int parse_iq_line(const uint8_t* buf);
  int parse_minidump_line(const uint8_t* buf, bool& en,
                          bool& save_to_int);

  static CpType s_wan_modem_type;

  static void propget_wan_modem_type();
  static CpType get_modem_type(const uint8_t* name, size_t len);
  static const char* cp_type_to_name(CpType t);
  static ConfigList::iterator find(ConfigList& clist, CpType t);
  static LogList<IqConfig*>::iterator find_iq(LogList<IqConfig*>& iq_list,
                                              CpType t);
  static LogList<IqConfig*>::const_iterator find_iq(
      const LogList<IqConfig*>& iq_list, CpType t);
};

int get_dev_paths(CpType t, bool& same, LogString& log_path,
                  LogString& diag_path);

#endif  // !LOG_CONFIG_H_
