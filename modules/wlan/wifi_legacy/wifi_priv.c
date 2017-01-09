#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>
#include <cutils/sockets.h>
#include <utils/Log.h>

#ifdef CONFIG_HOSTAPD_ADVANCE
#include "libwpa_client/wpa_ctrl.h"
#endif

#define WCND_SOCKET_NAME	"wcnd"
#define WCND_CMD_STR_START_CP2  "wcn WIFI-OPEN"
#define WCND_CMD_STR_STOP_CP2  "wcn WIFI-CLOSE"
#define WCND_RESP_STR_WIFI_OK   "BTWIFI-CMD OK"

static int wcnd_socket = -1;


//NOTE: Bug#479595 Add for WCND RESET Feature BEG -->
static int connect_wcnd(void) {
    int client_fd = -1;
    int retry_count = 20;
    struct timeval rcv_timeout;

    client_fd = socket_local_client( WCND_SOCKET_NAME,
    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    while(client_fd < 0 && retry_count > 0) {
        retry_count--;
        ALOGD("%s: Unable bind server %s, waiting...\n",__func__, WCND_SOCKET_NAME);
        usleep(100*1000);
        client_fd = socket_local_client( WCND_SOCKET_NAME,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if(client_fd > 0) {
        rcv_timeout.tv_sec = 20;
        rcv_timeout.tv_usec = 0;
        if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, 
					(char*)&rcv_timeout, sizeof(rcv_timeout)) < 0) {
            ALOGE("%s: set receive timeout fail\n",__func__);
        }
    }

    return client_fd;
}

static int start_cp2(void) {
    char buffer[128];
    int n = 0;
    int ret = 0;

    ALOGD("%s: send %s to %s\n",__func__, WCND_CMD_STR_START_CP2, WCND_SOCKET_NAME);

    memset(buffer, 0, sizeof(buffer));

    n = strlen(WCND_CMD_STR_START_CP2) +1;

    TEMP_FAILURE_RETRY(write(wcnd_socket, WCND_CMD_STR_START_CP2, n));

    memset(buffer, 0, sizeof(buffer));

    ALOGD("%s: waiting for server %s\n",__func__, WCND_SOCKET_NAME);
    n = read(wcnd_socket, buffer, sizeof(buffer)-1);

    ALOGD("%s: get %d bytes %s\n", __func__, n, buffer);

    if(!strstr(buffer, WCND_RESP_STR_WIFI_OK)) ret = -1;

    return ret;
}

static int stop_cp2(void) {
    char buffer[128];
    int n = 0;
    int ret = 0;

    ALOGD("%s: send %s to %s\n",__func__, WCND_CMD_STR_STOP_CP2, WCND_SOCKET_NAME);

    memset(buffer, 0, sizeof(buffer));

    n = strlen(WCND_CMD_STR_STOP_CP2) +1;

    TEMP_FAILURE_RETRY(write(wcnd_socket, WCND_CMD_STR_STOP_CP2, n));

    memset(buffer, 0, sizeof(buffer));

    ALOGD("%s: waiting for server %s\n",__func__, WCND_SOCKET_NAME);
    n = read(wcnd_socket, buffer, sizeof(buffer)-1);

    ALOGD("%s: get %d bytes %s\n", __func__, n, buffer);

    if(!strstr(buffer, WCND_RESP_STR_WIFI_OK)) ret = -1;

    return ret;

}
//<-- Bug#479595 Add for WCND RESET Feature END


void sprd_wifi_preload_driver() {
    // Power on cp2
	 if(wcnd_socket < 0) {
        wcnd_socket = connect_wcnd();
    }
    if(wcnd_socket > 0 && (start_cp2() < 0)) {
        ALOGE("start CP2 FAIL");
    }
}

void sprd_wifi_after_unload_driver() {
    // Power off cp2
    if(wcnd_socket < 0) {
        wcnd_socket = connect_wcnd();
    }

    if(wcnd_socket > 0 && (stop_cp2() < 0)) {
        ALOGE("stop CP2 FAIL");
    }
}

//NOTE: Bug #474462 Add for SoftAp Advance Feature BEG-->

#ifdef CONFIG_HOSTAPD_ADVANCE

static const char IFNAME[]              = "IFNAME=";
#define IFNAMELEN			(sizeof(IFNAME) - 1)
static const char WPA_EVENT_IGNORE[]    = "CTRL-EVENT-IGNORE ";


static const char AP_IFACE_DIR[] = "/data/misc/wifi/hostapd";

static struct wpa_ctrl *hostapd_ctrl_conn;
static struct wpa_ctrl *hostapd_monitor_conn;

/* socket pair used to exit from a blocking read */
static int hostapd_exit_sockets[2];

int wifi_connect_to_hostapd(const char *ifname)
{
    char path[256];

    /* Clear out any stale socket files that might be left over. */
    wpa_ctrl_cleanup();

    /* Reset sockets used for exiting from hung state */
    hostapd_exit_sockets[0] = hostapd_exit_sockets[1] = -1;

    sprintf(path, "%s/%s", AP_IFACE_DIR, ifname);

    hostapd_ctrl_conn = wpa_ctrl_open(path);
    if (hostapd_ctrl_conn == NULL) {
        ALOGE("Unable to open connection to hostapd on \"%s\": %s",
             path, strerror(errno));
        return -1;
    }
    hostapd_monitor_conn = wpa_ctrl_open(path);
    if (hostapd_monitor_conn == NULL) {
        wpa_ctrl_close(hostapd_ctrl_conn);
        hostapd_ctrl_conn = NULL;
        return -1;
    }
    if (wpa_ctrl_attach(hostapd_monitor_conn) != 0) {
        wpa_ctrl_close(hostapd_monitor_conn);
        wpa_ctrl_close(hostapd_ctrl_conn);
        hostapd_ctrl_conn = hostapd_monitor_conn = NULL;
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, hostapd_exit_sockets) == -1) {
        wpa_ctrl_close(hostapd_monitor_conn);
        wpa_ctrl_close(hostapd_ctrl_conn);
        hostapd_ctrl_conn = hostapd_monitor_conn= NULL;
        return -1;
    }

    return 0;
}

