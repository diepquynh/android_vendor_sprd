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
  \brief  Some definitions which extend the standard C language

  
*/
/*******************************************************************************
 * $Id: std_lang_def.h 56360 2012-03-08 17:52:11Z philippe.kehl $
 ******************************************************************************/

#ifndef __STD_LANG_DEF_H__
#define __STD_LANG_DEF_H__ //!< multiple inclusion guard


/*! \name Constants for Boolean Values ( L , #L1 , #L2 , #L4 ) */
//@{
#define FALSE		0 			//!< Boolean FALSE Value
#define TRUE		1 			//!< Boolean TRUE Value
#define CONFIRMED	2			//!< Trilian CONFIRMED Value
#define GUESSED		3			//!< Guessed Value
//@}

#ifndef NULL
#define NULL 		((void*)0)	//!< Some standard librarys like ADS do not define NULL. 
#endif

/*! \name Dummy Argument Direction Declarator Constants */
//@{
#define IN                       //!< Dummy Input Argument Declarator
#define OUT                      //!< Dummy Output Argument Declarator
#define INOUT                    //!< Dummy Input/Output Argument Declarator
//@}

#endif // __STD_LANG_DEF_H__

