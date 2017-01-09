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
 * $Id: protocolubx.cpp 62559 2012-10-17 04:21:47Z michael.ammann $
 ******************************************************************************/
#include "stdafx.h"

#include "gpsconst.h"
#include "protocolubx.h"
#include "parserbuffer.h"

#include "ubx_log.h"

int CProtocolUBX::Parse(unsigned char* pBuffer, int iSize) 
{
	return ParseFunc(pBuffer, iSize);
}

int CProtocolUBX::ParseFunc(unsigned char* pBuffer, int iSize) 
{
	if (iSize == 0) 
		return CParserBuffer::WAIT;
	if (pBuffer[0] != UBX_CHAR_SYNC0) 
		return CParserBuffer::NOT_FOUND;
	if (iSize == 1) 
		return CParserBuffer::WAIT;
	if (pBuffer[1] != UBX_CHAR_SYNC1) 
		return CParserBuffer::NOT_FOUND;
	if (iSize < 6) 
		return CParserBuffer::WAIT;
	U2 iLength =   ((U2)pBuffer[4]) + 
				  (((U2)pBuffer[5]) << 8);
	// filter out all large messages (with the exception of the tunneling class messages)
	if ((iLength > UBX_MAX_SIZE) && 
		(pBuffer[2] != 0x08/*tunneling class*/))
		return CParserBuffer::NOT_FOUND;
	if (iSize < iLength + 6)
		return CParserBuffer::WAIT;
	// calculate the cksum
	U1 ckA = 0;
	U1 ckB = 0;
	for (int i = 2; i < iLength + 6; i++)
	{
		ckA += pBuffer[i];
		ckB += ckA;
	}
	// check the cksum
	if (iSize < iLength + UBX_FRM_SIZE-1)
		return CParserBuffer::WAIT;
	if (pBuffer[iLength+6] != ckA)
		return CParserBuffer::NOT_FOUND;
	if (iSize < iLength + UBX_FRM_SIZE)
		return CParserBuffer::WAIT;
	if (pBuffer[iLength+7] != ckB)
		return CParserBuffer::NOT_FOUND;
	return iLength + UBX_FRM_SIZE;
} 

void CProtocolUBX::Process(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	// enable next line if you like noise
	if      (pBuffer[2] == 0x01/*NAV*/ && pBuffer[3] == 0x06/*SOL*/)	ProcessNavSol(   pBuffer, iSize, pDatabase);
	else if (pBuffer[2] == 0x01/*NAV*/ && pBuffer[3] == 0x07/*PVT*/)	ProcessNavPvt(   pBuffer, iSize, pDatabase);
	else if (pBuffer[2] == 0x01/*NAV*/ && pBuffer[3] == 0x30/*SVINFO*/)	ProcessNavSvInfo(pBuffer, iSize, pDatabase);
#ifdef SUPL_ENABLED
	else if (pBuffer[2] == 0x02/*RXM*/ && pBuffer[3] == 0x12/*MEAS*/)	ProcessRxmMeas(pBuffer, iSize, pDatabase);
#endif	
}

int CProtocolUBX::NewMsg(U1 classId, U1 msgId, void* pPayload, int iPayloadSize, unsigned char **ppMsg)
{
    int nRetVal = iPayloadSize + UBX_FRM_SIZE;

    if ( ppMsg )
    {
        U1* pBuffer = new U1[nRetVal];

	    if (pBuffer)
	    {
           *ppMsg = pBuffer;

		    pBuffer[0] = UBX_CHAR_SYNC0;
		    pBuffer[1] = UBX_CHAR_SYNC1;
		    pBuffer[2] = classId;
		    pBuffer[3] = msgId;
		    pBuffer[4] = (U1) iPayloadSize;
		    pBuffer[5] = (U1)(iPayloadSize >> 8);
		    memcpy(&pBuffer[6], pPayload, iPayloadSize);

		    // calculate the cksum
		    U1 ckA = 0;
		    U1 ckB = 0;
		    for (int i = 2; i < iPayloadSize + 6; i++)
		    {
			    ckA += pBuffer[i];
			    ckB += ckA;
		    }
		    pBuffer[6+iPayloadSize] = ckA;
		    pBuffer[7+iPayloadSize] = ckB;
	    }
        else
        {
            nRetVal = 0;
            *ppMsg = NULL;
        }
    }
    else
    {
        nRetVal = 0;
    }

	return nRetVal;
}

