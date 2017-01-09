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

#ifndef BT_LIBBT_INCLUDE_BTIF_TEST_H_
#define BT_LIBBT_INCLUDE_BTIF_TEST_H_


#define BT_PROFILE_TEST_ID "test"

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


/** SPRD Test Interface: Non-signal Test Mode RX data Callback */
/* Receive Non-signal Test Mode RX data from controller. transmit it to APP*/
typedef void (*nonsig_test_rx_recv_callback)(bt_status_t status, uint8_t rssi, uint32_t pkt_cnt,
                                                uint32_t pkt_err_cnt,uint32_t bit_cnt,uint32_t bit_err_cnt);


/** BT-Test callback structure. */
typedef struct {
    /** set to sizeof(btav_callbacks_t) */
    size_t      size;
    nonsig_test_rx_recv_callback nonsig_test_rx_recv_cb;
    dut_mode_recv_callback dut_mode_recv_cb;
} btest_callbacks_t;



typedef struct {

    /** set to sizeof(btdut_interface_t) */
    size_t          size;
    /**
     * Register the BtDut callbacks
     */
    bt_status_t (*init)( btest_callbacks_t* callbacks );

    /**
     * SPRD Test Interface: Enter No Signalling TX Test mode.
     */
    bt_status_t (*set_nonsig_tx_testmode)(uint16_t enable, uint16_t le,
                        uint16_t pattern, uint16_t channel, uint16_t pac_type,
                        uint16_t pac_len, uint16_t power_type, uint16_t power_value,
                        uint16_t pac_cnt);

    /**
     * SPRD Test Interface: No Signalling RX Test mode.
     */
    bt_status_t (*set_nonsig_rx_testmode)(uint16_t enable, uint16_t le, uint16_t pattern, uint16_t channel,
                        uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t addr);

    /**
     * SPRD Test Interface: Inquire nonsig rx data.
     */
    bt_status_t (*get_nonsig_rx_data)(uint16_t le);
    bt_status_t (*dut_mode_send)(uint16_t opcode, uint8_t *buf, uint8_t len);
} btest_interface_t;

extern const btest_interface_t *btif_test_get_interface(void);


#endif  // BT_LIBBT_INCLUDE_BTIF_TEST_H_
