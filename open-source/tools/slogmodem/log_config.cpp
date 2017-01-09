/*
 *  log_config.cpp - The configuration class implementation.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HOST_TEST_
#include "prop_test.h"
#else
#include <cutils/properties.h>
#endif

#include "log_config.h"
#include "parse_utils.h"

CpType LogConfig::s_wan_modem_type{CT_UNKNOWN};

LogConfig::LogConfig(const char* tmp_config)
    : m_dirty{false},
      m_config_file(tmp_config),
      m_enable_md{false},
      m_md_save_to_int {false},
      m_save_in_slog_dir{false},
      m_log_file_size{5},
      m_data_size{25},
      m_sd_size{250},
      m_overwrite_old_log{true} {}

LogConfig::~LogConfig() {
  clear_ptr_container(m_iq_config);
  clear_ptr_container(m_config);
}

int LogConfig::parse_cmd_line(int argc, char** argv) {
  int i = 1;
  int ret = 0;

  while (i < argc) {
    if (!strcmp(argv[i], "-s")) {
      m_save_in_slog_dir = true;
    } else if (!strcmp(argv[i], "-m")) {
      // external storage mout point defined from commnd line
      ++i;
      if (i < argc) {
        m_mount_point = argv[i];
        info_log("external log path is appointed to %s",
                 ls2cstring(m_mount_point));
      } else {
        err_log("missing parameter of -d opetion.");
        ret = -1;
        break;
      }
    } else {
      // Unknown options
      err_log("Unknown option: %s", argv[i]);
      ret = -1;
      break;
    }
    ++i;
  }

  return ret;
}

int LogConfig::read_config(int argc, char** argv) {
  if (parse_cmd_line(argc, argv)) {
    return -1;
  }

  uint8_t* buf;
  FILE* pf;
  int line_num = 0;

  buf = new uint8_t[1024];

  const char* str_cfile = ls2cstring(m_config_file);
  pf = fopen(str_cfile, "rt");
  if (!pf) {
    const char* log_conf = CP_LOG_CONFIG_FILE;
    copy_file(log_conf, str_cfile);

    pf = fopen(str_cfile, "rt");
    if (!pf) {
      err_log("can not open %s", str_cfile);
      goto delBuf;
    }
  }

  // Read config file
  while (true) {
    char* p = fgets(reinterpret_cast<char*>(buf), 1024, pf);
    if (!p) {
      break;
    }
    ++line_num;
    int err = parse_line(buf);
    if (-1 == err) {
      err_log("invalid line: %d\n", line_num);
    }
  }

  propget_wan_modem_type();

  fclose(pf);
  delete[] buf;

  return 0;

delBuf:
  delete[] buf;
  return -1;
}

int LogConfig::parse_minidump_line(const uint8_t* buf, bool& en,
                                   bool& save_to_int) {
  const uint8_t* t;
  const uint8_t* locate;
  size_t tlen;

  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }

  if ((6 == tlen) && !memcmp(t, "enable", 6)) {
    en = true;
  } else {
    en = false;
  }

  buf = t + tlen;
  locate = get_token(buf, tlen);

  if (!locate) {
    return -1;
  }

  if((8 == tlen) && !memcmp(locate, "internal", 8)) {
    save_to_int = true;
  } else {
    save_to_int = false;
  }

  return 0;
}

int LogConfig::parse_enable_disable(const uint8_t* buf, bool& en) {
  const uint8_t* t;
  size_t tlen;

  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }

  if (6 == tlen && !memcmp(t, "enable", 6)) {
    en = true;
  } else {
    en = false;
  }

  return 0;
}

int LogConfig::parse_number(const uint8_t* buf, size_t& num) {
  unsigned long n;
  char* endp;

  n = strtoul(reinterpret_cast<const char*>(buf), &endp, 0);
  if ((ULONG_MAX == n && ERANGE == errno) || !isspace(*endp)) {
    return -1;
  }

  num = static_cast<size_t>(n);
  return 0;
}

int LogConfig::parse_stream_line(const uint8_t* buf) {
  const uint8_t* t;
  size_t tlen;
  const uint8_t* pn;
  size_t nlen;

  // Get the modem name
  pn = get_token(buf, nlen);
  if (!pn) {
    return -1;
  }
  CpType cp_type = get_modem_type(pn, nlen);
  if (CT_UNKNOWN == cp_type) {
    // Ignore unknown CP
    return 0;
  }

  // Get enable state
  bool is_enable;

  buf = pn + nlen;
  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }
  if (2 == tlen && !memcmp(t, "on", 2)) {
    is_enable = true;
  } else {
    is_enable = false;
  }

  // Size
  buf = t + tlen;
  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }
  char* endp;
  unsigned long sz = strtoul(reinterpret_cast<const char*>(t), &endp, 0);
  if ((ULONG_MAX == sz && ERANGE == errno) ||
      (' ' != *endp && '\t' != *endp && '\r' != *endp && '\n' != *endp &&
       '\0' != *endp)) {
    return -1;
  }

  // Level
  buf = reinterpret_cast<const uint8_t*>(endp);
  t = get_token(buf, tlen);
  if (!t) {
    return -1;
  }
  unsigned long level = strtoul(reinterpret_cast<const char*>(t), &endp, 0);
  if ((ULONG_MAX == level && ERANGE == errno) ||
      (' ' != *endp && '\t' != *endp && '\r' != *endp && '\n' != *endp &&
       '\0' != *endp)) {
    return -1;
  }

  ConfigList::iterator it = find(m_config, cp_type);
  if (it != m_config.end()) {
    ConfigEntry* pe = *it;
    str_assign(pe->modem_name, reinterpret_cast<const char*>(pn), nlen);
    pe->enable = is_enable;
    pe->size = sz;
    pe->level = static_cast<int>(level);

    err_log("slogcp: duplicate CP %s in config file",
            ls2cstring(pe->modem_name));
  } else {
    ConfigEntry* pe = new ConfigEntry{reinterpret_cast<const char*>(pn),
                                      nlen,
                                      cp_type,
                                      is_enable,
                                      sz,
                                      static_cast<int>(level)};
    m_config.push_back(pe);
  }

  return 0;
}

int LogConfig::parse_iq_line(const uint8_t* buf) {
  size_t tlen;
  const uint8_t* tok;

  // Get the modem name
  tok = get_token(buf, tlen);
  if (!tok) {
    return -1;
  }
  CpType cp_type = get_modem_type(tok, tlen);
  if (CT_UNKNOWN == cp_type) {
    // Ignore unknown CP
    err_log("invalid I/Q CP type");
    return 0;
  }

  IqConfig* iq = new IqConfig;

  iq->cp_type = cp_type;
  iq->gsm_iq = false;
  iq->wcdma_iq = false;

  bool iq_got = false;
  bool iq_type_err = false;
  while (1) {
    buf = tok + tlen;
    tok = get_token(buf, tlen);
    if (!tok) {
      break;
    }

    IqType iqt = get_iq_type(tok, tlen);
    if (IT_UNKNOWN == iqt) {
      // Invalid I/Q type
      iq_type_err = true;
      break;
    }
    iq_got = true;
    if (IT_GSM == iqt) {
      iq->gsm_iq = true;
    } else {
      iq->wcdma_iq = true;
    }
  }

  if (iq_type_err) {
    delete iq;
    err_log("invalid I/Q type");
    return 0;
  }
  if (!iq_got) {
    // No I/Q type
    delete iq;
    err_log("no I/Q type");
    return 0;
  }

  m_iq_config.push_back(iq);

  return 0;
}

int LogConfig::parse_line(const uint8_t* buf) {
  // Search for the first token
  const uint8_t* t;
  size_t tlen;
  int err = -1;

  t = get_token(buf, tlen);
  if (!t || '#' == *t) {
    return 0;
  }

  // What line?
  buf += tlen;
  switch (tlen) {
    case 2:
      if (!memcmp(t, "iq", 2)) {
        err = parse_iq_line(buf);
      }
      break;
    case 6:
      if (!memcmp(t, "stream", 6)) {
        err = parse_stream_line(buf);
      }
      break;
    case 8:
      if (!memcmp(t, "minidump", 8)) {
        err = parse_minidump_line(buf, m_enable_md, m_md_save_to_int);
      }
      break;
    case 11:
      if (!memcmp(t, "sd_log_size", 11)) {
        err = parse_number(buf, m_sd_size);
      }
      break;
    case 13:
      if (!memcmp(t, "log_file_size", 13)) {
        err = parse_number(buf, m_log_file_size);
      } else if (!memcmp(t, "data_log_size", 13)) {
        err = parse_number(buf, m_data_size);
      } else if (!memcmp(t, "log_overwrite", 13)) {
        err = parse_enable_disable(buf, m_overwrite_old_log);
      }
      break;

    default:
      break;
  }

  return err;
}

CpType LogConfig::get_modem_type(const uint8_t* name, size_t len) {
  CpType type = CT_UNKNOWN;

  switch (len) {
    case 5:
      if (!memcmp(name, "agdsp", 5)) {
        type = CT_AGDSP;
      }
      break;
    case 6:
      if (!memcmp(name, "cp_wcn", 6)) {
        type = CT_WCN;
      }
      break;
    case 7:
      if (!memcmp(name, "cp_gnss", 7)) {
        type = CT_GNSS;
      }
      break;
    case 8:
      if (!memcmp(name, "cp_wcdma", 8)) {
        type = CT_WCDMA;
      } else if (!memcmp(name, "cp_3mode", 8)) {
        type = CT_3MODE;
      } else if (!memcmp(name, "cp_4mode", 8)) {
        type = CT_4MODE;
      } else if (!memcmp(name, "cp_5mode", 8)) {
        type = CT_5MODE;
      }
      break;
    case 11:
      if (!memcmp(name, "cp_td-scdma", 11)) {
        type = CT_TD;
      }
      break;
    default:
      break;
  }

  return type;
}

int LogConfig::save() {
  FILE* pf = fopen(ls2cstring(m_config_file), "w+t");

  if (!pf) {
    return -1;
  }

  // Mini dump
  if (m_md_save_to_int) {
    fprintf(pf, "minidump %s\t%s\n", m_enable_md ? "enable" : "disable", "internal");
  } else {
    fprintf(pf, "minidump %s\t%s\n", m_enable_md ? "enable" : "disable", "external");
  }

  // Max log file size
  fprintf(pf, "log_file_size %u\n", static_cast<unsigned>(m_log_file_size));
  // Max log size on /data partition
  fprintf(pf, "data_log_size %u\n", static_cast<unsigned>(m_data_size));
  // Max log size on SD card
  fprintf(pf, "sd_log_size %u\n", static_cast<unsigned>(m_sd_size));
  // Overwrite old log
  fprintf(pf, "log_overwrite %s\n", m_overwrite_old_log ? "enable" : "disable");

  // I/Q config
  LogList<IqConfig*>::iterator it;

  fprintf(pf, "\n");
  for (it = m_iq_config.begin(); it != m_iq_config.end(); ++it) {
    IqConfig* iq = (*it);
    fprintf(pf, "iq %s ", cp_type_to_name(iq->cp_type));
    if (iq->gsm_iq) {
      fprintf(pf, "GSM ");
    }
    if (iq->wcdma_iq) {
      fprintf(pf, "WCDMA");
    }
    fprintf(pf, "\n");
  }

  fprintf(pf, "\n");
  fprintf(pf, "#Type\tName\tState\tSize\tLevel\n");

  for (ConfigIter it = m_config.begin(); it != m_config.end(); ++it) {
    ConfigEntry* pe = *it;
    fprintf(pf, "stream\t%s\t%s\t%u\t%u\n", ls2cstring(pe->modem_name),
            pe->enable ? "on" : "off", static_cast<unsigned>(pe->size),
            pe->level);
  }

  fclose(pf);

  m_dirty = false;

  return 0;
}

void LogConfig::enable_log(CpType cp, bool en /*= true*/) {
  for (ConfigList::iterator it = m_config.begin(); it != m_config.end(); ++it) {
    ConfigEntry* p = *it;
    if (p->type == cp) {
      if (p->enable != en) {
        p->enable = en;
        m_dirty = true;
      }
      break;
    }
  }
}

