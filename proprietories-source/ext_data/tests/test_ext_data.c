/*
 * This is a simple program used to test ext_data's basic functions, only
 * available in userdebug version.
 *
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cutils/sockets.h>

#include "utils.h"
#include "auto_test.h"

static void send_request(int fd, char *cmd, int len) {
    int error;

    (void) write(fd, cmd, len);
    (void) read(fd, (char *)&error, sizeof(error));
    return;
}

void test_ifup(int fd) {
    send_request(fd, "<ifup>seth_lte0;IPV4;0",
                 SSLEN("<ifup>seth_lte0;IPV4;0") + 1);
    send_request(fd, "<ifup>seth_lte0;IPV6;0",
                 SSLEN("<ifup>seth_lte0;IPV6;0") + 1);
    return;
}

void test_ifdown(int fd) {
    send_request(fd, "<ifdown>seth_lte0;IPV4;0",
                 SSLEN("<ifdown>seth_lte0;IPV4;0") + 1);
    send_request(fd, "<ifdown>seth_lte0;IPV6;0",
                 SSLEN("<ifdown>seth_lte0;IPV6;0") + 1);
    return;
}

void test_autotest(int fd) {
    system("ip addr flush dev seth_lte0");

    /* IPV4 autotest, miss seth_lte0's ip address */
    send_request(fd, "<ifup>seth_lte0;IPV4;1",
                 SSLEN("<ifup>seth_lte0;IPV4;1") + 1);

    /* IPV4 normal autotest */
    system("echo 1111 ff:ff:ff:ff:ff:ff 172.16.21.1 aaaa SPRD >"
           DHCP_LEASES_FILE);
    system("ip link set dev seth_lte0 up");
    system("ip addr add dev seth_lte0 1.1.1.1/24");
    send_request(fd, "<ifup>seth_lte0;IPV4;1",
                 SSLEN("<ifup>seth_lte0;IPV4;1") + 1);

    /* IPV4 autotest, miss dhcp host */
    system("echo >" DHCP_LEASES_FILE);
    send_request(fd, "<ifup>seth_lte0;IPV4;1",
                 SSLEN("<ifup>seth_lte0;IPV4;1") + 1);

    /* IPV4 autotest, with a large dhcp host */
    system("echo 1111 ff:ff:ff:ff:ff:ff 172.161.211.11111 aaaa SPRD >"
           DHCP_LEASES_FILE);
    send_request(fd, "<ifup>seth_lte0;IPV4;1",
                 SSLEN("<ifup>seth_lte0;IPV4;1") + 1);

    /* IPV6 autotest, but miss ipv6 global address */
    send_request(fd, "<ifup>seth_lte0;IPV6;1",
                 SSLEN("<ifup>seth_lte0;IPV6;1") + 1);
    /* IPV6 normal autotest */
    system("ip addr add dev seth_lte0 fe80::123/64");
    system("ip addr add dev seth_lte0 2003::123/64");
    send_request(fd, "<ifup>seth_lte0;IPV6;1",
                 SSLEN("<ifup>seth_lte0;IPV6;1") + 1);

    /* Save the result */
    system("echo Autotest > /data/ext_autotest.log");
    system("iptables -w -S >> /data/ext_autotest.log");
    system("iptables -w -t nat -S >> /data/ext_autotest.log");
    system("echo ipv4 rt info: >> /data/ext_autotest.log");
    system("ip rule >> /data/ext_autotest.log");
    system("ip route ls table all >> /data/ext_autotest.log");
    system("echo ipv6 rt info: >> /data/ext_autotest.log");
    system("ip -6 rule >> /data/ext_autotest.log");
    system("ip -6 route ls table all >> /data/ext_autotest.log");

    /* Stop the autotest */
    send_request(fd, "<ifdown>seth_lte0;IPV4;1",
                 SSLEN("<ifdown>seth_lte0;IPV4;1") + 1);
    send_request(fd, "<ifdown>seth_lte0;IPV6;1",
                 SSLEN("<ifdown>seth_lte0;IPV6;1") + 1);
    return;
}

void test_filter_miscpkt(int fd) {
    system("setprop net.seth_lte0.ip_type 1");
    send_request(fd, "<preifup>seth_lte0;IPV4;0",
                 SSLEN("<preifup>seth_lte0;IPV4;0") + 1);

    /* Save the result */
    system("ip6tables -w -S OUTPUT> /data/ext_miscpkt.log");
    send_request(fd, "<ifdown>seth_lte0;IPV4;0",
                 SSLEN("<ifdown>seth_lte0;IPV4;0") + 1);

    system("setprop net.seth_lte0.ip_type 3");
    return;
}

void test_wrong_cmd(int fd) {
    char misc_cmd[32] = {0, };

    memset(misc_cmd, 'e', sizeof(misc_cmd) - 1);
    send_request(fd, misc_cmd, sizeof(misc_cmd));

    /* cutted command test */
    send_request(fd, "<preifdown>seth;", SSLEN("<preifdown>seth;") + 1);
    return;
}

void test_toolong_cmd(int fd) {
    char long_cmd[1024];

    memset(long_cmd, 'a', sizeof(long_cmd) - 1);
    long_cmd[sizeof(long_cmd) - 1] = '\0';

    send_request(fd, long_cmd, sizeof(long_cmd));
    return;
}

int main(int argc, char *argv[]) {
    int fd;

    fd = socket_local_client(EXT_DATA_SOCKNAME,
         ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        printf("Connect to ext_data failed: %s\n", strerror(errno));
        return -1;
    }

    test_wrong_cmd(fd);
    test_ifup(fd);
    test_ifdown(fd);
    test_autotest(fd);
    test_filter_miscpkt(fd);
    test_toolong_cmd(fd);

    close(fd);
    printf("Done\n");
    return 0;
}
