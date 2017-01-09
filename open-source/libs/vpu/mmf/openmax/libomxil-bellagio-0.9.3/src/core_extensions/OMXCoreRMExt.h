/**
  src/core_extensions/OMXCoreRMExt.h

  This extension of the core provides functions for the resource manager used to retrieve
	the quality levels of the components available.

  Copyright (C) 2008-2010 STMicroelectronics

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

#ifndef __OMXCORERMEXT_H__
#define __OMXCORERMEXT_H__

#include <OMX_Core.h>
#include "extension_struct.h"

enum OMX_RESOURCETYPE
{
	OMX_ResourceTypeMemory,
	OMX_ResourceTypeCpu
};

/**
 * @brief Returns the supported quality levels for a scalable OMXIL component.
 * @param cComponentName [IN] Component name
 * @param ppQualityLevel [OUT] List of supported quality levels
 * @param pNrOfQualityLevels [OUT] Number of quality levels in the list
 * @return OMX_ErrorNone
 * @return OMX_ErrorInvalidComponentName
 * @return OMX_ErrorComponentNotFound
 * @pre ppQualityLevel != NULL
 * @pre pNrOfQualityLevels !=NULL
 */
OMX_ERRORTYPE getSupportedQualityLevels(OMX_STRING cComponentName, OMX_U32** ppQualityLevel, OMX_U32* pNrOfQualityLevels);
/**
 * @brief Returns the multiresource estimates for a given OMXIL component name and
 * quality level.
 * @param cComponentName [IN] Component name
 * @param nQualityLevel [IN] quality level (applicable for scalable components)
 * @param pMultiResourceEstimates [OUT] The multiresource estimates
 * @return OMX_ErrorNone
 * @return OMX_ErrorInvalidComponentName
 * @return OMX_ErrorComponentNotFound
 * @verbatim
 * * The resource estimates for a given quality level are the basis for generating
 * * the resource budgets. Later on, when components are instantiated and the bit
 * * stream properties are known, resource budget adaptations might be required,
 * or * if not feasible, the quality level might need to be downgraded.
 * @endverbatim
 */
OMX_ERRORTYPE getMultiResourceEstimates(OMX_STRING cComponentName, OMX_U32 nQualityLevel, multiResourceDescriptor* pMultiResourceEstimates);
OMX_ERRORTYPE readRegistryFile();
#endif

