
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "eut_opt.h"
#include "bt_eut_sprd.h"
#include "engopt.h"
#include <fcntl.h>
#include <hardware/bluetooth.h>

#define UNUSED __attribute__((unused))

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG ("SPRDENG")
/* driver MACRO */
#define HCI_DUT_GET_RXDATA (0xFCE3)
#define HCI_DUT_SET_TXPWR (0xFCE1)
#define HCI_DUT_SET_RXGIAN (0xFCE2)
#define HCI_LE_RECEIVER_TEST_OPCODE (0x201D)
#define HCI_LE_TRANSMITTER_TEST_OPCODE (0x201E)
#define HCI_LE_END_TEST_OPCODE (0x201F)

#define BT_EUT_SLEEP_MAX_COUNT (50)

static BTEUT_TX_ELEMENT g_bteut_tx = { 0x00 };
static BTEUT_RX_ELEMENT g_bteut_rx = { 0x00 };
static BTEUT_RX_DATA g_bt_rx_data = { 0x00 };

static bteut_txrx_status g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
static bteut_testmode g_bteut_testmode = BTEUT_TESTMODE_LEAVE;

static bluetooth_device_t *g_bt_device;
static const bt_interface_t *g_sBtInterface = NULL;
static bt_status_t g_bt_status;
/* Set to 1 when the Bluedroid stack is enabled */
static unsigned char g_bteut_bt_enabled = 0;

/* current is Classic or BLE */
static bteut_bt_mode g_bt_mode = BTEUT_BT_MODE_OFF;
static bteut_eut_running g_bteut_runing = BTEUT_EUT_RUNNING_OFF;

/**************************Static Function*******************************/
static void bt_set_default(void);
static int bteut_load(void);
static void bteut_unload(void);
static int bteut_init_bt(void);
static int bteut_enable_bt(void);
static int bteut_disable_bt(void);
static void bt_adapter_state_changed(bt_state_t state);
static int bt_str2bd(const char *str, bt_bdaddr_t *addr);

static int bt_init_eut(void);
static int bt_uninit_eut(void);

static int bt_le_transmitter(void);
static int bt_le_receiver(void);
static int bt_le_stop(void);

void nonsig_test_rx_rece_callback(bt_status_t status, uint8_t rssi, uint32_t packets,
                                  uint32_t packets_err, uint32_t bits, uint32_t bits_err);
void dut_mode_recv(uint16_t opcode, uint8_t *buf, uint8_t len);

static bt_callbacks_t g_bt_callbacks = {
    sizeof(bt_callbacks_t),
    bt_adapter_state_changed,
    NULL,          /* adapter_properties_cb */
    NULL,          /* remote_device_properties_cb */
    NULL,          /* device_found_cb */
    NULL,          /* discovery_state_changed_cb */
    NULL,          /* pin_request_cb  */
    NULL,          /* ssp_request_cb  */
    NULL,          /* bond_state_changed_cb */
    NULL,          /* acl_state_changed_cb */
    NULL,          /* thread_evt_cb */
    dut_mode_recv, /*dut_mode_recv_cb */
//    NULL, /*authorize_request_cb */
#if BLE_INCLUDED == TRUE
    NULL, /* le_test_mode_cb */
#else
    NULL,
#endif
    NULL,                        /* energy_info_cb only exist in android 5.x*/
    nonsig_test_rx_rece_callback /* nonsig_test_rx_recv_cb */
};

/**************************Function Definition***************************/

