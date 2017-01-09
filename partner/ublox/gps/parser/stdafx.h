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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#ifndef UNIX_API
	#include <tchar.h>
	#include <crtdbg.h>
	#include "wtypes.h"
	#define PARSER_ASSERT(a) _ASSERT(a)
#else
	#include <time.h>
	
	#define __drv_floatUsed
	#define _T(a) a
	#define _TCHAR char
	#define _tfopen_s(a,b,c) (!(*(a) = fopen(b,c)))
	#define _tmain main
#endif

#ifndef ATLTRY
	#define ATLTRY(a) a
#endif


// Reference additional headers your program requires here
