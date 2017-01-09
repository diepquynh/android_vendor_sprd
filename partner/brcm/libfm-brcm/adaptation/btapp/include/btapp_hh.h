/*****************************************************************************
**
**  Name:             btapp_hh.h
**
**  Description:     This file contains btapp interface definition
**
**  Copyright (c) 2000-2007, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_HH_H
#define BTAPP_HH_H

/* HH - HID host */
#ifndef BTUI_HH_MAX_HID
#define BTUI_HH_MAX_HID         4
#endif

#define BTUI_HH_APP_ID_MI       0x01
#define BTUI_HH_APP_ID_KB       0x02

typedef struct
{
    UINT8      dev_handle;
    BOOLEAN    is_connected;
    BD_ADDR    bd_addr;

#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))
    int        fd;
#else
    BOOLEAN    pre_mod_key[4]; /* used only for keybd boot report parsing */
#endif
} tBTUI_HH_DEVICE;

typedef struct
{
    tCTRL_HANDLE     btl_if_handle;
    tBTUI_HH_DEVICE  connected_hid[BTUI_HH_MAX_HID];
    UINT32           connected_dev_num;
    tBTUI_HH_DEVICE  *p_curr_dev;
} tBTUI_HH_CB;

extern tBTUI_HH_CB btui_hh_cb;


extern void  btapp_hh_init(void);
extern void  btapp_hh_disable(void);

extern tBTUI_HH_DEVICE *btapp_hh_find_dev_by_handle(UINT8 handle);
extern tBTUI_HH_DEVICE *btapp_hh_find_unused_dev_entry(void);

#endif