/********************************************************************
*   name   bt_set_default
*   ---------------------------
*   descrition: set default value of all attribute, include TX and RX
*   ----------------------------
*   ----------------------------------------------------
*   return
*   void
*   ------------------
*   other:
*
*
********************************************************************/
static void bt_set_default(void) {
    ENG_LOG("ADL entry %s()", __func__);

    /* TX parameters */
    g_bteut_tx.pattern = BT_EUT_TX_PATTERN_DEAFULT_VALUE;
    g_bteut_tx.channel = BT_EUT_TX_CHANNEL_DEAFULT_VALUE;
    g_bteut_tx.pkttype = BT_EUT_TX_PKTTYPE_DEAFULT_VALUE;
    g_bteut_tx.pktlen = BT_EUT_PKTLEN_DEAFULT_VALUE;
    g_bteut_tx.txpwr.power_type = BT_EUT_POWER_TYPE_DEAFULT_VALUE;
    g_bteut_tx.txpwr.power_value = BT_EUT_POWER_VALUE_DEAFULT_VALUE;

    /* RX parameters */
    g_bteut_rx.pattern = BT_EUT_RX_PATTERN_DEAFULT_VALUE;
    g_bteut_rx.channel = BT_EUT_RX_CHANNEL_DEAFULT_VALUE;
    g_bteut_rx.pkttype = BT_EUT_RX_PKTTYPE_DEAFULT_VALUE;
    g_bteut_rx.rxgain.mode = BT_EUT_RX_RXGAIN_DEAFULT_VALUE;
    strncpy(g_bteut_rx.addr, BT_EUT_RX_ADDR_DEAFULT_VALUE, BT_MAC_STR_MAX_LEN);

    ENG_LOG("ADL leaving %s()", __func__);
}

/********************************************************************
*   name
*   ---------------------------
*   descrition:
*   ----------------------------
*   para        IN/OUT      type            note
*
*   ----------------------------------------------------
*   return
*   ------------------
*   other:
*
********************************************************************/
static int bt_init_eut(void) {
    int ret = -1;

    ENG_LOG("ADL entry %s()", __func__);

    /* set default value of static global variable */
    bt_set_default();

    /* load */
    ret = bteut_load();
    if (0 != ret) {
        ENG_LOG("ADL %s(), called bteut_load() is ERROR, return -1", __func__);
        return -1;
    }

    /* init */
    ret = bteut_init_bt();
    if (0 != ret) {
        ENG_LOG("ADL %s(), called bteut_init_bt() is ERROR, return -1", __func__);
        bteut_unload();
        return -1;
    }

    /* enable */
    ret = bteut_enable_bt();
    if (0 != ret) {
        ENG_LOG("ADL %s(), called bteut_enable_bt() is ERROR, return -1", __func__);
        bteut_unload();
        return -1;
    }

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);

    return ret;
}

/********************************************************************
*   name   bt_uninit_eut
*   ---------------------------
*   descrition:
*   ----------------------------
*   para        IN/OUT      type            note
*
*   ----------------------------------------------------
*   return
*   ------------------
*   other:
*
********************************************************************/
static int bt_uninit_eut(void) {
    int ret = -1;

    ENG_LOG("ADL entry %s()", __func__);

    /* disable */
    ret = bteut_disable_bt();

    /* unload */
    bteut_unload();

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);

    return ret;
}

