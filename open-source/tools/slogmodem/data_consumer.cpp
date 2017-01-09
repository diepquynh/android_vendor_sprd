/*
 *  data_consumer.h - The data consumer base class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-13 Zhang Ziyi
 *  Initial version.
 */
#include "data_consumer.h"

DataConsumer::DataConsumer(const LogString& cp_name, CpStorage& cp_stor)
    : m_cp_name(cp_name),
      m_diag_dev{0},
      m_stor(cp_stor),
      m_client{0},
      m_callback{0} {}