void CProtocolUBX::ProcessNavSol(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	#pragma pack (push, 1)
	typedef struct {
		U4 tow; I4 frac; I2 week; U1 fix; U1 flags;
		I4 X; I4 Y;	I4 Z; U4 acc; I4 VX; I4 VY; I4 VZ; U4 Vacc;
		U2 dop;	U1 flags2; U1 nsv; U4 res2;
	} STRUCT;
	#pragma pack (pop)
	enum { GPSFIXOK = 0x01, DGPS  = 0x02, WEEKSET  = 0x04, TOWSET  = 0x08 };
	if (iSize == (int)(sizeof(STRUCT) + UBX_FRM_SIZE))
	{
		STRUCT s, *p=&s;
		memcpy(p, &pBuffer[6], sizeof(s));
		//STRUCT* p = (STRUCT*)&pBuffer[6];
		pDatabase->MsgOnce(CDatabase::MSG_UBX_NAVSOL);
		CheckSetTtag(pDatabase, p->tow);
		if (p->flags & TOWSET)
			pDatabase->Set(CDatabase::DATA_UBX_GPSTIME_TOW,				p->tow * 1e-3 + p->frac * 1e-9);
		if (p->flags & WEEKSET)
			pDatabase->Set(CDatabase::DATA_UBX_GPSTIME_WEEK,			p->week);
		pDatabase->Set(CDatabase::DATA_UBX_GPSFIXOK,					(p->flags & GPSFIXOK)>0);
		pDatabase->Set(CDatabase::DATA_UBX_GPSFIX,						p->fix);
		pDatabase->Set(CDatabase::DATA_UBX_DGPS,						(p->flags & DGPS)>0);
		if ((p->flags & GPSFIXOK)>0)
		{
			pDatabase->Set(CDatabase::DATA_UBX_POSITION_ECEF_X,			p->X * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_POSITION_ECEF_Y,			p->Y * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_POSITION_ECEF_Z,			p->Z * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_POSITION_ECEF_ACCURACY,	p->acc * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_VELOCITY_ECEF_VX,		p->VX * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_VELOCITY_ECEF_VY,		p->VY * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_VELOCITY_ECEF_VZ,		p->VZ * 1e-2); 
			pDatabase->Set(CDatabase::DATA_UBX_VELOCITY_ECEF_ACCURACY,	p->Vacc * 1e-2); 
		}
		pDatabase->Set(CDatabase::DATA_POSITION_DILUTION_OF_PRECISION,	p->dop * 1e-2);
		pDatabase->Set(CDatabase::DATA_SATELLITES_USED_COUNT,			p->nsv);
	}
}

