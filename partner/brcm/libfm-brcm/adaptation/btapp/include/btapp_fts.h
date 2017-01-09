/*****************************************************************************
**
**  Name:             btapp_fts.h
**
**  Description:     This file contains btapp interface
**				     definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/


#ifndef BTAPP_FTS_H
#define BTAPP_FTS_H

typedef struct
{
    UINT32  bytes_transferred;
#if( defined BTA_FT_INCLUDED ) && (BTA_FT_INCLUDED == TRUE)
    tBTA_FT_ACCESS  access_flag;    /* BTA_FT_ACCESS_ALLOW or BTA_FT_ACCESS_FORBID */
#endif
} tBTUI_FTS_CB;

extern tBTUI_FTS_CB btui_fts_cb;

extern void btapp_fts_init(void);
extern void btapp_fts_disable(void);

#endif