static void bt_build_err_resp(char *resp) {
    snprintf(resp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%s",
             (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_ERROR : EUT_BT_ERROR), "error");
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

    ENG_LOG("ADL entry %s(), bt_mode = %d, g_bt_mode = %d, testmode = %d, g_bteut_testmode = %d",
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
                ret = g_sBtInterface->dut_mode_configure(0);
                ENG_LOG(
                    "ADL %s(), case BTEUT_TESTMODE_LEAVE: called dut_mode_configure(), ret = %d",
                    __func__, ret);

                if (0 != ret) {
                    ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE: called dut_mode_configure(), ret "
                            "= %d, goto err",
                            __func__, ret);
                    goto err;
                }

                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_LEAVE: set g_bteut_runing to RUNNING_OFF",
                        __func__);
                g_bteut_runing = BTEUT_EUT_RUNNING_OFF;
            }

            bt_uninit_eut();

            ENG_LOG(
                "ADL %s(), case BTEUT_TESTMODE_LEAVE: set g_bteut_testmode to BTEUT_TESTMODE_LEAVE",
                __func__);
            g_bteut_testmode = BTEUT_TESTMODE_LEAVE;
        } break;

        case BTEUT_TESTMODE_ENTER_EUT: {
            unsigned char i = 0;
            ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT:", __func__);

            if (BTEUT_TESTMODE_ENTER_EUT != g_bteut_testmode) {
                g_bteut_testmode = BTEUT_TESTMODE_ENTER_EUT;

                ret = bt_init_eut();
                if (0 != ret) {
                    ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: called "
                            "bt_init_eut(g_bteut_bt_enabled) is error, goto err",
                            __func__);
                    goto err;
                }

                /* if BT enabled is incomplete, does not call dut_mode_configure() */
                while (i++ < BT_EUT_SLEEP_MAX_COUNT) {
                    if (1 == g_bteut_bt_enabled) {
                        ENG_LOG("ADL %s(), i = %d, g_bteut_bt_enabled = 1, break", __func__, i);
                        break;
                    }

                    ENG_LOG("ADL %s(), i = %d, g_bteut_bt_enabled = %d", __func__, i,
                            g_bteut_bt_enabled);
                    usleep(100 * 1000);
                }

                if (BT_EUT_SLEEP_MAX_COUNT == i) {
                    ENG_LOG("ADL %s(), loop is ERROR, i == BT_EUT_SLEEP_MAX_COUNT, return -1",
                            __func__);
                    return -1;
                }

                ret = g_sBtInterface->dut_mode_configure(1);
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: called dut_mode_configure(1), "
                        "ret = %d",
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
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: now is ENTER NONSIG, error.",
                        __func__);
                goto err;
            }

            ret = bt_init_eut();
            if (0 != ret) {
                ENG_LOG("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: called bt_init_eut() is "
                        "error, goto err",
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

        ret = g_sBtInterface->dut_mode_send(HCI_DUT_SET_TXPWR, buf, 3);
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
        ret = g_sBtInterface->dut_mode_send(HCI_DUT_SET_RXGIAN, buf, 1);
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
#ifdef SPRD_WCN_MARLIN
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

        ENG_LOG("ADL %s(), call set_nonsig_tx_testmode(), enable = 0, is_ble = %d, g_bt_mode = %d, "
                "the rest of other parameters all 0.",
                __func__, is_ble, g_bt_mode);
#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
        ret = g_sBtInterface->set_nonsig_tx_testmode(0, is_ble, 0, 0, 0, 0, 0, 0, 0);
#endif
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

        ENG_LOG(
            "ADL %s(), call set_nonsig_tx_testmode(), enable = 1, le = %d, pattern = %d, channel = "
            "%d, pac_type = %d, pac_len = %d, pwr_type = %d, pwr_value = %d, pkt_cnt = %d",
            __func__, is_ble, (int)g_bteut_tx.pattern, g_bteut_tx.channel, g_bteut_tx.pkttype,
            g_bteut_tx.pktlen, g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value, pktcnt);
#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
        ret = g_sBtInterface->set_nonsig_tx_testmode(
            1, is_ble, g_bteut_tx.pattern, g_bteut_tx.channel, g_bteut_tx.pkttype,
            g_bteut_tx.pktlen, g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value, pktcnt);
#endif
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

        ENG_LOG("ADL %s(), call set_nonsig_rx_testmode(), enable = 0, le = 0, the rest of other "
                "parameters all 0.",
                __func__);
#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
        ret = g_sBtInterface->set_nonsig_rx_testmode(0, is_ble, 0, 0, 0, 0, addr);
#endif
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
        ENG_LOG("ADL %s(), call set_nonsig_rx_testmode(), enable = 1, le = 0, pattern = %d, "
                "channel = %d, pac_type = %d, rxgain_value = %d, addr = %s",
                __func__, (int)g_bteut_rx.pattern, g_bteut_rx.channel, g_bteut_rx.pkttype,
                rxgain_value, g_bteut_rx.addr);
#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
        ret = g_sBtInterface->set_nonsig_rx_testmode(1, is_ble, g_bteut_rx.pattern,
                                                     g_bteut_rx.channel, g_bteut_rx.pkttype,
                                                     rxgain_value, addr);
#endif
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
    ENG_LOG("ADL entry %s(), rf_status = %d, g_bteut_testmode = %d, g_bteut_runing = %d", __func__,
            bt_txrx_status, g_bteut_testmode, (int)g_bteut_runing);

    /* reset of is_update variable of g_bt_rx_data */
    g_bt_rx_data.is_update = 0;

    ENG_LOG("ADL %s(), g_bt_mode = %d", __func__, g_bt_mode);
    if (BTEUT_TESTMODE_ENTER_NONSIG == g_bteut_testmode) {
        unsigned char i = 0;
        if (BTEUT_TXRX_STATUS_RXING == bt_txrx_status) {
            unsigned char ble = 0;
            if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
                ble = 0;
            } else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
                ble = 1;
            }

            ENG_LOG("ADL %s(), call get_nonsig_rx_data(), ble = %d", __func__, ble);
#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
            ret = g_sBtInterface->get_nonsig_rx_data(ble);
#endif
            ENG_LOG("ADL %s(), called get_nonsig_rx_data(), ret = %d", __func__, ret);

            if (0 != ret) {
                ENG_LOG("ADL %s(), call get_nonsig_rx_data() is ERROR, ret = %d, goto err",
                        __func__, ret);
                goto err;
            }
        } else {
            ENG_LOG("ADL %s(), rf_status is ERROR, ret = %d, goto err", __func__, bt_txrx_status);
            goto err;
        }

        while (i++ < BT_EUT_SLEEP_MAX_COUNT) {
            if (1 == g_bt_rx_data.is_update) {
                ENG_LOG("ADL %s(), i = %d, is_update = 1, break", __func__, i);
                break;
            }

            ENG_LOG("ADL %s(), i = %d, is_update = %d", __func__, i, g_bt_rx_data.is_update);
            usleep(100 * 1000);
        }

        if (BT_EUT_SLEEP_MAX_COUNT == i) {
            ENG_LOG("ADL %s(), loop is ERROR, i == BT_EUT_SLEEP_MAX_COUNT, return -1", __func__);
            return -1;
        }

        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d,%d,%d,%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXDATA_REQ_RET : BT_RXDATA_REQ_RET),
                 g_bt_rx_data.error_bits, g_bt_rx_data.total_bits, g_bt_rx_data.error_packets,
                 g_bt_rx_data.total_packets, g_bt_rx_data.rssi);

    } else if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode &&
               BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
        unsigned char i = 0;
        ret = g_sBtInterface->dut_mode_send(HCI_DUT_GET_RXDATA, NULL, 0);
        ENG_LOG("ADL %s(), callED dut_mode_send(HCI_DUT_GET_RXDATA), ret = %d", __func__, ret);

        if (0 != ret) {
            ENG_LOG(
                "ADL %s(), callED dut_mode_send(HCI_DUT_GET_RXDATA) is ERROR, ret = %d, goto err",
                __func__, ret);
            goto err;
        }

        while (i++ < BT_EUT_SLEEP_MAX_COUNT) {
            if (1 == g_bt_rx_data.is_update) {
                ENG_LOG("ADL %s(), i = %d, is_update = 1, break", __func__, i);
                break;
            }

            ENG_LOG("ADL %s(), i = %d, is_update = %d", __func__, i, g_bt_rx_data.is_update);
            usleep(100 * 1000);
        }

        if (BT_EUT_SLEEP_MAX_COUNT == i) {
            ENG_LOG("ADL %s(), loop is ERROR, i == BT_EUT_SLEEP_MAX_COUNT, return -1", __func__);
            return -1;
        }

        snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
                 (BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXDATA_REQ_RET : BT_RXDATA_REQ_RET),
                 g_bt_rx_data.rssi);
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
*   name   nonsig_test_rx_rece_callback
*   ---------------------------
*   descrition: callback
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
void nonsig_test_rx_rece_callback(bt_status_t status, uint8_t rssi, uint32_t pkt_cnt,
                                  uint32_t pkt_err_cnt, uint32_t bit_cnt, uint32_t bit_err_cnt) {
    ENG_LOG("ADL entry %s(), status = %d, rssi = %d, pkt_cnt = %d, pkt_err_cnt = %d, bit_cnt = %d, "
            "bit_err_cnt = %d",
            __func__, status, rssi, pkt_cnt, pkt_err_cnt, bit_cnt, bit_err_cnt);

    if (0 == status) {
        g_bt_rx_data.rssi = rssi;
        g_bt_rx_data.error_bits = bit_err_cnt;
        g_bt_rx_data.total_bits = bit_cnt;
        g_bt_rx_data.error_packets = pkt_err_cnt;
        g_bt_rx_data.total_packets = pkt_cnt;

        g_bt_rx_data.is_update = 1;  //用来标记改组数据是否得到更新,即,是否是从底层取得的有效值.
    } else {
        /* Try get data from Chip */
        ENG_LOG("ADL %s(), status is 1, try again.", __func__);
        g_bt_rx_data.is_update = 0;

        {
            unsigned char ble = 0;
            if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
                ble = 0;
            } else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
                ble = 1;
            }

#if defined(SPRD_WCNBT_MARLIN) || defined(SPRD_WCNBT_SR2351)
            g_sBtInterface->get_nonsig_rx_data(ble);
#endif
        }
    }

    ENG_LOG("ADL leaving %s(), ", __func__);
}

