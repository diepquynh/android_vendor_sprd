#include <fcntl.h>
#include <hardware/bluetooth.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bt_eng_sprd.h"
#include "bt_engpc_sprd.h"

#define ENG_LOG BTENG_LOGD

#define BT_EUT_SLEEP_MAX_COUNT (50)

static BTEUT_TX_ELEMENT g_bteut_tx = {.pattern = BT_EUT_TX_PATTERN_DEAFULT_VALUE,
                                      .channel = BT_EUT_TX_CHANNEL_DEAFULT_VALUE,
                                      .pkttype = BT_EUT_TX_PKTTYPE_DEAFULT_VALUE,
                                      .pktlen = BT_EUT_PKTLEN_DEAFULT_VALUE,
                                      .txpwr.power_type = BT_EUT_POWER_TYPE_DEAFULT_VALUE,
                                      .txpwr.power_value = BT_EUT_POWER_VALUE_DEAFULT_VALUE };
static BTEUT_RX_ELEMENT g_bteut_rx = {.pattern = BT_EUT_RX_PATTERN_DEAFULT_VALUE,
                                      .channel = BT_EUT_RX_CHANNEL_DEAFULT_VALUE,
                                      .pkttype = BT_EUT_RX_PKTTYPE_DEAFULT_VALUE,
                                      .rxgain.mode = BT_EUT_RX_RXGAIN_DEAFULT_VALUE,
                                      .addr = BT_EUT_RX_ADDR_DEAFULT_VALUE };
static BTEUT_RX_DATA g_bt_rx_data = { 0x00 };

static bteut_txrx_status g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
static bteut_testmode g_bteut_testmode = BTEUT_TESTMODE_LEAVE;

static bt_status_t g_bt_status;
/* Set to 1 when the Bluedroid stack is enabled */
// static unsigned char g_bteut_bt_enabled = 0;

/* current is Classic or BLE */
static bteut_bt_mode g_bt_mode = BTEUT_BT_MODE_OFF;
static bteut_eut_running g_bteut_runing = BTEUT_EUT_RUNNING_OFF;

/**************************Function Definition***************************/
static void bt_build_err_resp(char *resp) {
    snprintf(resp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%s",
             (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_ERROR : EUT_BT_ERROR), "error");
}

int get_sub_str(const char *buf, char **revdata, char a, char *delim, unsigned char count,
                unsigned char substr_max_len) {
    int len, len1, len2;
    char *start = NULL;
    char *substr = NULL;
    char *end = buf;
    int str_len = strlen(buf);

    start = strchr(buf, a);
    substr = strstr(buf, delim);

    if (!substr) {
        /* if current1 not exist, return this function.*/
        return 0;
    }

    while (end && *end != '\0') {
        end++;
    }

    if ((NULL != start) && (NULL != end)) {
        char *tokenPtr = NULL;
        unsigned int index = 1; /*must be inited by 1, because data[0] is command name */

        start++;
        substr++;
        len = substr - start - 1;

        /* get cmd name */
        memcpy(revdata[0], start, len);

        /* get sub str by delimeter */
        tokenPtr = strtok(substr, delim);
        while (NULL != tokenPtr && index < count) {
            strncpy(revdata[index++], tokenPtr, substr_max_len);

            /* next */
            tokenPtr = strtok(NULL, delim);
        }
    }

    return 0;
}

int get_cmd_index(char *buf) {
    int i = 0;
    int index = -1;
    char *start = NULL;
    char *cur = NULL;
    int str_len = 0;
    char name_str[64 + 1] = { 0x00 };

    start = strchr(buf, '=');
    if (NULL == start) {
        ENG_LOG("ADL leaving %s(), start is NULL, buf = %s", __func__, buf);
        return -1;
    }

    /* search ',' '?' '\r' */
    /* skip '=' */
    start++;

    if (NULL != (cur = strchr(buf, '?'))) {
        cur++; /* include '?' to name_str */
        str_len = cur - start;
        strncpy(name_str, (char *)start, str_len);
    } else if (NULL != (cur = strchr(buf, ',')) || NULL != (cur = strchr(buf, '\r'))) {
        str_len = cur - start;
        strncpy(name_str, (char *)start, str_len);
    } else {
        ENG_LOG("ADL leaving %s(), cmd is error, buf = %s", __func__, buf);
        return -1;
    }

    for (i = 0; i < (int)NUM_ELEMS(bt_eut_cmds); i++) {
        if (0 == strcmp(name_str, bt_eut_cmds[i].name)) {
            index = bt_eut_cmds[i].index;
            break;
        }
    }

    return index;
}

