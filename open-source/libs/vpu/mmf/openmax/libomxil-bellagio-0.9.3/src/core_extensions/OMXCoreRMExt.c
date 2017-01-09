/**
  src/core_extensions/OMXCoreRMExt.c

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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "OMXCoreRMExt.h"
#include "st_static_component_loader.h"

static int data_loaded = 0;
static stLoaderComponentType** qualityList;
static int qualityListItems = 0;

OMX_ERRORTYPE getSupportedQualityLevels(OMX_STRING cComponentName, OMX_U32** ppQualityLevel, OMX_U32* pNrOfQualityLevels) {
	OMX_ERRORTYPE err;
	int found = 0;
	int j,i,k;
	if (pNrOfQualityLevels == NULL) {
		return OMX_ErrorUndefined;
	}

	if (!data_loaded) {
		err = readRegistryFile();
		if (err != OMX_ErrorNone) {
			return err;
		}
		data_loaded = 1;
	}
	for (i = 0; i<qualityListItems; i++) {
		if(!strcmp(qualityList[i]->name, cComponentName)) {
			DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s found requested component %s with quality levels %i\n", __func__, cComponentName, (int)qualityList[i]->nqualitylevels);
			*pNrOfQualityLevels = qualityList[i]->nqualitylevels;
			if (ppQualityLevel == NULL) {
				return OMX_ErrorNone;
			}
			for (k=0; k<(int)qualityList[i]->nqualitylevels; k++) {
				(*ppQualityLevel)[k] = k+1;
			}
			found = 1;
		} else {
			for(j=0;j<(int)qualityList[i]->name_specific_length;j++) {
	        if(!strcmp(qualityList[i]->name_specific[j], cComponentName)) {
		        	DEBUG(DEB_LEV_SIMPLE_SEQ, "Found requested component %s IN SPECIFIC COMPONENT \n", cComponentName);
						*pNrOfQualityLevels = qualityList[i]->nqualitylevels;
						if (ppQualityLevel == NULL) {
							return OMX_ErrorNone;
						}
						for (k=0; k<(int)qualityList[i]->nqualitylevels; k++) {
							(*ppQualityLevel)[k] = k+1;
						}
						found = 1;
		     }
			}
		}
		if(found) {
			break;
		}
	}
	if(!found) {
		DEBUG(DEB_LEV_ERR, "Not found any component\n");
		*pNrOfQualityLevels = 0;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE getMultiResourceEstimates(OMX_STRING cComponentName, OMX_U32 nQualityLevel, multiResourceDescriptor* pMultiResourceEstimates) {
	OMX_ERRORTYPE err;
	int found = 0;
	int j,i = 0;

	if (pMultiResourceEstimates == NULL) {
		return OMX_ErrorUndefined;
	}
	if (!data_loaded) {
		err = readRegistryFile();
		if (err != OMX_ErrorNone) {
			return err;
		}
		data_loaded = 1;
	}
	for (i = 0; i<qualityListItems; i++) {
		if(!strcmp(qualityList[i]->name, cComponentName)) {
			DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s found requested component %s with quality level %i\n", __func__, cComponentName, (int)nQualityLevel);
			if ((nQualityLevel>0) && (nQualityLevel<=qualityList[i]->nqualitylevels)) {
				found = 1;
				pMultiResourceEstimates->CPUResourceRequested = qualityList[i]->multiResourceLevel[nQualityLevel-1]->CPUResourceRequested;
				pMultiResourceEstimates->MemoryResourceRequested = qualityList[i]->multiResourceLevel[nQualityLevel-1]->MemoryResourceRequested;
				break;
			}
		} else {
			for(j=0;j<(int)qualityList[i]->name_specific_length;j++) {
		        if(!strcmp(qualityList[i]-> name_specific[j], cComponentName)) {
		        	DEBUG(DEB_LEV_SIMPLE_SEQ, "Found requested component %s IN SPECIFIC COMPONENT \n", cComponentName);
					if ((nQualityLevel>0) && (nQualityLevel<=qualityList[i]->nqualitylevels)) {
						found = 1;
						pMultiResourceEstimates->CPUResourceRequested = qualityList[i]->multiResourceLevel[nQualityLevel-1]->CPUResourceRequested;
						pMultiResourceEstimates->MemoryResourceRequested = qualityList[i]->multiResourceLevel[nQualityLevel-1]->MemoryResourceRequested;
						break;
					}
					if (found) {
						break;
					}
		        }
			}
		}
		if(found) {
			break;
		}
	}
	if(!found) {
		pMultiResourceEstimates->CPUResourceRequested = -1;
		pMultiResourceEstimates->MemoryResourceRequested = -1;
	}
	return OMX_ErrorNone;
}

/** This function reads the .omxregister file and retrieve all the information about resources and quality levels.
 */
