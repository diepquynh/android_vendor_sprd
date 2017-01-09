/*
 *  AT Distributor
 *
 *  Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __ATD_H
#define __ATD_H
#include <atc.h>
int at_send(char *at, int len, int timeout_ms/* millisecond */);
int at_send_async(char *at, int len);
int register_unsolicited(char *prefix_unso, at_response_cb at_response_cb);
#endif