void bt_eut_parse(char *buf, char *rsp) {
    // spreadtrum
    char args0[32 + 1] = { 0x00 };
    char args1[32 + 1] = { 0x00 };
    char args2[32 + 1] = { 0x00 };
    char args3[32 + 1] = { 0x00 };

    char *data[4] = { args0, args1, args2, args3 };
    int cmd_index = -1;
    ENG_LOG("\r\n");
    ENG_LOG("ADL entry %s(), buf = %s", __func__, buf);

    get_sub_str(buf, data, '=', ",", 4, 32);
    cmd_index = get_cmd_index(buf);
    int module_index = BT_MODULE_INDEX;

    ENG_LOG("ADL %s(), args0 = %s, args1 = %s, args2 = %s, args3 = %s cmd_index = %d", __func__,
            args0, args1, args2, args3, cmd_index);

    switch (cmd_index) {
        case TX_INDEX: {
            ENG_LOG("ADL %s(), case TX_INDEX, module_index = %d", __func__, module_index);

            if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
                int pktcnt = 0;

                if (0 != strlen(data[3])) {
                    pktcnt = atoi(data[3]);
                }

                ENG_LOG("ADL %s(), case TX_INDEX, call bt_tx_set(), pktcnt = %d", __func__, pktcnt);
                bt_tx_set(atoi(data[1]), atoi(data[2]), pktcnt, rsp);
            } else {
                ENG_LOG("ADL %s(), case TX_INDEX, module_index is ERROR", __func__);
            }
        } break;

        case RX_INDEX: {
            ENG_LOG("ADL %s(), case RX_INDEX, module_index = %d", __func__, module_index);

            if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
                ENG_LOG("ADL %s(), case RX_INDEX, call bt_rx_set()", __func__);
                bt_rx_set(atoi(data[1]), rsp);
            } else {
                ENG_LOG("ADL %s(), case RX_INDEX, module_index is ERROR", __func__);
            }
        } break;

        case TX_REQ_INDEX: {
            ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index = %d", __func__, module_index);

            if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
                ENG_LOG("ADL %s(), case TX_REQ_INDEX, call bt_tx_get()", __func__);
                bt_tx_get(rsp);
            } else {
                ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index is ERROR", __func__);
            }
        } break;

        case RX_REQ_INDEX: {
            ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index = %d", __func__, module_index);

            if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
                ENG_LOG("ADL %s(), case RX_REQ_INDEX, call bt_rx_get()", __func__);
                bt_rx_get(rsp);
            } else {
                ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index is ERROR", __func__);
            }
        } break;

        /* TX Channel */
        case BT_TXCH_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXCH_REQ_INDEX", __func__);
            bt_channel_get(BTEUT_CMD_TYPE_TX, rsp);
        } break;

        case BT_TXCH_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXCH_INDEX", __func__);
            bt_channel_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
        } break;

        /* RX Channel */
        case BT_RXCH_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXCH_REQ_INDEX", __func__);
            bt_channel_get(BTEUT_CMD_TYPE_RX, rsp);
        } break;

        case BT_RXCH_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXCH_INDEX", __func__);
            bt_channel_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
        } break;

        /* Pattern */
        case BT_TXPATTERN_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPATTERN_REQ_INDEX", __func__);
            bt_pattern_get(BTEUT_CMD_TYPE_TX, rsp);
        } break;

        case BT_TXPATTERN_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPATTERN_INDEX", __func__);
            bt_pattern_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
        } break;

        case BT_RXPATTERN_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXPATTERN_REQ_INDEX", __func__);
            bt_pattern_get(BTEUT_CMD_TYPE_RX, rsp);
        } break;

        case BT_RXPATTERN_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXPATTERN_INDEX", __func__);
            bt_pattern_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
        } break;

        /* PKT Type */
        case BT_TXPKTTYPE_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPKTTYPE_REQ_INDEX", __func__);
            bt_pkttype_get(BTEUT_CMD_TYPE_TX, rsp);
        } break;

        case BT_TXPKTTYPE_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPATTERN_INDEX", __func__);
            bt_pkttype_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
        } break;

        case BT_RXPKTTYPE_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXPATTERN_REQ_INDEX", __func__);
            bt_pkttype_get(BTEUT_CMD_TYPE_RX, rsp);
        } break;

        case BT_RXPKTTYPE_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXPATTERN_INDEX", __func__);
            bt_pkttype_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
        } break;

        /* PKT Length */
        case BT_TXPKTLEN_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPKTLEN_REQ_INDEX", __func__);
            bt_txpktlen_get(rsp);
        } break;

        case BT_TXPKTLEN_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPKTLEN_INDEX", __func__);
            bt_txpktlen_set(atoi(data[1]), rsp);
        } break;

        /* TX Power */
        case BT_TXPWR_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPKTLEN_REQ_INDEX", __func__);
            bt_txpwr_get(rsp);
        } break;

        case BT_TXPWR_INDEX: {
            ENG_LOG("ADL %s(), case BT_TXPKTLEN_INDEX", __func__);
            bt_txpwr_set(atoi(data[1]), atoi(data[2]), rsp);
        } break;

        /* RX Gain */
        case BT_RXGAIN_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXGAIN_REQ_INDEX", __func__);
            bt_rxgain_get(rsp);
        } break;

        case BT_RXGAIN_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXGAIN_INDEX", __func__);
            bt_rxgain_set(atoi(data[1]), atoi(data[2]), rsp);
        } break;

        /* ADDRESS */
        case BT_ADDRESS_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_ADDRESS_REQ_INDEX", __func__);
            bt_address_get(rsp);
        } break;

        case BT_ADDRESS_INDEX: {
            ENG_LOG("ADL %s(), case BT_ADDRESS_INDEX", __func__);
            bt_address_set(data[1], rsp);
        } break;

        /* RX received data */
        case BT_RXDATA_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_RXDATA_REQ_INDEX", __func__);
            bt_rxdata_get(rsp);
        } break;

        /* Testmode */
        case BT_TESTMODE_REQ_INDEX: {
            ENG_LOG("ADL %s(), case BT_TESTMODE_REQ_INDEX", __func__);

            if (BT_MODULE_INDEX == module_index) {
                bt_testmode_get(BTEUT_BT_MODE_CLASSIC, rsp);
            } else if (BLE_MODULE_INDEX == module_index) {
                bt_testmode_get(BTEUT_BT_MODE_BLE, rsp);
            }
        } break;

        /* Testmode */
        case BT_TESTMODE_INDEX: {
            ENG_LOG("ADL %s(), case BT_TESTMODE_INDEX, module_index = %d", __func__, module_index);
            if (BT_MODULE_INDEX == module_index) {
                bt_testmode_set(BTEUT_BT_MODE_CLASSIC, atoi(data[1]), rsp);
            } else if (BLE_MODULE_INDEX == module_index) {
                bt_testmode_set(BTEUT_BT_MODE_BLE, atoi(data[1]), rsp);
            }
        } break;

        //-----------------------------------------------------
        default:
            strcpy(rsp, "can not match the at command");
            return 0;
    }
}

