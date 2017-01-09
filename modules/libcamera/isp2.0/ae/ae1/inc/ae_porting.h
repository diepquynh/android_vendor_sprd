/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _AE_PORTING_H_
#define _AE_PORTING_H_
    
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/ 
#ifdef __cplusplus
extern "C"  {
	
#endif				/*  */
	
/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */ 
#ifdef WIN32
#ifdef TRUE
#undef TRUE
#endif				/*  */
	
#ifdef FALSE
#undef FALSE
#endif				/*  */
	
#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */
	
#ifndef NULL
#define NULL  0
#endif				/*  */
	
#define SCI_NULL                                 0
#ifndef PNULL
#define PNULL                                    0
#endif				/*  */
/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */ 
	typedef unsigned char BOOLEAN;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long int uint32;
	typedef unsigned __int64 uint64;
	typedef unsigned int uint;
	
typedef signed char int8;
	typedef signed short int16;
	typedef signed long int int32;
	
typedef unsigned char boolean;
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned long int uint32_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned int uint_t;
	
typedef signed char int8_t;
	typedef signed short int16_t;
	typedef signed long int int32_t;
	typedef signed __int64 int64_t;
	
typedef unsigned __int64 __u64;
	typedef __int64 __s64;
	
#ifdef WIN_UNIT_TEST
#define LOCAL
#define CONST
#else				/*  */
#define	LOCAL		static
#define	CONST		const
#endif				//WIN_UNIT_TEST
	
#define	VOLATILE	volatile
	
#define PNULL		0
	
//Added by Xueliang.Wang on 28/03/2002
#define PUBLIC
	
/*
	Bit define
*/ 
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
	
/* some usefule marcos */ 
#define Bit(_i)              ((u32) 1<<(_i))
	
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
	
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#define  WORD_LO(_xxx)  ((uint8) ((int16)(_xxx)))
#define  WORD_HI(_xxx)  ((uint8) ((int16)(_xxx) >> 8)) 
	
#define RND8( _x )       ((((_x) + 7) / 8 ) * 8 ) /*rounds a number up to the nearest multiple of 8 */
	
#define  UPCASE( _c ) ( ((_c) >= 'a' && (_c) <= 'z') ? ((_c) - 0x20) : (_c) )
	
#define  DECCHK( _c ) ((_c) >= '0' && (_c) <= '9')
	
#define  DTMFCHK( _c ) ( ((_c) >= '0' && (_c) <= '9') || ((_c) >= 'A' && (_c) <= 'D') || ((_c) >= 'a' && (_c) <= 'd') || ((_c) == '*') || ((_c) == '#')) 
#define  HEXCHK( _c ) ( ((_c) >= '0' && (_c) <= '9') ||((_c) >= 'A' && (_c) <= 'F') || ((_c) >= 'a' && (_c) <= 'f')) 
#define  ARR_SIZE( _a )  ( sizeof( (_a) ) / sizeof( (_a[0]) ) )
	
#ifndef UNUSED
#define UNUSED
#endif		
/*  */
typedef uint32_t pthread_mutex_t;

int pthread_mutex_init(pthread_mutex_t * t, uint32_t mode);
int pthread_mutex_destroy(pthread_mutex_t * t);
int pthread_mutex_lock(pthread_mutex_t * t);
int pthread_mutex_unlock(pthread_mutex_t * t);
int property_get(const char *key, char *value, const char *default_value);
	
#endif
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/ 
#ifdef __cplusplus
} 
#endif/*  */

#endif	/* SCI_TYPES_H */

