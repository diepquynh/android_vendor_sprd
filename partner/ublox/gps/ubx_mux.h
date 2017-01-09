/*******************************************************************************
 *
 * Copyright (C) u-blox ag
 * u-blox ag, Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * This  source  file  is  the  sole  property  of  u-blox  AG. Reproduction or
 * utilization of this source in whole or part is forbidden without the written 
 * consent of u-blox AG.
 *
 *******************************************************************************
 *
 * Project: PE_AND
 *
 ******************************************************************************/
/*!
	\file
	\brief MUX message definitions

*/
/*******************************************************************************
 * $Id: ubx_mux.h 56360 2012-03-08 17:52:11Z philippe.kehl $
 ******************************************************************************/

#ifndef __UBX_MUX_H__
#define __UBX_MUX_H__

#define UDP_MUX_PACKET_HEADER   'X'

//! policies for the flags 
//@{
#define CLIENT_NMEA_ENABLED     0x01          //!< enable the receiving of NMEA messages
#define CLIENT_UBX_ENABLED      0x02          //!< enable the receiving of UBX messages
#define CLIENT_MUX_ENABLED      0x04          //!< enable the receiving of MUX messages
#define CLIENT_TRASPARENT_MODE  0x08          //!< the client will receive all the traffic
//@}

#define UDP_MUX_PING            0x00
#define UDP_MUX_START_SESSION   0x01
#define UDP_MUX_NUM_CLIENT      0x02
#define UDP_MUX_END_SESSION     0x03

#endif //__UBX_MUX_H__
