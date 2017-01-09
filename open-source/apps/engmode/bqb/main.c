#define LOG_TAG "libbt-bqb"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <cutils/sockets.h>
#include <sys/un.h>
#include <poll.h>
#include <signal.h>
#include "bt_vendor_lib.h"
#include "vendor.h"
#include "bt_hci_bdroid.h"

/*
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
*/
#define UNUSED(expr) do { (void)(expr); } while (0)

#define RDWR_FD_FAIL (-2)
#define GENERIC_FAIL (-1)

#define PSKEY_CMD 0xFCA0
#define VENDOR_CMD_NONSIGNAL_START 0xFCD1
#define VENDOR_CMD_NONSIGNAL_STOP 0xFCD2

int s_bt_fd = -1;
int bt_aging_running = 1;
int sockfd;
void* handle;

int bt_pskey_received_flag = false;
int bt_aging_started_flag = false;
int fd_array[CH_MAX];
#define RESPONSE_LENGTH 100
uint8_t resp[RESPONSE_LENGTH] = {0};
uint8_t local_bdaddr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


extern bt_vendor_interface_t *lib_interface;
extern bool vendor_open(const uint8_t *local_bdaddr);
extern void vendor_close(void);
int deal_with_event(uint8_t *buf, int read_data_len);


/* dump received event or sent command */
void dump_data(uint8_t *p_buf, int data_len) {
    int i = 0;
    int buf_printf_len = data_len * 5 + 1;
    char *p_buf_pskey_printf = malloc(buf_printf_len);
    memset(p_buf_pskey_printf, 0, buf_printf_len);
    char *p = p_buf_pskey_printf;
    for(i=0; i < data_len; i++)
        p += snprintf(p, buf_printf_len, "0x%02x ", p_buf[i]);
    ALOGI("%s: %s", __func__,  p_buf_pskey_printf);
    free(p_buf_pskey_printf);
    p_buf_pskey_printf = NULL;
}

/* send hci command */
int send_cmd(uint8_t *p_buf, int data_len) {
    int i;
    ssize_t ret = 0;

    dump_data(p_buf, data_len);

    /* Send command via HC's xmit_cb API */
    ALOGD("sensen, wirte command size=%d", data_len);
    uint16_t total = 0;
    while(data_len) {
        ret = write(s_bt_fd, p_buf + total, data_len);
        ALOGD("sensen,  data_len= %d, ret size=%d", data_len, ret);
        switch (ret) {
            case -1:
                ALOGE("%s error writing to serial port: %s", __func__, strerror(errno));
                return total;
            case 0:  // don't loop forever in case write returns 0.
                return total;
            default:
                total += ret;
                data_len -= ret;
                break;
        }
    }
    return 0;
}

/* receive event in while loop */
int recv_event(tINT_CMD_CBACK callback) {
    int buf_printf_len = 0;
    int i = 0;
    int read_data_len = 0;
    uint8_t new_state = BT_VND_LPM_WAKE_ASSERT;
    char *event_resp = malloc(RESPONSE_LENGTH);
    memset(event_resp, 0, RESPONSE_LENGTH);
    char *p_recv = event_resp + sizeof(HC_BT_HDR) - 1;

    while (1) {
        usleep(100 * 1000);
        lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &new_state);
        read_data_len += read(s_bt_fd, p_recv + read_data_len, RESPONSE_LENGTH);
        if (read_data_len < 0){
            ALOGI("Failed to read ack, read_data_len=%d(%s)", read_data_len ,strerror(errno));
            return -1;
        } else if(read_data_len < 3) {
            continue;
        }

        dump_data(p_recv, read_data_len);

        if (p_recv[0] == 0x04 && read_data_len >= (p_recv[2] + 2)){
            ALOGI("read  ACK(0x04) ok \n");
            break;
        }else if(p_recv[0] == 0x04) {
            continue;
        } else {
            ALOGE("read ACK(0x%x)is not expect,retry\n,",p_recv[0]);
        }
    }
    callback(event_resp);
    return read_data_len;
}



/* bringup bt up */
int bt_on() {
    ALOGD("sensen %s", __func__);
    int power_state = BT_VND_PWR_OFF;
    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    power_state = BT_VND_PWR_ON;
    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    lib_interface->op(BT_VND_OP_USERIAL_OPEN, &fd_array);
    s_bt_fd = fd_array[0];
    ALOGD("sensen %s completed fd_array[0]=%d", __func__, fd_array[0]);

    lib_interface->op(BT_VND_OP_FW_CFG, NULL);
    ALOGD("sensen %s completed", __func__);
    return s_bt_fd;
}

/* close bt */
void bt_off() {
    ALOGD("sensen %s", __func__);
    lib_interface->op(BT_VND_OP_EPILOG, NULL);
    int power_state = BT_VND_PWR_OFF;
    lib_interface->op(BT_VND_OP_USERIAL_CLOSE, NULL);

    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    lib_interface->cleanup();
    s_bt_fd = -1;
    ALOGD("sensen %s completed", __func__);
}

