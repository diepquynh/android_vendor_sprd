/*---------------------------------------------------------------------------**
Copyright (C) 2004-2009 ACUTElogic Corp.
ACUTElogic Confidential. All rights reserved.

    <<<<< comply with MISRA standard. >>>>>
**---------------------------------------------------------------------------**
$HeadURL: http://localhost:50120/chassis/svn/_chassis_/Projects/AlStd/trunk/Public/include/AlEmbStd.h $
$LastChangedDate:: 2009-08-20 09:54:18 #$
$LastChangedRevision: 1606 $
$LastChangedBy: sakou $
**-------------------------------------------------------------------------**
 Permission  is hereby granted to licensees of ACUTELOGIC  CORPORATION
    software to use or abstract this computer program for the sole purpose
    of implementing ACUTELOGIC CORPORATION software.
    No other rights to reproduce, use, or disseminate this program,
    whether in part or in whole, are granted.
**------------------------------------------------------------------------*//**
ALC Standard Header for embedded system

 This file will be overwritten in every project.
 All c-source files must include this file.
 @file
 @author Masashi Seino

 @date 2004/06/21 tamura	add AL_DIS/EN,NO/YES,/OFF/ON,FALSE/TRUE... macro
 @date 2004/06/11 tamura	chg GLOBAL--> PUBLIC,add MACRO (),
 @date 2004/06/01 start

 @note <<<<< comply with ALEMB Rev.0.0 >>>>>
*//*-------------------------------------------------------------------------*/
#ifndef	ALC_EMB_STD_H
#define ALC_EMB_STD_H

/*-------------------------------------------------------------------------------
	Object scope
--------------------------------------------------------------------------------*/
	#define	PRIVATE			static				/**< def. for file   scope var 	*/
	#define	PUBLIC								/**< def. for global scope var 	*/
	#define	EXTERN			extern

/*-------------------------------------------------------------------------------
	Bit operator
--------------------------------------------------------------------------------*/
	#define	AL_BIT_AND(a,b)		((a) & (b))
	#define	AL_BIT_OR(a,b)			((a) | (b))
	#define	AL_BIT_XOR(a,b)		((a) ^ (b))
	#define	AL_BIT_REV(a)			(~(a))


	/* clrbit: 1=clear,0=not change */
	#define	AL_BIT_CLR(data,clrbit)			( AL_BIT_AND( (data) ,  AL_BIT_REV((clrbit) ) ) )
	/*---------------------------------------------**
	** Set masked bit data after clear bit.
	**
	**	AL_BIT_SET(0xABCD,0x0F00,0xFACE)
	**	=> result = 0xAACD
	**---------------------------------------------*/
	#define	AL_BIT_SET(tgt,msk,set)	( AL_BIT_OR( (AL_BIT_CLR( (tgt) , (msk) )) , (AL_BIT_AND( (set), (msk))) ) )

/*-------------------------------------------------------------------------------
	Build S/W Macro
--------------------------------------------------------------------------------*/
	#define	AL_DSBL				(0)
	#define	AL_ENBL				(1)

	#define DEBUG_CODE_SENS		(1)

#endif /* ALC_EMB_STD_H*/
/*EOF*/
