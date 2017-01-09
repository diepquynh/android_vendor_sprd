#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cutils/properties.h>

#define LOG_TAG "pskey"
#include "cutils/log.h"

#include "bt_vendor_sprd.h"
#define BT_PSKEY_TRACE_BUF_SIZE 256
#define MAX_BOARD_TYPE_LEN 32
#define MAX_PSKEY_PATH_LEN 100

#define _FILE_PARSE_DEBUG_
#define  CMD_ITEM_TABLE(ITEM, MEM_OFFSET, TYPE)    { ITEM,   (unsigned long)( &(  ((BT_PSKEY_CONFIG_T *)(0))->MEM_OFFSET )),   TYPE }

#define PSKEY_PATH_SYSTEM  "/system/etc/connectivity_configure.ini"
#define PSKEY_PATH_PRODUCTINFO  "/productinfo/connectivity_configure.ini"
#define PSKEY_PATH_SYSTEM_BA  "/system/etc/marlinba/connectivity_configure.ini"
#define PSKEY_PATH_PRODUCTINFO_BA  "/productinfo/marlinba/connectivity_configure.ini"
#define MARLIN_HW_VER_AA  "AA"
#define MARLIN_HW_VER_BA  "BA"
#define MARLIN_HW_VER_OTHER  "00"

typedef struct
{
    char item[64];
    uint32  par[32];
    int  num;
}cmd_par;

typedef struct
{
    char *item;
    unsigned long mem_offset;
    int type;
}cmd_par_table;

static cmd_par_table g_pskey_table[] =
{
    CMD_ITEM_TABLE("pskey_cmd", pskey_cmd, 4),

    CMD_ITEM_TABLE("g_dbg_source_sink_syn_test_data", g_dbg_source_sink_syn_test_data, 1),
    CMD_ITEM_TABLE("g_sys_sleep_in_standby_supported", g_sys_sleep_in_standby_supported, 1),
    CMD_ITEM_TABLE("g_sys_sleep_master_supported", g_sys_sleep_master_supported, 1),
    CMD_ITEM_TABLE("g_sys_sleep_slave_supported", g_sys_sleep_slave_supported, 1),

    CMD_ITEM_TABLE("default_ahb_clk", default_ahb_clk, 4),
    CMD_ITEM_TABLE("device_class", device_class, 4),
    CMD_ITEM_TABLE("win_ext", win_ext, 4),

    CMD_ITEM_TABLE("g_aGainValue", g_aGainValue, 4),
    CMD_ITEM_TABLE("g_aPowerValue", g_aPowerValue, 4),

    CMD_ITEM_TABLE("feature_set", feature_set, 1),
    CMD_ITEM_TABLE("device_addr", device_addr, 1),

    CMD_ITEM_TABLE("g_sys_sco_transmit_mode", g_sys_sco_transmit_mode, 1), //true tramsmit by uart, otherwise by share memory
    CMD_ITEM_TABLE("g_sys_uart0_communication_supported", g_sys_uart0_communication_supported, 1), //true use uart0, otherwise use uart1 for debug
    CMD_ITEM_TABLE("edr_tx_edr_delay", edr_tx_edr_delay, 1),
    CMD_ITEM_TABLE("edr_rx_edr_delay", edr_rx_edr_delay, 1),

    CMD_ITEM_TABLE("g_wbs_nv_117", g_wbs_nv_117, 2),

    CMD_ITEM_TABLE("is_wdg_supported", is_wdg_supported, 4),

    CMD_ITEM_TABLE("share_memo_rx_base_addr", share_memo_rx_base_addr, 4),
    //CMD_ITEM_TABLE("share_memo_tx_base_addr", share_memo_tx_base_addr, 4),

    CMD_ITEM_TABLE("g_wbs_nv_118", g_wbs_nv_118, 2),
    CMD_ITEM_TABLE("g_nbv_nv_117", g_nbv_nv_117, 2),

    CMD_ITEM_TABLE("share_memo_tx_packet_num_addr", share_memo_tx_packet_num_addr, 4),
    CMD_ITEM_TABLE("share_memo_tx_data_base_addr", share_memo_tx_data_base_addr, 4),

    CMD_ITEM_TABLE("g_PrintLevel", g_PrintLevel, 4),

    CMD_ITEM_TABLE("share_memo_tx_block_length", share_memo_tx_block_length, 2),
    CMD_ITEM_TABLE("share_memo_rx_block_length", share_memo_rx_block_length, 2),
    CMD_ITEM_TABLE("share_memo_tx_water_mark", share_memo_tx_water_mark, 2),
    //CMD_ITEM_TABLE("share_memo_tx_timeout_value", share_memo_tx_timeout_value, 2),
    CMD_ITEM_TABLE("g_nbv_nv_118", g_nbv_nv_118, 2),

    CMD_ITEM_TABLE("uart_rx_watermark", uart_rx_watermark, 2),
    CMD_ITEM_TABLE("uart_flow_control_thld", uart_flow_control_thld, 2),
    CMD_ITEM_TABLE("comp_id", comp_id, 4),
    CMD_ITEM_TABLE("pcm_clk_divd", pcm_clk_divd, 2),

    CMD_ITEM_TABLE("bt_reserved", reserved, 4)
};

