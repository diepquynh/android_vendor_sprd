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
	\brief UBX message definitions

*/
/***********************************************************
 * $Id: ubx_messageDef.h 60116 2012-07-05 15:09:23Z jon.bowern $
 ***********************************************************/

#ifndef __PROTO_UBX_STRUCT_DEF_H_10174BF8_39D22F9A
#define __PROTO_UBX_STRUCT_DEF_H_10174BF8_39D22F9A

#include <stdint.h>

/*----------------*/
/* Standard types */
/*----------------*/
typedef signed int			   I;   //!<  = I4
typedef unsigned int		   U;   //!<  = U4
typedef unsigned long		   BL;  //!<  = L4 (TRUE or FALSE only)
typedef	signed char			   I1;	//!<  signed 1 byte integer
typedef	signed short		   I2;	//!<  signed 2 byte integer
typedef	signed int			   I4;	//!<  signed 4 byte integer
typedef	int64_t                I8;  //!<  signed 8 byte integer
typedef	unsigned char		   U1;	//!<  unsigned 1 byte integer
typedef	unsigned char		   X1;	//!<  unsigned 1 byte integer, to be interpreted as bitmask
typedef	unsigned short	       U2;	//!<  unsigned 2 byte integer
typedef	unsigned short		   X2;	//!<  unsigned 2 byte integer, to be interpreted as bitmask
typedef	unsigned int		   U4;	//!<  unsigned 4 byte integer
typedef	unsigned int		   X4;	//!<  unsigned 4 byte integer, to be interpreted as bitmask
typedef	uint64_t               U8;	//!<  unsigned 8 byte integer
typedef	float				   R4;	//!<  4 byte floating point
typedef	double				   R8;	//!<  8 byte floating point
typedef	char				   CH;	//!<  ASCII character
typedef	unsigned char		   L1;	//!<  1 byte logical (TRUE or FALSE only)
typedef	unsigned short		   L2;	//!<  2 byte logical (TRUE or FALSE only)
typedef	unsigned int		   L4;	//!<  4 byte logical (TRUE or FALSE only)
typedef	unsigned int		    L;	//!<  4 byte logical (TRUE or FALSE only)

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

//! UBX Protocol Header
typedef struct GPS_UBX_HEAD_s
{
	U2 prefix;					//!< Prefix, always #GPS_UBX_PREFIX
	U1 classId;					//!< UBX Class Id
	U1 msgId;					//!< UBX Message Id
	U2 size;					//!< Payload Size
} GPS_UBX_HEAD_t, *GPS_UBX_HEAD_pt;

#define GPS_UBX_SYNC_CHAR_1 0xB5u    //!< First synchronization character of UBX Protocol
#define GPS_UBX_SYNC_CHAR_2 0x62u    //!< Second synchronization character of UBX Protocol
#define GPS_UBX_PREFIX      (GPS_UBX_SYNC_CHAR_2<<8|GPS_UBX_SYNC_CHAR_1) //!< UBX Protocol Prefix
#define GPS_UBX_PREFIX_SIZE 2u       //!< UBX Protocol Prefix Size in bytes
#define GPS_UBX_CHKSUM_SIZE 2u       //!< UBX Protocol Checksum Size in bytes

#define GPS_UBX_HEAD_SIZE sizeof(GPS_UBX_HEAD_t)                      //!< UBX Protocol Header Size
#define GPS_UBX_FRAME_SIZE (GPS_UBX_HEAD_SIZE+GPS_UBX_CHKSUM_SIZE) //!< Total size of the UBX Frame



//================================================================
//! NAV_POSECEF: Periodic/Polled 
/*!
Position Solution in ECEF 
-


This Message's id is #UBXID_NAV_POSECEF
*/
//================================================================

typedef struct GPS_UBX_NAV_POSECEF_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	I4  ecefX;                    //!< ECEF X coordinate 
	I4  ecefY;                    //!< ECEF Y coordinate 
	I4  ecefZ;                    //!< ECEF Z coordinate 
	U4  pAcc;                     //!< Position Accuracy Estimate 

} GPS_UBX_NAV_POSECEF_t,*GPS_UBX_NAV_POSECEF_pt;


#define UBXID_NAV_POSECEF 0x0101 //!< message id for NAV-POSECEF
	

//================================================================
//! NAV_POSLLH: Periodic/Polled 
/*!
Geodetic Position Solution 
This message outputs the Geodetic position in the currently  selected Ellipsoid. The default is the WGS84 Ellipsoid, but  can be changed with the message <r href=\"CFG-DAT'>CFG-DAT</r>. 


This Message's id is #UBXID_NAV_POSLLH
*/
//================================================================

typedef struct GPS_UBX_NAV_POSLLH_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	I4  lon;                      //!< Longitude 
	I4  lat;                      //!< Latitude 
	I4  height;                   //!< Height above Ellipsoid 
	I4  hMSL;                     //!< Height above mean sea level 
	U4  hAcc;                     //!< Horizontal Accuracy Estimate 
	U4  vAcc;                     //!< Vertical Accuracy Estimate 

} GPS_UBX_NAV_POSLLH_t,*GPS_UBX_NAV_POSLLH_pt;


#define UBXID_NAV_POSLLH 0x0102 //!< message id for NAV-POSLLH
	

//================================================================
//! NAV_STATUS: Periodic/Polled 
/*!
Receiver Navigation Status 
-


This Message's id is #UBXID_NAV_STATUS
*/
//================================================================

typedef struct GPS_UBX_NAV_STATUS_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	U1  gpsFix;                   //!< GPSfix Type 
	X1  flags;                    //!< Navigation Status Flags  
	X1  diffStat;                 //!< Differential Status 
	U1  res;                      //!< Reserved 
	U4  ttff;                     //!< Time to first fix (millisecond time tag) 
	U4  msss;                     //!< Milliseconds since Startup / Reset 

} GPS_UBX_NAV_STATUS_t,*GPS_UBX_NAV_STATUS_pt;


//! \name Bit Definitions for #GPS_UBX_NAV_STATUS_s::flags
//@{
#define GPS_UBX_NAV_STATUS_FLAGS_GPSFIXOK_MASK 0x01  //!< Mask for field gpsFixOk in bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_GPSFIXOK_GET(val)  (U)(((val)&GPS_UBX_NAV_STATUS_FLAGS_GPSFIXOK_MASK)>>0)  //!< Get gpsFixOk from bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_DIFFSOLN_MASK 0x02  //!< Mask for field diffSoln in bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_DIFFSOLN_GET(val)  (U)(((val)&GPS_UBX_NAV_STATUS_FLAGS_DIFFSOLN_MASK)>>1)  //!< Get diffSoln from bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_WKNSET_MASK 0x04  //!< Mask for field wknSet in bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_WKNSET_GET(val)  (U)(((val)&GPS_UBX_NAV_STATUS_FLAGS_WKNSET_MASK)>>2)  //!< Get wknSet from bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_TOWSET_MASK 0x08  //!< Mask for field towSet in bitmask flags
#define GPS_UBX_NAV_STATUS_FLAGS_TOWSET_GET(val)  (U)(((val)&GPS_UBX_NAV_STATUS_FLAGS_TOWSET_MASK)>>3)  //!< Get towSet from bitmask flags

//@}
//! \name Bit Definitions for #GPS_UBX_NAV_STATUS_s::diffStat
//@{
#define GPS_UBX_NAV_STATUS_DIFFSTAT_DGPSISTAT_MASK 0x03  //!< Mask for field dgpsIStat in bitmask diffStat
#define GPS_UBX_NAV_STATUS_DIFFSTAT_DGPSISTAT_GET(val)  (U)(((val)&GPS_UBX_NAV_STATUS_DIFFSTAT_DGPSISTAT_MASK)>>0)  //!< Get dgpsIStat from bitmask diffStat

//@}


#define UBXID_NAV_STATUS 0x0103 //!< message id for NAV-STATUS
	

//================================================================
//! NAV_DOP: Periodic/Polled 
/*!
Dilution of precision 
* DOP values are dimensionless.  
* All DOP values are scaled by a factor of 100. that is, if the unit transmits a value of e.g. 156, it means that the DOP value is 1.56. 


This Message's id is #UBXID_NAV_DOP
*/
//================================================================

typedef struct GPS_UBX_NAV_DOP_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	U2  gDOP;                     //!< Geometric DOP 
	U2  pDOP;                     //!< Position DOP 
	U2  tDOP;                     //!< Time DOP 
	U2  vDOP;                     //!< Vertical DOP 
	U2  hDOP;                     //!< Horizontal DOP 
	U2  nDOP;                     //!< Northing DOP 
	U2  eDOP;                     //!< Easting DOP 

} GPS_UBX_NAV_DOP_t,*GPS_UBX_NAV_DOP_pt;


#define UBXID_NAV_DOP 0x0104 //!< message id for NAV-DOP
	

//================================================================
//! NAV_SOL: Periodic/Polled
/*!
Navigation Solution Information
This message is combiningPosition, velocity and time solution in ECEF,including accuracy figures


This Message's id is #UBXID_NAV_SOL
*/
//================================================================

typedef struct GPS_UBX_NAV_SOL_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week
	I4  fTOW;                     //!< Fractional Nanoseconds remainder of rounded ms above, range -500000 .. 500000
	I2  week;                     //!< GPS week (GPS time)
	U1  gpsFix;                   //!< GPSfix Type, range 0..4
	X1  flags;                    //!< Fix Status Flags
	I4  ecefX;                    //!< ECEF X coordinate
	I4  ecefY;                    //!< ECEF Y coordinate
	I4  ecefZ;                    //!< ECEF Z coordinate
	U4  pAcc;                     //!< 3D Position Accuracy Estimate
	I4  ecefVX;                   //!< ECEF X velocity
	I4  ecefVY;                   //!< ECEF Y velocity
	I4  ecefVZ;                   //!< ECEF Z velocity
	U4  sAcc;                     //!< Speed Accuracy Estimate
	U2  pDOP;                     //!< Position DOP
	U1  res1;                     //!< reserved
	U1  numSV;                    //!< Number of SVs used in Nav Solution
	U4  res2;                     //!< reserved

} GPS_UBX_NAV_SOL_t,*GPS_UBX_NAV_SOL_pt;


//! \name Bit Definitions for #GPS_UBX_NAV_SOL_s::flags
//@{
#define GPS_UBX_NAV_SOL_FLAGS_GPSFIXOK_MASK 0x01  //!< Mask for field GPSfixOK in bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_GPSFIXOK_GET(val)  (U)(((val)&GPS_UBX_NAV_SOL_FLAGS_GPSFIXOK_MASK)>>0)  //!< Get GPSfixOK from bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_DIFFSOLN_MASK 0x02  //!< Mask for field DiffSoln in bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_DIFFSOLN_GET(val)  (U)(((val)&GPS_UBX_NAV_SOL_FLAGS_DIFFSOLN_MASK)>>1)  //!< Get DiffSoln from bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_WKNSET_MASK 0x04  //!< Mask for field WKNSET in bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_WKNSET_GET(val)  (U)(((val)&GPS_UBX_NAV_SOL_FLAGS_WKNSET_MASK)>>2)  //!< Get WKNSET from bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_TOWSET_MASK 0x08  //!< Mask for field TOWSET in bitmask flags
#define GPS_UBX_NAV_SOL_FLAGS_TOWSET_GET(val)  (U)(((val)&GPS_UBX_NAV_SOL_FLAGS_TOWSET_MASK)>>3)  //!< Get TOWSET from bitmask flags

//@}


#define UBXID_NAV_SOL 0x0106 //!< message id for NAV-SOL
	

//================================================================
//! NAV_VELECEF: Periodic/Polled 
/*!
Velocity Solution in ECEF 
-


This Message's id is #UBXID_NAV_VELECEF
*/
//================================================================

typedef struct GPS_UBX_NAV_VELECEF_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	I4  ecefVX;                   //!< ECEF X velocity 
	I4  ecefVY;                   //!< ECEF Y velocity 
	I4  ecefVZ;                   //!< ECEF Z velocity 
	U4  sAcc;                     //!< Speed Accuracy Estimate 

} GPS_UBX_NAV_VELECEF_t,*GPS_UBX_NAV_VELECEF_pt;


#define UBXID_NAV_VELECEF 0x0111 //!< message id for NAV-VELECEF
	

//================================================================
//! NAV_VELNED: Periodic/Polled 
/*!
Velocity Solution in NED 
-


This Message's id is #UBXID_NAV_VELNED
*/
//================================================================

typedef struct GPS_UBX_NAV_VELNED_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	I4  velN;                     //!< NED north velocity 
	I4  velE;                     //!< NED east velocity 
	I4  velD;                     //!< NED down velocity 
	U4  speed;                    //!< Speed (3-D) 
	U4  gSpeed;                   //!< Ground Speed (2-D) 
	I4  heading;                  //!< Heading 2-D 
	U4  sAcc;                     //!< Speed Accuracy Estimate 
	U4  cAcc;                     //!< Course / Heading Accuracy Estimate 

} GPS_UBX_NAV_VELNED_t,*GPS_UBX_NAV_VELNED_pt;


#define UBXID_NAV_VELNED 0x0112 //!< message id for NAV-VELNED
	

//================================================================
//! NAV_TIMEGPS: Periodic/Polled 
/*!
GPS Time Solution 
-


This Message's id is #UBXID_NAV_TIMEGPS
*/
//================================================================

typedef struct GPS_UBX_NAV_TIMEGPS_s
{
	U4  iTOW;                     //!< GPS Millisecond time of Week 
	I4  fTOW;                     //!< Fractional Nanoseconds remainder of rounded ms above, range -500000 .. 500000 
	I2  week;                     //!< GPS week (GPS time) 
	I1  leapS;                    //!< Leap Seconds (GPS-UTC) 
	X1  valid;                    //!< Validity Flags
	U4  tAcc;                     //!< Time Accuracy Estimate 

} GPS_UBX_NAV_TIMEGPS_t,*GPS_UBX_NAV_TIMEGPS_pt;


//! \name Bit Definitions for #GPS_UBX_NAV_TIMEGPS_s::valid
//@{
#define GPS_UBX_NAV_TIMEGPS_VALID_TOW_MASK 0x01  //!< Mask for field tow in bitmask valid
#define GPS_UBX_NAV_TIMEGPS_VALID_TOW_GET(val)  (U)(((val)&GPS_UBX_NAV_TIMEGPS_VALID_TOW_MASK)>>0)  //!< Get tow from bitmask valid
#define GPS_UBX_NAV_TIMEGPS_VALID_WEEK_MASK 0x02  //!< Mask for field week in bitmask valid
#define GPS_UBX_NAV_TIMEGPS_VALID_WEEK_GET(val)  (U)(((val)&GPS_UBX_NAV_TIMEGPS_VALID_WEEK_MASK)>>1)  //!< Get week from bitmask valid
#define GPS_UBX_NAV_TIMEGPS_VALID_UTC_MASK 0x04  //!< Mask for field utc in bitmask valid
#define GPS_UBX_NAV_TIMEGPS_VALID_UTC_GET(val)  (U)(((val)&GPS_UBX_NAV_TIMEGPS_VALID_UTC_MASK)>>2)  //!< Get utc from bitmask valid

//@}


#define UBXID_NAV_TIMEGPS 0x0120 //!< message id for NAV-TIMEGPS
	

//================================================================
//! NAV_TIMEUTC: Periodic/Polled 
/*!
UTC Time Solution 
-


This Message's id is #UBXID_NAV_TIMEUTC
*/
//================================================================

