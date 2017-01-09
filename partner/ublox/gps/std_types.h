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
  \brief  Standard Numeric Data Types

  Basic numeric data have been defined for useability and easy porting of the 
  project to other targets. The idea is to have small types which provide 
  information about the ist range. These types are also known as the Tcl SECS 
  Notation. Microsoft uses the same types in their "UnmanagedType Enumeration" 
  in the .NET Framework Class Library. The types have been extended with other 
  types which are quite common in our software.
*/
/*******************************************************************************
 * $Id: std_types.h 56360 2012-03-08 17:52:11Z philippe.kehl $
 ******************************************************************************/

#ifndef __STD_TYPES_H__
#define __STD_TYPES_H__ //!< multiple inclusion guard

#ifndef _MSC_VER
#include <stdint.h>
#endif

#define FALSE      0    //!< false value
#define TRUE       1    //!< true value
#define CONFIRMED  2    //!< confirmed value

/*! \name Signed Integer Types */
//@{
typedef signed int				I;  	//!< equal to I4 (on ARM7TDMI)
#ifdef _MSC_VER
typedef __int8					I1;		//!< signed 1 byte integer
typedef __int16					I2;		//!< signed 2 byte integer
typedef __int32					I4;		//!< signed 4 byte integer
typedef __int64					I8;		//!< signed 8 byte integer
#else
typedef int8_t					I1; 	//!< signed 1 byte integer
typedef int16_t					I2; 	//!< signed 2 byte integer
typedef int32_t					I4; 	//!< signed 4 byte integer
typedef int64_t					I8;		//!< signed 8 byte integer
#endif
//@}

/*! \name Unsigned Integer Types */ 
//@{
typedef unsigned int       		U;  	//!< equal to U4 (on ARM7TDMI)
#ifdef _MSC_VER
typedef unsigned __int8			U1;		//!< unsigned 1 byte integer
typedef unsigned __int16		U2;		//!< unsigned 2 byte integer
typedef unsigned __int32		U4;		//!< unsigned 4 byte integer
typedef unsigned __int64		U8;		//!< unsigned 8 byte integer
#else
typedef uint8_t					U1; 	//!< unsigned 1 byte integer
typedef uint16_t				U2; 	//!< unsigned 2 byte integer
typedef uint32_t				U4; 	//!< unsigned 4 byte integer
typedef uint64_t				U8; 	//!< unsigned 8 byte integer
#endif
//@}

/*! \name Logical (or Boolean) Types */
//@{
typedef U						L;  	//!< equal to L4 (on ARM7TDMI)
typedef U1						L1; 	//!< 1 byte logical (FALSE, TRUE and CONFIRMED only)
typedef U2						L2; 	//!< 2 byte logical (FALSE, TRUE and CONFIRMED only)
typedef U4						L4;		//!< 4 byte logical (FALSE, TRUE and CONFIRMED only)
//@}

/*! \name Floating Point Types */
//@{
typedef float              		R4; 	//!< 4 byte floating point
typedef double             		R8; 	//!< 8 byte floating point
//@}

/*! \name Character Type */
//@{
typedef char               		CH; 	//!< ASCII character
//@}

/*! \name Complex Number Types */
//@{
//! Complex 2-byte data type; in-phase component is in high byte
typedef struct C2_s
{
	I1 Q;                   				//!< 1 byte quadrature (imaginary) component
	I1 I;                   				//!< 1 byte in-phase (real) component
}                          		C2;

//! Complex 4-byte data type; in-phase component is in high half-word
typedef struct C4_s
{
	I2 Q;                   				//!< 2 byte quadrature (imaginary) component
	I2 I;                   				//!< 2 byte in-phase (real) component
}                          		C4;

//! Complex 8-byte data type; in-phase component is in high word
typedef struct C8_s
{
	I4 Q;                   				//!< 4 byte quadrature (imaginary) component
	I4 I;                   				//!< 4 byte in-phase (real) component
}                          		C8;
//@}

//! Helper Macro for FIXEDPOINT()
#define _FIXEDPOINTA(t,scale) F##t##_##scale
//! Create Fixed Point Type
#define FIXEDPOINT(t,scale) _FIXEDPOINTA(t,scale)

//! \name Unsigned Fixpoint Types
//@{
typedef U1 FU1_4; 				//!< 1 byte unsigned, scale 4: U.F
typedef U1 FU1_5; 				//!< 1 byte unsigned, scale 5: uuu.fF

typedef U2 FU2_4;				//!< 2 bytes unsigned, scale 4:  UUU.F
typedef U2 FU2_8;               //!< 2 bytes unsigned, scale 8:  UU.FF
typedef U2 FU2_12;              //!< 2 bytes unsigned, scale 12: U.FFF
typedef U2 FU2_16;              //!< 2 bytes unsigned, scale 16: .FFFF

typedef U4 FU4_4; 				//!< 4 bytes unsigned, scale 4:  UUUUUUU.F
typedef U4 FU4_8; 				//!< 4 bytes unsigned, scale 8:  UUUUUU.FF
typedef U4 FU4_12;				//!< 4 bytes unsigned, scale 12: UUUUU.FFF
typedef U4 FU4_16;				//!< 4 bytes unsigned, scale 16: UUUU.FFFF
typedef U4 FU4_20;				//!< 4 bytes unsigned, scale 20: UUU.FFFFF
typedef U4 FU4_24;				//!< 4 bytes unsigned, scale 24: UU.FFFFFF
typedef U4 FU4_28;				//!< 4 bytes unsigned, scale 28: U.FFFFFFF
typedef U4 FU4_32;				//!< 4 bytes unsigned, scale 32: .FFFFFFFF

