/*
 *  log_config_no_modemtype.cpp - Get WAN MODEM type from property
 *                                persist.modem.x.enable.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-1-27 Zhang Ziyi
 *  Initial version.
 */
#include <cstring>
#ifdef HOST_TEST_
  #include "prop_test.h"
#else
  #include <cutils/properties.h>
#endif

#include "log_config.h"

void LogConfig::propget_wan_modem_type() {
  char modem_enable[PROPERTY_VALUE_MAX];

  property_get(MODEM_TD_DEVICE_PROPERTY, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    s_wan_modem_type = CT_TD;
    return;
  }

  property_get(MODEM_W_DEVICE_PROPERTY, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    s_wan_modem_type = CT_WCDMA;
    return;
  }

  property_get(MODEM_L_DEVICE_PROPERTY, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    s_wan_modem_type = CT_5MODE;
    return;
  }

  property_get(MODEM_FL_DEVICE_PROPERTY, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    s_wan_modem_type = CT_4MODE;
    return;
  }

  property_get(MODEM_TL_DEVICE_PROPERTY, modem_enable, "");
  if (!strcmp(modem_enable, "1")) {
    s_wan_modem_type = CT_3MODE;
    return;
  }

  if (CT_UNKNOWN == s_wan_modem_type) {
    err_log("modem type unkown");
  }
}
