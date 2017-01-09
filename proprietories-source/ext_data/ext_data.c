/*
 * Convert shell script(ext_data.sh) to source code. The basic functions such
 * as configure ip address, up/down seth netcard... are moved to phoneserver.
 * And filter misc packets, data transmission test... are implemented here.
 *
 * Liping Zhang <liping.zhang@spreadtrum.com>
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <linux/rtnetlink.h>

#include "utils.h"
#include "auto_test.h"

static void do_pre_ifup(struct command *c) {
    char path[128];

    /* For ZTE lab test, do not drop RA from local IP */
    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/accept_ra_from_local",
             c->ifname);
    write_file(path, "1");

    /* Reduce the time cost of getting the ipv6's global address */
    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/router_solicitation_interval",
             c->ifname);
    write_file(path, "1");
    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/router_solicitations",
             c->ifname);
    write_file(path, "12");

    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/accept_dad",
             c->ifname);
    write_file(path, "-1");

    /* For CTS and BIP test pass! */
    if (ipv6_need_disable(c->ifname))
        exec_cmd("ip6tables -w -I OUTPUT -o %s -p icmpv6 -j DROP", c->ifname);
    else
        exec_cmd("ip6tables -w -D OUTPUT -o %s -p icmpv6 -j DROP", c->ifname);
    return;
}

static void do_ifup(struct command *c) {
    if (c->is_autotest)
        start_autotest(c);
    return;
}

static void do_ifdown(struct command *c) {
    exec_cmd("ip6tables -w -D OUTPUT -o %s -p icmpv6 -j DROP", c->ifname);

    if (c->is_autotest)
        stop_autotest(c);
    return;
}

int process_cmd(struct command *c) {
    switch (c->cmdtype) {
    case CMD_TYPE_PREIFUP:
        do_pre_ifup(c);
        break;

    case CMD_TYPE_IFUP:
        do_ifup(c);
        break;

    case CMD_TYPE_IFDOWN:
        do_ifdown(c);
        break;

    default:
        break;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    struct timeval tv;
    int srvfd;

    signal(SIGPIPE, SIG_IGN);

    srvfd = socket_local_server(EXT_DATA_SOCKNAME,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (srvfd < 0) {
        ALOGE("Fail to create unix socket server: %s\n", strerror(errno));
        return -1;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (1) {
        struct command c;
        char cmd[64];
        int clifd;

        clifd = accept(srvfd, NULL, NULL);
        ALOGD("New client %d connected!\n", clifd);

        /*
          * Prevent some 'error' clients do not read the result, then suspend
          * our write() call.
          */
        (void) setsockopt(clifd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);

        while (read_cmd(clifd, cmd, sizeof cmd) > 0) {
            struct timeval start, end;
            int err = 0;

            gettimeofday(&start, NULL);
            ALOGI("**********Start[%s]**********\n", cmd);

            if (parse_cmd(cmd, &c) == 0)
                err = process_cmd(&c);

            if (write_result(clifd, &err, sizeof(err)) < 0)
                break;

            gettimeofday(&end, NULL);
            ALOGI("**********End[%lds, %ldus]**********\n",
                  end.tv_sec - start.tv_sec, end.tv_usec - start.tv_usec);
        }

        ALOGD("Client %d bye!\n", clifd);
        close(clifd);
    }

    close(srvfd);
    return 0;
}
