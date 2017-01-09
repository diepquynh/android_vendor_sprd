//------------------------------------------------------------------------------
// File: config.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
// This file should be modified by some developers of C&M according to product version.
//------------------------------------------------------------------------------


#ifndef __CONFIG_H__
#define __CONFIG_H__


#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#	define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#	define PLATFORM_LINUX
#else
#	define PLATFORM_NON_OS
#endif


#if defined(_MSC_VER)
#	include <windows.h>
#	define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#  error "Unknown compiler."
#endif


#define API_VERSION 330



#if defined(PLATFORM_NON_OS) || defined (ANDROID) || defined(MFHMFT_EXPORTS)
//#define SUPPORT_FFMPEG_DEMUX
#else
#define SUPPORT_FFMPEG_DEMUX
#endif


#define SUPPORT_REF_FLAG_REPORT



//#define REPORT_PERFORMANCE


//#define SUPPORT_MEM_PROTECT










// #define BIT_CODE_FILE_PATH "../../../../design/asm_s/out/Mach.h"
// #define PROJECT_ROOT	"../../../../"

#define BIT_CODE_FILE_PATH "./Falcon.h"
#define PROJECT_ROOT "D:/project/ip/FalconSE/"

// #define BIT_CODE_FILE_PATH "D:/project/ip/Mach2SE/design/asm_s/out/Mach.h"
// #define PROJECT_ROOT "D:/project/ip/Mach2SE/"





// the bellows must be disable in case of CODA7L
// #define GDI_2_0


#endif	/* __CONFIG_H__ */