/********************************************************************
*   name   bt_str2bd
*   ---------------------------
*   descrition: covert string address to hex format
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
static int bt_str2bd(const char *str, bt_bdaddr_t *addr) {
    unsigned char i = 0;

    for (i = 0; i < 6; i++) {
        addr->address[i] = (unsigned char)strtoul(str, (char **)&str, 16);
        str++;
    }

    return 0;
}

/********************************************************************
*   name   bt_testmode_set
*   ---------------------------
*   descrition: set rx's address to global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   bt_mode     IN          bteut_bt_mode   the mode is BT or BLE
*   testmode    IN          bteut_testmode  test mode
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int bt_testmode_set(bteut_bt_mode bt_mode, bteut_testmode testmode, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), bt_mode = %d, g_bt_mode = %d, testmode = %d, "
            "g_bteut_testmode = %d",
            __func__, bt_mode, g_bt_mode, testmode, g_bteut_testmode);

    switch (testmode) {
        case BTEUT_TESTMODE_LEAVE: {
            ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE:", __func__);

            if (BTEUT_TESTMODE_ENTER_NONSIG == g_bteut_testmode) {
                if (BTEUT_BT_MODE_CLASSIC != g_bt_mode && BTEUT_BT_MODE_BLE != g_bt_mode) {
                    /* is not BT_MODE_OFF, is error */
                    ENG_LOG("ADL %s(), g_bt_mode is ERROR, g_bt_mode = %d", __func__, g_bt_mode);
                    goto err;
                }

                if (BTEUT_TESTMODE_LEAVE != g_bteut_testmode) {
                    if (BTEUT_TXRX_STATUS_TXING == g_bteut_txrx_status ||
                        BTEUT_TXRX_STATUS_RXING == g_bteut_txrx_status) {
                        ENG_LOG("ADL %s(), txrx_status is ERROR, txrx_status = %d", __func__,
                                g_bteut_txrx_status);
                        goto err;
                    }
                }

            } else if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode) {
                ret = engpc_bt_dut_mode_configure(0);
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE: called dut_mode_configure(), "
                        "ret = %d",
                        __func__, ret);

                if (0 != ret) {
                    ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE: called "
                            "dut_mode_configure(), ret = %d, goto err",
                            __func__, ret);
                    goto err;
                }

                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE: set g_bteut_runing to "
                        "RUNNING_OFF",
                        __func__);
                g_bteut_runing = BTEUT_EUT_RUNNING_OFF;
            }

            engpc_bt_off();

            ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE: set g_bteut_testmode to "
                    "BTEUT_TESTMODE_LEAVE",
                    __func__);
            g_bteut_testmode = BTEUT_TESTMODE_LEAVE;
        } break;

        case BTEUT_TESTMODE_ENTER_EUT: {
            ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT:", __func__);

            if (BTEUT_TESTMODE_ENTER_EUT != g_bteut_testmode) {
                g_bteut_testmode = BTEUT_TESTMODE_ENTER_EUT;

                ret = engpc_bt_on();
                if (0 != ret) {
                    ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: called "
                            "engpc_bt_on is error, goto err",
                            __func__);
                    goto err;
                }

                ret = engpc_bt_dut_mode_configure(1);
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: called "
                        "dut_mode_configure(1), ret = %d",
                        __func__, ret);

                if (0 != ret) {
                    ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: called "
                            "dut_mode_configure(1), ret = %d, goto err",
                            __func__, ret);
                    goto err;
                }

                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: set g_bteut_runing to "
                        "BTEUT_EUT_RUNNING_ON",
                        __func__);
                g_bteut_runing = BTEUT_EUT_RUNNING_ON;
            } else {
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: now is ENTER_EUT, error.",
                        __func__);
                goto err;
            }
        } break;

        case BTEUT_TESTMODE_ENTER_NONSIG: {
            ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG:", __func__);

#if 0
            if (BTEUT_BT_MODE_OFF != g_bt_mode)
            {
                /* is not BT_MODE_OFF, is error */
                ENG_LOG("ADL %s(), g_bt_mode is ERROR, g_bt_mode = %d", __func__, g_bt_mode);
                goto err;
            }
#endif
            if (BTEUT_TESTMODE_ENTER_NONSIG != g_bteut_testmode) {
                g_bteut_testmode = BTEUT_TESTMODE_ENTER_NONSIG;
            } else {
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: now is ENTER NONSIG, "
                        "error.",
                        __func__);
                goto err;
            }
            ENG_LOG("calling engpc_bt_on");
            ret = engpc_bt_on();
            if (0 != ret) {
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: called engpc_bt_on() "
                        "is error, goto err",
                        __func__);
                goto err;
            }
        } break;

        default:
            ENG_LOG("ADL %s(), case default", __func__);
    }

    ENG_LOG("ADL %s(), set bt_mode is %d", __func__, bt_mode);
    g_bt_mode = bt_mode; /* is CLASSIC OR BLE */

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_testmode_get
*   ---------------------------
*   descrition: get testmode's value from global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int bt_testmode_get(bteut_bt_mode bt_mode, char *rsp) {
    ENG_LOG("ADL entry %s(), bt_mode = %d", __func__, bt_mode);

    ENG_LOG("ADL %s(), set g_bt_mode to %d", __func__, bt_mode);
    g_bt_mode = bt_mode;

    snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
             (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TESTMODE_REQ_RET : BT_TESTMODE_REQ_RET),
             (int)g_bteut_testmode);

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;
}