typedef U8 FU8_4; 				//!< 8 bytes unsigned, scale 4:  UUUUUUUUUUUUUUU.F
typedef U8 FU8_8; 				//!< 8 bytes unsigned, scale 8:  UUUUUUUUUUUUUU.FF
typedef U8 FU8_12;				//!< 8 bytes unsigned, scale 12: UUUUUUUUUUUUU.FFF
typedef U8 FU8_16;				//!< 8 bytes unsigned, scale 16: UUUUUUUUUUUU.FFFF
typedef U8 FU8_20;				//!< 8 bytes unsigned, scale 20: UUUUUUUUUUU.FFFFF
typedef U8 FU8_24;				//!< 8 bytes unsigned, scale 24: UUUUUUUUUU.FFFFFF
typedef U8 FU8_28;				//!< 8 bytes unsigned, scale 28: UUUUUUUUU.FFFFFFF
typedef U8 FU8_32;				//!< 8 bytes unsigned, scale 32: UUUUUUUU.FFFFFFFF
typedef U8 FU8_36;				//!< 8 bytes unsigned, scale 36: UUUUUUU.FFFFFFFFF
typedef U8 FU8_40;				//!< 8 bytes unsigned, scale 40: UUUUUU.FFFFFFFFFF
typedef U8 FU8_44;				//!< 8 bytes unsigned, scale 44: UUUUU.FFFFFFFFFFF
typedef U8 FU8_48;				//!< 8 bytes unsigned, scale 48: UUUU.FFFFFFFFFFFF
typedef U8 FU8_52;				//!< 8 bytes unsigned, scale 52: UUU.FFFFFFFFFFFFF
typedef U8 FU8_56;				//!< 8 bytes unsigned, scale 56: UU.FFFFFFFFFFFFFF
typedef U8 FU8_60;				//!< 8 bytes unsigned, scale 60: U.FFFFFFFFFFFFFFF
typedef U8 FU8_64;				//!< 8 bytes unsigned, scale 64: .FFFFFFFFFFFFFFFF
//@}

//! \name Signed Fixpoint Types
//@{
typedef I1 FI1_4; 				//!< 1 byte signed, scale 4: I.F

typedef I2 FI2_4;				//!< 2 bytes signed, scale 4:  III.F
typedef I2 FI2_8;               //!< 2 bytes signed, scale 8:  II.FF
typedef I2 FI2_12;              //!< 2 bytes signed, scale 12: I.FFF
typedef I2 FI2_16;              //!< 2 bytes signed, scale 16: .FFFF

typedef I4 FI4_4; 				//!< 4 bytes signed, scale 4:  IIIIIII.F
typedef I4 FI4_8; 				//!< 4 bytes signed, scale 8:  IIIIII.FF
typedef I4 FI4_12;				//!< 4 bytes signed, scale 12: IIIII.FFF
typedef I4 FI4_16;				//!< 4 bytes signed, scale 16: IIII.FFFF
typedef I4 FI4_20;				//!< 4 bytes signed, scale 20: III.FFFFF
typedef I4 FI4_24;				//!< 4 bytes signed, scale 24: II.FFFFFF
typedef I4 FI4_28;				//!< 4 bytes signed, scale 28: I.FFFFFFF
typedef I4 FI4_31;              //!< 4 bytes signed, scale 31: i.fffFFFFFFF
typedef I4 FI4_32;				//!< 4 bytes signed, scale 32: .FFFFFFFF

typedef I8 FI8_4; 				//!< 8 bytes signed, scale 4:  IIIIIIIIIIIIIII.F
typedef I8 FI8_8; 				//!< 8 bytes signed, scale 8:  IIIIIIIIIIIIII.FF
typedef I8 FI8_12;				//!< 8 bytes signed, scale 12: IIIIIIIIIIIII.FFF
typedef I8 FI8_16;				//!< 8 bytes signed, scale 16: IIIIIIIIIIII.FFFF
typedef I8 FI8_20;				//!< 8 bytes signed, scale 20: IIIIIIIIIII.FFFFF
typedef I8 FI8_24;				//!< 8 bytes signed, scale 24: IIIIIIIIII.FFFFFF
typedef I8 FI8_28;				//!< 8 bytes signed, scale 28: IIIIIIIII.FFFFFFF
typedef I8 FI8_32;				//!< 8 bytes signed, scale 32: IIIIIIII.FFFFFFFF
typedef I8 FI8_36;				//!< 8 bytes signed, scale 36: IIIIIII.FFFFFFFFF
typedef I8 FI8_40;				//!< 8 bytes signed, scale 40: IIIIII.FFFFFFFFFF
typedef I8 FI8_44;				//!< 8 bytes signed, scale 44: IIIII.FFFFFFFFFFF
typedef I8 FI8_48;				//!< 8 bytes signed, scale 48: IIII.FFFFFFFFFFFF
typedef I8 FI8_52;				//!< 8 bytes signed, scale 52: III.FFFFFFFFFFFFF
typedef I8 FI8_56;				//!< 8 bytes signed, scale 56: II.FFFFFFFFFFFFFF
typedef I8 FI8_60;				//!< 8 bytes signed, scale 60: I.FFFFFFFFFFFFFFF
typedef I8 FI8_64;				//!< 8 bytes signed, scale 64: .FFFFFFFFFFFFFFFF
//@}

/*! \name Volatile Register Type */
//@{
#define REG  			volatile U4 	//!< 32 bit Register value
//@}

#endif // __STD_TYPES_H__
