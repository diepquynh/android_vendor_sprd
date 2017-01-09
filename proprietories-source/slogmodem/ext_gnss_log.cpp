/*
 *  ext_gnss_log.cpp - The external GNSS log and dump handler class declaration.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-8-26 George YAN
 *  Initial version.
 */

#include <poll.h>

#include "client_mgr.h"
#include "diag_dev_hdl.h"
#include "ext_gnss_log.h"
#include "log_ctrl.h"

ExtGnssLogHandler::ExtGnssLogHandler(LogController* ctrl, Multiplexer* multi,
                                     const LogConfig::ConfigEntry* conf,
                                     StorageManager& stor_mgr)
    : LogPipeHandler(ctrl, multi, conf, stor_mgr) {}

int ExtGnssLogHandler::start_dump(const struct tm& lt) { return -1; }

int ExtGnssLogHandler::start() { return start_logging(); }

int ExtGnssLogHandler::stop() { return stop_logging(); }