/********************************************************************
*   name   bt_address_set
*   ---------------------------
*   descrition: set rx's address to global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   addr        IN          const char *    MAC Address
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_address_set(const char *addr, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), addr = %s", __func__, addr);

    if ('\"' == *addr) {
        /* skip \" */
        addr++;
    }

    strncpy(g_bteut_rx.addr, addr, BT_MAC_STR_MAX_LEN);
    ENG_LOG("ADL %s(), addr = %s", __func__, g_bteut_rx.addr);

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_channel_get
*   ---------------------------
*   descrition: get rx's address from g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_address_get(char *rsp) {
    ENG_LOG("ADL entry %s()", __func__);

    snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%s",
             (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_ADDRESS_REQ_RET : BT_ADDRESS_REQ_RET),
             g_bteut_rx.addr);

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_channel_set
*   ---------------------------
*   descrition: set channel to global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          bteut_cmd_type  the command is TX or RX
*   ch          IN          int             channel
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/rx, whether is not BT Chip
*
********************************************************************/
int bt_channel_set(bteut_cmd_type cmd_type, int ch, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), cmd_type = %d, ch = %d", __func__, cmd_type, ch);

    if (BTEUT_CMD_TYPE_TX == cmd_type) {
        g_bteut_tx.channel = ch;
    } else if (BTEUT_CMD_TYPE_RX == cmd_type) {
        g_bteut_rx.channel = ch;
    } else {
        goto err;
        ENG_LOG("ADL %s(), cmd_type is error, goto err:", __func__);
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_channel_get
*   ---------------------------
*   descrition: get channel from g_bteut_tx/g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          bteut_cmd_type  the command is TX or RX
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_channel_get(bteut_cmd_type cmd_type, char *rsp) {
    ENG_LOG("ADL entry %s(), cmd_type = %d", __func__, cmd_type);

    if (BTEUT_CMD_TYPE_TX == cmd_type) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_CHANNEL_REQ_RET : BT_TX_CHANNEL_REQ_RET),
                 g_bteut_tx.channel);
    } else if (BTEUT_CMD_TYPE_RX == cmd_type) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_CHANNEL_REQ_RET : BT_RX_CHANNEL_REQ_RET),
                 g_bteut_rx.channel);
    } else {
        goto err;
        ENG_LOG("ADL %s(), cmd_type is error, goto err:", __func__);
    }

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_pattern_set
*   ---------------------------
*   descrition: set pattern to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   cmd_type    IN          cmd_type            the command is TX or RX
*   pattern     IN          int                 pattern
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/rx, whether is not BT Chip
*
********************************************************************/
int bt_pattern_set(bteut_cmd_type cmd_type, int pattern, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), cmd_type = %d, pattern = %d", __func__, cmd_type, pattern);

    if (BTEUT_CMD_TYPE_TX == cmd_type) {
        g_bteut_tx.pattern = pattern;
    } else if (BTEUT_CMD_TYPE_RX == cmd_type) {
        g_bteut_rx.pattern = pattern;
    } else {
        goto err;
        ENG_LOG("ADL %s(), cmd_type is error, goto err:", __func__);
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_pattern_get
*   ---------------------------
*   descrition: get pattern from g_bteut_tx/g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          cmd_type        the command is TX or RX
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_pattern_get(bteut_cmd_type cmd_type, char *rsp) {
    ENG_LOG("ADL entry %s(), cmd_type = %d", __func__, cmd_type);

    if (BTEUT_CMD_TYPE_TX == cmd_type) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_PATTERN_REQ_RET : BT_TX_PATTERN_REQ_RET),
                 g_bteut_tx.pattern);
    } else if (BTEUT_CMD_TYPE_RX == cmd_type) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_PATTERN_REQ_RET : BT_RX_PATTERN_REQ_RET),
                 g_bteut_rx.pattern);
    } else {
        goto err;
        ENG_LOG("ADL %s(), cmd_type is error, goto err:", __func__);
    }

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_pkttype_set
*   ---------------------------
*   descrition: set pkttype to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   cmd_type    IN          cmd_type            the command is TX or RX
*   pkttype     IN          int                 pkttype
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/rx, whether is not BT Chip
*
********************************************************************/
int bt_pkttype_set(bteut_cmd_type cmd_type, int pkttype, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), cmd_type = %d, pkttype = %d", __func__, cmd_type, pkttype);

    if (BTEUT_CMD_TYPE_TX == cmd_type) {
        g_bteut_tx.pkttype = pkttype;
    } else if (BTEUT_CMD_TYPE_RX == cmd_type) {
        g_bteut_rx.pkttype = pkttype;
    } else {
        goto err;
        ENG_LOG("ADL %s(), cmd_type is error, goto err:", __func__);
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_pkttype_get
*   ---------------------------
*   descrition: get pkttype from g_bteut_tx/g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          cmd_type        the command is TX or RX
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_pkttype_get(bteut_cmd_type cmd_type, char *rsp) {
    ENG_LOG("ADL entry %s(), cmd_type = %d", __func__, cmd_type);

    if (BTEUT_CMD_TYPE_TX == cmd_type) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_PKTTYPE_REQ_RET : BT_TX_PKTTYPE_REQ_RET),
                 (int)g_bteut_tx.pkttype);
    } else if (BTEUT_CMD_TYPE_RX == cmd_type) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_PKTTYPE_REQ_RET : BT_RX_PKTTYPE_REQ_RET),
                 (int)g_bteut_rx.pkttype);
    } else {
        goto err;
        ENG_LOG("ADL %s(), cmd_type is error, goto err:", __func__);
    }

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_txpktlen_set
*   ---------------------------
*   descrition: set pktlen to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   pktlen      IN          int                 pktlen
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpktlen_set(unsigned int pktlen, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), pktlen = %d", __func__, pktlen);

    g_bteut_tx.pktlen = pktlen;

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;
}