void LogConfig::enable_md(bool en /*= true*/) {
  if (m_enable_md != en) {
    m_enable_md = en;
    m_dirty = true;
  }
}

void LogConfig::enable_iq(CpType cpt, IqType iqt) {
  LogList<IqConfig*>::iterator it = find_iq(m_iq_config, cpt);
  IqConfig* iq;

  if (m_iq_config.end() == it) {
    iq = new IqConfig;
    iq->cp_type = cpt;
    iq->gsm_iq = false;
    iq->wcdma_iq = false;
    m_iq_config.push_back(iq);
  } else {
    iq = *it;
  }
  if (IT_GSM == iqt && !iq->gsm_iq) {
    iq->gsm_iq = true;
    m_dirty = true;
  } else if (IT_WCDMA == iqt && !iq->wcdma_iq) {
    iq->wcdma_iq = true;
    m_dirty = true;
  }
}

void LogConfig::disable_iq(CpType cpt, IqType iqt) {
  LogList<IqConfig*>::iterator it = find_iq(m_iq_config, cpt);
  IqConfig* iq;

  if (m_iq_config.end() == it) {
    return;
  }

  iq = *it;
  if (IT_ALL == iqt) {
    m_iq_config.erase(it);
    delete iq;
    m_dirty = true;
  } else if (IT_GSM == iqt && iq->gsm_iq) {
    iq->gsm_iq = false;
    m_dirty = true;
  } else if (IT_WCDMA == iqt && iq->wcdma_iq) {
    iq->wcdma_iq = false;
    m_dirty = true;
  }
}

