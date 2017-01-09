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
  \brief  Standard Macros

  Standard Macros are implemented in this file which are widely used within our 
  software. 
*/
/*******************************************************************************
 * $Id: std_macros.h 62029 2012-09-21 10:16:25Z jon.bowern $
 ******************************************************************************/

#ifndef __STD_MACROS_H__
#define __STD_MACROS_H__ //!< multiple inclusion guard

//! absolute value of x
/** 
	\note For floating points one should use fabs for R8 and fabsf for R4.
    \note x should be a variable or constant not an expression
	
    \param x a number
	\return the absolute value of x
 */
#define ABS(x)		( ((x) >= 0) ? (x) : -(x) )

//! minimum of a and b
/** 
    \note a and b should be variables or constants not expressions

    \param a a number
    \param b a number
	\return the minimum of a and b
 */
#ifndef MIN
#define MIN(a,b)	( ((a) <= (b)) ? (a) : (b) )
#endif

//! maximum of a and b
/** 
    \note a and b should be variables or constants not expressions

    \param a a number
    \param b a number
	\return the maximum of a and b
 */
#define MAX(a,b)	( ((a) >= (b)) ? (a) : (b) )

//! create a bitmask for bit a 
/** 
	The bit a must have a valid range of 0 to 31. 
	
	\note This macro may be used in drv_xxx_type files, 
		  as it is expanded by the conversion scripts .
	
	\param a bit
	\return bitmask with the bits a set to 1 
*/
#define BIT(a)		(((U4)1) << (U)(a))

//! create a bitmask for bit a 
/** 
	The bit a must have a valid range of 0 to 63. 
	
	\note This macro may be used in drv_xxx_type files, 
		  as it is expanded by the conversion scripts .
	
	\param a bit
	\return bitmask with the bits a set to 1 
*/
#define BIT8(a)		(((U8)1) << (U)(a))

//! create a bitmask for bit 0 to a-1
/** 
	The bit a must have a valid range of 0 to 32. 
	
	\note This macro may be used in drv_xxx_type files, 
		  as it is expanded by the conversion scripts .

	\param a bit
	\return bitmask with the bits 0 to a-1 set to 1 
*/
#define MASK(a)     ((U4)(((U)(a)<32) ? BIT((U)(a)&31)-1 : 0xFFFFFFFF))

//! create a bitmask for a to b bits 
/** 
	The bits a and b have a valid range of 0 to 31. 
	The bits a and b are included in the bitmask.
	
	\param a start bit
	\param b end bit
	\return bitmask with the bits a to b set to 1 
*/
#define BITS(a,b)	( ( ((MAX((U)(a),(U)(b)) - MIN((U)(a),(U)(b)) + 1) <=  0) ? (U)0x00000000 : \
						((MAX((U)(a),(U)(b)) - MIN((U)(a),(U)(b)) + 1) >= 32) ? (U)0xFFFFFFFF : \
						((BIT(MAX((U)(a),(U)(b)) - MIN((U)(a),(U)(b))) - 1) * 2 + 1) )			 \
							<< MIN((U)(a),(U)(b) ) )

//! count the leading zeros
/*! 
	count the number of leading zeros of a value 
	\param x the value to count the leading zeros
	\return the number of leading zeros 
*/
#define CLZ(x) (((x) & BIT(31)) ?  0 : \
				((x) & BIT(30)) ?  1 : \
				((x) & BIT(29)) ?  2 : \
				((x) & BIT(28)) ?  3 : \
				((x) & BIT(27)) ?  4 : \
				((x) & BIT(26)) ?  5 : \
				((x) & BIT(25)) ?  6 : \
				((x) & BIT(24)) ?  7 : \
									   \
				((x) & BIT(23)) ?  8 : \
				((x) & BIT(22)) ?  9 : \
				((x) & BIT(21)) ? 10 : \
				((x) & BIT(20)) ? 11 : \
				((x) & BIT(19)) ? 12 : \
				((x) & BIT(18)) ? 13 : \
				((x) & BIT(17)) ? 14 : \
				((x) & BIT(16)) ? 15 : \
									   \
				((x) & BIT(15)) ? 16 : \
				((x) & BIT(14)) ? 17 : \
				((x) & BIT(13)) ? 18 : \
				((x) & BIT(12)) ? 19 : \
				((x) & BIT(11)) ? 20 : \
				((x) & BIT(10)) ? 21 : \
				((x) & BIT( 9)) ? 22 : \
				((x) & BIT( 8)) ? 23 : \
									   \
				((x) & BIT( 7)) ? 24 : \
				((x) & BIT( 6)) ? 25 : \
				((x) & BIT( 5)) ? 26 : \
				((x) & BIT( 4)) ? 27 : \
				((x) & BIT( 3)) ? 28 : \
				((x) & BIT( 2)) ? 29 : \
				((x) & BIT( 1)) ? 30 : \
				((x) & BIT( 0)) ? 31 : \
								  32 )