/********************************************************************
*   name   bt_txpktlen_get
*   ---------------------------
*   descrition: get pkttype from g_bteut_tx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpktlen_get(char *rsp) {
    ENG_LOG("ADL entry %s()", __func__);

    snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
             (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TXPKTLEN_REQ_RET : BT_TXPKTLEN_REQ_RET),
             g_bteut_tx.pktlen);

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;
}

/********************************************************************
*   name   bt_txpwr_set
*   ---------------------------
*   descrition: set txpwr to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   txpwr       IN          int                 txpwr
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpwr_set(bteut_txpwr_type txpwr_type, unsigned int value, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), txpwr_type = %d, value = %d", __func__, (int)txpwr_type, value);
    ENG_LOG("ADL %s(), g_bteut_testmode = %d, g_bteut_runing = %d", __func__, (int)g_bteut_testmode,
            (int)g_bteut_runing);

    g_bteut_tx.txpwr.power_type = txpwr_type;
    g_bteut_tx.txpwr.power_value = value;

    if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode && BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
        unsigned char buf[3] = { 0x00 };
        buf[0] = (unsigned char)txpwr_type;
        buf[1] = (unsigned char)(value & 0x00FF);
        buf[2] = (unsigned char)((value & 0xFF00) >> 8);

        ret = engpc_bt_dut_mode_send(HCI_DUT_SET_TXPWR, buf, 3);
        ENG_LOG("ADL %s(), call dut_mode_send(HCI_DUT_SET_TXPWR), ret = %d", __func__, ret);

        if (0 != ret) {
            ENG_LOG("ADL %s(), call dut_mode_send() is ERROR, goto err", __func__);
            goto err;
        }
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_txpwr_get
*   ---------------------------
*   descrition: get txpwr type and value from g_bteut_tx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpwr_get(char *rsp) {
    ENG_LOG("ADL entry %s()", __func__);

    snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
             (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TXPWR_REQ_RET : BT_TXPWR_REQ_RET),
             (int)g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value);

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;
}

/********************************************************************
*   name   bt_rxgain_set
*   ---------------------------
*   descrition: set rx gain to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   mode        IN          bteut_gain_mode     RX Gain's mode
*   value       IN          unsigned int        value
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_rxgain_set(bteut_gain_mode mode, unsigned int value, char *rsp) {
    int ret = -1;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), rxgain_mode = %d, value = %d", __func__, (int)mode, value);

    g_bteut_rx.rxgain.mode = mode;
    if (BTEUT_GAIN_MODE_FIX == mode) {
        g_bteut_rx.rxgain.value = value;
    } else if (BTEUT_GAIN_MODE_AUTO == mode) {
        g_bteut_rx.rxgain.value = 0; /* set to 0 */
    } else {
        goto err;
        ENG_LOG("ADL %s(), mode is error, goto err:", __func__);
    }

    ENG_LOG("ADL %s(), g_bteut_testmode = %d, g_bteut_runing = %d, value = %d", __func__,
            (int)g_bteut_testmode, (int)g_bteut_runing, g_bteut_rx.rxgain.value);
    if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode && BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
        unsigned char buf[1] = { 0x00 };

        buf[0] = (unsigned char)g_bteut_rx.rxgain.value;
        ret = engpc_bt_dut_mode_send(HCI_DUT_SET_RXGIAN, buf, 1);
        ENG_LOG("ADL %s(), callED dut_mode_send(HCI_DUT_SET_RXGIAN), ret = %d", __func__, ret);

        if (0 != ret) {
            ENG_LOG("ADL %s(), call dut_mode_send() is ERROR, goto err", __func__);
            goto err;
        }
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_rxgain_get
*   ---------------------------
*   descrition: get rx gain from g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_rxgain_get(char *rsp) {
    bteut_gain_mode mode = BTEUT_GAIN_MODE_INVALID;
    mode = g_bteut_rx.rxgain.mode;

    ENG_LOG("ADL entry %s(), mode = %d", __func__, mode);

    if (BTEUT_GAIN_MODE_FIX == mode) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXGAIN_REQ_RET : BT_RXGAIN_REQ_RET), mode,
                 g_bteut_rx.rxgain.value);
    } else if (BTEUT_GAIN_MODE_AUTO == mode) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXGAIN_REQ_RET : BT_RXGAIN_REQ_RET), mode);
    } else {
        goto err;
        ENG_LOG("ADL %s(), mode is error, goto err:", __func__);
    }

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_tx_set
*   ---------------------------
*   descrition: set TX start or TX stop to Chip
*   ----------------------------
*   para        IN/OUT      type                note
*   on_off      IN          int                 1:start TX 0:Stop TX
*   tx_mode     IN          bteut_tx_mode       continues or single
*   pktcnt      IN          unsigned int        [options] send package count
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int bt_tx_set(int on_off, int instru_tx_mode, unsigned int pktcnt, char *rsp) {
    int ret = -1;
    char is_ble = 0;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), on_off = %d, tx_mode = %d, pktcnt = %d, g_bt_mode = %d", __func__,
            on_off, (int)instru_tx_mode, pktcnt, g_bt_mode);

    if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
        is_ble = 0;
    } else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
        is_ble = 1;
    } else if (BTEUT_BT_MODE_OFF == g_bt_mode) {
        ENG_LOG("ADL %s(), g_bt_mode is ERROR, g_bt_mode = %d, goto err;", __func__, g_bt_mode);
        goto err;
    }
