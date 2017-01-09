/* Copyright (C) 2016 Spreadtrum Communications Inc. */
#ifndef EXTDATA_UTILS_H_
#define EXTDATA_UTILS_H_

#ifdef __cplusplus
    extern "C" {
#endif

#define EXT_DATA_SOCKNAME   "ext_data"
#define LOG_TAG EXT_DATA_SOCKNAME
#include <cutils/log.h>

#define SSLEN(str)  (sizeof(str) - 1)

enum cmd_type {
    CMD_TYPE_PREIFUP,
    CMD_TYPE_IFUP,
    CMD_TYPE_IFDOWN,
    CMD_TYPE_END
};

#define PDP_ACTIVE_IPV4    0x0001
#define PDP_ACTIVE_IPV6    0x0002

struct command {
    enum cmd_type cmdtype;
    char *ifname;
    unsigned int pdp_type;
    int is_autotest;
};

int ipv6_need_disable(const char *ifname);
int read_cmd(int fd, char *cmd, size_t len);
int parse_cmd(char *cmd, struct command *c);
int write_result(int fd, void *buf, int len);
int exec_cmd(const char *cmd_fmt, ...) \
             __attribute__((__format__(printf, 1, 2)));
int write_file(const char *path, const char *value);

#ifdef __cplusplus
    }
#endif

#endif
