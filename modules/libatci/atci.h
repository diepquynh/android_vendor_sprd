/**
 * AT Command Interface Client Socket implementation
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 */

#ifndef ATCI_H_
#define ATCI_H_

#ifdef __cplusplus
extern "C" {
#endif

const char* sendCmd(int phoneId, const char* atCmd);
int sendATCmd(int phoneId, const char *atCmd, char *resp, size_t respLen);

#ifdef __cplusplus
}
#endif

#endif  // ATCI_H_