static int bt_getFileSize(char *file)
{
    struct stat temp;
    stat(file, &temp);
    return temp.st_size;
}

static int bt_find_type(char key)
{
    if( (key >= 'a' && key <= 'w') || (key >= 'y' && key <= 'z') || (key >= 'A' && key <= 'W') || (key >= 'Y' && key <= 'Z') || ('_' == key) )
        return 1;
    if( (key >= '0' && key <= '9') || ('-' == key) )
        return 2;
    if( ('x' == key) || ('X' == key) || ('.' == key) )
        return 3;
    if( (key == '\0') || ('\r' == key) || ('\n' == key) || ('#' == key) )
        return 4;
    return 0;
}

static void bt_getCmdOneline(unsigned char *str, cmd_par *cmd)
{
    int i, j, bufType, cType, flag;
    char tmp[BT_PSKEY_TRACE_BUF_SIZE];
    char c;
    bufType = -1;
    cType = 0;
    flag = 0;
    memset( cmd, 0, sizeof(cmd_par) );

    for(i = 0, j = 0; ; i++)
    {
        c = str[i];
        cType = bt_find_type(c);
        if( (1 == cType) || ( 2 == cType) || (3 == cType) )
        {
            tmp[j] = c;
            j++;
            if(-1 == bufType)
            {
                if(2 == cType)
                    bufType = 2;
                else
                    bufType = 1;
            }
            else if(2 == bufType)
            {
                if(1 == cType)
                    bufType = 1;
            }
            continue;
        }
        if(-1 != bufType)
        {
            tmp[j] = '\0';

            if((1 == bufType) && (0 == flag) )
            {
                strcpy(cmd->item, tmp);
                flag = 1;
            }
            else
            {
                /* compatible with  HEX */
                if (tmp[0] == '0' && (tmp[1] == 'x' || tmp[1] == 'X')) {
                cmd->par[cmd->num] = strtoul(tmp, 0, 16) & 0xFFFFFFFF;
                cmd->num++;
                } else {
                    cmd->par[cmd->num] = strtoul(tmp, 0, 10) & 0xFFFFFFFF;
                    cmd->num++;
                 }
            }
            bufType = -1;
            j = 0;
        }
        if(0 == cType )
            continue;
        if(4 == cType)
            return;
        }
        return;
}

static int bt_getDataFromCmd(cmd_par_table *pTable, cmd_par *cmd,  void *pData)
{
    int i;
    unsigned char  *p;
    if( (1 != pTable->type)  && (2 != pTable->type) && (4 != pTable->type) )
        return -1;
    p = (unsigned char *)(pData) + pTable->mem_offset;
#ifdef _FILE_PARSE_DEBUG_
    char tmp[BT_PSKEY_TRACE_BUF_SIZE] = {0};
    char string[16] = {0};
    sprintf(tmp, "###[pskey]%s, offset:%lu, num:%d, value:   ", pTable->item, pTable->mem_offset, cmd->num);
    for(i=0; i<cmd->num; i++)
    {
        memset(string, 0, 16);
        sprintf(string, "0x%x, ", cmd->par[i] );
        strcat(tmp, string);
    }
    ALOGI("%s\n", tmp);
#endif
    for(i = 0; i < cmd->num;  i++)
    {
        if(1 == pTable->type)
            *((unsigned char *)p + i) = (unsigned char)(cmd->par[i]);
        else if(2 == pTable->type)
            *((unsigned short *)p + i) = (unsigned short)(cmd->par[i]);
        else if(4 == pTable->type)
            *( (unsigned int *)p + i) = (unsigned int)(cmd->par[i]);
        else
            ALOGE("%s, type err\n", __func__);
    }
    return 0;
}

static cmd_par_table *bt_cmd_table_match(cmd_par *cmd)
{
    int i;
    cmd_par_table *pTable = NULL;
    int len = sizeof(g_pskey_table) / sizeof(cmd_par_table);
    if(NULL == cmd->item)
        return NULL;
    for(i = 0; i < len; i++)
    {
        if(NULL == g_pskey_table[i].item)
            continue;
        if( 0 != strcmp( g_pskey_table[i].item, cmd->item ) )
            continue;
        pTable = &g_pskey_table[i];
        break;
    }
    return pTable;
}