#ifdef SPRD_WCNBT_MARLIN
    char instru_type = (char)(instru_tx_mode >> 8);
    ENG_LOG("ADL %s(), instru_type = %x", __func__, instru_type);
    g_bteut_tx.pkttype |= (int)instru_type << 8;
#endif
    ENG_LOG("ADL %s(), on_off = %d, is_ble = %d", __func__, on_off, is_ble);
    if (0 == on_off) {
        if (BTEUT_TXRX_STATUS_TXING != g_bteut_txrx_status) {
            ENG_LOG("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d", __func__,
                    g_bteut_txrx_status);
            goto err;
        }

        ENG_LOG("ADL %s(), call set_nonsig_tx_testmode(), enable = 0, is_ble = %d, "
                "g_bt_mode = %d, the rest of other parameters all 0.",
                __func__, is_ble, g_bt_mode);

        ret = engpc_bt_set_nonsig_tx_testmode(0, is_ble, 0, 0, 0, 0, 0, 0, 0);

        ENG_LOG("ADL %s(), called set_nonsig_tx_testmode(), ret = %d", __func__, ret);

        if (0 == ret) {
            g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
        } else {
            ENG_LOG("ADL %s(), called set_nonsig_tx_testmode(), ret is ERROR, ret = %d", __func__,
                    ret);
            goto err;
        }
    } else if (1 == on_off) {
        if (BTEUT_TXRX_STATUS_OFF != g_bteut_txrx_status) {
            ENG_LOG("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d", __func__,
                    g_bteut_txrx_status);
            goto err;
        }

        ENG_LOG("ADL %s(), call set_nonsig_tx_testmode(), enable = 1, le = %d, pattern "
                "= %d, channel = %d, pac_type = %d, pac_len = %d, pwr_type = %d, "
                "pwr_value = %d, pkt_cnt = %d",
                __func__, is_ble, (int)g_bteut_tx.pattern, g_bteut_tx.channel, g_bteut_tx.pkttype,
                g_bteut_tx.pktlen, g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value,
                pktcnt);

        ret = engpc_bt_set_nonsig_tx_testmode(
            1, is_ble, g_bteut_tx.pattern, g_bteut_tx.channel, g_bteut_tx.pkttype,
            g_bteut_tx.pktlen, g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value, pktcnt);

        ENG_LOG("ADL %s(), called set_nonsig_tx_testmode(), ret = %d", __func__, ret);

        if (0 == ret) {
            g_bteut_txrx_status = BTEUT_TXRX_STATUS_TXING;
        } else {
            ENG_LOG("ADL %s(), called set_nonsig_tx_testmode(), ret is ERROR, ret = %d", __func__,
                    ret);
            goto err;
        }
    } else {
        ENG_LOG("ADL %s(), on_off's value is ERROR, goto err", __func__);
        goto err;
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_tx_get
*   ---------------------------
*   descrition: get tx's status
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int bt_tx_get(char *rsp) {
    bteut_txrx_status bt_txrx_status = g_bteut_txrx_status;
    ENG_LOG("ADL entry %s(), ", __func__);

    if (BTEUT_TXRX_STATUS_OFF == bt_txrx_status || BTEUT_TXRX_STATUS_TXING == bt_txrx_status) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_REQ_RET : BT_TX_REQ_RET),
                 (int)bt_txrx_status);
    } else {
        ENG_LOG("ADL %s(), g_bteut_status is ERROR, bt_txrx_status = %d", __func__, bt_txrx_status);
        goto err;
    }

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_rx_set
*   ---------------------------
*   descrition: set RX start or RX stop to Chip
*   ----------------------------
*   para        IN/OUT      type                note
*   on_off      IN          int                 1:start TX 0:Stop TX
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int bt_rx_set(int on_off, char *rsp) {
    int ret = -1;
    char is_ble = 0;
    bt_bdaddr_t addr;
    char cmd[BT_EUT_COMMAND_MAX_LEN + 1] = { 0x00 };

    ENG_LOG("ADL entry %s(), on_off = %d", __func__, on_off);

    if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
        is_ble = 0;
    } else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
        is_ble = 1;
    } else if (BTEUT_BT_MODE_OFF == g_bt_mode) {
        ENG_LOG("ADL %s(), g_bt_mode is ERROR, g_bt_mode = %d, goto err;", __func__, g_bt_mode);
        goto err;
    }

    if (0 == on_off) {
        if (BTEUT_TXRX_STATUS_RXING != g_bteut_txrx_status) {
            ENG_LOG("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d", __func__,
                    g_bteut_txrx_status);
            goto err;
        }

        ENG_LOG("ADL %s(), call set_nonsig_rx_testmode(), enable = 0, le = 0, the rest "
                "of other parameters all 0.",
                __func__);

        ret = engpc_bt_set_nonsig_rx_testmode(0, is_ble, 0, 0, 0, 0, addr);

        ENG_LOG("ADL %s(), called set_nonsig_rx_testmode(), ret = %d", __func__, ret);

        if (0 == ret) {
            g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
        } else {
            ENG_LOG("ADL %s(), called set_nonsig_rx_testmode(), ret is ERROR, ret = %d", __func__,
                    ret);
            goto err;
        }
    } else if (1 == on_off) {
        int rxgain_value = 0;

        if (BTEUT_TXRX_STATUS_OFF != g_bteut_txrx_status) {
            ENG_LOG("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d", __func__,
                    g_bteut_txrx_status);
            goto err;
        }

        if (0 == g_bteut_rx.rxgain.mode) {
            rxgain_value = 0;
        } else {
            rxgain_value = g_bteut_rx.rxgain.value;
        }

        bt_str2bd(g_bteut_rx.addr, &addr);
        ENG_LOG("ADL %s(), call set_nonsig_rx_testmode(), enable = 1, le = 0, pattern "
                "= %d, channel = %d, pac_type = %d, rxgain_value = %d, addr = %s",
                __func__, (int)g_bteut_rx.pattern, g_bteut_rx.channel, g_bteut_rx.pkttype,
                rxgain_value, g_bteut_rx.addr);

        ret = engpc_bt_set_nonsig_rx_testmode(1, is_ble, g_bteut_rx.pattern, g_bteut_rx.channel,
                                              g_bteut_rx.pkttype, rxgain_value, addr);

        ENG_LOG("ADL %s(), called set_nonsig_rx_testmode(), ret = %d", __func__, ret);

        if (0 == ret) {
            g_bteut_txrx_status = BTEUT_TXRX_STATUS_RXING;
        } else if (0 != ret) {
            ENG_LOG("ADL %s(), called set_nonsig_rx_testmode(), ret is ERROR, ret = %d", __func__,
                    ret);
            goto err;
        }
    } else {
        ENG_LOG("ADL %s(), on_off's value is ERROR, goto err", __func__);
        goto err;
    }

    strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
            BT_EUT_COMMAND_RSP_MAX_LEN);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return 0", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_rx_get
