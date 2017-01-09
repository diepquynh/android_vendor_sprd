/*
 *  req_err.h - The client request transaction error codes.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-6-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef _REQ_ERR_H_
#define _REQ_ERR_H_

// Error codes
const int LCR_SUCCESS = 0;
// Generic error
const int LCR_ERROR = -1;
// Some CP is in transaction
const int LCR_IN_TRANSACTION = -2;
// Log not enabled
const int LCR_LOG_DISABLED = -3;
// Sleep log not supported
const int LCR_SLEEP_LOG_NOT_SUPPORTED = -4;
// RingBuf not supported
const int LCR_RINGBUF_NOT_SUPPORTED = -5;
// CP asserted
const int LCR_CP_ASSERTED = -6;
// CP nonexistent
const int LCR_CP_NONEXISTENT = -7;

// Transaction started
const int LCR_IN_PROGRESS = 1;

#endif  // !_REQ_ERR_H_