//! count the tailing zeros
/*! 
	count the number of tailing zeros of a value 
	\param x the value to count the tailing zeros
	\return the number of tailing zeros 
*/
#define CTZ(x) (((x) & BIT( 0)) ?  0 : \
				((x) & BIT( 1)) ?  1 : \
				((x) & BIT( 2)) ?  2 : \
				((x) & BIT( 3)) ?  3 : \
				((x) & BIT( 4)) ?  4 : \
				((x) & BIT( 5)) ?  5 : \
				((x) & BIT( 6)) ?  6 : \
				((x) & BIT( 7)) ?  7 : \
									   \
				((x) & BIT( 8)) ?  8 : \
				((x) & BIT( 9)) ?  9 : \
				((x) & BIT(10)) ? 10 : \
				((x) & BIT(11)) ? 11 : \
				((x) & BIT(12)) ? 12 : \
				((x) & BIT(13)) ? 13 : \
				((x) & BIT(14)) ? 14 : \
				((x) & BIT(15)) ? 15 : \
									   \
				((x) & BIT(16)) ? 16 : \
				((x) & BIT(17)) ? 17 : \
				((x) & BIT(18)) ? 18 : \
				((x) & BIT(19)) ? 19 : \
				((x) & BIT(20)) ? 20 : \
				((x) & BIT(21)) ? 21 : \
				((x) & BIT(22)) ? 22 : \
				((x) & BIT(23)) ? 23 : \
									   \
				((x) & BIT(24)) ? 24 : \
				((x) & BIT(25)) ? 25 : \
				((x) & BIT(26)) ? 26 : \
				((x) & BIT(27)) ? 27 : \
				((x) & BIT(28)) ? 28 : \
				((x) & BIT(29)) ? 29 : \
				((x) & BIT(30)) ? 30 : \
				((x) & BIT(31)) ? 31 : \
								  32 )

//! calculate bit count for a given type
/*!
  calculate bit count for a given type. this is to
  avoid magic numbers such as  \c 32 or \c 64.

  \param t  type, e.g. #U4
  \return number of bits, e.g. 32
*/
#define BITCNT(t) ((U)(sizeof(t)*8))

//! binary log of constant c
/** Return binary log of constant c 
    
    \note use lib_uint_logb for variables
    \note c should be a constant not an expression
	
    \param c a number in the range 1 .. 2^32-1
	\return the binary log of c
 */
#define LOGB(c)     ( ((c) >= BIT(31)) ? 31 : \
					  ((c) >= BIT(30)) ? 30 : \
					  ((c) >= BIT(29)) ? 29 : \
					  ((c) >= BIT(28)) ? 28 : \
					  ((c) >= BIT(27)) ? 27 : \
					  ((c) >= BIT(26)) ? 26 : \
					  ((c) >= BIT(25)) ? 25 : \
					  ((c) >= BIT(24)) ? 24 : \
										  	  \
					  ((c) >= BIT(23)) ? 23 : \
					  ((c) >= BIT(22)) ? 22 : \
					  ((c) >= BIT(21)) ? 21 : \
					  ((c) >= BIT(20)) ? 20 : \
					  ((c) >= BIT(19)) ? 19 : \
					  ((c) >= BIT(18)) ? 18 : \
					  ((c) >= BIT(17)) ? 17 : \
					  ((c) >= BIT(16)) ? 16 : \
											  \
					  ((c) >= BIT(15)) ? 15 : \
					  ((c) >= BIT(14)) ? 14 : \
					  ((c) >= BIT(13)) ? 13 : \
					  ((c) >= BIT(12)) ? 12 : \
					  ((c) >= BIT(11)) ? 11 : \
					  ((c) >= BIT(10)) ? 10 : \
					  ((c) >= BIT( 9)) ?  9 : \
					  ((c) >= BIT( 8)) ?  8 : \
										  	  \
					  ((c) >= BIT( 7)) ?  7 : \
					  ((c) >= BIT( 6)) ?  6 : \
					  ((c) >= BIT( 5)) ?  5 : \
					  ((c) >= BIT( 4)) ?  4 : \
					  ((c) >= BIT( 3)) ?  3 : \
					  ((c) >= BIT( 2)) ?  2 : \
					  ((c) >= BIT( 1)) ?  1 : \
					  ((c) >= BIT( 0)) ?  0 : \
					  /* error */        -1 )
		
//! get the number of bits used for a numeric states
/** 
	Seraches for the first number x where 2^x is greather than a
	
	\param a number in the range 1 .. 2^32
	\return the number of bits used to cover the numeric range 0 to a
*/
#define NUMBITS(a) 	( (a < 0) ? 0 : \
                      (a == 1) ? 1 : \
                      			(1 + LOGB((a) - 1)) )