static int bt_getDataFromBuf(void *pData, unsigned char *pBuf, int file_len)
{
    int i, p;
    cmd_par cmd;
    cmd_par_table *pTable = NULL;
    if((NULL == pBuf) || (0 == file_len) || (NULL == pData) )
        return -1;
    for(i = 0, p = 0; i < file_len; i++)
    {
        if( ('\n' == *(pBuf + i)) || ( '\r' == *(pBuf + i)) || ( '\0' == *(pBuf + i) )   )
        {
            if(5 <= (i - p) )
            {
                bt_getCmdOneline((pBuf + p), &cmd);
                pTable = bt_cmd_table_match(&cmd);
                if(NULL != pTable)
                {
                    bt_getDataFromCmd(pTable, &cmd, pData);
                }
            }
            p = i + 1;
        }

    }
    return 0;
}

static int bt_dumpPskey(BT_PSKEY_CONFIG_T *p)
{
    ALOGI("pskey_cmd: 0x%08X", p->pskey_cmd);

    ALOGI("g_dbg_source_sink_syn_test_data: 0x%02X", p->g_dbg_source_sink_syn_test_data);
    ALOGI("g_sys_sleep_in_standby_supported: 0x%02X", p->g_sys_sleep_in_standby_supported);
    ALOGI("g_sys_sleep_master_supported: 0x%02X", p->g_sys_sleep_master_supported);
    ALOGI("g_sys_sleep_slave_supported: 0x%02X", p->g_sys_sleep_slave_supported);

    ALOGI("default_ahb_clk: %d", p->default_ahb_clk);
    ALOGI("device_class: 0x%08X", p->device_class);
    ALOGI("win_ext: 0x%08X", p->win_ext);

    ALOGI("g_aGainValue: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X", p->g_aGainValue[0], p->g_aGainValue[1], p->g_aGainValue[2], p->g_aGainValue[3], p->g_aGainValue[4], p->g_aGainValue[5]);
    ALOGI("g_aPowerValue: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X", p->g_aPowerValue[0], p->g_aPowerValue[1], p->g_aPowerValue[2], p->g_aPowerValue[3], p->g_aPowerValue[4]);


    ALOGI("feature_set(0~7): 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", p->feature_set[0], p->feature_set[1], p->feature_set[2],
            p->feature_set[3], p->feature_set[4], p->feature_set[5], p->feature_set[6], p->feature_set[7]);
    ALOGI("feature_set(8~15): 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", p->feature_set[8], p->feature_set[9], p->feature_set[10],
            p->feature_set[11], p->feature_set[12], p->feature_set[13], p->feature_set[14], p->feature_set[15]);
    ALOGI("device_addr: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", p->device_addr[0], p->device_addr[1], p->device_addr[2], p->device_addr[3], p->device_addr[4], p->device_addr[5]);

    ALOGI("g_sys_sco_transmit_mode: 0x%02X", p->g_sys_sco_transmit_mode);
    ALOGI("g_sys_uart0_communication_supported: 0x%02X", p->g_sys_uart0_communication_supported);
    ALOGI("edr_tx_edr_delay: %d", p->edr_tx_edr_delay);
    ALOGI("edr_rx_edr_delay: %d", p->edr_rx_edr_delay);

    ALOGI("g_wbs_nv_117 : 0x%04X", p->g_wbs_nv_117 );

    ALOGI("is_wdg_supported: 0x%08X", p->is_wdg_supported);

    ALOGI("share_memo_rx_base_addr: 0x%08X", p->share_memo_rx_base_addr);
    //ALOGI("share_memo_tx_base_addr: 0x%08X", p->share_memo_tx_base_addr);
    ALOGI("g_wbs_nv_118 : 0x%04X", p->g_wbs_nv_118 );
    ALOGI("g_nbv_nv_117 : 0x%04X", p->g_nbv_nv_117 );


    ALOGI("share_memo_tx_packet_num_addr: 0x%08X", p->share_memo_tx_packet_num_addr);
    ALOGI("share_memo_tx_data_base_addr: 0x%08X", p->share_memo_tx_data_base_addr);

    ALOGI("g_PrintLevel: 0x%08X", p->g_PrintLevel);

    ALOGI("share_memo_tx_block_length: 0x%04X", p->share_memo_tx_block_length);
    ALOGI("share_memo_rx_block_length: 0x%04X", p->share_memo_rx_block_length);
    ALOGI("share_memo_tx_water_mark: 0x%04X", p->share_memo_tx_water_mark);
    //ALOGI("share_memo_tx_timeout_value: 0x%04X", p->share_memo_tx_timeout_value);
    ALOGI("g_nbv_nv_118 : 0x%04X", p->g_nbv_nv_118 );

    ALOGI("uart_rx_watermark: %d", p->uart_rx_watermark);
    ALOGI("uart_flow_control_thld: %d", p->uart_flow_control_thld);
    ALOGI("comp_id: 0x%08X", p->comp_id);
    ALOGI("pcm_clk_divd : 0x%04X", p->pcm_clk_divd );


    ALOGI("reserved(0~7): 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", p->reserved[0], p->reserved[1], p->reserved[2],
            p->reserved[3], p->reserved[4], p->reserved[5], p->reserved[6], p->reserved[7]);
    return 0;
}