*   ---------------------------
*   descrition: get rx's status
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int bt_rx_get(char *rsp) {
    bteut_txrx_status bt_txrx_status = g_bteut_txrx_status;
    ENG_LOG("ADL entry %s(), ", __func__);

    if (BTEUT_TXRX_STATUS_OFF == bt_txrx_status || BTEUT_TXRX_STATUS_RXING == bt_txrx_status) {
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_REQ_RET : BT_RX_REQ_RET),
                 (int)bt_txrx_status);
    } else {
        ENG_LOG("ADL %s(), bt_txrx_status is ERROR, bt_txrx_status = %d", __func__, bt_txrx_status);
        goto err;
    }

    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bt_rxdata_get
*   ---------------------------
*   descrition: get rx data value from chip
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int bt_rxdata_get(char *rsp) {
    int ret = -1;
    bteut_txrx_status bt_txrx_status = g_bteut_txrx_status;
    ENG_LOG("ADL entry %s(), rf_status = %d, g_bteut_testmode = %d, g_bteut_runing = "
            "%d",
            __func__, bt_txrx_status, g_bteut_testmode, (int)g_bteut_runing);

    ENG_LOG("ADL %s(), g_bt_mode = %d", __func__, g_bt_mode);
    if (BTEUT_TESTMODE_ENTER_NONSIG == g_bteut_testmode) {
        unsigned char i = 0;
        char buf[255] = { 0 };
        uint16_t result_len = 0;

        int rssi;
        int pkt_cnt;
        int pkt_err_cnt;
        int bit_cnt;
        int bit_err_cnt;

        if (BTEUT_TXRX_STATUS_RXING == bt_txrx_status) {
            unsigned char ble = 0;
            if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
                ble = 0;
            } else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
                ble = 1;
            }

            ENG_LOG("ADL %s(), call engpc_bt_get_nonsig_rx_data(), ble = %d", __func__, ble);

            ret = engpc_bt_get_nonsig_rx_data(ble, buf, sizeof(buf), &result_len);
            if (!memcmp(buf, OK_STR, strlen(OK_STR))) {
                ENG_LOG("memcpy OK_STR");
                sscanf(buf, "OK rssi:%d, pkt_cnt:%d, pkt_err_cnt:%d, bit_cnt:%d, "
                            "bit_err_cnt:%d",
                       &rssi, &pkt_cnt, &pkt_err_cnt, &bit_cnt, &bit_err_cnt);
            } else if (!memcmp(buf, FAIL_STR, strlen(FAIL_STR))) {
                ENG_LOG("memcpy FAIL_STR");
                return -1;
            } else {
                ENG_LOG("memcpy else");
                return -1;
            }

            ENG_LOG("ADL %s(), called engpc_bt_get_nonsig_rx_data(), ret = %d", __func__, ret);

            if (0 != ret) {
                ENG_LOG("ADL %s(), call engpc_bt_get_nonsig_rx_data() is ERROR, ret = "
                        "%d, goto err",
                        __func__, ret);
                goto err;
            }
        } else {
            ENG_LOG("ADL %s(), rf_status is ERROR, ret = %d, goto err", __func__, bt_txrx_status);
            goto err;
        }

        if (result_len <= 0) {
            ENG_LOG("ADL %s(), result_len = %d, goto err", __func__, result_len);
            goto err;
        }
        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d,%d,%d,%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXDATA_REQ_RET : BT_RXDATA_REQ_RET),
                 bit_err_cnt, bit_cnt, pkt_err_cnt, pkt_cnt, rssi);

    } else if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode &&
               BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
        unsigned char i = 0;
        char buf[255] = { 0 };
        uint16_t result_len = 0;
        ret = engpc_bt_dut_mode_get_rx_data(buf, sizeof(buf), &result_len);
        int rssi;
        int rx_status;
        if (!memcmp(buf, OK_STR, strlen(OK_STR))) {
            ENG_LOG("memcpy OK_STR");
            sscanf(buf, "OK HCI_DUT_GET_RXDATA status:%d, rssi:%d", &rx_status, &rssi);
        } else if (!memcmp(buf, FAIL_STR, strlen(FAIL_STR))) {
            ENG_LOG("memcpy FAIL_STR");
            return -1;
        } else {
            ENG_LOG("memcpy else");
            return -1;
        }

        ENG_LOG("ADL %s(), callED dut_mode_send(HCI_DUT_GET_RXDATA), ret = %d", __func__, ret);

        if (0 != ret) {
            ENG_LOG("ADL %s(), callED dut_mode_send(HCI_DUT_GET_RXDATA) is ERROR, ret = "
                    "%d, goto err",
                    __func__, ret);
            goto err;
        }

        if (result_len <= 0) {
            ENG_LOG("ADL %s(), result_len = %d, goto err", __func__, result_len);
            goto err;
        }

        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXDATA_REQ_RET : BT_RXDATA_REQ_RET), rssi);
    }
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s", __func__, rsp);
    return 0;

