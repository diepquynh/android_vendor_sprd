/**
  src/components/videoscheduler/omx_video_scheduler_component.h

  This component implements a video scheduler

  Copyright (C) 2008-2009 STMicroelectronics
  Copyright (C) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#ifndef _OMX_VIDEO_SCHEDULER_H_
#define _OMX_VIDEO_SCHEDULER_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <omx_base_filter.h>
#include <omx_base_video_port.h>
#include <omx_base_clock_port.h>

#define VIDEO_SCHEDULER_COMP_NAME "OMX.st.video.scheduler"
#define VIDEO_SCHEDULER_COMP_ROLE "video.scheduler"
#define MAX_VIDEOSCHED_COMPONENTS 10

#define VIDEOSCHED_QUALITY_LEVELS 2
static int videoSchedQualityLevels []={1, 456192, 1, 304128};

/** video scheduler component private structure.
  * @param xScale the scale of the media clock
  * @param eState the state of the media clock
  * @param frameDropFlag the flag active on scale change indicates that frames are to be dropped
  * @param dropFrameCount counts the number of frames dropped
  */
DERIVEDCLASS(omx_video_scheduler_component_PrivateType, omx_base_filter_PrivateType)
#define omx_video_scheduler_component_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
  OMX_S32                      xScale; \
  OMX_TIME_CLOCKSTATE          eState; \
  OMX_BOOL                     frameDropFlag;\
  int                          dropFrameCount;
ENDCLASS(omx_video_scheduler_component_PrivateType)

/* Component private entry points declaration */
OMX_ERRORTYPE omx_video_scheduler_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName);
OMX_ERRORTYPE omx_fbdev_sink_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_fbdev_sink_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);

OMX_ERRORTYPE omx_video_scheduler_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

void omx_video_scheduler_component_BufferMgmtCallback(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_BUFFERHEADERTYPE* inputbuffer,
  OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE omx_video_scheduler_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_video_scheduler_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

/* to handle the communication at the clock port */
OMX_BOOL omx_video_scheduler_component_ClockPortHandleFunction(
  omx_video_scheduler_component_PrivateType* omx_video_scheduler_component_Private,
  OMX_BUFFERHEADERTYPE* inputbuffer);

OMX_ERRORTYPE omx_video_scheduler_component_port_SendBufferFunction(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE omx_video_scheduler_component_port_FlushProcessingBuffers(omx_base_PortType *openmaxStandPort);
#endif