typedef struct GPS_UBX_NAV_TIMEUTC_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of Week 
	U4  tAcc;                     //!< Time Accuracy Estimate 
	I4  nano;                     //!< Nanoseconds of second, range -500000000 .. 500000000 (UTC) 
	U2  year;                     //!< Year, range 1999..2099 (UTC) 
	U1  month;                    //!< Month, range 1..12 (UTC) 
	U1  day;                      //!< Day of Month, range 1..31 (UTC) 
	U1  hour;                     //!< Hour of Day, range 0..23 (UTC) 
	U1  min;                      //!< Minute of Hour, range 0..59 (UTC) 
	U1  sec;                      //!< Seconds of Minute, range 0..59 (UTC) 
	X1  valid;                    //!< Validity Flags

} GPS_UBX_NAV_TIMEUTC_t,*GPS_UBX_NAV_TIMEUTC_pt;


//! \name Bit Definitions for #GPS_UBX_NAV_TIMEUTC_s::valid
//@{
#define GPS_UBX_NAV_TIMEUTC_VALID_VALIDTOW_MASK 0x01  //!< Mask for field validTOW in bitmask valid
#define GPS_UBX_NAV_TIMEUTC_VALID_VALIDTOW_GET(val)  (U)(((val)&GPS_UBX_NAV_TIMEUTC_VALID_VALIDTOW_MASK)>>0)  //!< Get validTOW from bitmask valid
#define GPS_UBX_NAV_TIMEUTC_VALID_VALIDWKN_MASK 0x02  //!< Mask for field validWKN in bitmask valid
#define GPS_UBX_NAV_TIMEUTC_VALID_VALIDWKN_GET(val)  (U)(((val)&GPS_UBX_NAV_TIMEUTC_VALID_VALIDWKN_MASK)>>1)  //!< Get validWKN from bitmask valid
#define GPS_UBX_NAV_TIMEUTC_VALID_VALIDUTC_MASK 0x04  //!< Mask for field validUTC in bitmask valid
#define GPS_UBX_NAV_TIMEUTC_VALID_VALIDUTC_GET(val)  (U)(((val)&GPS_UBX_NAV_TIMEUTC_VALID_VALIDUTC_MASK)>>2)  //!< Get validUTC from bitmask valid

//@}


#define UBXID_NAV_TIMEUTC 0x0121 //!< message id for NAV-TIMEUTC
	

//================================================================
//! NAV_CLOCK: Periodic/Polled 
/*!
Clock Solution  
-


This Message's id is #UBXID_NAV_CLOCK
*/
//================================================================

typedef struct GPS_UBX_NAV_CLOCK_s
{
	U4  iTOW;                     //!< GPS Millisecond Time of week 
	I4  clkB;                     //!< clock bias in nanoseconds 
	I4  clkD;                     //!< clock drift in nanoseconds per second 
	U4  tAcc;                     //!< Time Accuracy Estimate 
	U4  fAcc;                     //!< Frequency Accuracy Estimate 

} GPS_UBX_NAV_CLOCK_t,*GPS_UBX_NAV_CLOCK_pt;


#define UBXID_NAV_CLOCK 0x0122 //!< message id for NAV-CLOCK
	

//================================================================
//! NAV_SVINFO: Periodic/Polled 
/*!
Space Vehicle Information 
-


This Message's id is #UBXID_NAV_SVINFO
*/
//================================================================

typedef struct GPS_UBX_NAV_SVINFO_s
{
	U4  iTOW;                     //!< GPS Millisecond time of week 
	U1  numCh;                    //!< Number of channels 
	X1  globalFlags;              //!< Bitmask
	U2  res2;                     //!< Reserved 
	//REPEAT: GPS_UBX_NAV_SVINFO_CHN_t repeat0[numCh];          

} GPS_UBX_NAV_SVINFO_t,*GPS_UBX_NAV_SVINFO_pt;


//! \name Bit Definitions for #GPS_UBX_NAV_SVINFO_s::globalFlags
//@{
#define GPS_UBX_NAV_SVINFO_GLOBALFLAGS_ISU5_MASK 0x01  //!< Mask for field isU5 in bitmask globalFlags
#define GPS_UBX_NAV_SVINFO_GLOBALFLAGS_ISU5_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_GLOBALFLAGS_ISU5_MASK)>>0)  //!< Get isU5 from bitmask globalFlags

//@}


//! Optional Sub-Structure of #GPS_UBX_NAV_SVINFO_t
typedef struct GPS_UBX_NAV_SVINFO_CHN_s
{
	U1  chn;                      //!< channel number 
	U1  svid;                     //!< Satellite ID 
	X1  flags;                    //!< Bitmask
	X1  quality;                  //!< Bitfield
	U1  cno;                      //!< Carrier to Noise Ratio (Signal Strength) 
	I1  elev;                     //!< Elevation in integer degrees 
	I2  azim;                     //!< Azimuth in integer degrees 
	I4  prRes;                    //!< Pseudo range residual in centimetres 

} GPS_UBX_NAV_SVINFO_CHN_t,*GPS_UBX_NAV_SVINFO_CHN_pt;

//! \name Bit Definitions for #GPS_UBX_NAV_SVINFO_CHN_s::flags
//@{
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_SVUSED_MASK 0x01  //!< Mask for field svUsed in bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_SVUSED_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_FLAGS_SVUSED_MASK)>>0)  //!< Get svUsed from bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_DIFFCORR_MASK 0x02  //!< Mask for field diffCorr in bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_DIFFCORR_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_FLAGS_DIFFCORR_MASK)>>1)  //!< Get diffCorr from bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITAVAIL_MASK 0x04  //!< Mask for field orbitAvail in bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITAVAIL_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITAVAIL_MASK)>>2)  //!< Get orbitAvail from bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITEPH_MASK 0x08  //!< Mask for field orbitEph in bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITEPH_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITEPH_MASK)>>3)  //!< Get orbitEph from bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_UNHEALTHY_MASK 0x10  //!< Mask for field unhealthy in bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_UNHEALTHY_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_FLAGS_UNHEALTHY_MASK)>>4)  //!< Get unhealthy from bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITALM_MASK 0x20  //!< Mask for field orbitAlm in bitmask flags
#define GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITALM_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_FLAGS_ORBITALM_MASK)>>5)  //!< Get orbitAlm from bitmask flags

//@}
//! \name Bit Definitions for #GPS_UBX_NAV_SVINFO_CHN_s::quality
//@{
#define GPS_UBX_NAV_SVINFO_CHN_QUALITY_QUALITYIND_MASK 0x0F  //!< Mask for field qualityInd in bitmask quality
#define GPS_UBX_NAV_SVINFO_CHN_QUALITY_QUALITYIND_GET(val)  (U)(((val)&GPS_UBX_NAV_SVINFO_CHN_QUALITY_QUALITYIND_MASK)>>0)  //!< Get qualityInd from bitmask quality

//@}


#define UBXID_NAV_SVINFO 0x0130 //!< message id for NAV-SVINFO
	

//================================================================
//! NAV_SBAS: Periodic/Polled 
/*!
SBAS Status Data 
This message outputs the status of the SBAS sub system 


This Message's id is #UBXID_NAV_SBAS
*/
//================================================================

typedef struct GPS_UBX_NAV_SBAS_s
{
	U4  iTOW;                     //!< GPS Millisecond time of week 
	U1  geo;                      //!< PRN Number of the GEO where correction and integrity data is used from 
	U1  mode;                     //!< SBAS Mode 
	I1  sys;                      //!< SBAS System (WAAS/EGNOS/...) 
	X1  service;                  //!< SBAS Services available 
	U1  cnt;                      //!< Number of SV data following 
	U1  res[3];                   //!< Reserved 
	//REPEAT: GPS_UBX_NAV_SBAS_SVID_t repeat0[cnt];            

} GPS_UBX_NAV_SBAS_t,*GPS_UBX_NAV_SBAS_pt;


//! \name Bit Definitions for #GPS_UBX_NAV_SBAS_s::service
//@{
#define GPS_UBX_NAV_SBAS_SERVICE_RANGING_MASK 0x01  //!< Mask for field Ranging in bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_RANGING_GET(val)  (U)(((val)&GPS_UBX_NAV_SBAS_SERVICE_RANGING_MASK)>>0)  //!< Get Ranging from bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_CORRECTIONS_MASK 0x02  //!< Mask for field Corrections in bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_CORRECTIONS_GET(val)  (U)(((val)&GPS_UBX_NAV_SBAS_SERVICE_CORRECTIONS_MASK)>>1)  //!< Get Corrections from bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_INTEGRITY_MASK 0x04  //!< Mask for field Integrity in bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_INTEGRITY_GET(val)  (U)(((val)&GPS_UBX_NAV_SBAS_SERVICE_INTEGRITY_MASK)>>2)  //!< Get Integrity from bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_TESTMODE_MASK 0x08  //!< Mask for field Testmode in bitmask service
#define GPS_UBX_NAV_SBAS_SERVICE_TESTMODE_GET(val)  (U)(((val)&GPS_UBX_NAV_SBAS_SERVICE_TESTMODE_MASK)>>3)  //!< Get Testmode from bitmask service

//@}


//! Optional Sub-Structure of #GPS_UBX_NAV_SBAS_t
typedef struct GPS_UBX_NAV_SBAS_SVID_s
{
	U1  svid;                     //!< SV Id 
	U1  flags;                    //!< Flags for this SV 
	U1  udre;                     //!< Monitoring status  
	U1  svSys;                    //!< System (WAAS/EGNOS/...) 
	U1  svService;                //!< Services available 
	U1  res0;                     //!< Reserved 
	I2  prc;                      //!< Pseudo Range correction in [cm] 
	I2  res1;                     //!< Reserved 
	I2  ic;                       //!< Ionosphere correction in [cm] 

} GPS_UBX_NAV_SBAS_SVID_t,*GPS_UBX_NAV_SBAS_SVID_pt;



#define UBXID_NAV_SBAS 0x0132 //!< message id for NAV-SBAS
	

//================================================================
//! RXM_RAW: Periodic/Polled
/*!
Raw Measurement Data
This message contains all information needed to be able to generate a <a href=\"http://www.aiub-download.unibe.ch/rinex/'>RINEX</a> file.


This Message's id is #UBXID_RXM_RAW
*/
//================================================================

typedef struct GPS_UBX_RXM_RAW_s
{
	I4  iTOW;                     //!< Measurement integer millisecond GPS time of week  (Receiver Time)
	I2  week;                     //!< Measurement GPS week number (Receiver Time).
	U1  numSV;                    //!< # of satellites following.
	U1  res1;                     //!< Reserved
	//REPEAT: GPS_UBX_RXM_RAW_CPMES_t repeat0[numSV];          

} GPS_UBX_RXM_RAW_t,*GPS_UBX_RXM_RAW_pt;


//! Optional Sub-Structure of #GPS_UBX_RXM_RAW_t
typedef struct GPS_UBX_RXM_RAW_CPMES_s
{
	R8  cpMes;                    //!< Carrier phase measurement [L1 cycles]
	R8  prMes;                    //!< Pseudorange measurement [m]
	R4  doMes;                    //!< Doppler measurement [Hz]
	U1  sv;                       //!< Space Vehicle Number
	I1  mesQI;                    //!< Nav Measurements Quality Indicator:
	I1  cno;                      //!< Signal strength C/No. (dbHz)
	U1  lli;                      //!< Loss of lock indicator (RINEX definition)

} GPS_UBX_RXM_RAW_CPMES_t,*GPS_UBX_RXM_RAW_CPMES_pt;



#define UBXID_RXM_RAW 0x0210 //!< message id for RXM-RAW
	

//================================================================
//! RXM_SFRB: Periodic
/*!
Subframe Buffer
The content of one single subframe buffer
+For GPS satellites, the 10 dwrd values contain the parity checked subframe data for 10 Words. Each dwrd has 24 Bits with valid data (Bits 23 to 0). The remaining 8 bits (31 to 24) have an undefined value. The direction within the Word is that the higher order bits are received from the SV first. Example: The Preamble can be found in dwrd[0], at bit position 23 down to 16. For more details on the data format please refer to the ICD-GPS-200C Interface document.
+For SBAS satellites, the 250 Bit message block can be found in dwrd[0] to dwrd[6] for the first 224 bits. The remaining 26 bits are in dwrd[7], whereas Bits 25 and 24 are the last two data bits, and Bits 23 down to 0 are the parity bits. For more information on SBAS data format, please refer to RTCA/DO-229C (MOPS), Appendix A.


This Message's id is #UBXID_RXM_SFRB
*/
//================================================================

typedef struct GPS_UBX_RXM_SFRB_s
{
	U1  chn;                      //!< Channel Number
	U1  svid;                     //!< ID of Satellite transmitting Subframe
	X4  dwrd[10];                 //!< Words of Data

} GPS_UBX_RXM_SFRB_t,*GPS_UBX_RXM_SFRB_pt;


#define UBXID_RXM_SFRB 0x0211 //!< message id for RXM-SFRB
	

//================================================================
//! RXM_SVSI: Periodic/Polled
/*!
SV Status Info
Status of the receiver manager knowledge about GPS Orbit Validity


This Message's id is #UBXID_RXM_SVSI
*/
//================================================================

typedef struct GPS_UBX_RXM_SVSI_s
{
	I4  iTOW;                     //!< Measurement integer millisecond GPS time of week
	I2  week;                     //!< Measurement GPS week number.
	U1  numVis;                   //!< number of visible satellites
	U1  numSV;                    //!< number of per-SV data blocks following
	//REPEAT: GPS_UBX_RXM_SVSI_SVID_t repeat0[numSV];          

} GPS_UBX_RXM_SVSI_t,*GPS_UBX_RXM_SVSI_pt;


//! Optional Sub-Structure of #GPS_UBX_RXM_SVSI_t
typedef struct GPS_UBX_RXM_SVSI_SVID_s
{
	U1  svid;                     //!< Satellite ID
	X1  svFlag;                   //!< Information Flags
	I2  azim;                     //!< Azimuth
	I1  elev;                     //!< Elevation
	X1  age;                      //!< Age of Almanach and Ephemeris: 

} GPS_UBX_RXM_SVSI_SVID_t,*GPS_UBX_RXM_SVSI_SVID_pt;

//! \name Bit Definitions for #GPS_UBX_RXM_SVSI_SVID_s::svFlag
//@{
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_URA_MASK 0x0F  //!< Mask for field ura in bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_URA_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_SVFLAG_URA_MASK)>>0)  //!< Get ura from bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_HEALTHY_MASK 0x10  //!< Mask for field healthy in bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_HEALTHY_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_SVFLAG_HEALTHY_MASK)>>4)  //!< Get healthy from bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_EPHVAL_MASK 0x20  //!< Mask for field ephVal in bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_EPHVAL_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_SVFLAG_EPHVAL_MASK)>>5)  //!< Get ephVal from bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_ALMVAL_MASK 0x40  //!< Mask for field almVal in bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_ALMVAL_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_SVFLAG_ALMVAL_MASK)>>6)  //!< Get almVal from bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_NOTAVAIL_MASK 0x80  //!< Mask for field notAvail in bitmask svFlag
#define GPS_UBX_RXM_SVSI_SVID_SVFLAG_NOTAVAIL_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_SVFLAG_NOTAVAIL_MASK)>>7)  //!< Get notAvail from bitmask svFlag

//@}
//! \name Bit Definitions for #GPS_UBX_RXM_SVSI_SVID_s::age
//@{
#define GPS_UBX_RXM_SVSI_SVID_AGE_ALMAGE_MASK 0x0F  //!< Mask for field almAge in bitmask age
#define GPS_UBX_RXM_SVSI_SVID_AGE_ALMAGE_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_AGE_ALMAGE_MASK)>>0)  //!< Get almAge from bitmask age
#define GPS_UBX_RXM_SVSI_SVID_AGE_EPHAGE_MASK 0xF0  //!< Mask for field ephAge in bitmask age
#define GPS_UBX_RXM_SVSI_SVID_AGE_EPHAGE_GET(val)  (U)(((val)&GPS_UBX_RXM_SVSI_SVID_AGE_EPHAGE_MASK)>>4)  //!< Get ephAge from bitmask age