OMX_ERRORTYPE readRegistryFile() {
	FILE* omxregistryfp;
	char* line = NULL;
	char *libname;
	int index_readline;
	int listindex;
	char *registry_filename;
	int numberOfLines = 0;
	int index;
	int tempindex;
	int wordlength;
	int roleindex;
	int numlevels;
	int i;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	qualityList = NULL;

	registry_filename = componentsRegistryGetFilename();
	omxregistryfp = fopen(registry_filename, "r");
	if (omxregistryfp == NULL){
		DEBUG(DEB_LEV_ERR, "Cannot open OpenMAX registry file %s\n", registry_filename);
		return OMX_ErrorUndefined;
	}
	free(registry_filename);
	libname = malloc(OMX_MAX_STRINGNAME_SIZE * 2);
	fseek(omxregistryfp, 0, 0);

	  while(1) {
		  index_readline = 0;
		  while(index_readline < MAX_LINE_LENGTH) {
			  *(line+index_readline) = fgetc(omxregistryfp);
			  if ((*(line+index_readline) == '\n') || (*(line+index_readline) == '\0')) {
				  break;
			  }
			  index_readline++;
		  }
		  *(line+index_readline) = '\0';
		  if ((index_readline >= MAX_LINE_LENGTH) || (index_readline == 0)) {
			  break;
		  }
		if ((*line == ' ') && (*(line+1) == '=')) {
			numberOfLines++;
		} else {
			continue;
		}
	}
	fseek(omxregistryfp, 0, 0);
	qualityList = malloc(numberOfLines * sizeof (stLoaderComponentType*));
	qualityListItems = numberOfLines;
	line = malloc(MAX_LINE_LENGTH);
	listindex = 0;

	  while(1) {
		  index_readline = 0;
		  while(index_readline < MAX_LINE_LENGTH) {
			  *(line+index_readline) = fgetc(omxregistryfp);
			  if ((*(line+index_readline) == '\n') || (*(line+index_readline) == '\0')) {
				  break;
			  }
			  index_readline++;
		  }
		  *(line+index_readline) = '\0';
		  if ((index_readline >= MAX_LINE_LENGTH) || (index_readline == 0)) {
			  break;
		  }

		if ((*line == ' ') && (*(line+1) == '=')) {
	        qualityList[listindex] = NULL;
			//qualityList[listindex] = calloc(1,sizeof(stLoaderComponentType));
			qualityList[listindex] = malloc(sizeof(stLoaderComponentType));
			memset(qualityList[listindex], 0x00, sizeof(stLoaderComponentType));
			index = 5;
			wordlength = 0;
			while((*(line + index) != ' ') && (*(line + index) != '\0')) {
				wordlength++;
				index++;
			}
			qualityList[listindex]->name = malloc(wordlength + 1);
			strncpy(qualityList[listindex]->name, (line + 5), wordlength);
			*(qualityList[listindex]->name + wordlength) = '\0';
	        // count rules
	        if (*(line + index) == '\n') {
				listindex++;
	        	continue;
	        }
	        index = index + 5;
	        tempindex = index;
	        qualityList[listindex]->name_specific_length = 0;
			while((*(line + tempindex) != ' ') && (*(line + tempindex) != '\0')) {
				while(*(line + tempindex) !=':') {
					tempindex++;
				}
				tempindex++;
				qualityList[listindex]->name_specific_length++;
			}
			//qualityList[listindex]->name_specific = calloc(qualityList[listindex]->name_specific_length, sizeof(char *));
			qualityList[listindex]->name_specific = malloc(qualityList[listindex]->name_specific_length*sizeof(char *));
			memset(qualityList[listindex]->name_specific,0x00, qualityList[listindex]->name_specific_length*sizeof(char *));
			roleindex = 0;
			while((*(line + index) != ' ') && (*(line + index) != '\n')) {
				wordlength = 0;
				tempindex = index;
				while(*(line + index) !=':') {
					wordlength++;
					index++;
				}
				tempindex = index - tempindex;
				qualityList[listindex]->name_specific[roleindex] = malloc(tempindex + 1);
				strncpy(qualityList[listindex]->name_specific[roleindex], (line+index-tempindex), tempindex);
				*(qualityList[listindex]->name_specific[roleindex]+tempindex) = '\0';
				roleindex++;
				index++;
			}
	        if (*(line + index) == '\0') {
				listindex++;
	        	continue;
	        }
	        // finally reached the quality levels
	        index = index + 5;
	        numlevels = 0;
			while(1) {
				if (*(line + index) != ' ') {
					numlevels = (numlevels * 10) + (*(line + index) -'0');
		        	index++;
				} else {
					qualityList[listindex]->nqualitylevels = numlevels;
					qualityList[listindex]->multiResourceLevel = malloc(sizeof(multiResourceDescriptor *) * qualityList[listindex]->nqualitylevels);
					for (i = 0; i<(int)qualityList[listindex]->nqualitylevels; i++) {
						qualityList[listindex]->multiResourceLevel[i] = malloc(sizeof(multiResourceDescriptor));
					}
		        	break;
				}
			}
			index++;
			for (i = 0; i<(int)qualityList[listindex]->nqualitylevels; i++) {
				qualityList[listindex]->multiResourceLevel[i]->CPUResourceRequested = 0;
				qualityList[listindex]->multiResourceLevel[i]->MemoryResourceRequested = 0;
				while(*(line + index) != ',') {
					qualityList[listindex]->multiResourceLevel[i]->CPUResourceRequested = (qualityList[listindex]->multiResourceLevel[i]->CPUResourceRequested * 10) + (*(line + index) - '0');
					index++;
				}
		        index++;
				while((*(line + index) != ' ') && (*(line + index) != '\n')) {
					qualityList[listindex]->multiResourceLevel[i]->MemoryResourceRequested = (qualityList[listindex]->multiResourceLevel[i]->MemoryResourceRequested * 10) + (*(line + index) - '0');
					index++;
				}
		        index++;
			}
			listindex++;
		}
	}

    if(line) {
    	free(line);
    	line = NULL;
    }
    free(libname);
    libname = NULL;
    fclose(omxregistryfp);
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
    return OMX_ErrorNone;
}
