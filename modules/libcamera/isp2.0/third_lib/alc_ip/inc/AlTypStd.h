/*---------------------------------------------------------------------------**  
Copyright (C) 2004-2008 ACUTElogic Corp.  
ACUTElogic Confidential. All rights reserved.  

    <<<<< comply with MISRA standard. >>>>> 
**---------------------------------------------------------------------------** 
$HeadURL: $ 
$LastChangedDate::                     #$ 
$LastChangedRevision:$ 
$LastChangedBy: $ 
**-------------------------------------------------------------------------** 
 Permission  is hereby granted to licensees of ACUTELOGIC  CORPORATION 
    software to use or abstract this computer program for the sole purpose 
    of implementing ACUTELOGIC CORPORATION software.    
    No other rights to reproduce, use, or disseminate this program,  
    whether in part or in whole, are granted. 
**------------------------------------------------------------------------*//** 
Acutelogic standard type definition for MSM8xxx
This file is made automatically. 
@n   Do not change this file
 
 @file
 @author Masashi Seino   
 @date 2004/10/01 start   
 @note All c-source files must include this file. 
*//*-------------------------------------------------------------------------*/     
#ifndef AL_TYP_STD_H
#define AL_TYP_STD_H
/*============================================================================**

Platform definition

**============================================================================*/
#define AL_PLATFORM_AA00 (1)
#define AL_PLATFORM_AB00 (2)
#define AL_PLATFORM_AC00 (3)
#define AL_PLATFORM_BC00 (4)
#define AL_PLATFORM_BD00 (5)
#define AL_PLATFORM_BE00 (6)
#define AL_PLATFORM_CA00 (7)
#define AL_PLATFORM_EA00 (8)
#define AL_PLATFORM_EA01 (9)
#define AL_PLATFORM_EA02 (10)
#define AL_PLATFORM_EB00 (11)
#define AL_PLATFORM_EB01 (12)
#define AL_PLATFORM_EB02 (13)
#define AL_PLATFORM_EB03 (14)
#define AL_PLATFORM_EB04 (15)
#define AL_PLATFORM_EC01 (16)
#define AL_PLATFORM_EC02 (17)
#define AL_PLATFORM_EC03 (18)
#define AL_PLATFORM_ED01 (19)
#define AL_PLATFORM_ED02 (20)
#define AL_PLATFORM_ED03 (21)
#define AL_PLATFORM_ED04 (22)
#define AL_PLATFORM_EE00 (23)
#define AL_PLATFORM_EE01 (24)
#define AL_PLATFORM_EF00 (25)
#define AL_PLATFORM_EF01 (26)
#define AL_PLATFORM_EF02 (27)
#define AL_PLATFORM_EG00 (28)
#define AL_PLATFORM_EG01 (29)
#define AL_PLATFORM_EG02 (30)
#define AL_PLATFORM_EH00 (31)
#define AL_PLATFORM_EH01 (32)
#define AL_PLATFORM_FA00 (33)
#define AL_PLATFORM_FB00 (34)
#define AL_PLATFORM_GA00 (35)
#define AL_PLATFORM_GA01 (36)
#define AL_PLATFORM_GB00 (37)
#define AL_PLATFORM_GB01 (38)
#define AL_PLATFORM_GB02 (39)
#define AL_PLATFORM_GC01 (40)
#define AL_PLATFORM_HA00 (41)
#define AL_PLATFORM_IA00 (42)
#define AL_PLATFORM_JA00 (43)
#define AL_PLATFORM_JA01 (44)
#define AL_PLATFORM_KA00 (45)
#define AL_PLATFORM_LA00 (46)
#define AL_PLATFORM_LA01 (47)
#define AL_PLATFORM_LA02 (48)
#define AL_PLATFORM_MA00 (49)
#define AL_PLATFORM_NA00 (50)
#define AL_PLATFORM_OA00 (51)
#define AL_PLATFORM_PA00 (52)
#define AL_PLATFORM_QA00 (53)
#define AL_PLATFORM_RA00 (54)

/*----------------------------------------------------------**
Select MSM8xxx
**----------------------------------------------------------*/
#ifndef AL_PLATFORM
    #define AL_PLATFORM  (AL_PLATFORM_OA00)
#else
    #if AL_PLATFORM!=AL_PLATFORM_OA00
        #error PLATFORM DEFINITION ERROR
    #endif
#endif

/*============================================================================**

 Other Core dependenient Macro for ARMCortex-L

**============================================================================*/
#define AL_ENDIAN_LITTLE (0)
#define AL_ENDIAN_BIG (1)


#if defined(__BORLANDC__) || \
    defined(__WIN32__)    || \
    defined(_WIN32)       || \
    defined(__WIN64__)    || \
    defined(_WIN64)       ||  \
    defined(_Wp64) 
/*-----------	PC向けビルド環境	-----------*/
	#define AL_ENDIAN		(AL_ENDIAN_LITTLE)
#else
/*-----------	ARMCortex-L向けビルド環境	-----------*/
	#define AL_ENDIAN		(AL_ENDIAN_LITTLE)