//@}


#define UBXID_RXM_MEAS 0x0212 //!< message id for RXM-MEAS

typedef struct GPS_UBX_RXM_MEAS_s
{
	I4  gnssTow;                  //!< GNSS system time of week corresponding tonavigation epoch
	X4  info;                     //!< Satellite count and doppler center
	//REPEAT: GPS_UBX_RXM_MEAS_SVID_t repeat0[numSV];          

} GPS_UBX_RXM_MEAS_t,*GPS_UBX_RXM_MEAS_pt;


//! Optional Sub-Structure of #GPS_UBX_RXM_MEAS_t
typedef struct GPS_UBX_RXM_MEAS_SVID_s
{
	U1  svid;                     //!< Satellite ID
	U1  cno;                   	  //!< Signal C/N0
	U1  prRms;                    //!< prRms
	U1  mpInd;                    //!< Multipath indicator
	I4  redSigtow;                //!< Code phase measurement relative to gnssTow
	I4  doppler;				  //!< Doppler measurement

} GPS_UBX_RXM_MEAS_SVID_t,*GPS_UBX_RXM_MEAS_SVID_pt;




#define UBXID_RXM_SVSI 0x0220 //!< message id for RXM-SVSI
	

//================================================================
//! INF_ERROR: 
/*!
ASCII String output, indicating an error
This message has a variable length payload, representing an ASCII string.


This Message's id is #UBXID_INF_ERROR
*/
//================================================================

#define UBXID_INF_ERROR 0x0400 //!< message id for INF-ERROR
	

//================================================================
//! INF_WARNING: 
/*!
ASCII String output, indicating a warning
This message has a variable length payload, representing an ASCII string.


This Message's id is #UBXID_INF_WARNING
*/
//================================================================

#define UBXID_INF_WARNING 0x0401 //!< message id for INF-WARNING
	

//================================================================
//! INF_NOTICE: 
/*!
ASCII String output, with informational contents
This message has a variable length payload, representing an ASCII string.


This Message's id is #UBXID_INF_NOTICE
*/
//================================================================

#define UBXID_INF_NOTICE 0x0402 //!< message id for INF-NOTICE
	

//================================================================
//! INF_TEST: 
/*!
ASCII String output, indicating test output
This message has a variable length payload, representing an ASCII string.


This Message's id is #UBXID_INF_TEST
*/
//================================================================

#define UBXID_INF_TEST 0x0403 //!< message id for INF-TEST
	

//================================================================
//! INF_DEBUG: 
/*!
ASCII String output, indicating debug output
This message has a variable length payload, representing an ASCII string.


This Message's id is #UBXID_INF_DEBUG
*/
//================================================================

#define UBXID_INF_DEBUG 0x0404 //!< message id for INF-DEBUG
	

//================================================================
//! ACK_NAK: Answer
/*!
Message Not-Acknowledged
Output upon processing of an input message


This Message's id is #UBXID_ACK_NAK
*/
//================================================================

typedef struct GPS_UBX_ACK_NAK_s
{
	U1  clsID;                    //!< Class Id of the Not-Acknowledged Message
	U1  msgID;                    //!< Message Id of the Not-Acknowledged Message

} GPS_UBX_ACK_NAK_t,*GPS_UBX_ACK_NAK_pt;


#define UBXID_ACK_NAK 0x0500 //!< message id for ACK-NAK
	

//================================================================
//! ACK_ACK: Answer
/*!
Message Acknowledged
Output upon processing of an input message


This Message's id is #UBXID_ACK_ACK
*/
//================================================================

typedef struct GPS_UBX_ACK_ACK_s
{
	U1  clsID;                    //!< Class Id of the Acknowledged Message
	U1  msgID;                    //!< Message Id of the Acknowledged Message

} GPS_UBX_ACK_ACK_t,*GPS_UBX_ACK_ACK_pt;


#define UBXID_ACK_ACK 0x0501 //!< message id for ACK-ACK
	

//================================================================
//! CFG_PRT_POLL: Poll Request
/*!
Polls the configuration for one I/O Port
Sending this message with a port ID as payload results in having the receiver return the configuration for the given port. If the payload is omitted, the configuration for the incoming port is returned


This Message's id is #UBXID_CFG_PRT
*/
//================================================================

typedef struct GPS_UBX_CFG_PRT_POLL_s
{
	U1  PortID;                   //!< Port Identifier Number

} GPS_UBX_CFG_PRT_POLL_t,*GPS_UBX_CFG_PRT_POLL_pt;


#define UBXID_CFG_PRT 0x0600 //!< message id for CFG-PRT
	

//================================================================
//! CFG_PRT_UART_U5: Get/Set
/*!
Get/Set Port Configuration for UART, USB Port(s)
Several configurations can be concatenated to one input message. In this case the payload length can be a multiple of the normal length. Output messages from the module contain only one configuration unit. Note that some fields are interpreted differentlydepending on communication port type (see alternate descriptions of this message)

   \note: For the USB Port, the mode and baudrate fields are ignored.

This Message's id is #UBXID_CFG_PRT
*/
//================================================================

typedef struct GPS_UBX_CFG_PRT_UART_U5_s
{
	U1  portID;                   //!< Port Identifier Number
	U1  res0;                     //!< Reserved	
	U2  res1;                     //!< Reserved
	X4  mode;                     //!< A bit mask describing the UART mode
	U4  baudRate;                 //!< Baudrate in bits/second
	X2  inProtoMask;              //!< A mask describing which input protocols are active 
	X2  outProtoMask;             //!< A mask describing which output protocols are active.
	X2  flags;                    //!< Reserved, set to 0
	U2  pad;                      //!< Reserved, set to 0

} GPS_UBX_CFG_PRT_UART_U5_t,*GPS_UBX_CFG_PRT_UART_U5_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_PRT_UART_U5_s::mode
//@{
#define GPS_UBX_CFG_PRT_UART_U5_MODE_CHARLEN_MASK 0x000000C0  //!< Mask for field charLen in bitmask mode
#define GPS_UBX_CFG_PRT_UART_U5_MODE_CHARLEN_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_MODE_CHARLEN_MASK)>>6)  //!< Get charLen from bitmask mode
#define GPS_UBX_CFG_PRT_UART_U5_MODE_PARITY_MASK 0x00000E00  //!< Mask for field parity in bitmask mode
#define GPS_UBX_CFG_PRT_UART_U5_MODE_PARITY_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_MODE_PARITY_MASK)>>9)  //!< Get parity from bitmask mode
#define GPS_UBX_CFG_PRT_UART_U5_MODE_NSTOPBITS_MASK 0x00003000  //!< Mask for field nStopBits in bitmask mode
#define GPS_UBX_CFG_PRT_UART_U5_MODE_NSTOPBITS_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_MODE_NSTOPBITS_MASK)>>12)  //!< Get nStopBits from bitmask mode

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_PRT_UART_U5_s::inProtoMask
//@{
#define GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_UBX_MASK 0x0001  //!< Mask for field ubx in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_UBX_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_UBX_MASK)>>0)  //!< Get ubx from bitmask inProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_NMEA_MASK 0x0002  //!< Mask for field nmea in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_NMEA_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_NMEA_MASK)>>1)  //!< Get nmea from bitmask inProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_RTCM_MASK 0x0004  //!< Mask for field rtcm in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_RTCM_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_INPROTOMASK_RTCM_MASK)>>2)  //!< Get rtcm from bitmask inProtoMask

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_PRT_UART_U5_s::outProtoMask
//@{
#define GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_UBX_MASK 0x0001  //!< Mask for field ubx in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_UBX_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_UBX_MASK)>>0)  //!< Get ubx from bitmask outProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_NMEA_MASK 0x0002  //!< Mask for field nmea in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_NMEA_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_NMEA_MASK)>>1)  //!< Get nmea from bitmask outProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_RAW_MASK 0x0008  //!< Mask for field raw in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_RAW_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_UART_U5_OUTPROTOMASK_RAW_MASK)>>3)  //!< Get raw from bitmask outProtoMask

//@}


//#define UBXID_CFG_PRT 0x0600  // already defined, see above


//================================================================
//! CFG_PRT_SPI_U5: Get/Set
/*!
Get/Set Port Configuration for SPI Port(s)
Several configurations can be concatenated to one input message. In this case the payload length can be a multiple of the normal length. Output messages from the module contain only one configuration unit. Note that some fields are interpreted differentlydepending on communication port type (see alternate descriptions of this message)

   \note: Description of this message indicating the fields used in u-blox 5

This Message's id is #UBXID_CFG_PRT
*/
//================================================================

typedef struct GPS_UBX_CFG_PRT_SPI_U5_s
{
	U1  portID;                   //!< Port Identifier Number
	U1  res0;                     //!< Reserved	
	U2  res1;                     //!< Reserved
	X4  mode;                     //!< SPI Mode Flags
	U4  baudRate;                 //!< Unused, set to 0
	X2  inProtoMask;              //!< A mask describing which input protocols are active 
	X2  outProtoMask;             //!< A mask describing which output protocols are active.
	X2  flags;                    //!< Reserved, set to 0
	U2  pad;                      //!< Reserved, set to 0

} GPS_UBX_CFG_PRT_SPI_U5_t,*GPS_UBX_CFG_PRT_SPI_U5_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_PRT_SPI_U5_s::mode
//@{
#define GPS_UBX_CFG_PRT_SPI_U5_MODE_SPIMODE_MASK 0x00000006  //!< Mask for field spiMode in bitmask mode
#define GPS_UBX_CFG_PRT_SPI_U5_MODE_SPIMODE_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_MODE_SPIMODE_MASK)>>1)  //!< Get spiMode from bitmask mode
#define GPS_UBX_CFG_PRT_SPI_U5_MODE_FFCNT_MASK 0x0000FF00  //!< Mask for field ffCnt in bitmask mode
#define GPS_UBX_CFG_PRT_SPI_U5_MODE_FFCNT_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_MODE_FFCNT_MASK)>>8)  //!< Get ffCnt from bitmask mode

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_PRT_SPI_U5_s::inProtoMask
//@{
#define GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_UBX_MASK 0x0001  //!< Mask for field ubx in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_UBX_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_UBX_MASK)>>0)  //!< Get ubx from bitmask inProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_NMEA_MASK 0x0002  //!< Mask for field nmea in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_NMEA_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_NMEA_MASK)>>1)  //!< Get nmea from bitmask inProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_RTCM_MASK 0x0004  //!< Mask for field rtcm in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_RTCM_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_INPROTOMASK_RTCM_MASK)>>2)  //!< Get rtcm from bitmask inProtoMask

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_PRT_SPI_U5_s::outProtoMask
//@{
#define GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_UBX_MASK 0x0001  //!< Mask for field ubx in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_UBX_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_UBX_MASK)>>0)  //!< Get ubx from bitmask outProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_NMEA_MASK 0x0002  //!< Mask for field nmea in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_NMEA_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_NMEA_MASK)>>1)  //!< Get nmea from bitmask outProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_RAW_MASK 0x0008  //!< Mask for field raw in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_RAW_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_SPI_U5_OUTPROTOMASK_RAW_MASK)>>3)  //!< Get raw from bitmask outProtoMask

//@}


//#define UBXID_CFG_PRT 0x0600  // already defined, see above


//================================================================
//! CFG_PRT_I2C_U5: Get/Set
/*!
Get/Set Port Configuration for I2C Port(s)
Several configurations can be concatenated to one input message. In this case the payload length can be a multiple of the normal length. Output messages from the module contain only one configuration unit. Note that some fields are interpreted differentlydepending on communication port type (see alternate descriptions of this message)

   \note: Description of this message indicating the fields used in u-blox 5

This Message's id is #UBXID_CFG_PRT
*/
//================================================================

typedef struct GPS_UBX_CFG_PRT_I2C_U5_s
{
	U1  portID;                   //!< Port Identifier Number
	U1  res0;                     //!< Reserved	
	U2  res1;                     //!< Reserved
	X4  mode;                     //!< I2C Mode Flags
	U4  baudRate;                 //!< Unused, set to 0
	X2  inProtoMask;              //!< A mask describing which input protocols are active 
	X2  outProtoMask;             //!< A mask describing which output protocols are active.
	X2  flags;                    //!< Reserved, set to 0
	U2  pad;                      //!< Reserved, set to 0

} GPS_UBX_CFG_PRT_I2C_U5_t,*GPS_UBX_CFG_PRT_I2C_U5_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_PRT_I2C_U5_s::mode
//@{
#define GPS_UBX_CFG_PRT_I2C_U5_MODE_SLAVEADDR_MASK 0x000000FE  //!< Mask for field slaveAddr in bitmask mode
#define GPS_UBX_CFG_PRT_I2C_U5_MODE_SLAVEADDR_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_MODE_SLAVEADDR_MASK)>>1)  //!< Get slaveAddr from bitmask mode

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_PRT_I2C_U5_s::inProtoMask
//@{
#define GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_UBX_MASK 0x0001  //!< Mask for field ubx in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_UBX_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_UBX_MASK)>>0)  //!< Get ubx from bitmask inProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_NMEA_MASK 0x0002  //!< Mask for field nmea in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_NMEA_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_NMEA_MASK)>>1)  //!< Get nmea from bitmask inProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_RTCM_MASK 0x0004  //!< Mask for field rtcm in bitmask inProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_RTCM_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_INPROTOMASK_RTCM_MASK)>>2)  //!< Get rtcm from bitmask inProtoMask

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_PRT_I2C_U5_s::outProtoMask
//@{
#define GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_UBX_MASK 0x0001  //!< Mask for field ubx in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_UBX_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_UBX_MASK)>>0)  //!< Get ubx from bitmask outProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_NMEA_MASK 0x0002  //!< Mask for field nmea in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_NMEA_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_NMEA_MASK)>>1)  //!< Get nmea from bitmask outProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_RAW_MASK 0x0008  //!< Mask for field raw in bitmask outProtoMask
#define GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_RAW_GET(val)  (U)(((val)&GPS_UBX_CFG_PRT_I2C_U5_OUTPROTOMASK_RAW_MASK)>>3)  //!< Get raw from bitmask outProtoMask

//@}


//#define UBXID_CFG_PRT 0x0600  // already defined, see above


//================================================================
//! CFG_MSG_POLL: Poll Request
/*!
Poll a message configuration
-


This Message's id is #UBXID_CFG_MSG
*/
//================================================================

typedef struct GPS_UBX_CFG_MSG_POLL_s
{
	U1  classType;                //!< Message Class
	U1  msgID;                    //!< Message Identifier

} GPS_UBX_CFG_MSG_POLL_t,*GPS_UBX_CFG_MSG_POLL_pt;


#define UBXID_CFG_MSG 0x0601 //!< message id for CFG-MSG
	

//================================================================
//! CFG_MSG: Set/Get
/*!
Set Message Rate(s)
Set/Get message rate configuration (s) to/from the receiver
*  Several configurations can be concatenated to one input message. In this case the payload length can be a multiple of the normal length. Output messages from the module contain only one configuration unit.
*  Send rate is relative to the event a message is registered on. For example, if the rate of a navigation message is set to 2, the message is sent every second navigation solution.For configuring NMEA messages, the section <r href=\"NMEA_ids'>NMEA Messages Overview</r> describes Class and Identifier numbers used.


This Message's id is #UBXID_CFG_MSG
*/
//================================================================

typedef struct GPS_UBX_CFG_MSG_s
{
	U1  classType;                //!< Message Class
	U1  msgID;                    //!< Message Identifier
	U1  rate[4];                  //!< Send rate on I/O Target (4 Targets)

} GPS_UBX_CFG_MSG_t,*GPS_UBX_CFG_MSG_pt;


//#define UBXID_CFG_MSG 0x0601  // already defined, see above


