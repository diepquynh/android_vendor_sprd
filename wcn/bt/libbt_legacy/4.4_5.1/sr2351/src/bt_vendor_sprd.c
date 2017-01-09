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

#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>
#include <cutils/sockets.h>
/******************************************************************************
**  Externs
******************************************************************************/
extern int hw_config(int nState);

extern int is_hw_ready();

/******************************************************************************
**  Variables
******************************************************************************/
int s_bt_fd = -1;

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#if (HW_NEED_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif


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
/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    ALOGI("bt-vendor : init");

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

    if(client_fd < 0)
        return client_fd;

    rcv_timeout.tv_sec = 10;
    rcv_timeout.tv_usec = 0;
    if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcv_timeout, sizeof(rcv_timeout)) < 0)
    {
        ALOGE("bt-vendor : %s: set receive timeout fail\n",__func__);
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

    if (wcnd_socket < 0) {
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

    if (wcnd_socket < 0) {
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

    ALOGI("bt-vendor : op for %d", opcode);

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

                #if 0
                nState = *(int *) param;
                retval = hw_config(nState);
                if(nState == BT_VND_PWR_ON
                   && retval == 0
                   && is_hw_ready() == TRUE)
                {
                    retval = 0;
                }
                #endif
            }
            break;

        case BT_VND_OP_FW_CFG:
#if 0
            {
                    ALOGI("bt-vendor : BT_VND_OP_FW_CFG");

                    retval = sprd_config_init(s_bt_fd,NULL,NULL);
                    ALOGI("bt-vendor : sprd_config_init retval = %d.",retval);
                    if(bt_vendor_cbacks)
                    {
                        if(retval == 0)
                        {
                            ALOGI("Bluetooth Firmware and smd is initialized");
                            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
                        }
                        else
                        {
                            ALOGE("Error : hci, smd initialization Error");
                            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
                        }
                    }
            }
#endif //0
            break;

        case BT_VND_OP_SCO_CFG:
            {
                ALOGI("bt-vendor : BT_VND_OP_SCO_CFG");
                bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
            }
            break;

        case BT_VND_OP_USERIAL_OPEN:
            {
                int idx;
                ALOGI("bt-vendor : BT_VND_OP_USERIAL_OPEN");
                if(bt_hci_init_transport(&s_bt_fd) != -1)
                {
                    int (*fd_array)[] = (int (*) []) param;

                    for (idx=0; idx < CH_MAX; idx++)
                    {
                       (*fd_array)[idx] = s_bt_fd;
                    }
                    ALOGI("bt-vendor : BT_VND_OP_USERIAL_OPEN ok ! download pskey now");

                    retval = sprd_config_init(s_bt_fd,NULL,NULL);
                    ALOGI("bt-vendor : sprd_config_init retval = %d.",retval);
                    if(bt_vendor_cbacks)
                    {
                        if(retval == 0)
                        {
                            ALOGI("Bluetooth Firmware and smd is initialized");
                            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
                            retval = 1;
                        }
                        else
                        {
                            ALOGE("Error : hci, smd initialization Error");
                            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
                            retval = -1;
                         }
                     }

                 }
                 else
                {
                     ALOGI("bt-vendor : BT_VND_OP_USERIAL_OPEN failed");
                     retval = -1;
                }
            }
            break;

        case BT_VND_OP_USERIAL_CLOSE:
            {
                ALOGI("bt-vendor : BT_VND_OP_USERIAL_CLOSE");
                bt_hci_deinit_transport(s_bt_fd);
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
                ALOGI("bt-vendor : BT_VND_OP_LPM_SET_MODE");
                bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
            }
            break;

        case BT_VND_OP_LPM_WAKE_SET_STATE:
            ALOGI("bt-vendor : BT_VND_OP_LPM_WAKE_SET_STATE");
            break;
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
BT_PSKEY_CONFIG_T bt_para_setting={
5,
0,
0,
0,
0,
0x18cba80,
0x001f00,
0x1e,
{0x7a00,0x7600,0x7200,0x5200,0x2300,0x0300},
{0XCe418CFE,
 0Xd0418CFE,0Xd2438CFE,
 0Xd4438CFE,0xD6438CFE},
{0xFF, 0xFF, 0x8D, 0xFE, 0x9B, 0xFF, 0x79, 0x83,
  0xFF, 0xA7, 0xFF, 0x7F, 0x00, 0xE0, 0xF7, 0x3E},
{0x11, 0xFF, 0x0, 0x22, 0x2D, 0xAE},
0,
1,
5,
0x0e,
0xFFFFFFFF,
0x30,
0x3f,
0,
0x65,
0x0,
0x0,
0x0,
0x0,
0x3,
0x1,
0x1,
0x2,
0x4,
0x4,
0x0,
0x12,
0x4,
0x1,
0xd0,
0xc8,
{0x0000,0x0000,0x0000,0x0000}
};


static BOOLEAN checkBluetoothAddress(char* address)
{
    int i=0;
    char add_temp[BT_RAND_MAC_LENGTH+1]={0};
    char c;
    if (address == NULL)
    {
        return FALSE;
    }

    for (i=0; i < BT_RAND_MAC_LENGTH; i++)
    {
        c=add_temp[i]=toupper(address[i]);
        switch (i % 3)
        {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
                {
                    break;
                }
                return FALSE;
            case 2:
                if (c == ':')
                {
                    break;
                }
                return FALSE;
        }
    }

    if(strstr(add_temp, MAC_ERROR)!=NULL)
        return FALSE;

    return TRUE;
}

int read_mac_from_file(const char * file_path,  char * mac)
{
    int fd_btaddr=0;
    char bt_mac[BT_RAND_MAC_LENGTH+1] = {0};

    fd_btaddr = open(file_path, O_RDWR);
    if(fd_btaddr>=0)
    {
        read(fd_btaddr, bt_mac, BT_RAND_MAC_LENGTH);
        ALOGI("bt mac read ===%s==",bt_mac);
        if(checkBluetoothAddress(bt_mac))
        {
            ALOGI("bt mac already exists, no need to random it");
            memcpy(mac,bt_mac,BT_RAND_MAC_LENGTH);
            close(fd_btaddr);
            return 1;
        }
        close(fd_btaddr);
    }
    ALOGI("bt mac read fail.");

    return 0;
}


#if 0
//******************create bt addr***********************
static void mac_rand(char *btmac)
{
    int i=0, j=0;
    unsigned int randseed;

    ALOGI("mac_rand");

    //rand seed
    randseed = (unsigned int) time(NULL);
    ALOGI("%s: randseed=%d",__FUNCTION__, randseed);
    srand(randseed);

    //FOR BT
    i=rand(); j=rand();
    ALOGI("%s:  rand i=0x%x, j=0x%x",__FUNCTION__, i,j);
    sprintf(btmac, "00:%02x:%02x:%02x:%02x:%02x", \
                (unsigned char)((i>>8)&0xFF), \
                (unsigned char)((i>>16)&0xFF), \
                (unsigned char)((j)&0xFF), \
                (unsigned char)((j>>8)&0xFF), \
                (unsigned char)((j>>16)&0xFF));
}


static  BOOLEAN write_btmac2file(const char * file_path,char *btmac)
{
    int fd = -1;
    fd = open(file_path, O_CREAT|O_RDWR|O_TRUNC, 0660);
    if(fd > 0) {
        write(fd, btmac, strlen(btmac));
        close(fd);
        return TRUE;
    }
    return FALSE;
}
#endif
uint8 ConvertHexToBin(
                    uint8        *hex_ptr,     // in: the hexadecimal format string
                    uint16       length,       // in: the length of hexadecimal string
                    uint8        *bin_ptr      // out: pointer to the binary format string
                    ){
    uint8        *dest_ptr = bin_ptr;
    uint32        i = 0;
    uint8        ch;

    for(i=0; i<length; i+=2){
        // the bit 8,7,6,5
        ch = hex_ptr[i];
        // digital 0 - 9
        if (ch >= '0' && ch <= '9')
            *dest_ptr =(uint8)((ch - '0') << 4);
            // a - f
        else if (ch >= 'a' && ch <= 'f')
            *dest_ptr = (uint8)((ch - 'a' + 10) << 4);
            // A - F
        else if (ch >= 'A' && ch <= 'F')
            *dest_ptr = (uint8)((ch -'A' + 10) << 4);
        else{
            return 0;
        }

        // the bit 1,2,3,4
        ch = hex_ptr[i+1];
        // digtial 0 - 9
        if (ch >= '0' && ch <= '9')
            *dest_ptr |= (uint8)(ch - '0');
        // a - f
        else if (ch >= 'a' && ch <= 'f')
            *dest_ptr |= (uint8)(ch - 'a' + 10);
        // A - F
        else if (ch >= 'A' && ch <= 'F')
            *dest_ptr |= (uint8)(ch -'A' + 10);
        else{
            return 0;
        }

        dest_ptr++;
    }

    return 1;
}
//******************create bt addr end***********************

int sprd_config_init(int fd, char *bdaddr, struct termios *ti)
{
    int i,psk_fd,fd_btaddr,ret = 0,r,size=0,read_btmac=0;
    int write_cnt;
    unsigned char resp[30];
    BT_PSKEY_CONFIG_T bt_para_tmp;
    char bt_mac[30] = {0};
    char bt_mac_tmp[20] = {0};
    uint8 bt_mac_bin[32]     = {0};

    ALOGI("init_sprd_config in \n");
    /*
    mac_rand(bt_mac);
    ALOGI("bt random mac=%s",bt_mac);
    printf("bt_mac=%s\n",bt_mac);
    write_btmac2file(bt_mac);

    */
    if(access(BT_MAC_FILE, F_OK) == 0)
    {
        ALOGI("%s: %s exists",__FUNCTION__, BT_MAC_FILE);
        read_btmac=read_mac_from_file(BT_MAC_FILE,bt_mac);
    }
#if 0
    if(0==read_btmac)
    {
        if(access(BT_MAC_FILE_TEMP, F_OK) == 0)
        {
            ALOGI("%s: %s exists",__FUNCTION__, BT_MAC_FILE_TEMP);
            read_btmac=read_mac_from_file(BT_MAC_FILE_TEMP,bt_mac);
        }
    }

    if(0==read_btmac)
    {
        mac_rand(bt_mac);
        if(write_btmac2file(BT_MAC_FILE_TEMP,bt_mac))
            read_btmac=1;
    }
#endif
    if(read_btmac == 1)
    {
        for(i=0; i<6; i++)
        {
            bt_mac_tmp[i*2] = bt_mac[3*(5-i)];
            bt_mac_tmp[i*2+1] = bt_mac[3*(5-i)+1];
        }
        ALOGI("====bt_mac_tmp=%s", bt_mac_tmp);
        ConvertHexToBin((uint8*)bt_mac_tmp, strlen(bt_mac_tmp), bt_mac_bin);
    }

    /* Reset the BT Chip */
    memset(resp, 0, sizeof(resp));

    ret = bt_getPskeyFromFile(&bt_para_tmp);
    if(ret != 0)
    {
        ALOGI("get_pskey_from_file fail\n");
        /* Send command from hciattach*/
        if(read_btmac == 1)
        {
            memcpy(bt_para_setting.device_addr, bt_mac_bin, sizeof(bt_para_setting.device_addr));
        }
        if (write(fd, (char *)&bt_para_setting, sizeof(BT_PSKEY_CONFIG_T)) != sizeof(BT_PSKEY_CONFIG_T))
        {
            ALOGI("Failed to write pskey command from default arry\n");
            return -1;
        }
    }
    else
    {
        ALOGI("get_pskey_from_file ok, fd = %d \n", fd);
        /* Send command from pskey_bt.txt*/
        if(read_btmac == 1)
        {
            memcpy(bt_para_tmp.device_addr, bt_mac_bin, sizeof(bt_para_tmp.device_addr));
        }

        ALOGI("start to write pskey");
        write_cnt = write(fd, (char *)&bt_para_tmp, sizeof(BT_PSKEY_CONFIG_T));
        if(write_cnt < 0){
            ALOGI("Failed to write pskey, return %d(%s)", write_cnt,strerror(errno));
            return -1;
        }
        if(write_cnt != sizeof(BT_PSKEY_CONFIG_T)){
            ALOGI("write pskey size error, writed %d, expect %d", write_cnt,sizeof(BT_PSKEY_CONFIG_T));
            return -1;
        }
        ALOGI("pskey(%d bytes) has been send to controller\n", write_cnt);
    }
    ALOGI("sprd_config_init write pskey ok\n");

    while (1)
    {
        ALOGI("reading ACK from controller...\n");
        r = read(fd, resp, 1);
        if (r <= 0){
            ALOGI("Failed to read ack, return %d(%s)", r ,strerror(errno));
            return -1;
        }
        ALOGI("read %d bytes from controller\n",r);
        if (resp[0] == 0x05){
            ALOGI("read pskey ACK(0x05) ok \n");
            break;
        }else{
            ALOGI("read ACK(0x%x)is not expect,retry\n,",resp[0]);
        }
    }

    ALOGI("sprd_config_init ok \n");

    return 0;
}

