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
 ******************************************************************************/
/*!
  \file
  \brief 
*/
/*******************************************************************************
 * $Id: protocolubx.h 59522 2012-06-18 08:07:42Z jon.bowern $
 ******************************************************************************/
#ifndef __PROTOCOLUBX_H__
#define __PROTOCOLUBX_H__

#include "protocol.h"

// ubx specific type definitions used 
#ifdef _MSC_VER
typedef __int8					I1;		//!< signed 1 byte integer
typedef __int16					I2;		//!< signed 2 byte integer
typedef __int32					I4;		//!< signed 4 byte integer
typedef __int64					I8;		//!< signed 8 byte integer
typedef unsigned __int8			U1;		//!< unsigned 1 byte integer
typedef unsigned __int16		U2;		//!< unsigned 2 byte integer
typedef unsigned __int32		U4;		//!< unsigned 4 byte integer
typedef unsigned __int64		U8;		//!< unsigned 8 byte integer
#else
#include <stdint.h>
typedef int8_t					I1; 	//!< signed 1 byte integer
typedef int16_t					I2; 	//!< signed 2 byte integer
typedef int32_t					I4; 	//!< signed 4 byte integer
typedef int64_t					I8;		//!< signed 8 byte integer
typedef uint8_t					U1; 	//!< unsigned 1 byte integer
typedef uint16_t				U2; 	//!< unsigned 2 byte integer
typedef uint32_t				U4; 	//!< unsigned 4 byte integer
typedef uint64_t				U8; 	//!< unsigned 8 byte integer
#endif
typedef char					CH; 	//!< character

#ifdef SUPL_ENABLED	
#include "ubx_messageDef.h"
#endif

class CProtocolUBX : public CProtocol
{
public:
	enum { 
		UBX_CHAR_SYNC0 = 0xB5 /* 'µ' */,
		UBX_CHAR_SYNC1 = 0x62 /* 'b' */,
		UBX_FRM_SIZE   = 8,
		UBX_MAX_SIZE   = 2*1024,
	};

	int Parse(unsigned char* pBuffer, int iSize);
	static int ParseFunc(unsigned char* pBuffer, int iSize);
	void __drv_floatUsed Process(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	PROTOCOL_t GetType(void) { return UBX; }
    int NewMsg(U1 classId, U1 msgId, void* pPayload, int iPayloadSize, unsigned char **ppMsg);

protected:
	void __drv_floatUsed ProcessNavSol(   unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessNavPvt(   unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
	void __drv_floatUsed ProcessNavSvInfo(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
#ifdef SUPL_ENABLED		
	void __drv_floatUsed ProcessRxmMeas(unsigned char* pBuffer, int iSize, CDatabase* pDatabase);
#endif
	
	void __drv_floatUsed CheckSetTtag(CDatabase* pDatabase, U4 ttag);
};

#endif //__PROTOCOLUBX_H__