static int bt_get_config_ver(unsigned char *pBuf, int len)
{
    int i, p;
    cmd_par cmd;
    int ret = -1;
    for(i = 0, p = 0; i < len; i++)
    {
        if( ('\n' == *(pBuf + i)) || ( '\r' == *(pBuf + i)) || ( '\0' == *(pBuf + i) )   )
        {
            if(5 <= (i - p) )
            {
                bt_getCmdOneline((pBuf + p), &cmd);
                if( 0 == strcmp(cmd.item, "version") )
                {
                    ret = cmd.par[0];
                    break;
                }
                memset(&cmd, 0, sizeof(cmd_par) );
            }
            p = i + 1;
        }
    }
    return ret;
}

int bt_getPskeyFromFile(void *pData)
{
    int ret = -1;
    int fd;
    unsigned char *pBuf = NULL;
    int len;
    int board_type=0;
    char pskey_path[MAX_PSKEY_PATH_LEN] = {0};
    char value[PROP_VALUE_MAX] = {0};
    int ini_prodnv = 0;

    property_get("persist.ini.connectivity.prodnv",value,"0");
    ini_prodnv = atoi(value);
    ALOGI("%s, ini_prodnv = %d", __func__, ini_prodnv);

#ifdef GET_MARLIN_CHIPID
    property_get("marlin.hardware.version", value, "00");
    ALOGI("%s, marlin.hardware.version = %s", __func__, value);

    if(!strcmp(MARLIN_HW_VER_BA, value) && ini_prodnv) {
        ALOGI("%s, MARLIN_HW_VER_BA", __func__);
        if(!access(PSKEY_PATH_PRODUCTINFO_BA, F_OK|R_OK))
            strcpy(pskey_path, PSKEY_PATH_PRODUCTINFO_BA);
        else {
            ALOGI("%s, PSKEY_PATH_PRODUCTINFO_BA is not exist", __func__);
            strcpy(pskey_path, PSKEY_PATH_SYSTEM);
        }
    }
    else if(ini_prodnv) {
        if(!access(PSKEY_PATH_PRODUCTINFO, F_OK|R_OK))
            strcpy(pskey_path, PSKEY_PATH_PRODUCTINFO);
        else {
            ALOGI("%s, PSKEY_PATH_PRODUCTINFO is not exist", __func__);
            strcpy(pskey_path, PSKEY_PATH_SYSTEM);
        }
    }
    else if(!strcmp(MARLIN_HW_VER_BA, value) && !ini_prodnv) {
        ALOGI("%s, MARLIN_HW_VER_AA", __func__);
        if(!access(PSKEY_PATH_SYSTEM_BA, F_OK|R_OK))
            strcpy(pskey_path, PSKEY_PATH_SYSTEM_BA);
        else {
            ALOGI("%s, PSKEY_PATH_SYSTEM_BA is not exist", __func__);
            strcpy(pskey_path, PSKEY_PATH_SYSTEM);
        }
    }
    else {
        ALOGI("%s, MARLIN_HW_VER_AA", __func__);
        strcpy(pskey_path, PSKEY_PATH_SYSTEM);
    }
#else

    ALOGI("%s, strcpy", __func__);
    if(ini_prodnv)
        strcpy(pskey_path, PSKEY_PATH_PRODUCTINFO);
    else
        strcpy(pskey_path, PSKEY_PATH_SYSTEM);
#endif

    ALOGI("bt_getPskeyFromFile: file path is %s", pskey_path);
    fd = open(pskey_path, O_RDONLY, 0644);

    if(-1 != fd)
    {
        len = bt_getFileSize(pskey_path);
        pBuf = (unsigned char *)malloc(len);
        ret = read(fd, pBuf, len);
        if(-1 == ret)
        {
            ALOGE("%s read %s ret:%d\n", __FUNCTION__, pskey_path, ret);
            free(pBuf);
            close(fd);
            return -1;
        }
        close(fd);
    }
    else
    {
        ALOGE("%s open %s ret:%d\n", __FUNCTION__, pskey_path, fd);
        return -1;
    }

    ret = bt_getDataFromBuf(pData, pBuf, len);
    if(-1 == ret)
    {
        free(pBuf);
        return -1;
    }
    ALOGI("begin to dumpPskey");
    bt_dumpPskey((BT_PSKEY_CONFIG_T *)pData);
    free(pBuf);
    return 0;
}
