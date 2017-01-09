/*
 *  log_config_with_modemtype.cpp - Get WAN MODEM type from ro.radio.modemtype.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-1-27 Zhang Ziyi
 *  Initial version.
 */
#ifdef HOST_TEST_
  #include "prop_test.h"
#else
  #include <cutils/properties.h>
#endif

#include "log_config.h"

void LogConfig::propget_wan_modem_type() {
  char type[PROPERTY_VALUE_MAX];

  property_get(MODEM_TYPE_PROPERTY, type, "");
  if (!strcmp(type, "w")) {
    s_wan_modem_type = CT_WCDMA;
  } else if (!strcmp(type, "t")) {
    s_wan_modem_type = CT_TD;
  } else if (!strcmp(type, "tl")) {
    s_wan_modem_type = CT_3MODE;
  } else if (!strcmp(type, "lf")) {
    s_wan_modem_type = CT_4MODE;
  } else if (!strcmp(type, "l")) {
    s_wan_modem_type = CT_5MODE;
  } else {
    s_wan_modem_type = CT_UNKNOWN;
  }

  if (CT_UNKNOWN == s_wan_modem_type) {
    err_log("modem type unkown");
  }
}
