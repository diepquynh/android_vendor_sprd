/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      upio.c
 *
 *  Description:   Contains I/O functions, like
 *                      rfkill control
 *                      BT_WAKE/HOST_WAKE control
 *
 ******************************************************************************/

#define LOG_TAG "bt_upio"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include "bt_vendor_sprd.h"
#include "upio.h"
#include "userial_vendor.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef UPIO_DBG
#define UPIO_DBG FALSE
#endif

#if (UPIO_DBG == TRUE)
#define UPIODBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define UPIODBG(param, ...) {}
#endif

/******************************************************************************
**  Local type definitions
******************************************************************************/

#if (BT_WAKE_VIA_PROC == TRUE)

/* proc fs node for enable/disable lpm mode */
#ifndef VENDOR_LPM_PROC_NODE
#define VENDOR_LPM_PROC_NODE "/proc/bluetooth/sleep/lpm"
#endif

/* proc fs node for notifying write request */
#ifndef VENDOR_BTWRITE_PROC_NODE
#define VENDOR_BTWRITE_PROC_NODE "/proc/bluetooth/sleep/btwrite"
#endif

/*
 * Maximum btwrite assertion holding time without consecutive btwrite kicking.
 * This value is correlative(shorter) to the in-activity timeout period set in
 * the bluesleep LPM code. The current value used in bluesleep is 10sec.
 */
#ifndef PROC_BTWRITE_TIMER_TIMEOUT_MS
#define PROC_BTWRITE_TIMER_TIMEOUT_MS 8000
#endif

/* lpm proc control block */
typedef struct {
    uint8_t btwrite_active;
    uint8_t timer_created;
    timer_t timer_id;
    uint32_t timeout_ms;
} vnd_lpm_proc_cb_t;

static vnd_lpm_proc_cb_t lpm_proc_cb;
#endif

#ifndef UART_INFO_PATH
#define UART_INFO_PATH "/sys/devices/70000000.uart/uart_conf"
#endif

#ifndef GPIO_INFO_PATH
#define GPIO_INFO_PATH "/sys/kernel/debug/gpio"
#endif

static timer_t *userial_debug_timer = NULL;

#define WCND_SOCKET_NAME "wcnd"
#define WCND_BT_CMD_STR_START_CP2 "wcn BT-OPEN"
#define WCND_BT_CMD_STR_STOP_CP2 "wcn BT-CLOSE"
#define WCND_RESP_STR_BT_OK "BTWIFI-CMD OK"

/******************************************************************************
**  Static variables
******************************************************************************/

static uint8_t upio_state[UPIO_MAX_COUNT];
static int rfkill_id = -1;
static int bt_emul_enable = 0;
static char *rfkill_state_path = NULL;

#if (BT_WAKE_VIA_PROC == TRUE)
static int fd_btwrite = -1;
#endif

/******************************************************************************
**  Static functions
******************************************************************************/

/* for friendly debugging outpout string */
static char *lpm_mode[] = {"UNKNOWN", "disabled", "enabled"};

static char *lpm_state[] = {"UNKNOWN", "de-asserted", "asserted"};

/*****************************************************************************
**   Bluetooth On/Off Static Functions
*****************************************************************************/

static int init_rfkill()
{
    char path[64];
    char buf[16];
    int fd, sz, id;

    for (id = 0;; id++) {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            ALOGE("init_rfkill : open(%s) failed: %s (%d)\n", path, strerror(errno),
                  errno);
            return -1;
        }

        sz = read(fd, &buf, sizeof(buf));
        close(fd);

        if (sz >= 9 && memcmp(buf, "bluetooth", 9) == 0) {
            rfkill_id = id;
            break;
        }
    }

    asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", rfkill_id);
    return 0;
}

/*****************************************************************************
**   LPM Static Functions
*****************************************************************************/

#if (BT_WAKE_VIA_PROC == TRUE)
/*******************************************************************************
**
** Function        proc_btwrite_timeout
**
** Description     Timeout thread of proc/.../btwrite assertion holding timer
**
** Returns         None
**
*******************************************************************************/
static void proc_btwrite_timeout(union sigval arg)
{
    UPIODBG("..%s..", __FUNCTION__);
    lpm_proc_cb.btwrite_active = FALSE;
    UNUSED(arg);
    /* drive LPM down; this timer should fire only when BT is awake; */
    upio_set(UPIO_BT_WAKE, UPIO_DEASSERT, 1);
}