/********************************************************************
*   name   dut_mode_recv
*   ---------------------------
*   descrition: when driver get rxdata, this callback is called
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   ------------------
*   other:
*
********************************************************************/
void dut_mode_recv(uint16_t opcode, uint8_t *buf, uint8_t len) {
    int ret = 0;

    ENG_LOG("ADL entry %s(), opcode = %d, len = %d", __func__, opcode, len);

    if (HCI_DUT_GET_RXDATA == opcode && 2 == len) {
        char status = -1;
        char rssi = -1;

        status = *buf;
        rssi = *(buf + 1);
        ENG_LOG("ADL %s(), DUT MODE RECV, RX DATA : status = %d, RSSI = %d", __func__, status,
                rssi);

        if (0 == status) {
            g_bt_rx_data.rssi = rssi;
            g_bt_rx_data.is_update = 1;
        } else {
            /* Try get data from Chip */
            ENG_LOG("ADL %s(), status is 0, try again.", __func__);
            g_bt_rx_data.is_update = 0;
            ret = g_sBtInterface->dut_mode_send(HCI_DUT_GET_RXDATA, NULL, 0);
            ENG_LOG("ADL %s(), called dut_mode_send(HCI_DUT_GET_RXDATA), ret = %d", __func__, ret);
        }
    } else {
        /* ERROR */
        ENG_LOG("ADL leaving %s(), IN Parameters is ERROR, return.", __func__);
        return;
    }

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
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
static int bteut_load(void) {
    int err = 0;

    hw_module_t *module;
    hw_device_t *device;

    ENG_LOG("ADL entry %s()", __func__);

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const **)&module);
    if (err == 0) {
        err = module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            g_bt_device = (bluetooth_device_t *)device;
            g_sBtInterface = g_bt_device->get_bluetooth_interface();
        }
    }

    ENG_LOG("ADL leaving %s(), err = %s", __func__, strerror(err));

    return err;
}

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
static void bteut_unload(void) {
    ENG_LOG("ADL entry %s()", __func__);

    g_sBtInterface = NULL;

    ENG_LOG("ADL leaving %s()", __func__);
}

