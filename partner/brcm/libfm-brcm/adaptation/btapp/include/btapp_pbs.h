/*****************************************************************************
**
**  Name:             btapp_pbs.h
**
**  Description:     This file contains btapp interface
**				     definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/


#ifndef BTAPP_PBS_H
#define BTAPP_PBS_H

enum
{
  BTUI_PBS_DISABLE_ST = 0,
  BTUI_PBS_CLOSE_ST,
  BTUI_PBS_CONNECTED_ST
};

enum
{
  BTA_PBS_PATH_INVALID = 0,
  BTA_PBS_PATH_PHONE_PB,
  BTA_PBS_PATH_PHONE_ICH,
  BTA_PBS_PATH_PHONE_OCH,
  BTA_PBS_PATH_PHONE_MCH,
  BTA_PBS_PATH_PHONE_CCH,
  BTA_PBS_PATH_SIM_PB,
  BTA_PBS_PATH_SIM_ICH,
  BTA_PBS_PATH_SIM_OCH,
  BTA_PBS_PATH_SIM_MCH,
  BTA_PBS_PATH_SIM_CCH
};

typedef struct
{
    UINT8 path;
    int   index;
} tBTUI_PBS_PATH;

typedef struct
{
    int fd;
#if( defined BTA_PBS_INCLUDED ) && (BTA_PBS_INCLUDED == TRUE)
    UINT8 state;
    tBTA_PBS_ACCESS_TYPE  access_flag;    /* BTA_PBS_ACCESS_TYPE_ALLOW or BTA_PBS_ACCESS_TYPE_FORBID */
#endif
} tBTUI_PBS_CB;

extern tBTUI_PBS_CB btui_pbs_cb;

extern void btapp_pbs_init(void);
extern void btapp_pbs_disable(void);

#endif
