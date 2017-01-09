
/*****************************************************************************
**
**  Name:             btapp_bts.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2009, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/


#ifndef BTAPP_BTS_H
#define BTAPP_BTS_H

int btapp_bts_convert_rc_chan_to_scn(int rc_channel);
UINT32 bts_register_sdp_ftp(int scn, char *service_name);
UINT32 bts_register_sdp_ops(int scn, char *p_service_name);

#endif