//================================================================
//! CFG_MSG_SETCURRENT: Set/Get
/*!
Set Message Rate
Set message rate configuration for the current target


This Message's id is #UBXID_CFG_MSG
*/
//================================================================

typedef struct GPS_UBX_CFG_MSG_SETCURRENT_s
{
	U1  classType;                //!< Message Class
	U1  msgID;                    //!< Message Identifier
	U1  rate;                     //!< Send rate on current Target

} GPS_UBX_CFG_MSG_SETCURRENT_t,*GPS_UBX_CFG_MSG_SETCURRENT_pt;


//#define UBXID_CFG_MSG 0x0601  // already defined, see above


//================================================================
//! CFG_INF_POLL: Poll Request
/*!
Poll INF message configuration for one protocol
-


This Message's id is #UBXID_CFG_INF
*/
//================================================================

typedef struct GPS_UBX_CFG_INF_POLL_s
{
	U1  protocolID;               //!< Protocol Identifier, identifying the output protocol for this Poll Request. The following are valid Protocol Identifiers:

} GPS_UBX_CFG_INF_POLL_t,*GPS_UBX_CFG_INF_POLL_pt;


#define UBXID_CFG_INF 0x0602 //!< message id for CFG-INF
	

//================================================================
//! CFG_INF: Set/Get
/*!
Information message configuration
The value of INFMSG_mask<x> below are that each bit represents one of the INFclass messages (Bit 0 for ERROR, Bit 1 for WARNING and so on.). For a completelist, please see the <r href=\"UBX-INF'>Message Class INF</r>.Several configurations can be concatenated to one input message. In thiscase the payload length can be a multiple of the normal length. Output messages from the module contain only one configuration unit. Please note that I/O Targets 0, 1 and 2 correspond to serial ports 0, 1 and 2. I/O target 3 is reserved for future use.


This Message's id is #UBXID_CFG_INF
*/
//================================================================

typedef struct GPS_UBX_CFG_INF_s
{
	U1  protocolID;               //!< Protocol Identifier, identifying for which protocol the configuration is set/get. The following are valid Protocol Identifiers:
	U1  res0;                     //!< Reserved
	U2  res1;                     //!< Reserved
	X1  infMsgMask[4];            //!< A bit mask, saying which information messages are enabled on each I/O target

} GPS_UBX_CFG_INF_t,*GPS_UBX_CFG_INF_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_INF_s::infMsgMask
//@{
#define GPS_UBX_CFG_INF_INFMSGMASK_ERROR_MASK 0x01  //!< Mask for field ERROR in bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_ERROR_GET(val)  (U)(((val)&GPS_UBX_CFG_INF_INFMSGMASK_ERROR_MASK)>>0)  //!< Get ERROR from bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_WARNING_MASK 0x02  //!< Mask for field WARNING in bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_WARNING_GET(val)  (U)(((val)&GPS_UBX_CFG_INF_INFMSGMASK_WARNING_MASK)>>1)  //!< Get WARNING from bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_NOTICE_MASK 0x04  //!< Mask for field NOTICE in bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_NOTICE_GET(val)  (U)(((val)&GPS_UBX_CFG_INF_INFMSGMASK_NOTICE_MASK)>>2)  //!< Get NOTICE from bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_DEBUG_MASK 0x08  //!< Mask for field DEBUG in bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_DEBUG_GET(val)  (U)(((val)&GPS_UBX_CFG_INF_INFMSGMASK_DEBUG_MASK)>>3)  //!< Get DEBUG from bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_TEST_MASK 0x10  //!< Mask for field TEST in bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_TEST_GET(val)  (U)(((val)&GPS_UBX_CFG_INF_INFMSGMASK_TEST_MASK)>>4)  //!< Get TEST from bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_USER_MASK 0x80  //!< Mask for field USER in bitmask infMsgMask
#define GPS_UBX_CFG_INF_INFMSGMASK_USER_GET(val)  (U)(((val)&GPS_UBX_CFG_INF_INFMSGMASK_USER_MASK)>>7)  //!< Get USER from bitmask infMsgMask

//@}


//#define UBXID_CFG_INF 0x0602  // already defined, see above


//================================================================
//! CFG_RST: Command
/*!
Reset Receiver / Clear Backup Data Structures
-


This Message's id is #UBXID_CFG_RST
*/
//================================================================

typedef struct GPS_UBX_CFG_RST_s
{
	X2  navBbrMask;               //!< BBR Sections to clear. The following Special Sets apply:
	U1  resetMode;                //!< Reset Type
	U1  res;                      //!< Reserved

} GPS_UBX_CFG_RST_t,*GPS_UBX_CFG_RST_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_RST_s::navBbrMask
//@{
#define GPS_UBX_CFG_RST_NAVBBRMASK_EPH_MASK 0x0001  //!< Mask for field eph in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_EPH_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_EPH_MASK)>>0)  //!< Get eph from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_ALM_MASK 0x0002  //!< Mask for field alm in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_ALM_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_ALM_MASK)>>1)  //!< Get alm from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_HEALTH_MASK 0x0004  //!< Mask for field health in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_HEALTH_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_HEALTH_MASK)>>2)  //!< Get health from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_KLOB_MASK 0x0008  //!< Mask for field klob in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_KLOB_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_KLOB_MASK)>>3)  //!< Get klob from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_POS_MASK 0x0010  //!< Mask for field pos in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_POS_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_POS_MASK)>>4)  //!< Get pos from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_CLKD_MASK 0x0020  //!< Mask for field clkd in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_CLKD_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_CLKD_MASK)>>5)  //!< Get clkd from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_OSC_MASK 0x0040  //!< Mask for field osc in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_OSC_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_OSC_MASK)>>6)  //!< Get osc from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_UTC_MASK 0x0080  //!< Mask for field utc in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_UTC_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_UTC_MASK)>>7)  //!< Get utc from bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_RTC_MASK 0x0100  //!< Mask for field rtc in bitmask navBbrMask
#define GPS_UBX_CFG_RST_NAVBBRMASK_RTC_GET(val)  (U)(((val)&GPS_UBX_CFG_RST_NAVBBRMASK_RTC_MASK)>>8)  //!< Get rtc from bitmask navBbrMask

//@}


#define UBXID_CFG_RST 0x0604 //!< message id for CFG-RST
	

//================================================================
//! CFG_DAT_POLL: Poll Request
/*!
Poll Datum Setting
Upon sending of this message, the receiver returns CFG-DAT as defined below


This Message's id is #UBXID_CFG_DAT
*/
//================================================================

typedef struct GPS_UBX_CFG_DAT_POLL_s
{

} GPS_UBX_CFG_DAT_POLL_t,*GPS_UBX_CFG_DAT_POLL_pt;


#define UBXID_CFG_DAT 0x0606 //!< message id for CFG-DAT
	

//================================================================
//! CFG_DAT_STANDARD: Set
/*!
Set Standard Datum
See section <r href=\"datum_main'>Geodetic Datums</r> for a list of supported Datums


This Message's id is #UBXID_CFG_DAT
*/
//================================================================

typedef struct GPS_UBX_CFG_DAT_STANDARD_s
{
	U2  datumNum;                 //!< Datum Number

} GPS_UBX_CFG_DAT_STANDARD_t,*GPS_UBX_CFG_DAT_STANDARD_pt;


//#define UBXID_CFG_DAT 0x0606  // already defined, see above


//================================================================
//! CFG_DAT_CUSTOM: Set
/*!
Set User-defined Datum
-


This Message's id is #UBXID_CFG_DAT
*/
//================================================================

typedef struct GPS_UBX_CFG_DAT_CUSTOM_s
{
	R8  majA;                     //!< Semi-major Axis ( accepted range = 6,300,000.0 to 6,500,000.0 metres ).
	R8  flat;                     //!< 1.0 / Flattening ( accepted range is 0.0 to 500.0 ).
	R4  dX;                       //!< X Axis shift at the origin ( accepted range is +/- 5000.0 metres ).
	R4  dY;                       //!< Y Axis shift at the origin ( accepted range is +/- 5000.0 metres ).
	R4  dZ;                       //!< Z Axis shift at the origin ( accepted range is +/- 5000.0 metres ).
	R4  rotX;                     //!< Rotation about the X Axis ( accepted range is +/- 20.0 milli-arc seconds ).
	R4  rotY;                     //!< Rotation about the Y Axis ( accepted range is +/- 20.0 milli-arc seconds ).
	R4  rotZ;                     //!< Rotation about the Z Axis ( accepted range is +/- 20.0 milli-arc seconds ).
	R4  scale;                    //!< Scale change ( accepted range is 0.0 to 50.0 parts per million ).

} GPS_UBX_CFG_DAT_CUSTOM_t,*GPS_UBX_CFG_DAT_CUSTOM_pt;


//#define UBXID_CFG_DAT 0x0606  // already defined, see above


//================================================================
//! CFG_DAT: Get
/*!
Get currently selected Datum
The Parameter datumName is only valid, if datumNum is not equal to -1. In case datumNum is -1,the receiver is configured for a custom datum. The parameters from majA to scale are valid for both custom or standard datum formats.


This Message's id is #UBXID_CFG_DAT
*/
//================================================================

typedef struct GPS_UBX_CFG_DAT_s
{
	U2  datumNum;                 //!< Datum Number according to <r href=\"datum_main'>Geodetic Datums</r>
	CH  datumName[6];             //!< ASCII String with Datum Mnemonic
	R8  majA;                     //!< Semi-major Axis ( accepted range = 6,300,000.0 to 6,500,000.0 metres ).
	R8  flat;                     //!< 1.0 / Flattening ( accepted range is 0.0 to 500.0 ).
	R4  dX;                       //!< X Axis shift at the origin ( accepted range is +/- 5000.0 metres ).
	R4  dY;                       //!< Y Axis shift at the origin ( accepted range is +/- 5000.0 metres ).
	R4  dZ;                       //!< Z Axis shift at the origin ( accepted range is +/- 5000.0 metres ).
	R4  rotX;                     //!< Rotation about the X Axis ( accepted range is +/- 20.0 milli-arc seconds ).
	R4  rotY;                     //!< Rotation about the Y Axis ( accepted range is +/- 20.0 milli-arc seconds ).
	R4  rotZ;                     //!< Rotation about the Z Axis ( accepted range is +/- 20.0 milli-arc seconds ).
	R4  scale;                    //!< Scale change ( accepted range is 0.0 to 50.0 parts per million ).

} GPS_UBX_CFG_DAT_t,*GPS_UBX_CFG_DAT_pt;


//#define UBXID_CFG_DAT 0x0606  // already defined, see above


//================================================================
//! CFG_TP_POLL: Poll Request
/*!
Poll TimePulse Parameters
Sending this (empty / no-payload) message to the receiver results in the receiver returning a message of type CFG-TP with a payload as definedbelow


This Message's id is #UBXID_CFG_TP
*/
//================================================================

typedef struct GPS_UBX_CFG_TP_POLL_s
{

} GPS_UBX_CFG_TP_POLL_t,*GPS_UBX_CFG_TP_POLL_pt;


#define UBXID_CFG_TP 0x0607 //!< message id for CFG-TP
	

//================================================================
//! CFG_TP: Get/Set
/*!
Get/Set TimePulse Parameters
-


This Message's id is #UBXID_CFG_TP
*/
//================================================================

typedef struct GPS_UBX_CFG_TP_s
{
	U4  interval;                 //!< time interval for time pulse
	U4  length;                   //!< length of time pulse
	I1  status;                   //!< time pulse config setting
	U1  timeRef;                  //!< alignment to reference time:
	U1  flags;                    //!< bitmask
	U1  res;                      //!< reserved
	I2  antennaCableDelay;        //!< Antenna Cable Delay
	I2  rfGroupDelay;             //!< Receiver RF Group Delay
	I4  userDelay;                //!< User Time Function Delay (positive delay results in earlier pulse)

} GPS_UBX_CFG_TP_t,*GPS_UBX_CFG_TP_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_TP_s::flags
//@{
#define GPS_UBX_CFG_TP_FLAGS_SYNCMODE_MASK 0x01  //!< Mask for field syncMode in bitmask flags
#define GPS_UBX_CFG_TP_FLAGS_SYNCMODE_GET(val)  (U)(((val)&GPS_UBX_CFG_TP_FLAGS_SYNCMODE_MASK)>>0)  //!< Get syncMode from bitmask flags

//@}


//#define UBXID_CFG_TP 0x0607  // already defined, see above


//================================================================
//! CFG_RATE_POLL: Poll Request
/*!
Poll Navigation/Measurement Rate Settings
Sending this (empty / no-payload) message to the receiver results in the receiver returning a message of type CFG-RATE with a payload as defined below


This Message's id is #UBXID_CFG_RATE
*/
//================================================================

typedef struct GPS_UBX_CFG_RATE_POLL_s
{

} GPS_UBX_CFG_RATE_POLL_t,*GPS_UBX_CFG_RATE_POLL_pt;


#define UBXID_CFG_RATE 0x0608 //!< message id for CFG-RATE
	

//================================================================
//! CFG_RATE: Get/Set
/*!
Navigation/Measurement Rate Settings
The u-blox positioning technology supports navigation update rates higher or lower than 1 update per second. The calculation of the navigation solution will always be aligned to the top of a second. 
* The update rate has a direct influence on the power consumption. The more fixes that are required, the more CPU power and communication resources are required.
* For most applications a 1 Hz update rate would be sufficient.


This Message's id is #UBXID_CFG_RATE
*/
//================================================================

typedef struct GPS_UBX_CFG_RATE_s
{
	U2  measRate;                 //!< Measurement Rate, GPS measurements are taken every measRate milliseconds
	U2  navRate;                  //!< Navigation Rate, in number of measurement cycles. On u-blox 5, this parameter cannot be changed, and is always equals 1.
	U2  timeRef;                  //!< alignment to reference time: 0 = UTC time, 1 = GPS time

} GPS_UBX_CFG_RATE_t,*GPS_UBX_CFG_RATE_pt;


//#define UBXID_CFG_RATE 0x0608  // already defined, see above


//================================================================
//! CFG_CFG: Command
/*!
Clear, Save and Load configurations 
The three masks are made up of individual bits, each bit indicating the section ID on which the corresponding action shall be carried out.Please note that commands can be combined. The sequence of execution is Clear, Save, Load


This Message's id is #UBXID_CFG_CFG
*/
//================================================================

typedef struct GPS_UBX_CFG_CFG_s
{
	X4  clearMask;                //!< Mask with configuration ids to Clear (=Load Factory Defaults to Active Settings)
	X4  saveMask;                 //!< Mask with configuration ids to Save (=Save Active Settings to Non-volatile Memory), see ID description of clearMask
	X4  loadMask;                 //!< Mask with configuration ids to Load (=Load Settings from Non-volatile Memory to Active Settings), see ID description of clearMask

} GPS_UBX_CFG_CFG_t,*GPS_UBX_CFG_CFG_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_CFG_s::clearMask
//@{
#define GPS_UBX_CFG_CFG_CLEARMASK_IOPORT_MASK 0x0001  //!< Mask for field ioPort in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_IOPORT_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_IOPORT_MASK)>>0)  //!< Get ioPort from bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_MSGCONF_MASK 0x0002  //!< Mask for field msgConf in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_MSGCONF_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_MSGCONF_MASK)>>1)  //!< Get msgConf from bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_INFMSG_MASK 0x0004  //!< Mask for field infMsg in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_INFMSG_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_INFMSG_MASK)>>2)  //!< Get infMsg from bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_NAVCONF_MASK 0x0008  //!< Mask for field navConf in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_NAVCONF_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_NAVCONF_MASK)>>3)  //!< Get navConf from bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_RXMCONF_MASK 0x0010  //!< Mask for field rxmConf in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_RXMCONF_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_RXMCONF_MASK)>>4)  //!< Get rxmConf from bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_LPMCONF_MASK 0x0020  //!< Mask for field lpmConf in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_LPMCONF_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_LPMCONF_MASK)>>5)  //!< Get lpmConf from bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_ANTCONF_MASK 0x0400  //!< Mask for field antConf in bitmask clearMask
#define GPS_UBX_CFG_CFG_CLEARMASK_ANTCONF_GET(val)  (U)(((val)&GPS_UBX_CFG_CFG_CLEARMASK_ANTCONF_MASK)>>10)  //!< Get antConf from bitmask clearMask