void wifi_close_hostapd_connection(const char *ifname)
{
    if (hostapd_ctrl_conn != NULL) {
        wpa_ctrl_close(hostapd_ctrl_conn);
        hostapd_ctrl_conn = NULL;
    }

    if (hostapd_monitor_conn != NULL) {
        wpa_ctrl_close(hostapd_monitor_conn);
        hostapd_monitor_conn = NULL;
    }

    if (hostapd_exit_sockets[0] >= 0) {
        close(hostapd_exit_sockets[0]);
        hostapd_exit_sockets[0] = -1;
    }

    if (hostapd_exit_sockets[1] >= 0) {
        close(hostapd_exit_sockets[1]);
        hostapd_exit_sockets[1] = -1;
    }

}

void wifi_stop_connect_to_hostapd(const char *ifname)
{
    /* unblocks the monitor receive socket for termination */
    TEMP_FAILURE_RETRY(write(hostapd_exit_sockets[0], "T", 1));
}

int wifi_hostapd_ctrl_recv(char *reply, size_t *reply_len)
{
    int res;
    int ctrlfd = wpa_ctrl_get_fd(hostapd_monitor_conn);
    struct pollfd rfds[2];

    memset(rfds, 0, 2 * sizeof(struct pollfd));
    rfds[0].fd = ctrlfd;
    rfds[0].events |= POLLIN;
    rfds[1].fd = hostapd_exit_sockets[1];
    rfds[1].events |= POLLIN;
    res = TEMP_FAILURE_RETRY(poll(rfds, 2, -1));
    if (res < 0) {
        ALOGE("Error poll = %d", res);
        return res;
    }

    //SPRD: Bug #604866 in case of hostapd_monitor_conn has been closed BEG-->
    if ((rfds[1].revents & POLLIN) || hostapd_monitor_conn == NULL) {
        ALOGE("hostapd_monitor_conn has been closed, return -2");
        return -2;
    }
    //<-- in case of hostapd_monitor_conn has been closed END
    if (rfds[0].revents & POLLIN) {
        return wpa_ctrl_recv(hostapd_monitor_conn, reply, reply_len);
    }

    /* it is not rfds[0], then it must be rfts[1] (i.e. the exit socket)
     * or we timed out. In either case, this call has failed ..
     */
    return -2;
}

#define WPA_HOSTAPD_EVENT_TERMINATING "AP-TERMINATING"
int wifi_hostapd_wait_on_socket(char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int result;
    char *match, *match2;

    if (hostapd_monitor_conn == NULL) {
        return snprintf(buf, buflen, WPA_HOSTAPD_EVENT_TERMINATING);
    }

    result = wifi_hostapd_ctrl_recv(buf, &nread);

    /* Terminate reception on exit socket */
    if (result == -2) {
        return snprintf(buf, buflen, WPA_HOSTAPD_EVENT_TERMINATING);
    }

    if (result < 0) {
        ALOGD("wifi_hostapd_ctrl_recv failed: %s\n", strerror(errno));
        return snprintf(buf, buflen, WPA_HOSTAPD_EVENT_TERMINATING);
    }
    buf[nread] = '\0';
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
        /* Fabricate an event to pass up */
        ALOGD("Received EOF on hostapd socket\n");
        return snprintf(buf, buflen, WPA_HOSTAPD_EVENT_TERMINATING);
    }
    /*
     * Events strings are in the format
     *
     *     IFNAME=iface <N>CTRL-EVENT-XXX
     *        or
     *     <N>CTRL-EVENT-XXX
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */

    if (strncmp(buf, IFNAME, IFNAMELEN) == 0) {
        match = strchr(buf, ' ');
        if (match != NULL) {
            if (match[1] == '<') {
                match2 = strchr(match + 2, '>');
                if (match2 != NULL) {
                    nread -= (match2 - match);
                    memmove(match + 1, match2 + 1, nread - (match - buf) + 1);
                }
            }
        } else {
            return snprintf(buf, buflen, "%s", WPA_EVENT_IGNORE);
        }
    } else if (buf[0] == '<') {
        match = strchr(buf, '>');
        if (match != NULL) {
            nread -= (match + 1 - buf);
            memmove(buf, match + 1, nread + 1);
            ALOGV("supplicant generated event without interface - %s\n", buf);
        }
    } else {
        /* let the event go as is! */
        ALOGW("supplicant generated event without interface and without message level - %s\n", buf);
    }

    return nread;
}

int wifi_hostapd_wait_for_event(char *buf, size_t buflen)
{
    return wifi_hostapd_wait_on_socket(buf, buflen);
}

int wifi_hostapd_command(const char *cmd, char *reply, size_t *reply_len)
{
    int ret;
    if (hostapd_ctrl_conn == NULL) {
        ALOGV("Not connected to hostapd - \"%s\" command dropped.\n", cmd);
        return -1;
    }
    ret = wpa_ctrl_request(hostapd_ctrl_conn, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        ALOGD("hostapd: '%s' command timed out.\n", cmd);
        /* unblocks the monitor receive socket for termination */
        TEMP_FAILURE_RETRY(write(hostapd_exit_sockets[0], "T", 1));
        return -2;
    } else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;
}
#endif //<-- CONFIG_HOSTAPD_ADVANCE
//<-- Bug #474462 Add for SoftAp Advance Feature END