static bool set_wake_alarm(uint64_t delay_millis, bool UNUSED should_wake, alarm_cb cb,
                           void *data) {
    static timer_t timer;
    static bool timer_created;

    if (!timer_created) {
        struct sigevent sigevent;
        memset(&sigevent, 0, sizeof(sigevent));
        sigevent.sigev_notify = SIGEV_THREAD;
        sigevent.sigev_notify_function = (void (*)(union sigval))cb;
        sigevent.sigev_value.sival_ptr = data;
        timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
        timer_created = true;
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = delay_millis / 1000;
    new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    timer_settime(timer, 0, &new_value, NULL);

    return true;
}

static int acquire_wake_lock(const UNUSED char *lock_name) { return BT_STATUS_SUCCESS; }

static int release_wake_lock(const UNUSED char *lock_name) { return BT_STATUS_SUCCESS; }

static bt_os_callouts_t callouts = {
    sizeof(bt_os_callouts_t), set_wake_alarm, acquire_wake_lock, release_wake_lock,
};

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
static int bteut_init_bt(void) {
    int ret = -1;
    ENG_LOG("ADL entry %s()", __func__);

    g_bt_status = g_sBtInterface->init(&g_bt_callbacks);

    if (BT_STATUS_SUCCESS == g_bt_status) {
        g_bt_status = g_sBtInterface->set_os_callouts(&callouts);
    }

    if (BT_STATUS_SUCCESS == g_bt_status || BT_STATUS_DONE == g_bt_status) {
        ret = 0;
    }

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

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
    if (g_bteut_bt_enabled) {
        ENG_LOG("ADL leaving %s(), Bluetooth is already enabled", __func__);
        return -1;
    }

    g_bt_status = g_sBtInterface->enable(false);

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
    if (!g_bteut_bt_enabled) {
        ENG_LOG("ADL leaving %s()Bluetooth is already disabled", __func__);
        return -1;
    }

    g_bt_status = g_sBtInterface->disable();

    if (BT_STATUS_SUCCESS == g_bt_status || BT_STATUS_DONE == g_bt_status) {
        ret = 0;
    }

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
}

/********************************************************************
*   name   bt_adapter_state_changed
*   ---------------------------
*   descrition: indicate bt adapter state when its change
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
static void bt_adapter_state_changed(bt_state_t state) {
    ENG_LOG("ADL entry %s(), ADAPTER STATE UPDATED = %s", __func__,
            (BT_STATE_OFF == state) ? "OFF" : "ON");

    if (state == BT_STATE_ON) {
        g_bteut_bt_enabled = 1;
    } else {
        g_bteut_bt_enabled = 0;
    }

    ENG_LOG("ADL leaving %s(), g_bteut_bt_enabled = %d", __func__, g_bteut_bt_enabled);
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

    ret = g_sBtInterface->le_test_mode(HCI_LE_TRANSMITTER_TEST_OPCODE, buf, 3);
    ENG_LOG("ADL %s(), callED le_test_mode(HCI_LE_TRANSMITTER_TEST_OPCODE), ret = %d", __func__,
            ret);

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
    ret = g_sBtInterface->le_test_mode(HCI_LE_RECEIVER_TEST_OPCODE, buf, 1);
    ENG_LOG("ADL %s(), callED le_test_mode(HCI_LE_RECEIVER_TEST_OPCODE), ret = %d", __func__, ret);

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

    ret = g_sBtInterface->le_test_mode(HCI_LE_END_TEST_OPCODE, NULL, 0);
    ENG_LOG("ADL %s(), callED le_test_mode(HCI_LE_END_TEST_OPCODE), ret = %d", __func__, ret);

    ENG_LOG("ADL leaving %s(), ret = %d", __func__, ret);
    return ret;
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

/****************************************end of the
 * file*************************************************/