//! Limit to Maximum
/*!
  Limit a value to a given upper Limit

  \param val    value to limit
  \param lim    limit
  \return value clipped if above \a lim
*/
#define LIMIT_UP(val,lim) ((val) > (lim) ? (lim) : (val))

//! Limit to Minimum
/*!
  Limit a value to a given lower Limit

  \param val    value to limit
  \param lim    limit
  \return value clipped if below \a lim
*/
#define LIMIT_LO(val,lim) ((val) < (lim) ? (lim) : (val))

//! signal a unused argument or variable to avoid compiler warnings
/** 
	\param a the unused variable or argument
*/
#define UNUSED(a)	((void) (a))

//! Align Value
/*!
  This macro can be used to align sizes or addresses to a specific \a nByte boundary,
  where \a nByte must be of the form 2^k. E.g. ALIGN(foo,sizeof(U4)) rounds up \c foo
  to be a multiple of 4

  \param val    value to align
  \param nBytes alignment size
  \return aligned value
*/
#define ALIGN(val,nBytes)											  \
	(((val)+((nBytes)-1))&(~((nBytes)-1)))

//! execute the function at address 
/** 
	This macro calls the function at the address pAdr. No arguments are
	passed to the function and the return value is ignored. In other words 
	the function should have the following declaration.

	\code
	void FunctionName(void); 
	\endcode
		
	\param pAdr the address of the function
*/
#define EXECUTE(pAdr)	((void (*)(void))((U4)(pAdr)))()

//! endless define 
/** Use this define in endless while loops in the following way:
	This macro inhibits lint warning 716.

	\code
	while (ENDLESS)
	{
		// do something here 
	}
	\endcode
*/
#define ENDLESS	/*lint -save -e716*/ TRUE /*lint -restore*/

//! fallthrough define
/** Use this define to signal a fallthrough in a switch statement from 
	one case to the following one.
	
	\code
	switch (s)
	{
		case a:
			break;
		case b:
			FALLTHROUGH
		default:
	}
	\endcode
*/
#define FALLTHROUGH	/*lint -fallthrough @fallthrough@ */

//! cast an address into an array 
/** 
    Splint does not support cast to arrays. This marco will take 
	care of the missing cast support.
	\param type the type of one array element
	\param num the number of elements in the array 
	\param addr the address to cast 
	\return the casted array 
*/
#ifdef S_SPLINT_S
 #define ARRAYCAST(type, num, addr)	( (type*)(addr)) // size unused
#else
 #define ARRAYCAST(type, num, addr)	(*(type (*)[num]) (addr))
#endif

//! returns number of elements of an array 
/**
	This macro is very usful to calculate the number of elements of an 
	array. The number of elements is calculated by dividing the size of the 
	array by the size of one element.

	\param array an array
	\return the number of elements 
*/
#define	NUMOF(array) 	(sizeof(array)/sizeof(*(array)))

//! \name Special Cast Macros
//@{
//! Cast C2 to U2
/*!
  Cast a 2-byte complex value to a 2-byte unsigned integer

  \note does not work for literal constants or expressions, i.e. \a c should be an lvalue

  \param c  the C2 value to be casted
  \return the casted U2 value
*/
#define C2_TO_U2(c) (*(U2*)&(c))

//! Cast U2 to C2
/*!
  Cast a 2-byte unsigned integer to a 2-byte complex value

  \note does not work for literal constants or expressions, i.e. \a u should be an lvalue

  \param u  the U2 value to be casted
  \return the casted C2 value
*/
#define U2_TO_C2(u) (*(C2*)&(u))

//! Cast C4 to U4
/*!
  Cast a 4-byte complex value to a 4-byte unsigned integer

  \note does not work for literal constants or expressions, i.e. \a c should be an lvalue

  \param c  the C4 value to be casted
  \return the casted U4 value
*/
#define C4_TO_U4(c) (*(U4*)&(c))

//! Cast U4 to C4
/*!
  Cast a 4-byte unsigned integer to a 4-byte complex value

  \note does not work for literal constants or expressions, i.e. \a u should be an lvalue

  \param u  the U4 value to be casted
  \return the casted C4 value
*/
#define U4_TO_C4(u) (*(C4*)&(u))
//@}

//! Byte access to register
/*!
  Access a byte from a 32bit register

  \param reg    the register to access (type REG)
  \param ofs    byte offset (0..3)
  \return byte represented as U1
*/
#define READ_REG_BYTE(reg,ofs) \
 *(((U1*)&reg)+((ofs)&0x3))

//! Halfword access to register
/*!
  Access a Halfword from a 32bit register

  \param reg    the register to access (type REG)
  \param ofs    halfword offset (0..1)
  \return halfword represented as U2
*/
#define READ_REG_HWORD(reg,ofs) \
 *(((U2*)&reg)+((ofs)&0x1))


#endif // __STD_MACROS_H__