void CProtocolUBX::ProcessNavPvt(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	#pragma pack (push, 1)
	typedef struct {
		U4 tow; U2 year; U1 month/*1-12*/; U1 day/*1-31*/; U1 hour/*0-23*/; U1 min/*0-59*/; U1 sec/*0-60*/; U1 valid; 
		U4 tAcc; I4 nano; U1 fixType; U1 flags; U1 flags2; U1 numSv;
		I4 lon/*1e-7 deg*/; I4 lat/*1e-7 deg*/; I4 height/*mm*/; I4 hMSL/*mm*/; U4 hAcc/*mm*/; U4 vAcc/*mm*/;
		I4 velN/*mm/s*/; I4 velE/*mm/s*/; I4 velD/*mm/s*/; I4 gSpeed/*mm/s*/; I4 heading/*1e-5deg*/;
		U4 sAcc/*mm/s*/; I4 headingAcc/*1e-5deg*/; U2 pDOP; U2 reserved2; U4 revInfo;
	} STRUCT;
	#pragma pack (pop)
	enum { 
		DATE = 0x01, TIME = 0x02, FULLYRESOLVED = 0x04,			// valid
		GNSSFIXOK = 0x01, DGPS = 0x02, PSMSTATE = 0x1C,						// flags
		TE = 0x01, SUFPOS = 0x02, OMITCHECK = 0x04, FIXVALIDATED = 0x04,	// flags2
	};
	if (iSize == (int)(sizeof(STRUCT) + UBX_FRM_SIZE))
	{
		STRUCT s, *p=&s;
		memcpy(p, &pBuffer[6], sizeof(s));
		//STRUCT* p = (STRUCT*)&pBuffer[6];
		pDatabase->MsgOnce(CDatabase::MSG_UBX_NAVPVT);
		CheckSetTtag(pDatabase, p->tow);
		if (p->valid & TIME)
		{
			int hour = p->hour;
			int min = p->min;
			double sec = p->sec + p->nano * 1e-9;
			if (sec < 0.0)
			{
				sec += 60.0;
				if (min > 0)
					min --;
				else if (hour > 0)
				{
					min += 59;
					hour --;
				}
				else 
					sec = 0.0; // truncate that fractional part, yes we could loose some precision
			}
			pDatabase->Set(CDatabase::DATA_TIME_HOUR,   hour);
			pDatabase->Set(CDatabase::DATA_TIME_MINUTE, min);
			pDatabase->Set(CDatabase::DATA_TIME_SECOND, sec);
		}
		if (p->valid & DATE)
		{
			pDatabase->Set(CDatabase::DATA_DATE_YEAR,  p->year);
			pDatabase->Set(CDatabase::DATA_DATE_MONTH, p->month);
			pDatabase->Set(CDatabase::DATA_DATE_DAY,   p->day);
		}
		pDatabase->Set(CDatabase::DATA_UBX_GPSFIXOK,					(p->flags & GNSSFIXOK)>0);
		pDatabase->Set(CDatabase::DATA_UBX_GPSFIX,						p->fixType);
		pDatabase->Set(CDatabase::DATA_UBX_DGPS,						(p->flags & DGPS)>0);
		if ((p->flags & GNSSFIXOK)>0)
		{
			pDatabase->Set(CDatabase::DATA_LATITUDE_DEGREES,			p->lat * 1e-7); 
			pDatabase->Set(CDatabase::DATA_LONGITUDE_DEGREES,			p->lon * 1e-7); 
			pDatabase->Set(CDatabase::DATA_ERROR_RADIUS_METERS,			p->hAcc * 1e-3);
			pDatabase->Set(CDatabase::DATA_ALTITUDE_ELLIPSOID_METERS,	p->height * 1e-3);
			pDatabase->Set(CDatabase::DATA_ALTITUDE_SEALEVEL_METERS,	p->hMSL * 1e-3);
			pDatabase->Set(CDatabase::DATA_ALTITUDE_ELLIPSOID_ERROR_METERS, p->vAcc * 1e-3);
			pDatabase->Set(CDatabase::DATA_SPEED_KNOTS,					p->gSpeed * (1e-3 / METERS_PER_NAUTICAL_MILE));
			pDatabase->Set(CDatabase::DATA_TRUE_HEADING_DEGREES,		p->heading * 1e-5);
		}
		pDatabase->Set(CDatabase::DATA_POSITION_DILUTION_OF_PRECISION,	p->pDOP * 1e-2);
		pDatabase->Set(CDatabase::DATA_SATELLITES_USED_COUNT,			p->numSv);
	}
}

void CProtocolUBX::ProcessNavSvInfo(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	#pragma pack (push, 1)
	typedef struct { U4 tow; U1 nch; U1 globalFlags; U2 res2; } STRUCT;
	typedef struct { U1 ch; U1 svid; U1 flags; I1 qi; U1 cno; I1 el; I2 az; I4 prres; } STRUCT_CH;
	#pragma pack (pop)
	enum { USED = 0x01, DGPS = 0x02, ALM = 0x04, EPH = 0x08, nHEALTH = 0x10, ALP = 0x20, AMB = 0x80 };
	if (iSize >= (int)(sizeof(STRUCT) + UBX_FRM_SIZE))
	{
		STRUCT s, *p=&s;
		memcpy(p, &pBuffer[6], sizeof(s));
		//STRUCT* p = (STRUCT*)&pBuffer[6];
		pDatabase->MsgOnce(CDatabase::MSG_UBX_SVINFO);
		CheckSetTtag(pDatabase, p->tow);
		if (iSize == (int)(sizeof(STRUCT) + UBX_FRM_SIZE + p->nch * sizeof(STRUCT_CH)))
		{
			STRUCT_CH ch, *pCh = &ch;
			//STRUCT_CH* pCh = (STRUCT_CH*)(p+1);
			int ixInView = 0;
			int ixUsed = 0;
			for(int ix=0; ix < p->nch; ix++/*, pCh++*/) 
			{
				memcpy(pCh, &pBuffer[6+sizeof(STRUCT)+ix*sizeof(STRUCT_CH)], sizeof(ch));
				unsigned int svid = CDatabase::ConvertPrn2NmeaSvid(pCh->svid);
				bool bAzEl = (pCh->az >= -180) && (pCh->az <= 360) && (pCh->el >= -90) && (pCh->el <= 90);
				if (svid && (ixInView <= CDatabase::MAX_SATELLITES_IN_VIEW) && (bAzEl || pCh->cno))
				{
					pDatabase->Set(DATA_SATELLITES_IN_VIEW_PRNS_(ixInView), svid);
					if (bAzEl)
					{
						pDatabase->Set(DATA_SATELLITES_IN_VIEW_ELEVATION_(ixInView), (double)pCh->el);
						pDatabase->Set(DATA_SATELLITES_IN_VIEW_AZIMUTH_(ixInView),	 CDatabase::Degrees360(pCh->az));
					}
					if (pCh->cno)
						pDatabase->Set(DATA_SATELLITES_IN_VIEW_STN_RATIO_(ixInView), (double)pCh->cno);
					pDatabase->Set(DATA_UBX_SATELLITES_IN_VIEW_ORB_STA_(ixInView), 
								((pCh->flags & ALM) ? 2 : 0) | 
								((pCh->flags & EPH) ? 1 : 0));
					ixInView ++;
					// not used:  pch->ch,  (pch->prres*1e-2)
				}
				if (svid && (ixUsed <= CDatabase::MAX_SATELLITES_USED) && (pCh->flags & USED))
				{
					pDatabase->Set(DATA_SATELLITES_USED_PRNS_(ixUsed), svid);
					ixUsed++;
				}
			}
			pDatabase->Set(CDatabase::DATA_SATELLITES_IN_VIEW,	  ixInView);
			pDatabase->Set(CDatabase::DATA_SATELLITES_USED_COUNT, ixUsed);
		}
	}
}

