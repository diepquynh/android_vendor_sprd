/* Copyright (C) 2016 Spreadtrum Communications Inc. */
#ifndef EXTDATA_AUTO_TEST_H_
#define EXTDATA_AUTO_TEST_H_

#ifdef __cplusplus
    extern "C" {
#endif

#define DHCP_LEASES_FILE    "/data/misc/dhcp/dnsmasq.leases"

void start_autotest(struct command *c);
void stop_autotest(struct command *c);

#ifdef __cplusplus
    }
#endif

#endif