#endif

#define MIN_HANDLING_BIT_WIDTH	(8)
#define BYTE_SIZE_OF( xxx )		(sizeof(xxx) * (MIN_HANDLING_BIT_WIDTH/8))

/*============================================================================**

 Standard typedef for ARMCortex-L

**============================================================================*/
/*----------------------------------------------------------**
  integer
**----------------------------------------------------------*/
typedef signed char	SI_08;        /**<8bit signed integer*/
typedef unsigned char	UI_08;        /**<8bit unsigned integer*/
typedef signed short	SI_16;        /**<16bit signed integer*/
typedef unsigned short	UI_16;        /**<16bit unsigned integer*/
typedef signed int	SI_32;        /**<32bit signed integer*/
typedef unsigned int	UI_32;        /**<32bit unsigned integer*/
/*----------------------------------------------------------**
   fixed point
**----------------------------------------------------------*/
typedef signed short	SQ_16;        /**<16bit (S0.15) Fixed point */
typedef unsigned short	UQ_16;        /**<16bit (0.16) Fixed point */
typedef signed int	SQ_32;        /**<32bit (S15.16) Fixed point */
typedef unsigned int	UQ_32;        /**<32bit (16.16) Fixed point */
/*----------------------------------------------------------**
 Floationg point
**----------------------------------------------------------*/
typedef float	FT_32;
typedef double	FT_64;

/*----------------------------------------------------------**
  Boolean
**----------------------------------------------------------*/
/** Boolean */
typedef enum tg_bool {
    bAlFalse	= 0,	/**< False */
    bAlTrue		= 1,	/**< True */
    bAlEnb		= 0,	/**< Enable */
    bAlDsb		= 1,	/**< Disable */
    bAlOn		= 0,	/**< On */
    bAlOff		= 1,	/**< Off */

	/* dmy for ARM compiler*/
	eTB_BOOL_Dmy			=65536,
} TB_BOOL;
/** Boolean for Enable/Disable*/
typedef	TB_BOOL	TB_ENBL;
/** Boolean for ON/OFF*/
typedef	TB_BOOL	TB_ONOFF;


/*----------------------------------------------------------**
   general-purpose
**----------------------------------------------------------*/
typedef void* VP_32;        /**<General purpose pointer*/

/*============================================================================**

    Common Type

**============================================================================*/
/*----------------------------------------------------------**
   Point
**----------------------------------------------------------*/
/**SQ_32 format Point type*/
typedef struct    {
	SQ_32	msqX;		/**< x*/
	SQ_32	msqY;		/**< Y*/
}TT_PointSQ32;

/** UI_32 format Point type*/
typedef struct 
{
	UI_16	muiX;		/**< x*/
	UI_16	muiY;		/**< y*/
} TT_PointUI16;


/*============================================================================**

    Version Info Type

**============================================================================*/
/*----------------------------------------------------------**
   Public version
**----------------------------------------------------------*/
typedef struct tg_AlVersion
{
	UI_08			muiMajor;		///< +00h Version: Major (bin)
	UI_08			muiMinor;		///< +02h Version: Minor (bin)
	UI_16			muiLocal;		///< +04h Version: Local (bin)
} TT_AlVersion;

/*----------------------------------------------------------**
   Acutelogic private version
**----------------------------------------------------------*/
typedef struct tg_AlIntanalVer
{
	//---    Naming    -----------------------------------------------------------**
	UI_08			muiName[8];		///< +00h Naming : IP or Name (7 Characters)
	UI_08			muiCate[8];		///< +08h Naming : Category   (7 Characters)
	UI_08			muiType[8];		///< +10h Naming : Type       (7 Characters)
	//---    Version    -----------------------------------------------------------**
	TT_AlVersion    mttVer;			///< +18h Global   Version
	UI_32			muiBugFix; 		///< +1Eh Internal Version: BugFix(bin)
	UI_08			muiCustom[8];	///< +20h Internal Version: Customer(7 Characters)
	UI_16			muiRelYear;		///< +28h Release Year   (BCD)
	UI_16			muiRelDate;		///< +2Ah Release Date   (BCD)
	//---    Additional    -------------------------------------------------------**
	UI_16			muiBuildYear;	///< +2Ch Build Year  (BCD)
	UI_16			muiBuildDate;	///< +2Eh Build Date  (BCD)
	UI_16			muiBuildTime;	///< +30h Build time  (BCD)
	UI_16			muiResv1;		///< +32h Reserved 1
	UI_16			muiResv2;		///< +34h Reserved 2
	UI_16			muiResv3;		///< +36h Reserved 3
	UI_16			muiResv4;		///< +38h Reserved 4
	UI_16			muiResv5;		///< +3Ah Reserved 5
	UI_16			muiResv6;		///< +3Ch Reserved 6
	UI_16			muiResv7;		///< +3Eh Reserved 7
} TT_AlInternalVer;
#endif    //#ifndef AL_TYP_STD_H

