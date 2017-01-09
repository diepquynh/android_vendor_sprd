
#ifndef __BT_ENG_SPRD_H__
#define __BT_ENG_SPRD_H__

#include <android/log.h>
#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "BTENG"

#define BTENG_DEBUG

#ifdef BTENG_DEBUG
#define BTENG_LOGD(x...) ALOGD(x)
#define BTENG_LOGE(x...) ALOGE(x)
#else
#define BTENG_LOGD(x...) \
    do {                 \
    } while (0)
#define BTENG_LOGE(x...) \
    do {                 \
    } while (0)
#endif

#define UNUSED __attribute__((unused))

#define OK_STR "OK"
#define FAIL_STR "FAIL"

#define CASE_RETURN_STR(const) \
    case const:                \
        return #const;
#define UNUSED __attribute__((unused))

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10) /* 0xFC00 */
#define NONSIG_TX_ENABLE (0x00D1 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_TX_DISABLE (0x00D2 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_ENABLE (0x00D3 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_GETDATA (0x00D4 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_DISABLE (0x00D5 | HCI_GRP_VENDOR_SPECIFIC)

#define NONSIG_LE_TX_ENABLE (0x00D6 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_TX_DISABLE (0x00D7 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_ENABLE (0x00D8 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_GETDATA (0x00D9 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_DISABLE (0x00DA | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_DUT_SET_TXPWR (0x00E1 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_SET_RXGIAN (0x00E2 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_GET_RXDATA (0x00E3 | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_LE_RECEIVER_TEST_OPCODE (0x201D)
#define HCI_LE_TRANSMITTER_TEST_OPCODE (0x201E)
#define HCI_LE_END_TEST_OPCODE (0x201F)

typedef enum {
    BT_ENG_NONE_ERROR,
    BT_ENG_CMD_INVALID,
    BT_ENG_PARA_INVALID,
    BT_ENG_HAL_LOAD_ERROR,
    BT_ENG_INIT_ERROR,
    BT_ENG_ENABLE_ERROR,
    BT_ENG_DISABLE_ERROR,
    BT_ENG_SET_DUT_MODE_ERROR,
    BT_ENG_CLEANUP_ERROR,
    BT_ENG_STATUS_ERROR,
    BT_ENG_TX_TEST_ERROR,
    BT_ENG_RX_TEST_ERROR,
    BT_ENG_RX_TEST_RECV_DATA_ERROR
} BT_ENG_ERROR_E;

#endif