bool LogConfig::wcdma_iq_enabled(CpType cpt) const {
  LogList<IqConfig*>::const_iterator it = find_iq(m_iq_config, cpt);
  bool ret = false;

  if (m_iq_config.end() != it && (*it)->wcdma_iq) {
    ret = true;
  }

  return ret;
}

LogConfig::ConfigList::iterator LogConfig::find(ConfigList& clist, CpType t) {
  ConfigList::iterator it;

  for (it = clist.begin(); it != clist.end(); ++it) {
    ConfigEntry* p = *it;
    if (p->type == t) {
      break;
    }
  }

  return it;
}

int get_dev_paths(CpType t, bool& same, LogString& log_path,
                  LogString& diag_path) {
  char prop_val[PROPERTY_VALUE_MAX];
  int err = -1;

  // Get path from property
  switch (t) {
    case CT_WCDMA:
      property_get(MODEM_W_DIAG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        char log_prop[PROPERTY_VALUE_MAX];
        property_get(MODEM_W_LOG_PROPERTY, log_prop, "");
        if (log_prop[0] && strcmp(prop_val, log_prop)) {
          same = false;
          log_path = log_prop;
          diag_path = prop_val;
        } else {
          same = true;
          log_path = prop_val;
        }
        err = 0;
      }
      break;
    case CT_TD:
      property_get(MODEM_TD_LOG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        log_path = prop_val;
        property_get(MODEM_TD_DIAG_PROPERTY, prop_val, "");
        if (prop_val[0]) {
          same = false;
          diag_path = prop_val;
          err = 0;
        } else {
          log_path.clear();
        }
      }
      break;
    case CT_3MODE:
      property_get(MODEM_TL_LOG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        log_path = prop_val;
        property_get(MODEM_TL_DIAG_PROPERTY, prop_val, "");
        if (prop_val[0]) {
          same = false;
          diag_path = prop_val;
          err = 0;
        } else {
          log_path.clear();
        }
      }
      break;
    case CT_4MODE:
      property_get(MODEM_FL_LOG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        log_path = prop_val;
        property_get(MODEM_FL_DIAG_PROPERTY, prop_val, "");
        if (prop_val[0]) {
          same = false;
          diag_path = prop_val;
          err = 0;
        } else {
          log_path.clear();
        }
      }
      break;
    case CT_5MODE:
      property_get(MODEM_L_LOG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        log_path = prop_val;
        property_get(MODEM_L_DIAG_PROPERTY, prop_val, "");
        if (prop_val[0]) {
          same = false;
          diag_path = prop_val;
          err = 0;
        } else {
          log_path.clear();
        }
      }
      break;
    case CT_WCN:
      property_get(MODEM_WCN_DIAG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        same = true;
        log_path = prop_val;
        err = 0;
      }
      break;
    case CT_GNSS:
      property_get(MODEM_GNSS_DIAG_PROPERTY, prop_val, "");
      if (prop_val[0]) {
        same = true;
        log_path = prop_val;
        err = 0;
      }
      break;
    case CT_AGDSP:
      same = true;
      log_path = "/dev/audio_dsp_log";
      err = 0;
      break;
    default:  // Unknown
      break;
  }

  return err;
}

