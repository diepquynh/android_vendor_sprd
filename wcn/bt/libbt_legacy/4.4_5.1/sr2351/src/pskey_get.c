#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LOG_TAG "pskey"
#include "cutils/log.h"

#include "bt_vendor_sprd.h"
#define BT_PSKEY_TRACE_BUF_SIZE 256
#define MAX_BOARD_TYPE_LEN 32
#define MAX_PSKEY_PATH_LEN 100

#define _FILE_PARSE_DEBUG_
#define  CMD_ITEM_TABLE(ITEM, MEM_OFFSET, TYPE)    { ITEM,   (unsigned long)( &(  ((BT_PSKEY_CONFIG_T *)(0))->MEM_OFFSET )),   TYPE }

typedef struct
{
    char item[64];
    int  par[32];
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
    CMD_ITEM_TABLE("pskey_cmd", pskey_cmd, 1),
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
    CMD_ITEM_TABLE("g_sys_sco_transmit_mode", g_sys_sco_transmit_mode, 1),
    CMD_ITEM_TABLE("g_sys_uart0_communication_supported", g_sys_uart0_communication_supported, 1),
    CMD_ITEM_TABLE("edr_tx_edr_delay", edr_tx_edr_delay, 1),
    CMD_ITEM_TABLE("edr_rx_edr_delay", edr_rx_edr_delay, 1),
    CMD_ITEM_TABLE("g_PrintLevel", g_PrintLevel, 4),
    CMD_ITEM_TABLE("uart_rx_watermark", uart_rx_watermark, 2),
    CMD_ITEM_TABLE("uart_flow_control_thld", uart_flow_control_thld, 2),
    CMD_ITEM_TABLE("comp_id", comp_id, 4),
    CMD_ITEM_TABLE("pcm_clk_divd", pcm_clk_divd, 2),
    CMD_ITEM_TABLE("half_word_reserved", half_word_reserved, 2),
    CMD_ITEM_TABLE("pcm_config", pcm_config, 4),
    CMD_ITEM_TABLE("ref_clk", ref_clk, 1),
    CMD_ITEM_TABLE("FEM_status", FEM_status, 1),
    CMD_ITEM_TABLE("gpio_cfg", gpio_cfg, 1),
    CMD_ITEM_TABLE("gpio_PA_en", gpio_PA_en, 1),
    CMD_ITEM_TABLE("wifi_tx", wifi_tx, 1),
    CMD_ITEM_TABLE("bt_tx", bt_tx, 1),
    CMD_ITEM_TABLE("wifi_rx", wifi_rx, 1),
    CMD_ITEM_TABLE("bt_rx", bt_rx, 1),
    CMD_ITEM_TABLE("wb_lna_bypass", wb_lna_bypass, 1),
    CMD_ITEM_TABLE("gain_LNA", gain_LNA, 1),
    CMD_ITEM_TABLE("IL_wb_lna_bypass", IL_wb_lna_bypass, 1),
    CMD_ITEM_TABLE("Rx_adaptive", Rx_adaptive, 1),
    CMD_ITEM_TABLE("up_bypass_switching_point0", up_bypass_switching_point0, 1),
    CMD_ITEM_TABLE("low_bypass_switching_point0", low_bypass_switching_point0, 1),

    CMD_ITEM_TABLE("bt_reserved", reserved[0], 4),

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
        if( (1 == cType) || ( 2 == cType) || (3 == cType)  )
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
                cmd->par[cmd->num] = strtoul(tmp,NULL, 0);
                cmd->num++;
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
    sprintf(tmp, "###[pskey]%s, offset:%d, num:%d, value:   ", pTable->item, pTable->mem_offset, cmd->num);
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
    int i;
    char tmp[BT_PSKEY_TRACE_BUF_SIZE];
    char string[16];

    ALOGI("pskey_cmd:0x%x\n", p->pskey_cmd);
    ALOGI("g_dbg_source_sink_syn_test_data:0x%x\n", p->g_dbg_source_sink_syn_test_data);
    ALOGI("g_sys_sleep_in_standby_supported:0x%x\n", p->g_sys_sleep_in_standby_supported);
    ALOGI("g_sys_sleep_master_supported:0x%x\n", p->g_sys_sleep_master_supported);
    ALOGI("g_sys_sleep_slave_supported:0x%x\n", p->g_sys_sleep_slave_supported);
    ALOGI("default_ahb_clk:0x%x\n",  p->default_ahb_clk);
    ALOGI("device_class:0x%x\n", p->device_class);
    ALOGI("win_ext:0x%x\n", p->win_ext);
    memset(tmp,0, BT_PSKEY_TRACE_BUF_SIZE);
    memset(string,0,16);
    sprintf(tmp, "g_aGainValue: ");
    for(i=0; i<6; i++)
    {
        sprintf(string, "0x%x, ", p->g_aGainValue[i]);
        strcat(tmp, string);
    }
    ALOGI("%s\n", tmp);

    memset(tmp,0, BT_PSKEY_TRACE_BUF_SIZE);
    memset(string,0,16);
    sprintf(tmp, "g_aPowerValue: ");
    for(i=0; i<5; i++)
    {
        sprintf(string, "0x%x, ", p->g_aPowerValue[i]);
        strcat(tmp, string);
    }
    ALOGI("%s\n", tmp);
      memset(tmp,0, BT_PSKEY_TRACE_BUF_SIZE);
    memset(string,0,16);
    sprintf(tmp, "feature_set: ");
    for(i=0; i<16; i++)
    {
        sprintf(string, "0x%x, ", p->feature_set[i]);
        strcat(tmp, string);
    }
    ALOGI("%s\n", tmp);

    memset(tmp,0, BT_PSKEY_TRACE_BUF_SIZE);
    memset(string,0,16);
    sprintf(tmp, "device_addr: ");
    for(i=0; i<6; i++)
    {
        sprintf(string, "0x%x, ", p->device_addr[i]);
        strcat(tmp, string);
    }
    ALOGI("%s\n", tmp);

    ALOGI("g_sys_sco_transmit_mode:0x%x\n", p->g_sys_sco_transmit_mode);
    ALOGI("g_sys_uart0_communication_supported:0x%x\n",  p->g_sys_uart0_communication_supported);
    ALOGI("edr_tx_edr_delay:0x%x\n", p->edr_tx_edr_delay);
    ALOGI("edr_rx_edr_delay:0x%x\n", p->edr_rx_edr_delay);
    ALOGI("g_PrintLevel:0x%x\n", p->g_PrintLevel);
    ALOGI("uart_rx_watermark:0x%x\n", p->uart_rx_watermark);
    ALOGI("uart_flow_control_thld:0x%x\n", p->uart_flow_control_thld);
    ALOGI("comp_id:0x%x\n", p->comp_id);
    ALOGI("pcm_clk_divd:0x%x\n", p->pcm_clk_divd);
    ALOGI("half_word_reserved:0x%x\n", p->half_word_reserved);
    ALOGI("pcm_config:0x%x\n", p->pcm_config);

    ALOGI("ref_clk:0x%x\n", p->ref_clk);
    ALOGI("FEM_status:0x%x\n", p->FEM_status);
    ALOGI("gpio_cfg:0x%x\n", p->gpio_cfg);
    ALOGI("gpio_PA_en:0x%x\n", p->gpio_PA_en);
    ALOGI("wifi_tx:0x%x\n", p->wifi_tx);
    ALOGI("bt_tx:0x%x\n", p->bt_tx);
    ALOGI("wifi_rx:0x%x\n", p->wifi_rx);
    ALOGI("bt_rx:0x%x\n", p->bt_rx);
    ALOGI("wb_lna_bypass:0x%x\n", p->wb_lna_bypass);
    ALOGI("gain_LNA:0x%x\n", p->gain_LNA);
    ALOGI("IL_wb_lna_bypass:0x%x\n", p->IL_wb_lna_bypass);
    ALOGI("Rx_adaptive:0x%x\n", p->Rx_adaptive);
    ALOGI("up_bypass_switching_point0:0x%x\n", p->up_bypass_switching_point0);
    ALOGI("low_bypass_switching_point0:0x%x\n", p->low_bypass_switching_point0);

    memset(tmp,0, BT_PSKEY_TRACE_BUF_SIZE);
    memset(string,0,16);
    sprintf(tmp, "reserved: ");
    for(i=0; i<4; i++)
    {
        sprintf(string, "0x%x, ", p->reserved[i]);
        strcat(tmp, string);
    }
    ALOGI("%s\n", tmp);

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
    char ssp_property[128] = {0};
    char pskey_path[MAX_PSKEY_PATH_LEN] = {0};

#ifdef HW_ADC_ADAPT_SUPPORT
    char *CFG_2351_PATH[] = {
        "/system/etc/connectivity_configure_hw100.ini",
        "/system/etc/connectivity_configure_hw102.ini",
        "/system/etc/connectivity_configure_hw104.ini"
    };
    char *CFG_2351_PRODUCTINFO_PATH[] = {
        "/productinfo/connectivity_configure_hw100.ini",
        "/productinfo/connectivity_configure_hw102.ini",
        "/productinfo/connectivity_configure_hw104.ini"
    };
#else
    char *CFG_2351_PATH[] = {
        "/system/etc/connectivity_configure.ini"
    };
    char *CFG_2351_PRODUCTINFO_PATH[] = {
        "/productinfo/connectivity_configure.ini"
    };
#endif


#ifdef HW_ADC_ADAPT_SUPPORT

    char *BOARD_TYPE_PATH = "/dev/board_type";
    int fd_board_type;
    char board_type_str[MAX_BOARD_TYPE_LEN] = {0};

    fd_board_type = open(BOARD_TYPE_PATH, O_RDONLY);
    if (fd_board_type<0)
    {
        ALOGI("#### %s file open %s err ####\n", __FUNCTION__, BOARD_TYPE_PATH);
        board_type = 2; // default is 1.0.4
    }
    else
    {
        len = read(fd_board_type, board_type_str, MAX_BOARD_TYPE_LEN);
        if (strstr(board_type_str, "1.0.0"))
        {
            board_type = 0;
        }
        else if (strstr(board_type_str, "1.0.2"))
        {
            board_type = 1;
        }
        else
        {
            board_type = 2; // default is 1.0.4
        }
        ALOGI("#### %s get board type len %d %s type %d ####\n", __FUNCTION__, len, board_type_str, board_type);

        close(fd_board_type);
    }
#endif

    ALOGI("begin to bt_getPskeyFromFile");
    if(access(CFG_2351_PRODUCTINFO_PATH[board_type], F_OK|R_OK) == 0)
        strcpy(pskey_path, CFG_2351_PRODUCTINFO_PATH[board_type]);
    else
        strcpy(pskey_path, CFG_2351_PATH[board_type]);

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

    ALOGI("SSP setting from pskey: 0x%x", ((BT_PSKEY_CONFIG_T *)pData)->feature_set[6]);

    ret = property_get("persist.sys.bt.non.ssp", ssp_property, "close");
    if(ret >= 0 && !strcmp(ssp_property, "open")) {
        ALOGI("### disable BT SSP function due to property setting ###");
        ((BT_PSKEY_CONFIG_T *)pData)->feature_set[6] &= 0xF7;
    }

    ALOGI("begin to dumpPskey");
    bt_dumpPskey((BT_PSKEY_CONFIG_T *)pData);
    free(pBuf);
    return 0;
}