#ifdef SUPL_ENABLED	
void CProtocolUBX::ProcessRxmMeas(unsigned char* pBuffer, int iSize, CDatabase* pDatabase)
{
	if (iSize < (int)(sizeof(GPS_UBX_RXM_MEAS_t) + UBX_FRM_SIZE))
	{
		// No enough data received
		return;
	}
	
	pBuffer += 6;
	GPS_UBX_RXM_MEAS_t rmxMeasHeader;
	memcpy(&rmxMeasHeader, pBuffer, sizeof(rmxMeasHeader));
	pBuffer += sizeof(rmxMeasHeader);
		
	I4 svCount = rmxMeasHeader.info & 0xFF;
	I4 dopCenter = ((I4)rmxMeasHeader.info) >> 8;
	
	if (iSize < (int) ((sizeof(GPS_UBX_RXM_MEAS_t) + UBX_FRM_SIZE + (svCount * sizeof(GPS_UBX_RXM_MEAS_SVID_t)))))
	{
		// Not enough data received
		return;
	}

	pDatabase->Set(CDatabase::DATA_UBX_GNSS_TOW, rmxMeasHeader.gnssTow);
	pDatabase->Set(CDatabase::DATA_UBX_GNSS_DOP_CENTER, dopCenter);
	
	I4 gpsSvCount = 0;
	GPS_UBX_RXM_MEAS_SVID_t svData;
	for(I4 i = 0; i < svCount; i++)
	{
		memcpy(&svData, pBuffer, sizeof(GPS_UBX_RXM_MEAS_SVID_t));
		pBuffer += sizeof(GPS_UBX_RXM_MEAS_SVID_t);

		if ((svData.svid > 0) && (svData.svid <= 32))
		{
			pDatabase->Set(DATA_UBX_SATELLITES_IN_MEAS_(gpsSvCount), svData.svid);
			pDatabase->Set(DATA_UBX_SATELLITES_IN_MEAS_CNO_(gpsSvCount), svData.cno);
			
			R8 prRms = svData.prRms * 0.5;
			pDatabase->Set(DATA_UBX_SATELLITES_IN_MEAS_PRRMS_(gpsSvCount), prRms);
			pDatabase->Set(DATA_UBX_SATELLITES_IN_MEAS_MULTIPATH_IND_(gpsSvCount), svData.mpInd);
			pDatabase->Set(DATA_UBX_SATELLITES_IN_MEAS_REL_CODEPHASE_(gpsSvCount), svData.redSigtow);  
			pDatabase->Set(DATA_UBX_SATELLITES_IN_MEAS_DOPPLER_(gpsSvCount), svData.doppler);
			gpsSvCount++;
			
//			LOGV("%s: satId %3d - cno %2d - doppler %8.2f chips %10.5f", 
//				 __FUNCTION__, svData.svid, svData.cno, 
//				 (double) svData.doppler / (1<<12), 
//				 ((double)(svData.redSigtow & 0x1FFFFF) * 1023.0) / 0x200000);
		}
	}
	
	pDatabase->Set(CDatabase::DATA_UBX_SATELLITES_IN_MEAS_COUNT, gpsSvCount);
//	LOGV("%s: gpsSvCount %d", __FUNCTION__, gpsSvCount);
}
#endif

void CProtocolUBX::CheckSetTtag(CDatabase* pDatabase, U4 ttag)
{
	double d = ttag * 1e-3;
	if (!pDatabase->Check(CDatabase::DATA_UBX_TTAG, d))
		pDatabase->Commit();
	pDatabase->Set(CDatabase::DATA_UBX_TTAG, d);
}