//@}


#define UBXID_CFG_CFG 0x0609 //!< message id for CFG-CFG
	

//================================================================
//! CFG_ANT_POLL: Poll Request
/*!
Poll Antenna Control Settings
Sending this (empty / no-payload) message to the receiver results in the receiver returning a message of type CFG-ANT with a payload as definedbelow


This Message's id is #UBXID_CFG_ANT
*/
//================================================================

typedef struct GPS_UBX_CFG_ANT_POLL_s
{

} GPS_UBX_CFG_ANT_POLL_t,*GPS_UBX_CFG_ANT_POLL_pt;


#define UBXID_CFG_ANT 0x0613 //!< message id for CFG-ANT
	

//================================================================
//! CFG_ANT: Get/Set
/*!
Get/Set Antenna Control Settings
-


This Message's id is #UBXID_CFG_ANT
*/
//================================================================

typedef struct GPS_UBX_CFG_ANT_s
{
	X2  flags;                    //!< Antenna Flag Mask
	X2  pins;                     //!< Antenna Pin Configuration (READ-ONLY)

} GPS_UBX_CFG_ANT_t,*GPS_UBX_CFG_ANT_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_ANT_s::flags
//@{
#define GPS_UBX_CFG_ANT_FLAGS_SVCS_MASK 0x0001  //!< Mask for field svcs in bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_SVCS_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_FLAGS_SVCS_MASK)>>0)  //!< Get svcs from bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_SCD_MASK 0x0002  //!< Mask for field scd in bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_SCD_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_FLAGS_SCD_MASK)>>1)  //!< Get scd from bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_OCD_MASK 0x0004  //!< Mask for field ocd in bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_OCD_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_FLAGS_OCD_MASK)>>2)  //!< Get ocd from bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_PDWNONSCD_MASK 0x0008  //!< Mask for field pdwnOnSCD in bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_PDWNONSCD_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_FLAGS_PDWNONSCD_MASK)>>3)  //!< Get pdwnOnSCD from bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_RECOVERY_MASK 0x0010  //!< Mask for field recovery in bitmask flags
#define GPS_UBX_CFG_ANT_FLAGS_RECOVERY_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_FLAGS_RECOVERY_MASK)>>4)  //!< Get recovery from bitmask flags

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_ANT_s::pins
//@{
#define GPS_UBX_CFG_ANT_PINS_PINSWITCH_MASK 0x001F  //!< Mask for field pinSwitch in bitmask pins
#define GPS_UBX_CFG_ANT_PINS_PINSWITCH_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_PINS_PINSWITCH_MASK)>>0)  //!< Get pinSwitch from bitmask pins
#define GPS_UBX_CFG_ANT_PINS_PINSCD_MASK 0x03E0  //!< Mask for field pinSCD in bitmask pins
#define GPS_UBX_CFG_ANT_PINS_PINSCD_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_PINS_PINSCD_MASK)>>5)  //!< Get pinSCD from bitmask pins
#define GPS_UBX_CFG_ANT_PINS_PINOCD_MASK 0x7C00  //!< Mask for field pinOCD in bitmask pins
#define GPS_UBX_CFG_ANT_PINS_PINOCD_GET(val)  (U)(((val)&GPS_UBX_CFG_ANT_PINS_PINOCD_MASK)>>10)  //!< Get pinOCD from bitmask pins

//@}


//#define UBXID_CFG_ANT 0x0613  // already defined, see above


//================================================================
//! CFG_SBAS_U5: Command
/*!
SBAS Configuration
This message configures the SBAS receiver subsystem. (i.e. WAAS, EGNOS, MSAS)For detailed description of the SBAS services, please refer to Document RTCA/DO-229C (MOPS), available from http://www.rtca.org/


This Message's id is #UBXID_CFG_SBAS
*/
//================================================================

typedef struct GPS_UBX_CFG_SBAS_U5_s
{
	X1  mode;                     //!< SBAS Mode
	X1  usage;                    //!< SBAS Usage 
	U1  maxSBAS;                  //!< Maximum Number of SBAS channels searched (valid range: 0 - 3) to use
	X1  scanmode2;                //!< Continuation of scanmode bitmask below
	X4  scanmode1;                //!< Which SBAS PRN numbers to search for (Bitmask)

} GPS_UBX_CFG_SBAS_U5_t,*GPS_UBX_CFG_SBAS_U5_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_SBAS_U5_s::mode
//@{
#define GPS_UBX_CFG_SBAS_U5_MODE_ENABLED_MASK 0x01  //!< Mask for field enabled in bitmask mode
#define GPS_UBX_CFG_SBAS_U5_MODE_ENABLED_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_MODE_ENABLED_MASK)>>0)  //!< Get enabled from bitmask mode
#define GPS_UBX_CFG_SBAS_U5_MODE_TEST_MASK 0x02  //!< Mask for field test in bitmask mode
#define GPS_UBX_CFG_SBAS_U5_MODE_TEST_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_MODE_TEST_MASK)>>1)  //!< Get test from bitmask mode

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_SBAS_U5_s::usage
//@{
#define GPS_UBX_CFG_SBAS_U5_USAGE_RANGE_MASK 0x01  //!< Mask for field range in bitmask usage
#define GPS_UBX_CFG_SBAS_U5_USAGE_RANGE_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_USAGE_RANGE_MASK)>>0)  //!< Get range from bitmask usage
#define GPS_UBX_CFG_SBAS_U5_USAGE_DIFFCORR_MASK 0x02  //!< Mask for field diffCorr in bitmask usage
#define GPS_UBX_CFG_SBAS_U5_USAGE_DIFFCORR_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_USAGE_DIFFCORR_MASK)>>1)  //!< Get diffCorr from bitmask usage
#define GPS_UBX_CFG_SBAS_U5_USAGE_INTEGRITY_MASK 0x04  //!< Mask for field integrity in bitmask usage
#define GPS_UBX_CFG_SBAS_U5_USAGE_INTEGRITY_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_USAGE_INTEGRITY_MASK)>>2)  //!< Get integrity from bitmask usage

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_SBAS_U5_s::scanmode2
//@{
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN152_MASK 0x01  //!< Mask for field PRN152 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN152_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN152_MASK)>>0)  //!< Get PRN152 from bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN153_MASK 0x02  //!< Mask for field PRN153 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN153_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN153_MASK)>>1)  //!< Get PRN153 from bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN154_MASK 0x04  //!< Mask for field PRN154 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN154_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN154_MASK)>>2)  //!< Get PRN154 from bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN155_MASK 0x08  //!< Mask for field PRN155 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN155_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN155_MASK)>>3)  //!< Get PRN155 from bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN156_MASK 0x10  //!< Mask for field PRN156 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN156_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN156_MASK)>>4)  //!< Get PRN156 from bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN157_MASK 0x20  //!< Mask for field PRN157 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN157_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN157_MASK)>>5)  //!< Get PRN157 from bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN158_MASK 0x40  //!< Mask for field PRN158 in bitmask scanmode2
#define GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN158_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE2_PRN158_MASK)>>6)  //!< Get PRN158 from bitmask scanmode2

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_SBAS_U5_s::scanmode1
//@{
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN120_MASK 0x00000001  //!< Mask for field PRN120 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN120_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN120_MASK)>>0)  //!< Get PRN120 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN121_MASK 0x00000002  //!< Mask for field PRN121 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN121_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN121_MASK)>>1)  //!< Get PRN121 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN122_MASK 0x00000004  //!< Mask for field PRN122 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN122_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN122_MASK)>>2)  //!< Get PRN122 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN123_MASK 0x00000008  //!< Mask for field PRN123 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN123_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN123_MASK)>>3)  //!< Get PRN123 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN124_MASK 0x00000010  //!< Mask for field PRN124 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN124_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN124_MASK)>>4)  //!< Get PRN124 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN125_MASK 0x00000020  //!< Mask for field PRN125 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN125_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN125_MASK)>>5)  //!< Get PRN125 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN126_MASK 0x00000040  //!< Mask for field PRN126 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN126_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN126_MASK)>>6)  //!< Get PRN126 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN127_MASK 0x00000080  //!< Mask for field PRN127 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN127_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN127_MASK)>>7)  //!< Get PRN127 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN128_MASK 0x00000100  //!< Mask for field PRN128 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN128_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN128_MASK)>>8)  //!< Get PRN128 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN129_MASK 0x00000200  //!< Mask for field PRN129 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN129_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN129_MASK)>>9)  //!< Get PRN129 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN130_MASK 0x00000400  //!< Mask for field PRN130 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN130_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN130_MASK)>>10)  //!< Get PRN130 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN131_MASK 0x00000800  //!< Mask for field PRN131 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN131_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN131_MASK)>>11)  //!< Get PRN131 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN132_MASK 0x00001000  //!< Mask for field PRN132 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN132_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN132_MASK)>>12)  //!< Get PRN132 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN133_MASK 0x00002000  //!< Mask for field PRN133 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN133_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN133_MASK)>>13)  //!< Get PRN133 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN134_MASK 0x00004000  //!< Mask for field PRN134 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN134_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN134_MASK)>>14)  //!< Get PRN134 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN135_MASK 0x00008000  //!< Mask for field PRN135 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN135_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN135_MASK)>>15)  //!< Get PRN135 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN136_MASK 0x00010000  //!< Mask for field PRN136 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN136_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN136_MASK)>>16)  //!< Get PRN136 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN137_MASK 0x00020000  //!< Mask for field PRN137 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN137_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN137_MASK)>>17)  //!< Get PRN137 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN138_MASK 0x00040000  //!< Mask for field PRN138 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN138_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN138_MASK)>>18)  //!< Get PRN138 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN139_MASK 0x00080000  //!< Mask for field PRN139 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN139_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN139_MASK)>>19)  //!< Get PRN139 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN140_MASK 0x00100000  //!< Mask for field PRN140 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN140_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN140_MASK)>>20)  //!< Get PRN140 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN141_MASK 0x00200000  //!< Mask for field PRN141 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN141_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN141_MASK)>>21)  //!< Get PRN141 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN142_MASK 0x00400000  //!< Mask for field PRN142 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN142_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN142_MASK)>>22)  //!< Get PRN142 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN143_MASK 0x00800000  //!< Mask for field PRN143 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN143_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN143_MASK)>>23)  //!< Get PRN143 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN144_MASK 0x01000000  //!< Mask for field PRN144 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN144_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN144_MASK)>>24)  //!< Get PRN144 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN145_MASK 0x02000000  //!< Mask for field PRN145 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN145_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN145_MASK)>>25)  //!< Get PRN145 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN146_MASK 0x04000000  //!< Mask for field PRN146 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN146_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN146_MASK)>>26)  //!< Get PRN146 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN147_MASK 0x08000000  //!< Mask for field PRN147 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN147_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN147_MASK)>>27)  //!< Get PRN147 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN148_MASK 0x10000000  //!< Mask for field PRN148 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN148_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN148_MASK)>>28)  //!< Get PRN148 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN149_MASK 0x20000000  //!< Mask for field PRN149 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN149_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN149_MASK)>>29)  //!< Get PRN149 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN150_MASK 0x40000000  //!< Mask for field PRN150 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN150_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN150_MASK)>>30)  //!< Get PRN150 from bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN151_MASK 0x80000000  //!< Mask for field PRN151 in bitmask scanmode1
#define GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN151_GET(val)  (U)(((val)&GPS_UBX_CFG_SBAS_U5_SCANMODE1_PRN151_MASK)>>31)  //!< Get PRN151 from bitmask scanmode1

//@}


#define UBXID_CFG_SBAS 0x0616 //!< message id for CFG-SBAS
	

//================================================================
//! CFG_NMEA_POLL: Poll Request
/*!
Poll the NMEA protocol configuration
-


This Message's id is #UBXID_CFG_NMEA
*/
//================================================================

typedef struct GPS_UBX_CFG_NMEA_POLL_s
{

} GPS_UBX_CFG_NMEA_POLL_t,*GPS_UBX_CFG_NMEA_POLL_pt;


#define UBXID_CFG_NMEA 0x0617 //!< message id for CFG-NMEA
	

//================================================================
//! CFG_NMEA: Set/Get
/*!
Set/Get the NMEA protocol configuration
Set/Get the <r href=\"NMEA_overview'>NMEA protocol</r> configuration


This Message's id is #UBXID_CFG_NMEA
*/
//================================================================

typedef struct GPS_UBX_CFG_NMEA_s
{
	X1  filter;                   //!< filter flags
	U1  version;                  //!< 0x23 = NMEA version 2.3
	U1  numSV;                    //!< Maximum Number of SVs to report in NMEA protocol. 
	X1  flags;                    //!< flags 

} GPS_UBX_CFG_NMEA_t,*GPS_UBX_CFG_NMEA_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_NMEA_s::filter
//@{
#define GPS_UBX_CFG_NMEA_FILTER_POSFILT_MASK 0x01  //!< Mask for field posFilt in bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_POSFILT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FILTER_POSFILT_MASK)>>0)  //!< Get posFilt from bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_MSKPOSFILT_MASK 0x02  //!< Mask for field mskPosFilt in bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_MSKPOSFILT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FILTER_MSKPOSFILT_MASK)>>1)  //!< Get mskPosFilt from bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_TIMEFILT_MASK 0x04  //!< Mask for field timeFilt in bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_TIMEFILT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FILTER_TIMEFILT_MASK)>>2)  //!< Get timeFilt from bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_DATEFILT_MASK 0x08  //!< Mask for field dateFilt in bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_DATEFILT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FILTER_DATEFILT_MASK)>>3)  //!< Get dateFilt from bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_SBASFILT_MASK 0x10  //!< Mask for field sbasFilt in bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_SBASFILT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FILTER_SBASFILT_MASK)>>4)  //!< Get sbasFilt from bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_TRACKFILT_MASK 0x20  //!< Mask for field trackFilt in bitmask filter
#define GPS_UBX_CFG_NMEA_FILTER_TRACKFILT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FILTER_TRACKFILT_MASK)>>5)  //!< Get trackFilt from bitmask filter

//@}
//! \name Bit Definitions for #GPS_UBX_CFG_NMEA_s::flags
//@{
#define GPS_UBX_CFG_NMEA_FLAGS_COMPAT_MASK 0x01  //!< Mask for field compat in bitmask flags
#define GPS_UBX_CFG_NMEA_FLAGS_COMPAT_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FLAGS_COMPAT_MASK)>>0)  //!< Get compat from bitmask flags
#define GPS_UBX_CFG_NMEA_FLAGS_CONSIDER_MASK 0x02  //!< Mask for field consider in bitmask flags
#define GPS_UBX_CFG_NMEA_FLAGS_CONSIDER_GET(val)  (U)(((val)&GPS_UBX_CFG_NMEA_FLAGS_CONSIDER_MASK)>>1)  //!< Get consider from bitmask flags

//@}


//#define UBXID_CFG_NMEA 0x0617  // already defined, see above


//================================================================
//! CFG_USB_POLL: Poll Request
/*!
Poll a USB configuration
-


This Message's id is #UBXID_CFG_USB
*/
//================================================================

typedef struct GPS_UBX_CFG_USB_POLL_s
{

} GPS_UBX_CFG_USB_POLL_t,*GPS_UBX_CFG_USB_POLL_pt;


#define UBXID_CFG_USB 0x061B //!< message id for CFG-USB
	

