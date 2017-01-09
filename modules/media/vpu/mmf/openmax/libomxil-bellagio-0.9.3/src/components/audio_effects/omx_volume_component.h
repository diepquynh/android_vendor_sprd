/**
  src/components/audio_effects/omx_volume_component.h

  OpenMAX volume control component. This component implements a filter that
  controls the volume level of the audio PCM stream.

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

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

#ifndef _OMX_VOLUME_COMPONENT_H_
#define _OMX_VOLUME_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <string.h>
#include <omx_base_filter.h>
#define VOLUME_COMP_NAME "OMX.st.volume.component"
#define VOLUME_COMP_ROLE "volume.component"
#define MAX_VOLUME_COMPONENTS 10
#define VOLUME_QUALITY_LEVELS 2
static int volumeQualityLevels []={1, 65536, 1, 32768};

/** Twoport component private structure.
* see the define above
*/
DERIVEDCLASS(omx_volume_component_PrivateType, omx_base_filter_PrivateType)
#define omx_volume_component_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
  /** @param gain the volume gain value */ \
  float gain;
ENDCLASS(omx_volume_component_PrivateType)

/* Component private entry points declaration */
OMX_ERRORTYPE omx_volume_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE omx_volume_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

void omx_volume_component_BufferMgmtCallback(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_BUFFERHEADERTYPE* inputbuffer,
  OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE omx_volume_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_volume_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_volume_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE omx_volume_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

#endif
