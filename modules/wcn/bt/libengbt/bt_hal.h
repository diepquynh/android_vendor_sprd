/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
  **/
#ifndef BT_LIBENGBT_BT_HAL_SPRD_H_
#define BT_LIBENGBT_BT_HAL_SPRD_H_

#ifdef HAS_SPRD_BUILDCFG
#include "bdroid_buildcfg.h"
#include "btif_test.h"
#else
typedef void (*nonsig_test_rx_recv_callback)(bt_status_t status, uint8_t rssi, uint32_t pkt_cnt,
                                                uint32_t pkt_err_cnt,uint32_t bit_cnt,uint32_t bit_err_cnt);

#include "bt_types.h"
#include "hcidefs.h"
#include "btm_api.h"


#endif


typedef struct {
    /** set to sizeof(btav_callbacks_t) */
    size_t      size;
    nonsig_test_rx_recv_callback nonsig_test_rx_recv_cb;
    dut_mode_recv_callback dut_mode_recv_cb;
    le_test_mode_callback le_test_mode_cb;
} bthal_callbacks_t;



typedef struct {

    size_t          size;
    int (*enable)(bthal_callbacks_t* callbacks);
    int (*disable)(void);
    int (*dut_mode_configure)(uint8_t mode);
    int (*dut_mode_send)(uint16_t opcode, uint8_t *buf, uint8_t len);
    int (*le_test_mode)(uint16_t opcode, uint8_t *buf, uint8_t len);
    uint8_t (*is_enable)(void);
    int (*set_nonsig_tx_testmode)(uint16_t enable, uint16_t le,
                        uint16_t pattern, uint16_t channel, uint16_t pac_type,
                        uint16_t pac_len, uint16_t power_type, uint16_t power_value,
                        uint16_t pac_cnt);
    int (*set_nonsig_rx_testmode)(uint16_t enable, uint16_t le, uint16_t pattern, uint16_t channel,
                        uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t addr);
    int (*get_nonsig_rx_data)(uint16_t le);
} bt_test_kit_t;

const bt_test_kit_t *bt_test_kit_get_interface(void);


#endif  // BT_LIBENGBT_BT_ENG_SPRD_H_