//================================================================
//! CFG_USB: Get/Set
/*!
Get/Set USB Configuration
-


This Message's id is #UBXID_CFG_USB
*/
//================================================================

typedef struct GPS_UBX_CFG_USB_s
{
	U2  vendorID;                 //!< Vendor ID. This field shall only be set to registered 
	U2  productID;                //!< Product ID. Changing this field requres special Host drivers.
	U2  reserved1;                //!< This field is reserved. Always set to 0
	U2  reserved2;                //!< This field is reserved for special use. Always set to 1
	U2  powerConsumption;         //!< Power consumed by the device in mA
	X2  flags;                    //!< various configuration flags
	CH  vendorString[32];         //!< String containing the vendor name. 32 ASCII bytes including 0-termination.
	CH  productString[32];        //!< String containing the product name. 32 ASCII bytes including 0-termination.
	CH  serialNumber[32];         //!< String containing the serial number.  32 ASCII bytes including 0-termination.

} GPS_UBX_CFG_USB_t,*GPS_UBX_CFG_USB_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_USB_s::flags
//@{
#define GPS_UBX_CFG_USB_FLAGS_REENUM_MASK 0x0001  //!< Mask for field reEnum in bitmask flags
#define GPS_UBX_CFG_USB_FLAGS_REENUM_GET(val)  (U)(((val)&GPS_UBX_CFG_USB_FLAGS_REENUM_MASK)>>0)  //!< Get reEnum from bitmask flags
#define GPS_UBX_CFG_USB_FLAGS_POWERMODE_MASK 0x0002  //!< Mask for field powerMode in bitmask flags
#define GPS_UBX_CFG_USB_FLAGS_POWERMODE_GET(val)  (U)(((val)&GPS_UBX_CFG_USB_FLAGS_POWERMODE_MASK)>>1)  //!< Get powerMode from bitmask flags

//@}


//#define UBXID_CFG_USB 0x061B  // already defined, see above


//================================================================
//! CFG_TMODE_POL: Poll Request
/*!
Poll Time Mode Settings
Sending this (empty / no-payload) message to the receiver results in the receiver returning a message of type CFG-TMODE with a payload as defined below

   \note: This message is only supported on fw version >=V5.00 and if an appropriate license is installed in the receiver.

This Message's id is #UBXID_CFG_TMODE
*/
//================================================================

typedef struct GPS_UBX_CFG_TMODE_POL_s
{

} GPS_UBX_CFG_TMODE_POL_t,*GPS_UBX_CFG_TMODE_POL_pt;


#define UBXID_CFG_TMODE 0x061D //!< message id for CFG-TMODE
	

//================================================================
//! CFG_TMODE: Get/Set
/*!
Time Mode Settings
-

   \note: This message is only supported on fw version >=V5.00 and if an appropriate license is installed in the receiver.

This Message's id is #UBXID_CFG_TMODE
*/
//================================================================

typedef struct GPS_UBX_CFG_TMODE_s
{
	U4  timeMode;                 //!< Time Transfer Mode:	 
	I4  fixedPosX;                //!< Fixed Position ECEF X coordinate
	I4  fixedPosY;                //!< Fixed Position ECEF Y coordinate
	I4  fixedPosZ;                //!< Fixed Position ECEF Z coordinate
	U4  fixedPosVar;              //!< Fixed position 3D variance
	U4  svinMinDur;               //!< Survey-in minimum duration
	U4  svinVarLimit;             //!< Survey-in position variance limit

} GPS_UBX_CFG_TMODE_t,*GPS_UBX_CFG_TMODE_pt;


//#define UBXID_CFG_TMODE 0x061D  // already defined, see above


//================================================================
//! CFG_NAVX5_POLL: Poll Request
/*!
Poll Navigation Engine Expert Settings
Sending this (empty / no-payload) message to the receiver results in the receiver returning a message of type CFG-NAV5X with a payload as defined below.


This Message's id is #UBXID_CFG_NAV5X
*/
//================================================================

typedef struct GPS_UBX_CFG_NAV5X_POLL_s
{

} GPS_UBX_CFG_NAV5X_POLL_t,*GPS_UBX_CFG_NAV5X_POLL_pt;


#define UBXID_CFG_NAV5X 0x0623 //!< message id for CFG-NAV5X
	

//================================================================
//! CFG_NAV5X: Get/Set
/*!
Get/Set Navigation Engine Expert Settings
See the <r href=\"CFG-NAV5X-DESC'>Navigation Configuration Settings Description</r> for adetailed description of how these settings affect receiver operation.


This Message's id is #UBXID_CFG_NAV5X
*/
//================================================================

typedef struct GPS_UBX_CFG_NAV5X_s
{
	U2	version;				//!< message version
	U2	mask1;					//!< first settings mask (see #UBX_CFG_NAVX5_MASK_t)
	U4	mask2;					//!< second settings mask (see #UBX_CFG_NAVX5_MASK2_t)
	U1	tropoModel;				//!< troposphere model (see #NAV_SCI_TROPO_MODEL_t)
	U1	ionoModel;				//!< ionosphere model (see #NAV_SCI_IONO_MODEL_t)
	U1	minSVs;					//!< minimum SVs for navigation
	U1	maxSVs;					//!< maximum SVs for navigation
	U1	minCNO;					//!< minimum C/N0 for navigation [dbHz]
	U1	allowAlm;				//!< use orbit for navigation flag (allowAlm, inhibEph)
	L1	iniFix3D;				//!< initial fix must be 3D flag
	L1	enableRAIM;				//!< enable RAIM checks flag
	U1	signalUse;				//!< signal use mode (see #NAV_CTRL_SIGNALUSE_t)
	L1	ackAid;					//!< ack/nak aiding input
	U2	wknRollover;			//!< GPS weeknumber rollover
	U4	inhGPSSVs;				//!< inhibited SVs mask for GPS SVs 1-32
	L1	enableTrickyEnv;		//!< enable tricky environment mode flag
	L1	useDynModelConstr;		//!< use dynamic model constraints for sanity checking flag
	L1	usePPP;					//!< use Precise Point Positioning flag
	U1  aopFlags;               //!< use Autonomous Orbit Prediction flag (see #UBX_CFG_NAVX5_USEAOP_t)
	U1	dcmMode;				//!< delta carrier measurements mode (see #NAV_CTRL_DCMMODE_t)
	U1	accelProcNoiseScaleDR;	//!< scaling factor for acceleration process noise during dead reckoning [10^-2]
	U2	aopOrbMaxError;         //!< maximum accepted (modelled) orbit error [m]
	L1	useMMFGNSS;				//!< use map matching feedback in GNSS only mode
	U1	res1;					//!< reserved
	U2	res2;					//!< reserved
	U4	res3;					//!< reserved
} GPS_UBX_CFG_NAV5X_t,*GPS_UBX_CFG_NAV5X_pt;



//================================================================
//! CFG_NAV5_POLL: Poll Request
/*!
Poll Navigation Engine Settings
Sending this (empty / no-payload) message to the receiver results in the receiver returning a message of type CFG-NAV5 with a payload as defined below.


This Message's id is #UBXID_CFG_NAV5
*/
//================================================================

typedef struct GPS_UBX_CFG_NAV5_POLL_s
{

} GPS_UBX_CFG_NAV5_POLL_t,*GPS_UBX_CFG_NAV5_POLL_pt;


#define UBXID_CFG_NAV5 0x0624 //!< message id for CFG-NAV5
	

//================================================================
//! CFG_NAV5: Get/Set
/*!
Get/Set Navigation Engine Settings
See the <r href=\"CFG-NAV5-DESC'>Navigation Configuration Settings Description</r> for adetailed description of how these settings affect receiver operation.


This Message's id is #UBXID_CFG_NAV5
*/
//================================================================

typedef struct GPS_UBX_CFG_NAV5_s
{
	X2  mask;                     //!< Parameters Bitmask. Only the masked parameters will be applied.
	U1  dynModel;                 //!< Dynamic Platform model:
	U1  fixMode;                  //!< Position Fixing Mode.
	I4  fixedAlt;                 //!< Fixed altitude for 2D fix mode.
	U4  fixedAltVar;              //!< Fixed altitude variance for 2D mode.
	I1  minElev;                  //!< Minimum Elevation for a GNSS satellite to be used in NAV
	U1  drLimit;                  //!< Maximum time to perform DR (linear extrapolation) in case of GPS Signal Loss
	U2  pDop;                     //!< Position DOP Mask to use
	U2  tDop;                     //!< Time DOP Mask to use
	U2  pAcc;                     //!< Position Accuracy Mask
	U2  tAcc;                     //!< Time Accuracy Mask
	U1  staticHoldThresh;         //!< Static hold threshold
	U1  res1;                     //!< reserved, set to 0
	U4  res2;                     //!< reserved, set to 0
	U4  res3;                     //!< reserved, set to 0
	U4  res4;                     //!< reserved, set to 0

} GPS_UBX_CFG_NAV5_t,*GPS_UBX_CFG_NAV5_pt;


//! \name Bit Definitions for #GPS_UBX_CFG_NAV5_s::mask
//@{
#define GPS_UBX_CFG_NAV5_MASK_DYN_MASK 0x01  //!< Mask for field dyn in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_DYN_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_DYN_MASK)>>0)  //!< Get dyn from bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_MINEL_MASK 0x02  //!< Mask for field minEl in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_MINEL_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_MINEL_MASK)>>1)  //!< Get minEl from bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_FIXMODE_MASK 0x04  //!< Mask for field fixMode in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_FIXMODE_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_FIXMODE_MASK)>>2)  //!< Get fixMode from bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_DRLIM_MASK 0x08  //!< Mask for field drLim in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_DRLIM_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_DRLIM_MASK)>>3)  //!< Get drLim from bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_POSMASK_MASK 0x10  //!< Mask for field posMask in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_POSMASK_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_POSMASK_MASK)>>4)  //!< Get posMask from bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_TIMEMASK_MASK 0x20  //!< Mask for field timeMask in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_TIMEMASK_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_TIMEMASK_MASK)>>5)  //!< Get timeMask from bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_STATICHOLDMASK_MASK 0x40  //!< Mask for field staticHoldMask in bitmask mask
#define GPS_UBX_CFG_NAV5_MASK_STATICHOLDMASK_GET(val)  (U)(((val)&GPS_UBX_CFG_NAV5_MASK_STATICHOLDMASK_MASK)>>6)  //!< Get staticHoldMask from bitmask mask

//@}


//#define UBXID_CFG_NAV5 0x0624  // already defined, see above


//================================================================
//! MON_IO: Periodic/Polled
/*!
I/O Subsystem Status
The size of the message is determined by the NPRT number of ports the receiver supports.i.e. on Antaris this is always 4, on u-blox 5 the number of ports is 6.


This Message's id is #UBXID_MON_IO
*/
//================================================================

typedef struct GPS_UBX_MON_IO_s
{
	//REPEAT: GPS_UBX_MON_IO_RXBYTES_t repeat0[NPRT];           

} GPS_UBX_MON_IO_t,*GPS_UBX_MON_IO_pt;


//! Optional Sub-Structure of #GPS_UBX_MON_IO_t
typedef struct GPS_UBX_MON_IO_RXBYTES_s
{
	U4  rxBytes;                  //!< Number of bytes ever received
	U4  txBytes;                  //!< Number of bytes ever sent
	U2  parityErrs;               //!< Number of 100ms timeslots with parity errors 
	U2  framingErrs;              //!< Number of 100ms timeslots with framing errors 
	U2  overrunErrs;              //!< Number of 100ms timeslots with overrun errors 
	U2  breakCond;                //!< Number of 100ms timeslots with break conditions 
	U1  rxBusy;                   //!< Flag is receiver is busy
	U1  txBusy;                   //!< Flag is transmitter is busy
	U2  res;                      //!< reserved

} GPS_UBX_MON_IO_RXBYTES_t,*GPS_UBX_MON_IO_RXBYTES_pt;



#define UBXID_MON_IO 0x0A02 //!< message id for MON-IO
	

//================================================================
//! MON_VER: Answer to Poll
/*!
Receiver/Software Version
-


This Message's id is #UBXID_MON_VER
*/
//================================================================

typedef struct GPS_UBX_MON_VER_s
{
	CH  swVersion[30];            //!< Zero-terminated Software Version String
	CH  hwVersion[10];            //!< Zero-terminated Hardware Version String
	CH  extension[30];            //!< Installed Extension Package Version

} GPS_UBX_MON_VER_t,*GPS_UBX_MON_VER_pt;


#define UBXID_MON_VER 0x0A04 //!< message id for MON-VER
	

//================================================================
//! MON_MSGPP_U5: Periodic/Polled
/*!
Message Parse and Process Status
-


This Message's id is #UBXID_MON_MSGPP
*/
//================================================================

typedef struct GPS_UBX_MON_MSGPP_U5_s
{
	U2  msg1[8];                  //!< Number of successfully parsed messages for each protocol on target0
	U2  msg2[8];                  //!< Number of successfully parsed messages for each protocol on target1
	U2  msg3[8];                  //!< Number of successfully parsed messages for each protocol on target2
	U2  msg4[8];                  //!< Number of successfully parsed messages for each protocol on target3
	U2  msg5[8];                  //!< Number of successfully parsed messages for each protocol on target4
	U2  msg6[8];                  //!< Number of successfully parsed messages for each protocol on target5
	U4  skipped[6];               //!< Number skipped bytes for each target

} GPS_UBX_MON_MSGPP_U5_t,*GPS_UBX_MON_MSGPP_U5_pt;


#define UBXID_MON_MSGPP 0x0A06 //!< message id for MON-MSGPP
	

//================================================================
//! MON_RXBUF_U5: Periodic/Polled
/*!
Receiver Buffer Status
-


This Message's id is #UBXID_MON_RXBUF
*/
//================================================================

typedef struct GPS_UBX_MON_RXBUF_U5_s
{
	U2  pending[6];               //!< Number of bytes pending in receiver buffer for each target
	U1  usage[6];                 //!< Maximum usage receiver buffer during the last sysmon period for each target
	U1  peakUsage[6];             //!< Maximum usage receiver buffer for each target

} GPS_UBX_MON_RXBUF_U5_t,*GPS_UBX_MON_RXBUF_U5_pt;


#define UBXID_MON_RXBUF 0x0A07 //!< message id for MON-RXBUF
	

//================================================================
//! MON_TXBUF_U5: Periodic/Polled
/*!
Transmitter Buffer Status
-


This Message's id is #UBXID_MON_TXBUF
*/
//================================================================

typedef struct GPS_UBX_MON_TXBUF_U5_s
{
	U2  pending[6];               //!< Number of bytes pending in transmitter buffer for each target
	U1  usage[6];                 //!< Maximum usage transmitter buffer during the last sysmon period for each target
	U1  peakUsage[6];             //!< Maximum usage transmitter buffer for each target
	U1  tUsage;                   //!< Maximum usage of transmitter buffer during the last sysmon period for all targets 
	U1  tPeakusage;               //!< Maximum usage of transmitter buffer for all targets 
	X1  errors;                   //!< Error bitmask 
	U1  res;                      //!< reserved

} GPS_UBX_MON_TXBUF_U5_t,*GPS_UBX_MON_TXBUF_U5_pt;


//! \name Bit Definitions for #GPS_UBX_MON_TXBUF_U5_s::errors
//@{
#define GPS_UBX_MON_TXBUF_U5_ERRORS_LIMIT_MASK 0x3F  //!< Mask for field limit in bitmask errors
#define GPS_UBX_MON_TXBUF_U5_ERRORS_LIMIT_GET(val)  (U)(((val)&GPS_UBX_MON_TXBUF_U5_ERRORS_LIMIT_MASK)>>0)  //!< Get limit from bitmask errors
#define GPS_UBX_MON_TXBUF_U5_ERRORS_ALLOC_MASK 0x80  //!< Mask for field alloc in bitmask errors
#define GPS_UBX_MON_TXBUF_U5_ERRORS_ALLOC_GET(val)  (U)(((val)&GPS_UBX_MON_TXBUF_U5_ERRORS_ALLOC_MASK)>>7)  //!< Get alloc from bitmask errors

