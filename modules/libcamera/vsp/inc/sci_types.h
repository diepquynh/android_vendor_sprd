/******************************************************************************
 ** File Name:      sci_types.h                                               *
 ** Author:         jakle zhu                                                 *
 ** DATE:           10/22/2001                                                *
 ** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This header file contains general types and macros that   *
 **         		are of use to all modules.The values or definitions are   *
 ** 				dependent on the specified target.  T_WINNT specifies     *
 **					Windows NT based targets, otherwise the default is for    *
 **					ARM targets.                                              *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 10/22/2001     Jakle zhu     Create.                                      *
 ******************************************************************************/
#ifndef SCI_TYPES_H
#define SCI_TYPES_H

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    extern   "C"
    {
#endif

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#ifndef NULL
  #define NULL  0
#endif

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */
typedef unsigned char		BOOLEAN;
//typedef unsigned char		Bool;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
//typedef unsigned int		uint;

typedef signed char			int8;
typedef signed short		int16;
typedef signed int			int32;

#ifdef WIN_UNIT_TEST
#define LOCAL
#define CONST
#else
#define	LOCAL		static
#define	CONST		const
#endif  //WIN_UNIT_TEST

#define	VOLATILE	volatile


#define PNULL		0

//Added by Xueliang.Wang on 28/03/2002
#define PUBLIC 
//#define	PRIVATE		static
#define	SCI_CALLBACK

// @Xueliang.Wang moved it from os_api.h(2002-12-30)
// Thread block ID.
typedef uint32          BLOCK_ID;

#define IRQ_FUNC        __irq

#ifdef WIN32
    #define PACK
#else
    #define PACK __packed    /* Byte alignment for communication structures.*/
#endif
          
/* some usefule marcos */
#define Bit(_i)              ((u32) 1<<(_i))

#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )

#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#define  WORD_LO(_xxx)  ((uint8) ((int16)(_xxx)))
#define  WORD_HI(_xxx)  ((uint8) ((int16)(_xxx) >> 8)) 

#define RND8( _x )       ((((_x) + 7) / 8 ) * 8 ) /*rounds a number up to the nearest multiple of 8 */


#define  UPCASE( _c ) ( ((_c) >= 'a' && (_c) <= 'z') ? ((_c) - 0x20) : (_c) )

#define  DECCHK( _c ) ((_c) >= '0' && (_c) <= '9')

#define  DTMFCHK( _c ) ( ((_c) >= '0' && (_c) <= '9') ||\
	((_c) >= 'A' && (_c) <= 'D') ||\
	((_c) >= 'a' && (_c) <= 'd') ||\
	((_c) == '*') ||\
((_c) == '#'))

#define  HEXCHK( _c ) ( ((_c) >= '0' && (_c) <= '9') ||\
                       ((_c) >= 'A' && (_c) <= 'F') ||\
                       ((_c) >= 'a' && (_c) <= 'f') )

#define  ARR_SIZE( _a )  ( sizeof( (_a) ) / sizeof( (_a[0]) ) )



/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif

#endif  /* SCI_TYPES_H */


