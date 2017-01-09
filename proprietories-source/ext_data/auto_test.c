/* Copyright (C) 2016 Spreadtrum Communications Inc. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netutils/ifc.h>

#include "utils.h"
#include "auto_test.h"

#define ROUTE_TABLE_LAN_NETWORK    66
#define ROUTE_TABLE_WAN_NETWORK    67

#define ROUTE_TABLE_PRIORITY       9000

static int get_ipv4_ifaddr(const char *ifname, in_addr_t *addr) {
    ifc_init();
    if (ifc_get_addr(ifname, addr)) {
        ALOGE("Can't get the %s's ipv4 address: %s\n", ifname,
              strerror(errno));
        ifc_close();
        return -1;
    }
    ifc_close();

    return 0;
}

static int get_ipv4_pcaddr(in_addr_t *addr) {
    char ipstr[INET_ADDRSTRLEN];
    FILE *fp;
    int rc;

    fp = fopen(DHCP_LEASES_FILE, "r");
    if (fp == NULL) {
        ALOGE("Can't open %s: %s\n", DHCP_LEASES_FILE,
              strerror(errno));
        return -1;
    }

    if  ((rc=fscanf(fp, "%*s %*s %15s", ipstr)) == 1) {
        if (inet_pton(AF_INET, ipstr, addr) != 1) {
            ALOGE("Can't convert %s to addr format\n", ipstr);
            fclose(fp);
            return -1;
        }

        fclose(fp);
        return 0;
    }

    ALOGE("get_ipv4_pcaddr fscanf error: %d\n", rc);
    fclose(fp);
    return -1;
}

static int has_ipv6_globaladdr(const char *ifname) {
    char addrstr[INET6_ADDRSTRLEN];
    char name[64];
    FILE *f;

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        ALOGE("Fail to open /proc/net/if_inet6: %s\n", strerror(errno));
        return 0;
    }

    /* Format:
     * 20010db8000a0001fc446aa4b5b347ed 03 40 00 01    wlan0
     */
    while (fscanf(f, "%32s %*02x %*02x %*02x %*02x %63s\n",
        addrstr, name) == 2) {
        if (strcmp(name, ifname))
            continue;

        if (strncmp(addrstr, "fe80", SSLEN("fe80")) != 0) {
            ALOGD("Get %s's ipv6 global address: %s\n", ifname, addrstr);
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

static void start_autotest_v4(struct command *c) {
    char localip[INET_ADDRSTRLEN];
    char pcv4ip[INET_ADDRSTRLEN];
    in_addr_t ifaddr, pcaddr;

    if (get_ipv4_ifaddr(c->ifname, &ifaddr))
        return;
    if (get_ipv4_pcaddr(&pcaddr))
        return;

    (void) inet_ntop(AF_INET, &ifaddr, localip, sizeof localip);
    (void) inet_ntop(AF_INET, &pcaddr, pcv4ip, sizeof pcv4ip);
    ALOGD("Localip=%s, pcv4ip=%s\n", localip, pcv4ip);

    /* Flush old iptables rules */
    exec_cmd("iptables -w -F");
    exec_cmd("iptables -w -P FORWARD ACCEPT");
    exec_cmd("iptables -w -t nat -F");
    exec_cmd("iptables -w -t mangle -F");

    /* Add ip rule and route policy */
    exec_cmd("ip rule del table %d", ROUTE_TABLE_LAN_NETWORK);
    exec_cmd("ip route flush table %d", ROUTE_TABLE_LAN_NETWORK);
    exec_cmd("ip rule add from all iif rndis0 lookup %d",
             ROUTE_TABLE_LAN_NETWORK);
    exec_cmd("ip route add default via %s dev %s table %d", localip, c->ifname,
             ROUTE_TABLE_LAN_NETWORK);

    exec_cmd("ip route del default");
    exec_cmd("ip route add default via %s dev %s", localip, c->ifname);

    exec_cmd("iptables -w -t nat -A PREROUTING -i %s -j DNAT --to-destination"
             " %s", c->ifname, pcv4ip);
    exec_cmd("iptables -w -t nat -A POSTROUTING -s %s -j SNAT --to-source %s",
             pcv4ip, localip);
    /* Drop the misc pakcets from PC */
    exec_cmd("iptables -w -I FORWARD -o %s -p all ! -d %s/16 -j DROP",
             c->ifname, localip);

    /* Drop dns and ntp packets from UE */
    exec_cmd("iptables -w -I OUTPUT -s %s -p udp --dport 53 -j DROP", localip);
    exec_cmd("iptables -w -I OUTPUT -s %s -p udp --dport 123 -j DROP", localip);

    return;
}

static void start_autotest_v6(struct command *c) {
    const int max_retry = 10;
    int retry;

    exec_cmd("ndc tether radvd remove_upstream %s", c->ifname);

    exec_cmd("ip -6 rule del table %d", ROUTE_TABLE_LAN_NETWORK);
    exec_cmd("ip -6 route flush table %d", ROUTE_TABLE_LAN_NETWORK);
    exec_cmd("ip -6 rule del table %d", ROUTE_TABLE_WAN_NETWORK);
    exec_cmd("ip -6 route flush table %d", ROUTE_TABLE_WAN_NETWORK);

    /*
     * In ext_data.sh, there's a sleep(5) call here. I think if the ipv6 global
     * address is obtained, we can UP the radvd right now.
     */
    for (retry = 0; retry < max_retry; retry++) {
        if (has_ipv6_globaladdr(c->ifname))
            break;

        usleep(500 * 1000);
    }

    if (retry == max_retry)
        ALOGE("Cannot get the %s's ipv6 global address\n", c->ifname);

    exec_cmd("ndc tether radvd add_upstream %s", c->ifname);

    /*
     * Image the Network topology showed below, UE act as an IPV6 Router:
                              /------\                   +------+
       +---+                  |      |                   |      |
       |   |2003::3     rndis0|      |seth_lte0   2005::1|      |
       |PC |------------------|  UE  |-------------------|SERVER|
       |   |           2003::2|      |2003::1            |      |
       +---+                  |      |                   |      |
                              \------/                   +------+
     * If the packet was from SERVER to PC, ie src=2005::1,dst=2003::3,
     * UE will be confused that how to deliver this packet, 2003::3 maybe
     * local at seth_lte0 side or rndis0 side. So add policy rule to avoid
     * this embarrassing scenes.
     */
    exec_cmd("ip -6 rule add iif rndis0 lookup %d pref %d",
             ROUTE_TABLE_LAN_NETWORK, ROUTE_TABLE_PRIORITY);
    exec_cmd("ip -6 route add default dev %s table %d", c->ifname,
             ROUTE_TABLE_LAN_NETWORK);
    exec_cmd("ip -6 rule add iif %s lookup %d pref %d", c->ifname,
             ROUTE_TABLE_WAN_NETWORK, ROUTE_TABLE_PRIORITY);
    exec_cmd("ip -6 route add default dev rndis0 table %d",
             ROUTE_TABLE_WAN_NETWORK);

    exec_cmd("ip -6 route del default");
    exec_cmd("ip -6 route add default dev %s", c->ifname);
    return;
}

void start_autotest(struct command *c) {
    if (c->pdp_type & PDP_ACTIVE_IPV4)
        start_autotest_v4(c);
    if (c->pdp_type & PDP_ACTIVE_IPV6)
        start_autotest_v6(c);
}

static void stop_autotest_v4(struct command *c) {
    exec_cmd("iptables -w -F");
    exec_cmd("iptables -w -X");
    return;
}

static void stop_autotest_v6(struct command *c) {
    exec_cmd("ndc tether radvd remove_upstream %s", c->ifname);
    return;
}

void stop_autotest(struct command *c) {
    if (c->pdp_type & PDP_ACTIVE_IPV4)
        stop_autotest_v4(c);
    if (c->pdp_type & PDP_ACTIVE_IPV6)
        stop_autotest_v6(c);
    return;
}
