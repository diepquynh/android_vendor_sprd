/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 * $History$
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved.
 *
 * \file
 * Type.h
 *
 * \brief
 * definition of type
 *
 * \date
 * 2016/01/03
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */

#ifndef __AFV1_TYPE_H__
#define  __AFV1_TYPE_H__


//=========================================================================//
// Public Definition
//=========================================================================//
typedef signed char           int8_t;
typedef unsigned char         uint8_t;
typedef signed short int      int16_t;
typedef unsigned short int    uint16_t;
typedef signed int 	          int32_t;
typedef unsigned int 	      uint32_t;
#ifdef WIN32
typedef __int64               int64_t;
typedef unsigned __int64      uint64_t;
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

typedef int8_t     int8;
typedef uint8_t    uint8;
typedef int16_t    int16;
typedef uint16_t   uint16;
typedef int32_t    int32;
typedef uint32_t   uint32;
typedef int64_t    int64;
typedef uint64_t   uint64;
typedef uint8      ERRCODE;


#endif