/******************************************************************************
 **
 ** Function      upio_start_stop_timer
 **
 ** Description   Arm user space timer in case lpm is left asserted
 **
 ** Returns       None
 **
 *****************************************************************************/
void upio_start_stop_timer(int action)
{
    struct itimerspec ts;

    if (action == UPIO_ASSERT) {
        lpm_proc_cb.btwrite_active = TRUE;
        if (lpm_proc_cb.timer_created == TRUE) {
            ts.it_value.tv_sec = PROC_BTWRITE_TIMER_TIMEOUT_MS / 1000;
            ts.it_value.tv_nsec = 1000000 * (PROC_BTWRITE_TIMER_TIMEOUT_MS % 1000);
            ts.it_interval.tv_sec = 0;
            ts.it_interval.tv_nsec = 0;
        }
    } else {
        /* unarm timer if writing 0 to lpm; reduce unnecessary user space wakeup */
        memset(&ts, 0, sizeof(ts));
    }

    if (timer_settime(lpm_proc_cb.timer_id, 0, &ts, 0) == 0) {
        //UPIODBG("%s : timer_settime success", __FUNCTION__);
    } else {
        UPIODBG("%s : timer_settime failed", __FUNCTION__);
    }
}
#endif

/*****************************************************************************
**   UPIO Interface Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        upio_init
**
** Description     Initialization
**
** Returns         None
**
*******************************************************************************/
void upio_init(void)
{
    memset(upio_state, UPIO_UNKNOWN, UPIO_MAX_COUNT);
#if (BT_WAKE_VIA_PROC == TRUE)
    memset(&lpm_proc_cb, 0, sizeof(vnd_lpm_proc_cb_t));
#endif
}

/*******************************************************************************
**
** Function        upio_cleanup
**
** Description     Clean up
**
** Returns         None
**
*******************************************************************************/
void upio_cleanup(void)
{
#if (BT_WAKE_VIA_PROC == TRUE)
    if (lpm_proc_cb.timer_created == TRUE) timer_delete(lpm_proc_cb.timer_id);

    lpm_proc_cb.timer_created = FALSE;
    fd_btwrite = -1;
#endif
}

