/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 *
 *  Filename:      bt_vendor_sprd.c
 *
 *  Description:   SPRD vendor specific library implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <utils/Log.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include "bt_vendor_sprd.h"
#include "userial_vendor.h"
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>
#include <cutils/sockets.h>

/******************************************************************************
**  Variables
******************************************************************************/
int s_bt_fd = -1;
static pthread_t pskey_read_th;
static sem_t pskey_read_sem;
static volatile uint8 psk_result = 0xFF;
bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#if (HW_NEED_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif
void hw_pskey_send(BT_PSKEY_CONFIG_T * bt_par);
#ifndef VENDOR_BTWRITE_PROC_NODE
#define VENDOR_BTWRITE_PROC_NODE "/proc/bluetooth/sleep/btwrite"
#endif
#ifndef UART_INFO_PATH
#ifdef SPRD_WCNBT_MARLIN
#define UART_INFO_PATH "/sys/devices/70000000.uart/uart_conf"
#elif HAS_BLUETOOTH_SPRD
#define UART_INFO_PATH "/sys/devices/f5360000.uart/uart_conf"
#endif
#endif
#ifdef PA_POWER_ON
#define MARLIN_PA_ENABLE_PATH "/sys/devices/platform/sprd_wcn.0/pa_enable"
#define MARLIN_PA_ENABLE_VALUE "1"
#define MARLIN_PA_DISABLE_VALUE "0"
static int pa_enable_fd = -1;
#endif
#define UNUSED(x) (void)(x)

int btsleep_fd = -1;
uint8_t btsleep_gpio_state = 0;
/******************************************************************************
**  Local type definitions
******************************************************************************/
// definations use for start & stop cp2
#define WCND_SOCKET_NAME    "wcnd"
#define WCND_BT_CMD_STR_START_CP2  "wcn BT-OPEN"
#define WCND_BT_CMD_STR_STOP_CP2  "wcn BT-CLOSE"
#define WCND_RESP_STR_BT_OK   "BTWIFI-CMD OK"


/******************************************************************************
**  Functions
******************************************************************************/
int sprd_config_init(int fd, char *bdaddr, struct termios *ti);
void sprd_get_pskey(BT_PSKEY_CONFIG_T * pskey_t);
/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    ALOGI("init");

    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    //userial_vendor_init();
    //upio_init();

    //vnd_load_conf(VENDOR_LIB_CONF_FILE);

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);

    return 0;
}

static int connect_wcnd(void)
{
    int client_fd = -1;
    int retry_count = 20;
    struct timeval rcv_timeout;

    client_fd = socket_local_client( WCND_SOCKET_NAME,
    ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    while(client_fd < 0 && retry_count > 0)
    {
        retry_count--;
        ALOGD("bt-vendor : %s: Unable bind server %s, waiting...\n",__func__, WCND_SOCKET_NAME);
        usleep(100*1000);
        client_fd = socket_local_client( WCND_SOCKET_NAME,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    }

    if(client_fd > 0)
    {
        rcv_timeout.tv_sec = 10;
        rcv_timeout.tv_usec = 0;
        if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcv_timeout, sizeof(rcv_timeout)) < 0)
        {
            ALOGE("bt-vendor : %s: set receive timeout fail\n",__func__);
        }
    }

    ALOGD("bt-vendor : %s: connect to server status:%d\n",__func__, client_fd);

    return client_fd;
}

static int start_cp2(void)
{
    char buffer[128];
    int len = 0;
    int ret = -1;
    int wcnd_socket = connect_wcnd();

    if (wcnd_socket <= 0) {
        ALOGD("bt-vendor : %s: connect to server failed", __func__);
        return  -1;
    }

    len = strlen(WCND_BT_CMD_STR_START_CP2) +1;

    ALOGD("bt-vendor : %s: send start cp2 message to wcnd %s\n",__func__, WCND_BT_CMD_STR_START_CP2);

    ret = TEMP_FAILURE_RETRY(write(wcnd_socket, WCND_BT_CMD_STR_START_CP2, len));

    if (ret <= 0)
    {
        ALOGD("bt-vendor : %s: write to wcnd fail.", __func__);
        close(wcnd_socket);
        return ret;
    }

    memset(buffer, 0, 128);

    ALOGD("bt-vendor : %s: waiting for server %s\n",__func__, WCND_SOCKET_NAME);
    ret = read(wcnd_socket, buffer, 128);

    ALOGD("bt-vendor : %s: get %d bytes %s\n", __func__, ret, buffer);

    if(!strstr(buffer, WCND_RESP_STR_BT_OK)) ret = -1;

    close(wcnd_socket);
    return ret;
}