err:
    bt_build_err_resp(rsp);
    rsp_debug(rsp);

    ENG_LOG("ADL leaving %s(), rsp = %s, return -1", __func__, rsp);
    return -1;
}

/********************************************************************
*   name   bteut_load
*   ---------------------------
*   descrition: load BT lib
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
/*
static int bteut_load(void) {
  int err = 0;
  ENG_LOG("called not imlemented function %s()", __func__);
  return err;
}
*/
/********************************************************************
*   name   bteut_unload
*   ---------------------------
*   descrition: unload BT lib
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
/*
static void bteut_unload(void) {
  ENG_LOG("called not imlemented function %s()", __func__);
}
*/
/********************************************************************
*   name   bteut_init_bt
*   ---------------------------
*   descrition: init bt module
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
/*
static int bteut_init_bt(void) {
  int ret = -1;
  ENG_LOG("called not imlemented function %s()", __func__);
  return ret;
}
*/
/********************************************************************
*   name   bteut_enable_bt
*   ---------------------------
*   descrition: enable bt module
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
static int bteut_enable_bt(void) {
    int ret = -1;

    ENG_LOG("ADL entry %s()", __func__);

    g_bt_status = engpc_bt_on();

    if (BT_STATUS_SUCCESS == g_bt_status || BT_STATUS_DONE == g_bt_status) {
        ret = 0;
    }

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

/********************************************************************
*   name   bteut_disable_bt
*   ---------------------------
*   descrition: disable bt module
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
static int bteut_disable_bt(void) {
    int ret = -1;

    ENG_LOG("ADL entry %s()", __func__);

    g_bt_status = engpc_bt_off();

    if (BT_STATUS_SUCCESS == g_bt_status || BT_STATUS_DONE == g_bt_status) {
        ret = 0;
    }

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

/********************************************************************
*   name   bt_le_transmitter
*   ---------------------------
*   descrition: transmit data in BLE mode
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*   call this function must be first init BT
********************************************************************/
static int bt_le_transmitter(void) {
    unsigned char buf[3] = { 0x00 };
    bt_status_t ret = BT_STATUS_SUCCESS;

    ENG_LOG("ADL entry %s(), g_bt_mode = %d", __func__, g_bt_mode);

    if (BTEUT_BT_MODE_BLE != g_bt_mode) {
        ENG_LOG("ADL leaving %s(), bt_mode is ERROR, return -1", __func__);
        return -1;
    }

    ENG_LOG("ADL %s(), channel = %d, pktlen = %d, pattern = %d", __func__, g_bteut_tx.channel,
            g_bteut_tx.pktlen, g_bteut_tx.pattern);
    buf[0] = (unsigned char)g_bteut_tx.channel;
    buf[1] = (unsigned char)g_bteut_tx.pktlen;
    buf[2] = (unsigned char)g_bteut_tx.pattern;

    ret = engpc_bt_le_test_mode(HCI_LE_TRANSMITTER_TEST_OPCODE, buf, 3);
    ENG_LOG("ADL %s(), callED "
            "engpc_bt_le_test_mode(HCI_LE_TRANSMITTER_TEST_OPCODE), ret = %d",
            __func__, ret);

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

/********************************************************************
*   name   bt_le_receiver
*   ---------------------------
*   descrition: receive data in BLE mode
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*   call this function must be first init BT
********************************************************************/
static int bt_le_receiver(void) {
    unsigned char buf[1] = { 0x00 };
    bt_status_t ret = BT_STATUS_SUCCESS;

    ENG_LOG("ADL entry %s(), g_bt_mode = %d", __func__, g_bt_mode);

    if (BTEUT_BT_MODE_BLE != g_bt_mode) {
        ENG_LOG("ADL leaving %s(), bt_mode is ERROR, return -1", __func__);
        return -1;
    }

    ENG_LOG("ADL %s(), channel = %d", __func__, g_bteut_rx.channel);

    buf[0] = (unsigned char)g_bteut_rx.channel;
    ret = engpc_bt_le_test_mode(HCI_LE_RECEIVER_TEST_OPCODE, buf, 1);
    ENG_LOG("ADL %s(), callED "
            "engpc_bt_le_test_mode(HCI_LE_RECEIVER_TEST_OPCODE), ret = %d",
            __func__, ret);

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

/********************************************************************
*   name   bt_le_stop
*   ---------------------------
*   descrition: stop transmitter or receiver
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
static int bt_le_stop(void) {
    bt_status_t ret = BT_STATUS_SUCCESS;

    ENG_LOG("ADL entry %s(), g_bt_mode = %d", __func__, g_bt_mode);

    if (BTEUT_BT_MODE_BLE != g_bt_mode) {
        ENG_LOG("ADL leaving %s(), bt_mode is ERROR, return -1", __func__);
        return -1;
    }

    ENG_LOG("ADL %s(), channel = %d", __func__, g_bteut_rx.channel);

    ret = engpc_bt_le_test_mode(HCI_LE_END_TEST_OPCODE, NULL, 0);
    ENG_LOG("ADL %s(), callED engpc_bt_le_test_mode(HCI_LE_END_TEST_OPCODE), ret "
            "= %d",
            __func__, ret);

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

/****************************************end of the
 * file*************************************************/