static int connect_wcnd(void)
{
    int client_fd = -1;
    int retry_count = 20;
    struct timeval rcv_timeout;

    client_fd = socket_local_client(
                    WCND_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    while (client_fd < 0 && retry_count > 0) {
        retry_count--;
        ALOGD("bt-vendor : %s: Unable bind server %s, waiting...\n", __func__,
              WCND_SOCKET_NAME);
        usleep(100 * 1000);
        client_fd = socket_local_client(
                        WCND_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if (client_fd < 0) return client_fd;

    rcv_timeout.tv_sec = 10;
    rcv_timeout.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&rcv_timeout,
                   sizeof(rcv_timeout)) < 0) {
        ALOGE("bt-vendor : %s: set receive timeout fail\n", __func__);
    }

    ALOGD("bt-vendor : %s: connect to server status:%d\n", __func__, client_fd);

    return client_fd;
}

static int start_cp2(void)
{
    char buffer[128];
    int len = 0;
    int ret = -1;
    int wcnd_socket = connect_wcnd();

    if (wcnd_socket < 0) {
        ALOGD("bt-vendor : %s: connect to server failed", __func__);
        return -1;
    }

    len = strlen(WCND_BT_CMD_STR_START_CP2) + 1;

    ALOGD("bt-vendor : %s: send start cp2 message to wcnd %s\n", __func__,
          WCND_BT_CMD_STR_START_CP2);

    ret = TEMP_FAILURE_RETRY(write(wcnd_socket, WCND_BT_CMD_STR_START_CP2, len));

    if (ret <= 0) {
        ALOGD("bt-vendor : %s: write to wcnd fail.", __func__);
        close(wcnd_socket);
        return ret;
    }

    memset(buffer, 0, 128);

    ALOGD("bt-vendor : %s: waiting for server %s\n", __func__, WCND_SOCKET_NAME);
    ret = read(wcnd_socket, buffer, 128);

    ALOGD("bt-vendor : %s: get %d bytes %s\n", __func__, ret, buffer);

    if (!strstr(buffer, WCND_RESP_STR_BT_OK)) ret = -1;

    close(wcnd_socket);
    return ret;
}

static int stop_cp2(void)
{
    char buffer[128];
    int len = 0;
    int ret = -1;
    int wcnd_socket = connect_wcnd();

    if (wcnd_socket < 0) {
        ALOGD("bt-vendor : %s: connect to server failed", __func__);
        return -1;
    }

    len = strlen(WCND_BT_CMD_STR_STOP_CP2) + 1;

    ALOGD("bt-vendor : %s: send stop cp2 message to wcnd %s\n", __func__,
          WCND_BT_CMD_STR_STOP_CP2);

    ret = TEMP_FAILURE_RETRY(write(wcnd_socket, WCND_BT_CMD_STR_STOP_CP2, len));

    if (ret <= 0) {
        ALOGD("bt-vendor : %s: write to wcnd fail.", __func__);
        close(wcnd_socket);
        return ret;
    }

    memset(buffer, 0, 128);

    ALOGD("bt-vendor : %s: waiting for server %s\n", __func__, WCND_SOCKET_NAME);
    ret = read(wcnd_socket, buffer, 128);

    ALOGD("bt-vendor : %s: get %d bytes %s\n", __func__, ret, buffer);

    if (!strstr(buffer, WCND_RESP_STR_BT_OK)) ret = -1;

    close(wcnd_socket);
    return ret;
}

/*******************************************************************************
**
** Function        upio_set_bluetooth_power
**
** Description     Interact with low layer driver to set Bluetooth power
**                 on/off.
**
** Returns         0  : SUCCESS or Not-Applicable
**                 <0 : ERROR
**
*******************************************************************************/
int upio_set_bluetooth_power(int on)
{
#if (BT_RFKILL_CTRL == TRUE)
    int sz;
    int fd = -1;
    int ret = -1;
    char buffer = '0';
    int upio_assert = UPIO_DEASSERT;

    switch(on)
    {
        case UPIO_BT_POWER_OFF:
            buffer = '0';
            upio_assert = UPIO_DEASSERT;
            break;

        case UPIO_BT_POWER_ON:
            buffer = '1';
            upio_assert = UPIO_ASSERT;
            break;
    }

    if (rfkill_id == -1)
    {
        if (init_rfkill())
            return ret;
    }

    fd = open(rfkill_state_path, O_WRONLY);

    if (fd < 0)
    {
        ALOGE("set_bluetooth_power : open(%s) for write failed: %s (%d)",
            rfkill_state_path, strerror(errno), errno);
        return ret;
    }

    sz = write(fd, &buffer, 1);

    if (sz < 0) {
        ALOGE("set_bluetooth_power : write(%s) failed: %s (%d)",
            rfkill_state_path, strerror(errno),errno);
        return sz;
    } else {
        close(fd);
    }

#else

    if (on == UPIO_BT_POWER_OFF) {
        stop_cp2();
    } else if (on == UPIO_BT_POWER_ON) {
        start_cp2();
    }

#endif

    switch (on) {
    case UPIO_BT_POWER_OFF:
#if (BT_WAKE_VIA_PROC == TRUE)
        upio_set(UPIO_LPM_MODE, UPIO_DEASSERT, 0);
#endif
        break;

    case UPIO_BT_POWER_ON:
#if (BT_WAKE_VIA_PROC == TRUE)
        upio_set(UPIO_LPM_MODE, UPIO_ASSERT, 0);
#endif
        break;
    }
    return 0;
}

/*******************************************************************************
**
** Function        upio_set
**
** Description     Set i/o based on polarity
**
** Returns         None
**
*******************************************************************************/
void upio_set(uint8_t pio, uint8_t action, uint8_t polarity)
{
    int rc;
#if (BT_WAKE_VIA_PROC == TRUE)
    char buffer;
#endif

    //UPIODBG("%s : pio %d action %d, polarity %d", __FUNCTION__, pio, action,
    ///        polarity);
    if (pio == UPIO_LPM_MODE) {
        UPIODBG("UPIO_LPM_MODE: %s", lpm_mode[action]);
    } else if (pio == UPIO_BT_WAKE) {
        UPIODBG("UPIO_BT_WAKE: %s", lpm_state[action]);
    }

    UNUSED(polarity);

    switch (pio) {
    case UPIO_LPM_MODE:
        if (upio_state[UPIO_LPM_MODE] == action) {
            UPIODBG("LPM is %s already", lpm_mode[action]);
            return;
        }

        upio_state[UPIO_LPM_MODE] = action;

#if (BT_WAKE_VIA_PROC == TRUE)
        if (fd_btwrite > 0) {
            close(fd_btwrite);
            fd_btwrite = -1;
        }
        fd_btwrite = open(VENDOR_BTWRITE_PROC_NODE, O_WRONLY);
        if (fd_btwrite < 0) {
            ALOGE("upio_set : open(%s) for write failed: %s (%d)",
                  VENDOR_BTWRITE_PROC_NODE, strerror(errno), errno);
        }
#if (PROC_BTWRITE_TIMER_TIMEOUT_MS != 0)
        else {
            if (action == UPIO_ASSERT) {
                // create btwrite assertion holding timer
                if (lpm_proc_cb.timer_created == FALSE) {
                    int status;
                    struct sigevent se;

                    se.sigev_notify = SIGEV_THREAD;
                    se.sigev_value.sival_ptr = &lpm_proc_cb.timer_id;
                    se.sigev_notify_function = proc_btwrite_timeout;
                    se.sigev_notify_attributes = NULL;

                    status = timer_create(CLOCK_MONOTONIC, &se, &lpm_proc_cb.timer_id);

                    if (status == 0) lpm_proc_cb.timer_created = TRUE;
                }
            }
            if (action == UPIO_DEASSERT) {
                if (fd_btwrite != -1) {
                    close(fd_btwrite);
                    fd_btwrite = -1;
                }
            }
        }
#endif
#endif
        break;

    case UPIO_BT_WAKE:
        if (upio_state[UPIO_BT_WAKE] == action) {
            //UPIODBG("BT_WAKE is %s already", lpm_state[action]);

#if (BT_WAKE_VIA_PROC == TRUE)
            if (lpm_proc_cb.btwrite_active == TRUE) {
                /*
                 * The proc btwrite node could have not been updated for
                 * certain time already due to heavy downstream path flow.
                 * In this case, we want to explicity touch proc btwrite
                 * node to keep the bt_wake assertion in the LPM kernel
                 * driver. The current kernel bluesleep LPM code starts
                 * a 10sec internal in-activity timeout timer before it
                 * attempts to deassert BT_WAKE line.
                 */
                /* re-arm user space timer */
                upio_start_stop_timer(action);
                return;
            }
#else
            return;
#endif
        }

        upio_state[UPIO_BT_WAKE] = action;

#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)

        userial_vendor_ioctl(
            ((action == UPIO_ASSERT) ? USERIAL_OP_ASSERT_BT_WAKE
             : USERIAL_OP_DEASSERT_BT_WAKE),
            NULL);

#elif(BT_WAKE_VIA_PROC == TRUE)

        /*
         *  Kick proc btwrite node only at UPIO_ASSERT
         */
#if (BT_WAKE_VIA_PROC_NOTIFY_DEASSERT == FALSE)
        if (action == UPIO_DEASSERT) return;
#endif
        if (fd_btwrite < 0) {
            ALOGE("upio_set : %s(%d) incorrent", VENDOR_BTWRITE_PROC_NODE,
                  fd_btwrite);
            return;
        }
#if (BT_WAKE_VIA_PROC_NOTIFY_DEASSERT == TRUE)
        if (action == UPIO_DEASSERT)
            buffer = BT_WAKE_PROC_DEASSERT;
        else
#endif
            buffer = BT_WAKE_PROC_ASSERT;

        if (write(fd_btwrite, &buffer, 1) < 0) {
            ALOGE("upio_set : write(%s) failed: %s (%d)", VENDOR_BTWRITE_PROC_NODE,
                  strerror(errno), errno);
        }
#if (PROC_BTWRITE_TIMER_TIMEOUT_MS != 0)
        else {
            /* arm user space timer based on action */
            upio_start_stop_timer(action);
        }
#endif

        UPIODBG("%s: proc btwrite assertion, buffer: %c, timer_armed %d %d",
                __FUNCTION__, buffer, lpm_proc_cb.btwrite_active,
                lpm_proc_cb.timer_created);
#endif

        break;

    case UPIO_HOST_WAKE:
        UPIODBG("upio_set: UPIO_HOST_WAKE");
        break;
    }
}

static void uipo_userial_debug_timeout(union sigval arg)
{
    UPIODBG("..%s..", __FUNCTION__);
    int fd, ret;
    char buf[1024] = {0};
    char buffer = '1';

    UNUSED(arg);

    fd = open(GPIO_INFO_PATH, O_RDONLY);
    if (fd < 0) {
        UPIODBG("%s open %s failed: %s (%d)", __FUNCTION__, GPIO_INFO_PATH,
                strerror(errno), errno);
    } else {
        ret = read(fd, buf, sizeof(buf));
        UPIODBG("gpio_info len %d", ret);
        if (ret > 0) {
            UPIODBG("gpio_info %s", buf);
        }
        close(fd);
    }

    fd = open(UART_INFO_PATH, O_WRONLY);
    if (fd < 0) {
        UPIODBG("%s open %s failed: %s (%d)", __FUNCTION__, UART_INFO_PATH,
                strerror(errno), errno);
    } else {
        if (write(fd, &buffer, 1) < 0) {
            UPIODBG("%s write(%s) failed: %s (%d) 1", __FUNCTION__, UART_INFO_PATH,
                    strerror(errno), errno);
        }
        close(fd);
    }
}

void uipo_userial_debug_enable(void)
{
    int fd;
    char buffer = '2';
    struct sigevent se;
    struct itimerspec ts;

    fd = open(UART_INFO_PATH, O_WRONLY);
    if (fd < 0) {
        UPIODBG("%s open %s failed: %s (%d)", __FUNCTION__, UART_INFO_PATH,
                strerror(errno), errno);
        return;
    }

    if (write(fd, &buffer, 1) < 0) {
        UPIODBG("%s write %s failed: %s (%d)", __FUNCTION__, UART_INFO_PATH,
                strerror(errno), errno);
    }
    close(fd);

    if (userial_debug_timer != NULL) {
        UPIODBG("%s start userial_debug_timer error already exist", __FUNCTION__);
        return;
    }

    userial_debug_timer = (timer_t *)malloc(sizeof(timer_t));
    if (userial_debug_timer == NULL) {
        UPIODBG("%s malloc userial_debug_timer error null", __FUNCTION__);
        return;
    }

    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = userial_debug_timer;
    se.sigev_notify_function = uipo_userial_debug_timeout;
    se.sigev_notify_attributes = NULL;
    if (timer_create(CLOCK_MONOTONIC, &se, userial_debug_timer) < 0) {
        UPIODBG("%s create userial_debug_timer failed: %s (%d)", __FUNCTION__,
                strerror(errno), errno);
        return;
    }

    ts.it_value.tv_sec = 8;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    if (timer_settime(*userial_debug_timer, 0, &ts, 0) < 0) {
        UPIODBG("%s : userial_debug_timer failed", __FUNCTION__);
    }
}

void uipo_userial_debug_disable(void)
{
    int fd;
    char buffer = '3';

    if (userial_debug_timer) {
        timer_delete(*userial_debug_timer);
    } else {
        UPIODBG("%s : userial_debug_timer do not exist", __FUNCTION__);
    }

    fd = open(UART_INFO_PATH, O_WRONLY);
    if (fd < 0) {
        UPIODBG("%s open %s failed: %s (%d)", __FUNCTION__, UART_INFO_PATH,
                strerror(errno), errno);
        return;
    }

    if (write(fd, &buffer, 1) < 0) {
        UPIODBG("%s write %s failed: %s (%d)", __FUNCTION__, UART_INFO_PATH,
                strerror(errno), errno);
    }
    close(fd);
}