//@}


#define UBXID_MON_TXBUF 0x0A08 //!< message id for MON-TXBUF
	

//================================================================
//! MON_HW: Periodic/Polled
/*!
Hardware Status
Status of different aspect of the hardware, such as Antenna, PIO/Peripheral Pins, Noise Level, Automatic Gain Control (AGC)


This Message's id is #UBXID_MON_HW
*/
//================================================================

typedef struct GPS_UBX_MON_HW_s
{
	X4  pinSel;                   //!< Mask of Pins Set as Peripheral/PIO
	X4  pinBank;                  //!< Mask of Pins Set as Bank A/B
	X4  pinDir;                   //!< Mask of Pins Set as Input/Output
	X4  pinVal;                   //!< Mask of Pins Value Low/High
	U2  noisePerMS;               //!< Noise Level as measured by the GPS Core
	U2  agcCnt;                   //!< AGC Monitor (counts SIGHI xor SIGLO, range 0 to 8191)
	U1  aStatus;                  //!< Status of the Antenna Supervisor State Machine (0=INIT, 1=DONTKNOW, 2=OK, 3=SHORT, 4=OPEN)
	U1  aPower;                   //!< Current PowerStatus of Antenna (0=OFF, 1=ON, 2=DONTKNOW)
	X1  flags;                    //!< Flags
	U1  res1;                     //!< reserved for future use
	X4  usedMask;                 //!< Mask of Pins that are used by the Virtual Pin Manager
	U1  VP[32];                   //!< Array of Pin Mappings for each of the 32 Physical Pins
	X4  pinIrq;                   //!< Mask of Pins Value using the PIO Irq
	//OPTIONAL: GPS_UBX_MON_HW_RESH _t repeat0[0];              

} GPS_UBX_MON_HW_t,*GPS_UBX_MON_HW_pt;


//! \name Bit Definitions for #GPS_UBX_MON_HW_s::flags
//@{
#define GPS_UBX_MON_HW_FLAGS_RTCCALIB_MASK 0x1  //!< Mask for field rtcCalib in bitmask flags
#define GPS_UBX_MON_HW_FLAGS_RTCCALIB_GET(val)  (U)(((val)&GPS_UBX_MON_HW_FLAGS_RTCCALIB_MASK)>>0)  //!< Get rtcCalib from bitmask flags

//@}


//! Repeated Sub-Structure of #GPS_UBX_MON_HW_t
typedef struct GPS_UBX_MON_HW_RESH_s
{
	U4  resH;                     //!< Reserved
	U4  resL;                     //!< Reserved

} GPS_UBX_MON_HW_RESH_t,*GPS_UBX_MON_HW_RESH_pt;



#define UBXID_MON_HW 0x0A09 //!< message id for MON-HW
	

//================================================================
//! AID_REQ: Virtual
/*!
Sends a poll (AID-DATA) for all GPS Aiding Data
If the virtual AID-REQ is configured to be output (see CFG-MSG), the receiver will output a request for aiding data (AID-DATA) after a start-up if its internally stored data (position, time, ephemeris, almana) don't allow it to perform a hot start.

   \note: AID-REQ is not a message but a placeholder for configuration purposes. 

This Message's id is #UBXID_AID_REQ
*/
//================================================================

typedef struct GPS_UBX_AID_REQ_s
{

} GPS_UBX_AID_REQ_t,*GPS_UBX_AID_REQ_pt;


#define UBXID_AID_REQ 0x0B00 //!< message id for AID-REQ
	

//================================================================
//! AID_INI_POLL_: Poll Request 
/*!
Poll GPS Initial Aiding Data 
-

   \note: This message has an empty payload! 

This Message's id is #UBXID_AID_INI
*/
//================================================================

typedef struct GPS_UBX_AID_INI_POLL__s
{

} GPS_UBX_AID_INI_POLL__t,*GPS_UBX_AID_INI_POLL__pt;


#define UBXID_AID_INI 0x0B01 //!< message id for AID-INI
	

//================================================================
//! AID_INI_U5_: Polled 
/*!
Aiding position, time, frequency, clock drift 
-


This Message's id is #UBXID_AID_INI
*/
//================================================================

typedef struct GPS_UBX_AID_INI_U5__s
{
	I4  ecefXOrLat;               //!< WGS84 ECEF X coordinate or latitude, depending on flags below 
	I4  ecefYOrLon;               //!< WGS84 ECEF Y coordinate or longitude, depending on flags below 
	I4  ecefZOrAlt;               //!< WGS84 ECEF Z coordinate or altitude, depending on flags below 
	U4  posAcc;                   //!< position accuracy (stddev) 
	X2  tmCfg;                    //!< time mark configuration 
	U2  wn;                       //!< actual week number 
	U4  tow;                      //!< actual time of week 
	I4  towNs;                    //!< sub-millisecond part of time of week 
	U4  tAccMs;                   //!< milliseconds part of time accuracy 
	U4  tAccNs;                   //!< nanoseconds part of time accuracy 
	I4  clkDOrFreq;               //!< clock drift or frequency, depending on flags below 
	U4  clkDAccOrFreqAcc;         //!< accuracy of clock drift or frequency, depending on flags below 
	X4  flags;                    //!< bitmask with the following flags 

} GPS_UBX_AID_INI_U5__t,*GPS_UBX_AID_INI_U5__pt;


//! \name Bit Definitions for #GPS_UBX_AID_INI_U5__s::tmCfg
//@{
#define GPS_UBX_AID_INI_U5__TMCFG_FEDGE_MASK 0x02  //!< Mask for field fEdge in bitmask tmCfg
#define GPS_UBX_AID_INI_U5__TMCFG_FEDGE_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__TMCFG_FEDGE_MASK)>>1)  //!< Get fEdge from bitmask tmCfg
#define GPS_UBX_AID_INI_U5__TMCFG_TM1_MASK 0x10  //!< Mask for field tm1 in bitmask tmCfg
#define GPS_UBX_AID_INI_U5__TMCFG_TM1_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__TMCFG_TM1_MASK)>>4)  //!< Get tm1 from bitmask tmCfg
#define GPS_UBX_AID_INI_U5__TMCFG_F1_MASK 0x40  //!< Mask for field f1 in bitmask tmCfg
#define GPS_UBX_AID_INI_U5__TMCFG_F1_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__TMCFG_F1_MASK)>>6)  //!< Get f1 from bitmask tmCfg

//@}
//! \name Bit Definitions for #GPS_UBX_AID_INI_U5__s::flags
//@{
#define GPS_UBX_AID_INI_U5__FLAGS_POS_MASK 0x01  //!< Mask for field pos in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_POS_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_POS_MASK)>>0)  //!< Get pos from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_TIME_MASK 0x02  //!< Mask for field time in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_TIME_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_TIME_MASK)>>1)  //!< Get time from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_CLOCKD_MASK 0x04  //!< Mask for field clockD in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_CLOCKD_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_CLOCKD_MASK)>>2)  //!< Get clockD from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_TP_MASK 0x08  //!< Mask for field tp in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_TP_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_TP_MASK)>>3)  //!< Get tp from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_CLOCKF_MASK 0x10  //!< Mask for field clockF in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_CLOCKF_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_CLOCKF_MASK)>>4)  //!< Get clockF from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_LLA_MASK 0x20  //!< Mask for field lla in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_LLA_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_LLA_MASK)>>5)  //!< Get lla from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_ALTINV_MASK 0x40  //!< Mask for field altInv in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_ALTINV_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_ALTINV_MASK)>>6)  //!< Get altInv from bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_PREVTM_MASK 0x80  //!< Mask for field prevTm in bitmask flags
#define GPS_UBX_AID_INI_U5__FLAGS_PREVTM_GET(val)  (U)(((val)&GPS_UBX_AID_INI_U5__FLAGS_PREVTM_MASK)>>7)  //!< Get prevTm from bitmask flags

//@}


//#define UBXID_AID_INI 0x0B01  // already defined, see above


//================================================================
//! AID_HUI_POLL: Poll Request
/*!
Poll GPS Health, UTC and ionosphere parameters
-

   \note: This message has an empty payload!

This Message's id is #UBXID_AID_HUI
*/
//================================================================

typedef struct GPS_UBX_AID_HUI_POLL_s
{

} GPS_UBX_AID_HUI_POLL_t,*GPS_UBX_AID_HUI_POLL_pt;


#define UBXID_AID_HUI 0x0B02 //!< message id for AID-HUI
	

//================================================================
//! AID_HUI: Input/Output Message
/*!
GPS Health, UTC and ionosphere parameters
This message contains a health bit mask, UTC time and Klobuchar parameters.For more information on these parameters, please see the ICD-GPS-200 documentation.


This Message's id is #UBXID_AID_HUI
*/
//================================================================

typedef struct GPS_UBX_AID_HUI_s
{
	X4  health;                   //!< Bitmask, every bit represenst a GPS SV (1-32). If the bit is set the SV is healthy.
	R8  utcA1;                    //!< UTC - parameter A1
	R8  utcA0;                    //!< UTC - parameter A0
	I4  utcTOW;                   //!< UTC - reference time of week
	I2  utcWNT;                   //!< UTC - reference week number
	I2  utcLS;                    //!< UTC - time difference due to leap seconds before event
	I2  utcWNF;                   //!< UTC - week number when next leap second event occurs
	I2  utcDN;                    //!< UTC - day of week when next leap second event occurs
	I2  utcLSF;                   //!< UTC - time difference due to leap seconds after event
	I2  utcSpare;                 //!< UTC - Spare to ensure structure is an multiple of 4 bytes
	R4  klobA0;                   //!< Klobuchar - alpha 0 
	R4  klobA1;                   //!< Klobuchar - alpha 1
	R4  klobA2;                   //!< Klobuchar - alpha 2
	R4  klobA3;                   //!< Klobuchar - alpha 3
	R4  klobB0;                   //!< Klobuchar - beta 0
	R4  klobB1;                   //!< Klobuchar - beta 1
	R4  klobB2;                   //!< Klobuchar - beta 2
	R4  klobB3;                   //!< Klobuchar - beta 3
	X4  flags;                    //!< flags

} GPS_UBX_AID_HUI_t,*GPS_UBX_AID_HUI_pt;


//! \name Bit Definitions for #GPS_UBX_AID_HUI_s::flags
//@{
#define GPS_UBX_AID_HUI_FLAGS_HEALTH_MASK 0x1  //!< Mask for field health in bitmask flags
#define GPS_UBX_AID_HUI_FLAGS_HEALTH_GET(val)  (U)(((val)&GPS_UBX_AID_HUI_FLAGS_HEALTH_MASK)>>0)  //!< Get health from bitmask flags
#define GPS_UBX_AID_HUI_FLAGS_UTC_MASK 0x2  //!< Mask for field utc in bitmask flags
#define GPS_UBX_AID_HUI_FLAGS_UTC_GET(val)  (U)(((val)&GPS_UBX_AID_HUI_FLAGS_UTC_MASK)>>1)  //!< Get utc from bitmask flags
#define GPS_UBX_AID_HUI_FLAGS_KLOB_MASK 0x4  //!< Mask for field klob in bitmask flags
#define GPS_UBX_AID_HUI_FLAGS_KLOB_GET(val)  (U)(((val)&GPS_UBX_AID_HUI_FLAGS_KLOB_MASK)>>2)  //!< Get klob from bitmask flags

//@}


//#define UBXID_AID_HUI 0x0B02  // already defined, see above


//================================================================
//! AID_DATA: Poll
/*!
Polls all GPS Initial Aiding Data
If this poll is received, the messages AID-INI, AID-HUI, AID-EPH and AID-ALM are sent (Antaris only). u-blox 5 uses this message just to poll.


This Message's id is #UBXID_AID_DATA
*/
//================================================================

typedef struct GPS_UBX_AID_DATA_s
{

} GPS_UBX_AID_DATA_t,*GPS_UBX_AID_DATA_pt;


#define UBXID_AID_DATA 0x0B10 //!< message id for AID-DATA
	

//================================================================
//! AID_ALM_POLL: Poll Request
/*!
Poll GPS Aiding Almanach Data
Poll GPS Aiding Data (Almanach) for all 32 SVs by sending this message to the receiver without any payload.The receiver will return 32 messages of type AID-ALM as defined below.

   \note: This message has an empty payload!

This Message's id is #UBXID_AID_ALM
*/
//================================================================

typedef struct GPS_UBX_AID_ALM_POLL_s
{

} GPS_UBX_AID_ALM_POLL_t,*GPS_UBX_AID_ALM_POLL_pt;


#define UBXID_AID_ALM 0x0B30 //!< message id for AID-ALM
	

//================================================================
//! AID_ALM_POLL_SINGLE: Poll Request
/*!
Poll GPS Aiding Almanach Data for a SV
Poll GPS Aiding Data (Almanach) for an SV by sending this message to the receiver. The receiver will return one message of type AID-ALM as defined below.


This Message's id is #UBXID_AID_ALM
*/
//================================================================

typedef struct GPS_UBX_AID_ALM_POLL_SINGLE_s
{
	U1  svid;                     //!< SV ID for which the receiver shall return 

} GPS_UBX_AID_ALM_POLL_SINGLE_t,*GPS_UBX_AID_ALM_POLL_SINGLE_pt;


//#define UBXID_AID_ALM 0x0B30  // already defined, see above


//================================================================
//! AID_ALM: Input/Output Message
/*!
GPS Aiding Almanach Input/Output Message
* 	If the WEEK Value is 0, DWRD0 to DWRD7 are not sent as the almanach is not available for the given SV.
* 	DWORD0 to DWORD7 contain the 8 words following the Hand-Over Word ( HOW ) from the GPS navigation message, either pages 1 to 24 of sub-frame 5 or pages 2 to 10 of subframe 4. See ICD-GPS-200 for a full description of the contents of the Almanac pages.
* 	In DWORD0 to DWORD7, the parity bits have been removed, and the 24 bits of data are located in Bits 0 to 23. Bits 24 to 31 are the sign-extension of the data.
* 	Example: Parameter e (Eccentricity) from Almanach Subframe 4/5, Word 3, Bits 69-84 within the subframe can be found in DWRD0, Bits 15-0 whereas Bit 0 is the LSB.


This Message's id is #UBXID_AID_ALM
*/
//================================================================

typedef struct GPS_UBX_AID_ALM_s
{
	U4  svid;                     //!< SV ID for which this 
	U4  week;                     //!< Issue Date of Almanach (GPS week number)
	//OPTIONAL: GPS_UBX_AID_ALM_DWRD_t repeat0[0];              

} GPS_UBX_AID_ALM_t,*GPS_UBX_AID_ALM_pt;


//! Repeated Sub-Structure of #GPS_UBX_AID_ALM_t
typedef struct GPS_UBX_AID_ALM_DWRD_s
{
	X4  dwrd[8];                  //!< Almanach Words

} GPS_UBX_AID_ALM_DWRD_t,*GPS_UBX_AID_ALM_DWRD_pt;



//#define UBXID_AID_ALM 0x0B30  // already defined, see above


//================================================================
//! AID_EPH_POLL: Poll Request
/*!
Poll GPS Aiding Ephemeris Data
Poll GPS Aiding Data (Ephemeris) for all 32 SVs by sending this message to the receiver without any payload. The receiver will return 32 messages of type AID-EPH as defined below.

   \note: This message has an empty payload!

This Message's id is #UBXID_AID_EPH
*/
//================================================================

typedef struct GPS_UBX_AID_EPH_POLL_s
{

} GPS_UBX_AID_EPH_POLL_t,*GPS_UBX_AID_EPH_POLL_pt;


