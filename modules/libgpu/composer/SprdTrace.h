/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 03/06/2014    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with display controller specification,  *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File: SprdTrace.h                 DESCRIPTION                             *
 **                                   Add Android Framework trace info        *
                                      for debugging                           *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_TRACE_H_
#define _SPRD_TRACE_H_


#define ATRACE_TAG (ATRACE_TAG_GRAPHICS | ATRACE_TAG_HAL)

#include <utils/Trace.h>
#include <cutils/trace.h>


#ifdef HWC_DEBUG_TRACE

#define HWC_TRACE_CALL                                            ATRACE_CALL()
#define HWC_TRACE_BEGIN_VSYNC        atrace_begin(ATRACE_TAG, "SprdVsyncEvent")
#define HWC_TRACE_BEGIN_WIDIBLIT   atrace_begin(ATRACE_TAG, "SprdWIDIBLITWork")
#define HWC_TRACE_END                                     atrace_end(ATRACE_TAG)

#else

#define HWC_TRACE_CALL
#define HWC_TRACE_BEGIN_VSYNC
#define HWC_TRACE_BEGIN_WIDIBLIT
#define HWC_TRACE_END

#endif


#endif
