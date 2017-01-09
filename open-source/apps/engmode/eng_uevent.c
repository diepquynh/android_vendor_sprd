#include "engopt.h"
#include "eng_uevent.h"
#include "cutils/uevent.h"
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define UEVENT_MSG_LEN  1024

extern sem_t g_armlog_sem;
int g_armlog_enable = 0;
extern int eng_usb_state(void);

struct uevent {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *usb_connect;
};

static void parse_event(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->path = "";
    uevent->subsystem = "";
    uevent->usb_connect = "";

    while(*msg) {
        if(!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if(!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            uevent->path = msg;
        } else if(!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if(!strncmp(msg, "USB_STATE=", 10)) {
            msg += 10;
            uevent->usb_connect = msg;
        }

        /* advance to after the next \0 */
        while(*msg++)
            ;
    }

    ENG_LOG("%s: event { '%s', '%s', '%s', '%s' }\n", __FUNCTION__,
            uevent->action, uevent->path, uevent->subsystem, uevent->usb_connect);
}

static void handle_device_event(struct uevent *uevent)
{
    if(0 == strncmp(uevent->usb_connect, "CONFIGURED", 10)) {
        // start cp log
        ENG_LOG("%s: enable arm log\n", __FUNCTION__);
        g_armlog_enable = 1;
        sem_post(&g_armlog_sem);
    }else if(0 == strncmp(uevent->usb_connect, "DISCONNECTED", 12)){
        // stop cp log
        ENG_LOG("%s: disable arm log\n", __FUNCTION__);
        g_armlog_enable = 0; 
    }
}

void handle_device_fd(int sock)
{
    char msg[UEVENT_MSG_LEN+2];
    int n;
    while ((n = uevent_kernel_multicast_recv(sock, msg, UEVENT_MSG_LEN)) > 0) {
        if(n >= UEVENT_MSG_LEN)   /* overflow -- discard */
            continue;

        msg[n] = '\0';
        msg[n+1] = '\0';

        struct uevent uevent;
        parse_event(msg, &uevent);
        handle_device_event(&uevent);
    }
}

void* eng_uevt_thread(void *x)
{
    struct pollfd ufd;
    int sock = -1;
    int nr;

    sock = uevent_open_socket(256*1024, true);
    if(-1 == sock){
        ENG_LOG("%s: socket init failed !\n", __FUNCTION__);
        return 0;
    }
    if(eng_usb_state()) {
	g_armlog_enable = 1;
	sem_post(&g_armlog_sem);
    }
    ufd.events = POLLIN;
    ufd.fd = sock;
    while(1) {
        ufd.revents = 0;
        nr = poll(&ufd, 1, -1);
        if (nr <= 0)
            continue;
        if (ufd.revents == POLLIN)
            handle_device_fd(sock);
    }

    return 0;    
}
