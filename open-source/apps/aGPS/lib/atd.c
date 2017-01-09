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

#include <atd.h>

int register_unsolicited(char *prefix_unso, at_response_cb cb)
{
	return atc_register_unsolicited(NULL, prefix_unso, cb);
}

int at_send(char *at, int len, int timeout_ms/* millisecond */)
{
	return atc_send(NULL, at, len, 1, timeout_ms);
}

int at_send_async(char *at, int len)
{
	return atc_send(NULL, at, len, 0, 0);
}