static int stop_cp2(void)
{
    char buffer[128];
    int len = 0;
    int ret = -1;
    int wcnd_socket = connect_wcnd();

    if (wcnd_socket <= 0) {
        ALOGD("bt-vendor : %s: connect to server failed", __func__);
        return  -1;
    }

    len = strlen(WCND_BT_CMD_STR_START_CP2) + 1;

    ALOGD("bt-vendor : %s: send stop cp2 message to wcnd %s\n", __func__, WCND_BT_CMD_STR_STOP_CP2);

    ret = TEMP_FAILURE_RETRY(write(wcnd_socket, WCND_BT_CMD_STR_STOP_CP2, len));

     if (ret <= 0)
    {
        ALOGD("bt-vendor : %s: write to wcnd fail.", __func__);
        close(wcnd_socket);
        return ret;
    }

    memset(buffer, 0, 128);

    ALOGD("bt-vendor : %s: waiting for server %s\n",__func__, WCND_SOCKET_NAME);
    ret = read(wcnd_socket, buffer, 128);

    ALOGD("bt-vendor : %s: get %d bytes %s\n", __func__, ret, buffer);

    if(!strstr(buffer, WCND_RESP_STR_BT_OK)) ret = -1;

    close(wcnd_socket);
    return ret;

}

/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;
    int nCnt = 0;
    int nState = -1;
    int uart_fd = -1;
    char buffer;

    ALOGI("%s for %d", __func__, opcode);

    switch(opcode)
    {
        case BT_VND_OP_POWER_CTRL:
            {
                nState = *(int *) param;

                ALOGI("bt-vendor : BT_VND_OP_POWER_CTRL, state: %d ", nState);
                if (nState == BT_VND_PWR_ON)
                    start_cp2();
                else if (nState == BT_VND_PWR_OFF)
                    stop_cp2();
            }
            break;

        case BT_VND_OP_FW_CFG:
            {
                ALOGD("%s BT_VND_OP_FW_CFG BT_HC_PRELOAD_BT", __func__);
                uart_fd = open(UART_INFO_PATH, O_WRONLY);
                ALOGD("open uart_fd fd %d", uart_fd);
                if(uart_fd > 0)
                {
                     buffer = '2';
                     if (write(uart_fd, &buffer, 1) < 0)
                     {
                         ALOGI("%s write(%s) failed: %s (%d) 2", __func__,
                         UART_INFO_PATH, strerror(errno),errno);
                      }
                     close(uart_fd);
                  }
                 BT_PSKEY_CONFIG_T pskey;
                 sprd_get_pskey(&pskey);
                 hw_pskey_send(&pskey);

                if (btsleep_fd >= 0){
                    buffer = '2';
                    ALOGI("%s BT_VND_OP_LPM_WAKE_SET_STATE %d", __func__, buffer);
                    if (write(btsleep_fd, &buffer, 1) < 0)
                    {
                        ALOGE("%s write(%s) failed: %s (%d)", __func__,
                                VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
                    }
                }
             }
            break;

        case BT_VND_OP_SCO_CFG:
            {
                ALOGI("%s BT_VND_OP_SCO_CFG", __func__);
                bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
            }
            break;

        case BT_VND_OP_USERIAL_OPEN:
            {
                int idx;
                ALOGI("%s BT_VND_OP_USERIAL_OPEN", __func__);
                btsleep_fd = open(VENDOR_BTWRITE_PROC_NODE, O_WRONLY);
                if (btsleep_fd < 0)
                {
                    ALOGE("%s open(%s) for write failed: %s (%d)", __func__,
                            VENDOR_BTWRITE_PROC_NODE, strerror(errno), errno);
                    retval = -1;
                    //break;
                }
                else
                {
                    buffer = '1';
                    if (write(btsleep_fd, &buffer, 1) < 0)
                    {
                        ALOGE("%s write(%s) failed: %s (%d)", __func__,
                                VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
                    }
                }
#if PA_POWER_ON
                pa_enable_fd = open(MARLIN_PA_ENABLE_PATH, O_WRONLY);
                if (pa_enable_fd < 0) {
                    ALOGE("%s open(%s) for write failed: %s (%d)", __func__,
                            MARLIN_PA_ENABLE_PATH, strerror(errno), errno);
                }
                else
                {
                    if (write(pa_enable_fd, MARLIN_PA_ENABLE_VALUE, strlen(MARLIN_PA_ENABLE_VALUE)) < 0) {
                        ALOGE("%s write(%s) failed: %s (%d)", __func__,
                                MARLIN_PA_ENABLE_VALUE, strerror(errno),errno);
                    }
                }
#endif
                if(bt_hci_init_transport(&s_bt_fd) != -1)
                {
                    int (*fd_array)[] = (int (*) []) param;

                    for (idx=0; idx < CH_MAX; idx++)
                    {
                        (*fd_array)[idx] = s_bt_fd;
                    }
                    ALOGI("%s BT_VND_OP_USERIAL_OPEN ok", __func__);
                    retval = 1;
                }
                else
                {
                    ALOGI("%s BT_VND_OP_USERIAL_OPEN failed", __func__);
                    retval = -1;
                }
            }
            break;

        case BT_VND_OP_USERIAL_CLOSE:
            {
                ALOGI("%s BT_VND_OP_USERIAL_CLOSE", __func__);
                bt_hci_deinit_transport(s_bt_fd);
                if (btsleep_fd >= 0){
                    buffer = '2';
                    ALOGI("%s BT_VND_OP_LPM_WAKE_SET_STATE %d", __func__, buffer);
                    if (write(btsleep_fd, &buffer, 1) < 0)
                    {
                        ALOGE("%s write(%s) failed: %s (%d)", __func__,
                                VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
                    }
                    btsleep_gpio_state = 0;
                    close(btsleep_fd);
                }
#if PA_POWER_ON
                if (pa_enable_fd > 0) {
                    if (write(pa_enable_fd, MARLIN_PA_DISABLE_VALUE, strlen(MARLIN_PA_DISABLE_VALUE)) < 0) {
                        ALOGE("%s write(%s) failed: %s (%d)", __func__,
                                MARLIN_PA_DISABLE_VALUE, strerror(errno),errno);
                    }
                }
#endif
            }
            break;

        case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
            {
                uint32_t *timeout_ms = (uint32_t *) param;
                *timeout_ms = 2000;//2s
            }
            break;

        case BT_VND_OP_LPM_SET_MODE:
            {
                ALOGI("%s BT_VND_OP_LPM_SET_MODE", __func__);
                bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
            }
            break;

        case BT_VND_OP_LPM_WAKE_SET_STATE:
            {
            uint8_t *state = (uint8_t *) param;
            uint8_t wake_assert = (*state == BT_VND_LPM_WAKE_ASSERT) ? \
                                        TRUE : FALSE;
            if(btsleep_fd >= 0)
            {
                if(wake_assert == TRUE)
                      buffer = '1';// wakeup
                else
                      buffer = '2';// can sleep
                if((wake_assert == TRUE)&&(btsleep_gpio_state == 1)){
                    break;
                }
                if((wake_assert != TRUE)&&(btsleep_gpio_state != 1))
                    break;
                if (write(btsleep_fd, &buffer, 1) < 0)
                {
                    ALOGE("bt_vendor : write(%s) failed: %s (%d)",
                        VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
                } else {
                     if(wake_assert == TRUE){
                         btsleep_gpio_state = 1;
                     }else{
                         btsleep_gpio_state = 0;
                     }
                 ALOGI("bt-vendor : BT_VND_OP_LPM_WAKE_SET_STATE %d",buffer);
                }
            }

            break;
            }
        case BT_VND_OP_EPILOG:
            {
#if (HW_NEED_END_WITH_HCI_RESET == FALSE)
                if (bt_vendor_cbacks)
                {
                    bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
                }
#else
                hw_epilog_process();
#endif
            }
            break;

         default:
            break;
    }

    return retval;
}

/** Closes the interface */
static void cleanup( void )
{
    ALOGI("cleanup");

    //upio_cleanup();

    bt_vendor_cbacks = NULL;
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup
};

// pskey file structure default value
static BT_PSKEY_CONFIG_T bt_para_setting={
    .pskey_cmd = 0x001C0101,

    .g_dbg_source_sink_syn_test_data = 0,
    .g_sys_sleep_in_standby_supported = 0,
    .g_sys_sleep_master_supported = 0,
    .g_sys_sleep_slave_supported = 0,

    .default_ahb_clk = 26000000,
    .device_class = 0x001F00,
    .win_ext = 30,

    .g_aGainValue = {0x0000F600, 0x0000D000, 0x0000AA00, 0x00008400, 0x00004400, 0x00000A00},
    .g_aPowerValue = {0x0FC80000, 0x0FF80000, 0x0FDA0000, 0x0FCC0000, 0x0FFC0000},

    .feature_set = {0xFF, 0xFF, 0x8D, 0xFE, 0x9B, 0x7F, 0x79, 0x83, 0xFF, 0xA7, 0xFF, 0x7F, 0x00, 0xE0, 0xF7, 0x3E},
    .device_addr = {0x6A, 0x6B, 0x8C, 0x8A, 0x8B, 0x8C},

    .g_sys_sco_transmit_mode = 0, //true tramsmit by uart, otherwise by share memory
    .g_sys_uart0_communication_supported = 1, //true use uart0, otherwise use uart1 for debug
    .edr_tx_edr_delay = 5,
    .edr_rx_edr_delay = 14,

    .g_wbs_nv_117 = 0x0031,

    .is_wdg_supported = 0,

    .share_memo_rx_base_addr = 0,
    //.share_memo_tx_base_addr = 0,
    .g_wbs_nv_118 = 0x0066,
    .g_nbv_nv_117 = 0x1063,

    .share_memo_tx_packet_num_addr = 0,
    .share_memo_tx_data_base_addr = 0,

    .g_PrintLevel = 0xFFFFFFFF,

    .share_memo_tx_block_length = 0,
    .share_memo_rx_block_length = 0,
    .share_memo_tx_water_mark = 0,
    //.share_memo_tx_timeout_value = 0,
    .g_nbv_nv_118 = 0x0E45,

    .uart_rx_watermark = 48,
    .uart_flow_control_thld = 63,
    .comp_id = 0,
    .pcm_clk_divd = 0x26,


    .reserved = {0}
};

/*
* Psk Commpete Event    Event Code    Event Preload Length    Event Parameters
*         0x04                        0x6F                 0x01                            0x00(success/unknown)
*/
static void read_pskey_response(void* p){
    int ret = 0;
    uint8 byte, buf[4];
    fd_set read_set;
    int *fd = (int *)p;
    struct timeval tv = {.tv_sec = 4, .tv_usec = 0};
    memset(buf, 0xFF, sizeof(buf));

    for (;;) {
        FD_ZERO(&read_set);
        FD_SET(*fd, &read_set);

        ret = select(*fd + 1, &read_set, NULL, NULL, &tv);
        ALOGI("%s select got: %d", __func__, ret);
        switch (ret) {
            case -1://fd error
            goto error;
            break;

            case 0://time out
            goto error;
            break;

            default://got
            ret = read(*fd, buf, 4);
            ALOGI("%s read len: %d, result: [0x%02x, 0x%02x, 0x%02x, 0x%02x]", __func__, ret, buf[0], buf[1], buf[2], buf[3]);
            if (ret > 0 && buf[0] == HCI_TYPE_EVENT && buf[1] == HCI_EVENT_PKSEY && buf[2] == HCI_PKSEY_LEN && buf[3] == HCI_PKSEY_RS_OK) {
                psk_result = 0;
                goto done;
            } else {
                goto error;
            }
            break;

        }

    }

done:
    ALOGI("%s success", __func__);
    sem_post(&pskey_read_sem);
    return;
error:
    ALOGI("%s failed", __func__);
    sem_post(&pskey_read_sem);
    return;
}

static int read_mac_address(char *file_name, uint8 *addr) {
    char buf[MAC_ADDR_FILE_LEN] = {0};
    uint32 addr_t[MAC_ADDR_LEN] = {0};
    int i = 0;
    int fd = open(file_name, O_RDONLY);
    ALOGI("%s read file: %s", __func__, file_name);
    if (fd < 0) {
        ALOGI("%s open %s error reason: %s", __func__, file_name, strerror(errno));
        return -1;
    }
    if (read(fd, buf, MAC_ADDR_BUF_LEN) < 0) {
        ALOGI("%s read %s error reason: %s", __func__, file_name, strerror(errno));
        goto done;
    }
    if (sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", &addr_t[0], &addr_t[1], &addr_t[2], &addr_t[3], &addr_t[4], &addr_t[5]) < 0) {
        ALOGI("%s sscanf %s error reason: %s", __func__, file_name, strerror(errno));
        goto done;
    }
    for (i = 0; i < MAC_ADDR_LEN; i++) {
        addr[i] = addr_t[i] & 0xFF;
    }
    ALOGI("%s %s addr: [%02X:%02X:%02X:%02X:%02X:%02X]", __func__, file_name, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

done:
    close(fd);
    return 0;
}


/*
* random bluetooth mac address
*/
static void random_mac_addr(uint8 *addr) {
    int fd, randseed, ret, mac_rd;
    uint8 addr_t[MAC_ADDR_LEN] = {0};

    ALOGI("%s", __func__);
    /* urandom seed build */
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0){
        ALOGE("%s: open urandom fail", __func__);
    } else {
        ret = read(fd, &randseed, sizeof(randseed));
        ALOGE("%s urandom:0x%08X", __func__, randseed);
        close(fd);
    }

    /* time seed build */
    if (fd < 0 || ret < 0) {
        struct timeval tt;
        if (gettimeofday(&tt, (struct timezone *)0) > 0) {
            randseed = (unsigned int) tt.tv_usec;
        } else {
            randseed = (unsigned int) time(NULL);
        }
        ALOGI("urandom fail, using system time for randseed");
    }

   ALOGI("%s: randseed = %u",__func__, randseed);
   srand(randseed);
   mac_rd = rand();

   addr_t[0] = 0x40; /* FOR */
   addr_t[1] = 0x45; /* SPRD */
   addr_t[2] = 0xDA; /* ADDR */
   addr_t[3] = (uint8)(mac_rd & 0xFF);
   addr_t[4] = (uint8)((mac_rd >> 8) & 0xFF);
   addr_t[5] = (uint8)((mac_rd >> 16) & 0xFF);

    memcpy(addr, addr_t, MAC_ADDR_LEN);
   ALOGI("%s: MAC ADDR: [%02X:%02X:%02X:%02X:%02X:%02X]",__func__, addr_t[0], addr_t[1], addr_t[2], addr_t[3], addr_t[4], addr_t[5]);
}

static void write_mac_file(char *file_name, uint8 *addr) {
    int fd, ret, i;
    char buf[MAC_ADDR_FILE_LEN] = {0};
    uint32 addr_t[MAC_ADDR_LEN] = {0};

    ALOGI("%s write file: %s", __func__, file_name);
    fd = open(file_name, O_CREAT|O_RDWR|O_TRUNC, 0666);
    if (fd < 0) {
        ALOGI("%s open %s error reason: %s", __func__, file_name, strerror(errno));
        return;
    }

    for (i = 0; i < MAC_ADDR_LEN; i++) {
        addr_t[i] = addr[i];
    }
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X\n", addr_t[0], addr_t[1], addr_t[2], addr_t[3], addr_t[4], addr_t[5]);

    ret = write(fd, buf, MAC_ADDR_BUF_LEN + 1);
    if (ret < 0) {
        ALOGI("%s write %s error reason: %s", __func__, file_name, strerror(errno));
        close(fd);
        return;
    }

    ret = fsync(fd);
    if (ret < 0) {
        ALOGI("%s fsync %s error reason: %s", __func__, file_name, strerror(errno));
    }
    close(fd);
    if (chmod(file_name, 0664) <0 ) {
        ALOGI("%s chmod %s error reason: %s", __func__, file_name, strerror(errno));
    }
    ALOGI("%s done", __func__);
}


static void mac_address_stream_compose(uint8 *addr) {
    uint8 tmp, i, j;
    for (i = 0, j = MAC_ADDR_LEN - 1; (i < MAC_ADDR_LEN / 2) && (i != j); i++, j--) {
        tmp = addr[i];
        addr[i] = addr[j];
        addr[j] = tmp;
    }
}

static void get_mac_address(uint8 *addr){
    int ret = -1;
    uint8 addr_t[6] = {0};

    ALOGI("%s", __func__);
    /* check misc mac file exist */
    ret = access(DATMISC_MAC_ADDR_PATH, F_OK);
    if (ret != 0) {
        ALOGI("%s %s miss", __func__, DATMISC_MAC_ADDR_PATH);
        return;
    }

    /* read mac file */
    read_mac_address(DATMISC_MAC_ADDR_PATH, addr_t);


    /* compose mac stream */
    mac_address_stream_compose(addr_t);

    memcpy(addr, addr_t, MAC_ADDR_LEN);

}

/*
* hci command preload stream,  special order
*/
static void pskey_stream_compose(uint8 * buf, BT_PSKEY_CONFIG_T *bt_par) {
    int i = 0;
    uint8 *p = buf;

    ALOGI("%s", __func__);

    UINT24_TO_STREAM(p, bt_par->pskey_cmd);
    UINT8_TO_STREAM(p, (uint8)(PSKEY_PREAMBLE_SIZE & 0xFF));

    UINT8_TO_STREAM(p, bt_par->g_dbg_source_sink_syn_test_data);
    UINT8_TO_STREAM(p, bt_par->g_sys_sleep_in_standby_supported);
    UINT8_TO_STREAM(p, bt_par->g_sys_sleep_master_supported);
    UINT8_TO_STREAM(p, bt_par->g_sys_sleep_slave_supported);

    UINT32_TO_STREAM(p, bt_par->default_ahb_clk);
    UINT32_TO_STREAM(p, bt_par->device_class);
    UINT32_TO_STREAM(p, bt_par->win_ext);

    for (i = 0; i < 6; i++) {
        UINT32_TO_STREAM(p, bt_par->g_aGainValue[i]);
    }
    for (i = 0; i < 5; i++) {
        UINT32_TO_STREAM(p, bt_par->g_aPowerValue[i]);
    }

    for (i = 0; i < 16; i++) {
        UINT8_TO_STREAM(p, bt_par->feature_set[i]);
    }
    for (i = 0; i < 6; i++) {
        UINT8_TO_STREAM(p, bt_par->device_addr[i]);
    }

    UINT8_TO_STREAM(p, bt_par->g_sys_sco_transmit_mode);
    UINT8_TO_STREAM(p, bt_par->g_sys_uart0_communication_supported);
    UINT8_TO_STREAM(p, bt_par->edr_tx_edr_delay);
    UINT8_TO_STREAM(p, bt_par->edr_rx_edr_delay);

    UINT16_TO_STREAM(p, bt_par->g_wbs_nv_117);

    UINT32_TO_STREAM(p, bt_par->is_wdg_supported);

    UINT32_TO_STREAM(p, bt_par->share_memo_rx_base_addr);
    //UINT32_TO_STREAM(p, bt_par->share_memo_tx_base_addr);
    UINT16_TO_STREAM(p, bt_par->g_wbs_nv_118);
    UINT16_TO_STREAM(p, bt_par->g_nbv_nv_117);

    UINT32_TO_STREAM(p, bt_par->share_memo_tx_packet_num_addr);
    UINT32_TO_STREAM(p, bt_par->share_memo_tx_data_base_addr);

    UINT32_TO_STREAM(p, bt_par->g_PrintLevel);

    UINT16_TO_STREAM(p, bt_par->share_memo_tx_block_length);
    UINT16_TO_STREAM(p, bt_par->share_memo_rx_block_length);
    UINT16_TO_STREAM(p, bt_par->share_memo_tx_water_mark);
    //UINT16_TO_STREAM(p, bt_par->share_memo_tx_timeout_value);
    UINT16_TO_STREAM(p, bt_par->g_nbv_nv_118);

    UINT16_TO_STREAM(p, bt_par->uart_rx_watermark);
    UINT16_TO_STREAM(p, bt_par->uart_flow_control_thld);
    UINT32_TO_STREAM(p, bt_par->comp_id);
    UINT16_TO_STREAM(p, bt_par->pcm_clk_divd);


    for (i = 0; i < 8; i++) {
        UINT32_TO_STREAM(p, bt_par->reserved[i]);
    }
}

static inline int start_pskey_read_thread(int fd) {
    ALOGI("%s", __func__);
    sem_init(&pskey_read_sem, 0, -1);
    s_bt_fd = fd;
    if (pthread_create(&pskey_read_th, NULL, (void *)read_pskey_response, &s_bt_fd) != 0) {
        ALOGI("%s create pskey thread failed", __func__);
        return -1;
    }
    return 0;
}

void sprd_bt_power_ctrl(unsigned char on){
    if (on) {
        start_cp2();
    } else {
        stop_cp2();
    }
}

void lpm_wake_up(void){
    ALOGD("lpm_wake_up");
    if (btsleep_fd < 0) {
        btsleep_fd = open(VENDOR_BTWRITE_PROC_NODE, O_WRONLY);
    }
    if (btsleep_fd < 0) {
        ALOGD("sprd_config_init : open(%s) failed: %s (%d)",
            VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
    } else {
        char buffer = '1';
        if (write(btsleep_fd, &buffer, 1) < 0) {
            ALOGD("sprd_config_init : write(%s) failed: %s (%d)",
                VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
        }
    }
}

int sprd_config_init(int fd, char *bdaddr, struct termios *ti)
{
    int ret = 0;
    BT_PSKEY_CONFIG_T bt_para_tmp;
    uint8 *buf = NULL;

    if (s_bt_fd < 0) {
        ALOGD("sprd_config_init copy fd: %d", fd);
        s_bt_fd = fd;
    }

    UNUSED(bdaddr);
    UNUSED(ti);

#ifdef PA_POWER_ON
    if (pa_enable_fd < 0) {
        pa_enable_fd = open(MARLIN_PA_ENABLE_PATH, O_WRONLY);
    }
    if (pa_enable_fd < 0) {
        ALOGE("%s open(%s) for write failed: %s (%d)", __func__,
                MARLIN_PA_ENABLE_PATH, strerror(errno), errno);
    }
    else
    {
        ALOGD("sprd_config_init pa enable");
        if (write(pa_enable_fd, MARLIN_PA_ENABLE_VALUE, strlen(MARLIN_PA_ENABLE_VALUE)) < 0) {
            ALOGE("%s write(%s) failed: %s (%d)", __func__,
                    MARLIN_PA_ENABLE_VALUE, strerror(errno),errno);
        }
    }
#endif

    lpm_wake_up();

    ALOGI("init_sprd_config in \n");

    ret = bt_getPskeyFromFile(&bt_para_tmp);
    if (ret < 0) {
        ALOGE("init_sprd_config bt_getPskeyFromFile failed\n");
        memcpy(&bt_para_tmp, &bt_para_setting, sizeof(BT_PSKEY_CONFIG_T));
    }
    buf = (uint8 *)malloc(PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);
    if (buf == NULL) {
        ALOGI("%s alloc stream memory failed", __func__);
        goto error;
    }
    memset(buf, 0, PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);

    /* get bluetooth mac address */
    get_mac_address(bt_para_tmp.device_addr);

    /* compose pskey hci command pkt */
    pskey_stream_compose(buf, &bt_para_tmp);


    if (start_pskey_read_thread(fd) < 0) {
        goto error;
    }

    ALOGI("%s write pskey stream", __func__);
    ret = write(fd, buf, PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);
    free(buf);
    buf = NULL;
    ALOGI("%s write pskey stream, ret: %d", __func__, ret);
    if (ret < 0) {
        ALOGI("%s write pskey stream failed", __func__);
        goto error;
    }

    /* wait for response */
    sem_wait(&pskey_read_sem);
    if (psk_result != 0x00) {
        goto error;
    }
    ALOGI("%s success", __func__);
    return 0;

error:
    if (buf != NULL) {
        free(buf);
        buf = NULL;
    }
    ALOGE("%s failed", __func__);
    return -1;
}

void sprd_get_pskey(BT_PSKEY_CONFIG_T * pskey_t) {
    BT_PSKEY_CONFIG_T pskey;

    ALOGD("%s", __func__);
    memset(&pskey, 0 , sizeof(BT_PSKEY_CONFIG_T));
    if (bt_getPskeyFromFile(&pskey) < 0 ) {
        ALOGE("%s bt_getPskeyFromFile failed", __func__);
        memcpy(pskey_t, &bt_para_setting, sizeof(BT_PSKEY_CONFIG_T));
        return;
    }
    /* get bluetooth mac address */
    get_mac_address(pskey.device_addr);
    memcpy(pskey_t, &pskey, sizeof(BT_PSKEY_CONFIG_T));
}

void sprd_pskey_response_cb(int ok) {
    if(bt_vendor_cbacks)
    {
#ifdef SPRD_WCNBT_MARLIN
        if(ok)
        {
            ALOGI("Bluetooth Firmware and smd is initialized");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
        }
        else
        {
            ALOGE("Error : hci, smd initialization Error");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }

#elif HAS_BLUETOOTH_SPRD
        if(ok)
        {
            ALOGI("Bluetooth Firmware and smd is initialized");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_BT_SUCCESS);
        }
        else
        {
            ALOGE("Error : hci, smd initialization Error");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_BT_FAIL);
        }
#endif

    }
}
