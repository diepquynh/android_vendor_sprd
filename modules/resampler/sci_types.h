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

//#include "migrate.h"
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

#define SCI_TRUE                    TRUE       // Boolean true value
#define SCI_FALSE                   FALSE       // Boolean false value

#ifndef NULL
  #define NULL  0
#endif

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */
typedef unsigned char			BOOLEAN;
typedef unsigned char			uint8;
typedef unsigned short			uint16;
typedef unsigned long int		uint32;
typedef long long int 			int64;
#define    SCI_Free                   free
#define    SCI_MEMSET                 memset
#define    SCI_ALLOC                  malloc
#define    SCI_Sleep                  usleep
#define    SCI_MEMCPY	              memcpy

#ifndef _X_64
//typedef unsigned __int64	 	uint64;
#else
typedef struct _X_UN_64_T {
	uint32 hiDWORD;
	uint32 loDWORD;
}X_UN_64_T;
typedef X_UN_64_T uint64;
typedef uint64 __uint64;
#endif

typedef            char				int8;
typedef signed short			int16;
typedef signed long int			int32;

#ifndef _X_64
//typedef __int64					int64;
#else
typedef struct _X_64_T {
	int32 hiDWORD;
	int32 loDWORD;
}X_64_T;
typedef X_64_T int64;
typedef int64 __int64;
#endif


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

/*
	Bit define
*/
#ifndef BIT_0
#define BIT_0               0x00000001
#define BIT_1               0x00000002
#define BIT_2               0x00000004
#define BIT_3               0x00000008
#define BIT_4               0x00000010
#define BIT_5               0x00000020
#define BIT_6               0x00000040
#define BIT_7               0x00000080
#define BIT_8               0x00000100
#define BIT_9               0x00000200
#define BIT_10              0x00000400
#define BIT_11              0x00000800
#define BIT_12              0x00001000
#define BIT_13              0x00002000
#define BIT_14              0x00004000
#define BIT_15              0x00008000
#define BIT_16              0x00010000
#define BIT_17              0x00020000
#define BIT_18              0x00040000
#define BIT_19              0x00080000
#define BIT_20              0x00100000
#define BIT_21              0x00200000
#define BIT_22              0x00400000
#define BIT_23              0x00800000
#define BIT_24              0x01000000
#define BIT_25              0x02000000
#define BIT_26              0x04000000
#define BIT_27              0x08000000
#define BIT_28              0x10000000
#define BIT_29              0x20000000
#define BIT_30              0x40000000 
#define BIT_31              0x80000000
#endif

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

/*
    @Lin.liu Added.(2002-11-19)
*/
#define  BCD_EXT    uint8


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif

#endif  /* SCI_TYPES_H */