const char* LogConfig::cp_type_to_name(CpType t) {
  const char* cp_name;

  switch (t) {
    case CT_WCDMA:
      cp_name = "cp_wcdma";
      break;
    case CT_TD:
      cp_name = "cp_td-scdma";
      break;
    case CT_3MODE:
      cp_name = "cp_3mode";
      break;
    case CT_4MODE:
      cp_name = "cp_4mode";
      break;
    case CT_5MODE:
      cp_name = "cp_5mode";
      break;
    case CT_WCN:
      cp_name = "cp_wcn";
      break;
    case CT_GNSS:
      cp_name = "cp_gnss";
      break;
    case CT_AGDSP:
      cp_name = "agdsp";
      break;
    default:
      cp_name = "(unknown)";
      break;
  }

  return cp_name;
}

IqType LogConfig::get_iq_type(const uint8_t* iq, size_t len) {
  IqType iqt = IT_UNKNOWN;

  if (3 == len && !memcmp(iq, "GSM", 3)) {
    iqt = IT_GSM;
  } else if (5 == len && !memcmp(iq, "WCDMA", 5)) {
    iqt = IT_WCDMA;
  }

  return iqt;
}

LogList<LogConfig::IqConfig*>::iterator LogConfig::find_iq(
    LogList<LogConfig::IqConfig*>& iq_list, CpType t) {
  LogList<IqConfig*>::iterator it;

  for (it = iq_list.begin(); it != iq_list.end(); ++it) {
    if ((*it)->cp_type == t) {
      break;
    }
  }

  return it;
}

LogList<LogConfig::IqConfig*>::const_iterator LogConfig::find_iq(
    const LogList<LogConfig::IqConfig*>& iq_list, CpType t) {
  LogList<IqConfig*>::const_iterator it;

  for (it = iq_list.begin(); it != iq_list.end(); ++it) {
    if ((*it)->cp_type == t) {
      break;
    }
  }

  return it;
}