#define UBXID_AID_EPH 0x0B31 //!< message id for AID-EPH
	

//================================================================
//! AID_EPH_POLL_SINGLE: Poll Request
/*!
Poll GPS Aiding Ephemeris Data for a SV
Poll GPS Constellation Data (Ephemeris) for an SV by sending this message to the receiver. The receiver will return one message of type AID-EPH as defined below.


This Message's id is #UBXID_AID_EPH
*/
//================================================================

typedef struct GPS_UBX_AID_EPH_POLL_SINGLE_s
{
	U1  svid;                     //!< SV ID for which the receiver shall return 

} GPS_UBX_AID_EPH_POLL_SINGLE_t,*GPS_UBX_AID_EPH_POLL_SINGLE_pt;


//#define UBXID_AID_EPH 0x0B31  // already defined, see above


//================================================================
//! AID_EPH: Input/Output Message
/*!
GPS Aiding Ephemeris Input/Output Message
* SF1D0 to SF3D7 is only sent if ephemeris is available for this SV. If not, the payload is reduced to 4 Bytes, indicating that this SV Number does not have valid ephemeris for the moment.
* SF1D0 to SF3D7  contain the 24 words following the Hand-Over Word ( HOW ) from the GPS navigation message, subframes 1 to 3. See ICD-GPS-200 for a full description of the contents of the Subframes.
* In SF1D0 to SF3D7, the parity bits have been removed, and the 24 bits of data are located in Bits 0 to 23. Bits 24 to 31 are the sign-extension of the data.


This Message's id is #UBXID_AID_EPH
*/
//================================================================

typedef struct GPS_UBX_AID_EPH_s
{
	U4  svid;                     //!< SV ID for which this ephemeris data is 
	U4  how;                      //!< Hand-Over Word of first Subframe,
	//OPTIONAL: GPS_UBX_AID_EPH_SF1D_t repeat0[0];              

} GPS_UBX_AID_EPH_t,*GPS_UBX_AID_EPH_pt;


//! Repeated Sub-Structure of #GPS_UBX_AID_EPH_t
typedef struct GPS_UBX_AID_EPH_SF1D_s
{
	X4  sf1d[8];                  //!< Subframe 1 Words
	X4  sf2d[8];                  //!< Subframe 2 Words
	X4  sf3d[8];                  //!< Subframe 3 Words

} GPS_UBX_AID_EPH_SF1D_t,*GPS_UBX_AID_EPH_SF1D_pt;



//#define UBXID_AID_EPH 0x0B31  // already defined, see above


//================================================================
//! AID_ALPSRV_REQ: Output Message
/*!
ALPSRV-REQ: ALP client requests AlmanacPlus data from server
This message is sent by the ALP client to the ALP server in order to request a data chunk. The given identifier must be prepended to the requested data when submitting the data.


This Message's id is #UBXID_AID_ALPSRV
*/
//================================================================

typedef struct GPS_UBX_AID_ALPSRV_REQ_s
{
	U1  idSize;                   //!< Identifier size. This data, beginning at message start, must prepend the returned data.
	U1  type;                     //!< Requested data type. Must be different from 0xff, otherwise this is not a data request.
	U2  ofs;                      //!< Requested data offset [16bit words]
	U2  size;                     //!< Requested data size [16bit words]
	U2  fileId;                   //!< Unused when requesting data, filled in when sending back the data
	U2  dataSize;                 //!< Actual data size. Unused when requesting data, filled in when sending back the data.
	U1  id1;                      //!< Identifier data
	U1  id2;                      //!< Identifier data
	U4  id3;                      //!< Identifier data

} GPS_UBX_AID_ALPSRV_REQ_t,*GPS_UBX_AID_ALPSRV_REQ_pt;


#define UBXID_AID_ALPSRV 0x0B32 //!< message id for AID-ALPSRV
	

//================================================================
//! AID_ALPSRV_SRV: Input Message
/*!
ALPSRV-SRV: ALP server sends AlmanacPlus data to client
This message is sent by the ALP server to the ALP client and is usually sent in response to a data request. The server copies the identifier from the request and fills in the dataSize and fileId fields.


This Message's id is #UBXID_AID_ALPSRV
*/
//================================================================

typedef struct GPS_UBX_AID_ALPSRV_SRV_s
{
	U1  idSize;                   //!< Identifier size
	U1  type;                     //!< Requested data type
	U2  ofs;                      //!< Requested data offset [16bit words]
	U2  size;                     //!< Requested data size [16bit words]
	U2  fileId;                   //!< Corresponding ALP file ID, must be filled in by the server!
	U2  dataSize;                 //!< Actual data contained in this message, must be filled in by the server!
	U1  id1;                      //!< Identifier data
	U1  id2;                      //!< Identifier data
	U4  id3;                      //!< Identifier data
	//REPEAT: GPS_UBX_AID_ALPSRV_SRV_DATA_t repeat0[dataSize];       

} GPS_UBX_AID_ALPSRV_SRV_t,*GPS_UBX_AID_ALPSRV_SRV_pt;


//! Optional Sub-Structure of #GPS_UBX_AID_ALPSRV_SRV_t
typedef struct GPS_UBX_AID_ALPSRV_SRV_DATA_s
{
	U1  data;                     //!< Data for the ALP client

} GPS_UBX_AID_ALPSRV_SRV_DATA_t,*GPS_UBX_AID_ALPSRV_SRV_DATA_pt;



//#define UBXID_AID_ALPSRV 0x0B32  // already defined, see above


//================================================================
//! AID_ALPSRV_CLI: Output Message
/*!
ALPSRV-CLI: ALP client sends AlmanacPlus data to server.
This message is sent by the ALP client to the ALP server in order to submit updated data. The server can either replace the current data at this position or choose to ignore this new data (which will result in degraded performance).


This Message's id is #UBXID_AID_ALPSRV
*/
//================================================================

typedef struct GPS_UBX_AID_ALPSRV_CLI_s
{
	U1  idSize;                   //!< Identifier size
	U1  type;                     //!< Set to 0xff to mark that is *not* a data request
	U2  ofs;                      //!< Data offset [16bit words]
	U2  size;                     //!< Data size [16bit words]
	U2  fileId;                   //!< Corresponding ALP file id
	//REPEAT: GPS_UBX_AID_ALPSRV_CLI_DATA_t repeat0[size];           

} GPS_UBX_AID_ALPSRV_CLI_t,*GPS_UBX_AID_ALPSRV_CLI_pt;


//! Optional Sub-Structure of #GPS_UBX_AID_ALPSRV_CLI_t
typedef struct GPS_UBX_AID_ALPSRV_CLI_DATA_s
{
	U2  data;                     //!< 16bit word data to be submitted to the ALP server

} GPS_UBX_AID_ALPSRV_CLI_DATA_t,*GPS_UBX_AID_ALPSRV_CLI_DATA_pt;



//#define UBXID_AID_ALPSRV 0x0B32  // already defined, see above


//================================================================
//! AID_ALP_TX: Input message
/*!
ALP file data transfer to the receiver
This message is used to transfer a chunk of data from the AlmanacPlus file to the receiver. Upon reception of this message, the receiver will write the payload data to its internal non-volatile memory, eventually also erasing that part of the memory first. Make sure that the payload size is even sized (i.e. always a multiple of 2). Do not use payloads larger than ~ 700 bytes, as this would exceed the receiver’s internal buffering capabilities. The receiver will (not-) acknowledge this message using the message alternatives given below. The host shall wait for an acknowledge message before sending the next chunk.


This Message's id is #UBXID_AID_ALP
*/
//================================================================

typedef struct GPS_UBX_AID_ALP_TX_s
{
	//REPEAT: GPS_UBX_AID_ALP_TX_ALPDATA_t repeat0[Variable];       

} GPS_UBX_AID_ALP_TX_t,*GPS_UBX_AID_ALP_TX_pt;


//! Optional Sub-Structure of #GPS_UBX_AID_ALP_TX_t
typedef struct GPS_UBX_AID_ALP_TX_ALPDATA_s
{
	U2  alpData;                  //!< ALP file data

} GPS_UBX_AID_ALP_TX_ALPDATA_t,*GPS_UBX_AID_ALP_TX_ALPDATA_pt;



#define UBXID_AID_ALP 0x0B50 //!< message id for AID-ALP
	

//================================================================
//! AID_ALP_END: Input message
/*!
Mark end of data transfer
This message is used to indicate that all chunks have been transferred, and normal receiver operation can resume. Upon reception of this message, the receiver will verify all chunks received so far, and enable AssistNow Offline and GPS receiver operation if successful. This message could also be sent to cancel an incomplete download.


This Message's id is #UBXID_AID_ALP
*/
//================================================================

typedef struct GPS_UBX_AID_ALP_END_s
{
	U1  dummy;                    //!< Value is ignored

} GPS_UBX_AID_ALP_END_t,*GPS_UBX_AID_ALP_END_pt;


//#define UBXID_AID_ALP 0x0B50  // already defined, see above


//================================================================
//! AID_ALP_ACK: Output message
/*!
Acknowledges a data transfer
This message from the receiver acknowledges successful processing of a previously received chunk of data with the “Chunk Transfer” Message. This message will also be sent once a “Stop” message has been received, and the integrity of all chunks received so far has been checked successfully.


This Message's id is #UBXID_AID_ALP
*/
//================================================================

typedef struct GPS_UBX_AID_ALP_ACK_s
{
	U1  ack;                      //!< Set to 0x01

} GPS_UBX_AID_ALP_ACK_t,*GPS_UBX_AID_ALP_ACK_pt;


//#define UBXID_AID_ALP 0x0B50  // already defined, see above


//================================================================
//! AID_ALP_NAK: Output message
/*!
Indicate problems with a data transfer
This message from the receiver indicates that an error has occurred while processing and storing the data received with the “Chunk Transfer” message. This message will also be sent once a stop command has been received, and the integrity of all chunks received failed.


This Message's id is #UBXID_AID_ALP
*/
//================================================================

typedef struct GPS_UBX_AID_ALP_NAK_s
{
	U1  nak;                      //!< Set to 0x00

} GPS_UBX_AID_ALP_NAK_t,*GPS_UBX_AID_ALP_NAK_pt;


//#define UBXID_AID_ALP 0x0B50  // already defined, see above


//================================================================
//! AID_ALP_STAT: Periodic/Polled
/*!
Poll the AlmanacPlus status
-


This Message's id is #UBXID_AID_ALP
*/
//================================================================

typedef struct GPS_UBX_AID_ALP_STAT_s
{
	U4  predTow;                  //!< Prediction start time of week
	U4  predDur;                  //!< Prediction duration from start of first data set to end of last data set
	I4  age;                      //!< Current age of ALP data
	U2  predWno;                  //!< Prediction start week number
	U2  almWno;                   //!< Truncated week number of reference almanac
	U4  res1;                     //!< Reserved for future use
	U1  svs;                      //!< Number of satellite data sets contained in the ALP data
	U1  res2;                     //!< Reserved for future use
	U1  res3;                     //!< Reserved for future use
	U1  res4;                     //!< Reserved for future use

} GPS_UBX_AID_ALP_STAT_t,*GPS_UBX_AID_ALP_STAT_pt;


//#define UBXID_AID_ALP 0x0B50  // already defined, see above


//================================================================
//! TIM_TP: Periodic/Polled
/*!
Timepulse Timedata
This message contains information for high precision timing. Note that contents are correct only if the timepulse is set to one pulse per second.


This Message's id is #UBXID_TIM_TP
*/
//================================================================

typedef struct GPS_UBX_TIM_TP_s
{
	U4  towMS;                    //!< Timepulse GPS time of week
	U4  towSubMS;                 //!< Submillisecond part of TOWMS
	I4  qErr;                     //!< Quantization error of timepulse.
	U2  week;                     //!< Timepulse GPS week number.
	X1  flags;                    //!< bitmask 
	U1  res;                      //!< unused

} GPS_UBX_TIM_TP_t,*GPS_UBX_TIM_TP_pt;


//! \name Bit Definitions for #GPS_UBX_TIM_TP_s::flags
//@{
#define GPS_UBX_TIM_TP_FLAGS_TIMEBASE_MASK 0x01  //!< Mask for field timeBase in bitmask flags
#define GPS_UBX_TIM_TP_FLAGS_TIMEBASE_GET(val)  (U)(((val)&GPS_UBX_TIM_TP_FLAGS_TIMEBASE_MASK)>>0)  //!< Get timeBase from bitmask flags
#define GPS_UBX_TIM_TP_FLAGS_UTC_MASK 0x02  //!< Mask for field utc in bitmask flags
#define GPS_UBX_TIM_TP_FLAGS_UTC_GET(val)  (U)(((val)&GPS_UBX_TIM_TP_FLAGS_UTC_MASK)>>1)  //!< Get utc from bitmask flags

//@}


#define UBXID_TIM_TP 0x0D01 //!< message id for TIM-TP
	

//================================================================
//! TIM_TM2: Periodic/Polled
/*!
Time mark data
This message contains information for high precision time stamping / pulse counting. 
+The delay figures given in <r href=\"CFG-TP'>CFG-TP</r> are also applied to the time results output in this message.


This Message's id is #UBXID_TIM_TM2
*/
//================================================================

typedef struct GPS_UBX_TIM_TM2_s
{
	U1  ch;                       //!< marker channel 0 or 1
	X1  flags;                    //!< Bitmask 
	U2  count;                    //!< edge counter.
	U2  wnR;                      //!< week number of last rising edge
	U2  wnF;                      //!< week number of last falling edge
	U4  towMsR;                   //!< tow in of rising edge [ms]
	U4  towSubMsR;                //!< millisecond fraction of tow of rising edge in nanoseconds
	U4  towMsF;                   //!< tow in of falling edge [ms]
	U4  towSubMsF;                //!< millisecond fraction of tow of falling edge in nanoseconds
	U4  accEst;                   //!< Accuracy estimate

} GPS_UBX_TIM_TM2_t,*GPS_UBX_TIM_TM2_pt;


//! \name Bit Definitions for #GPS_UBX_TIM_TM2_s::flags
//@{
#define GPS_UBX_TIM_TM2_FLAGS_MODE_MASK 0x01  //!< Mask for field mode in bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_MODE_GET(val)  (U)(((val)&GPS_UBX_TIM_TM2_FLAGS_MODE_MASK)>>0)  //!< Get mode from bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_RUN_MASK 0x02  //!< Mask for field run in bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_RUN_GET(val)  (U)(((val)&GPS_UBX_TIM_TM2_FLAGS_RUN_MASK)>>1)  //!< Get run from bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_TIMEBASE_MASK 0x18  //!< Mask for field timeBase in bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_TIMEBASE_GET(val)  (U)(((val)&GPS_UBX_TIM_TM2_FLAGS_TIMEBASE_MASK)>>3)  //!< Get timeBase from bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_UTC_MASK 0x20  //!< Mask for field utc in bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_UTC_GET(val)  (U)(((val)&GPS_UBX_TIM_TM2_FLAGS_UTC_MASK)>>5)  //!< Get utc from bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_TIME_MASK 0x40  //!< Mask for field time in bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_TIME_GET(val)  (U)(((val)&GPS_UBX_TIM_TM2_FLAGS_TIME_MASK)>>6)  //!< Get time from bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_NEWRISINGEDGE_MASK 0x80  //!< Mask for field newRisingEdge in bitmask flags
#define GPS_UBX_TIM_TM2_FLAGS_NEWRISINGEDGE_GET(val)  (U)(((val)&GPS_UBX_TIM_TM2_FLAGS_NEWRISINGEDGE_MASK)>>7)  //!< Get newRisingEdge from bitmask flags

//@}


#define UBXID_TIM_TM2 0x0D03 //!< message id for TIM-TM2
	
#endif //__PROTO_UBX_STRUCT_DEF_H__
