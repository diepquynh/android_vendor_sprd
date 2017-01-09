//--=========================================================================--
//  This implements some useful common functionalities 
//  for handling the register files used in Bellagio
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2015  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "../../../../config.h"
#include <st_static_component_loader.h>
#include <omx_vpudec_component.h>

#ifdef ANDROID
#include "android_support.h"
#endif

/** @brief The library entry point. It must have the same name for each
* library of the components loaded by the ST static component loader.
*
* This function fills the version, the component name and if existing also the roles
* and the specific names for each role. This base function is only an explanation.
* For each library it must be implemented, and it must fill data of any component
* in the library
*
* @param stComponents pointer to an array of components descriptors.If NULL, the
* function will return only the number of components contained in the library
*
* @return number of components contained in the library
*/

#define NUM_OF_COMPONENT 1


int omx_component_library_Setup(stLoaderComponentType **stComponents)
{
    OMX_U32 i;
    OMX_U32 index = 0;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n",__func__);

    if (stComponents == NULL) {
        DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);
        return NUM_OF_COMPONENT; // Return Number of Components - one for audio, two for video
    }



    /** component 1 - video decoder */
    stComponents[index]->componentVersion.s.nVersionMajor = 1;
    stComponents[index]->componentVersion.s.nVersionMinor = 1;
    stComponents[index]->componentVersion.s.nRevision = 1;
    stComponents[index]->componentVersion.s.nStep = 1;
    //stComponents[index]->name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
    stComponents[index]->name = malloc(OMX_MAX_STRINGNAME_SIZE);
    memset(stComponents[index]->name, 0x00, OMX_MAX_STRINGNAME_SIZE);
    if (stComponents[index]->name == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    strcpy(stComponents[index]->name, VIDEO_DEC_BASE_NAME);
    stComponents[index]->name_specific_length = VIDEO_DEC_NUM;
    stComponents[index]->constructor = omx_vpudec_component_Constructor;

    //stComponents[index]->name_specific = calloc(stComponents[index]->name_specific_length,sizeof(char *));
    //stComponents[index]->role_specific = calloc(stComponents[index]->name_specific_length,sizeof(char *));

    stComponents[index]->name_specific = malloc(stComponents[index]->name_specific_length*sizeof(char *));
    memset(stComponents[index]->name_specific, 0x00, stComponents[index]->name_specific_length*sizeof(char *));

    stComponents[index]->role_specific = malloc(stComponents[index]->name_specific_length*sizeof(char *));
    memset(stComponents[index]->role_specific, 0x00, stComponents[index]->name_specific_length*sizeof(char *));

    for(i=0; i<stComponents[index]->name_specific_length; i++) {
        //stComponents[index]->name_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
        stComponents[index]->name_specific[i] = malloc(OMX_MAX_STRINGNAME_SIZE);
        memset(stComponents[index]->name_specific[i], 0x00, OMX_MAX_STRINGNAME_SIZE);
        if (stComponents[index]->name_specific[i] == NULL) {
            return OMX_ErrorInsufficientResources;
        }
    }

    for(i=0; i<stComponents[index]->name_specific_length; i++) {
        //stComponents[index]->role_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
        stComponents[index]->role_specific[i] = malloc(OMX_MAX_STRINGNAME_SIZE);
        memset(stComponents[index]->role_specific[i], 0x00, OMX_MAX_STRINGNAME_SIZE);
        if (stComponents[index]->role_specific[i] == NULL) {
            return OMX_ErrorInsufficientResources;
        }
    }

    strcpy(stComponents[index]->name_specific[0], VIDEO_DEC_H264_NAME);
    strcpy(stComponents[index]->name_specific[1], VIDEO_DEC_MPEG2_NAME);
    /*
    strcpy(stComponents[index]->name_specific[2], VIDEO_DEC_MPEG4_NAME);
    strcpy(stComponents[index]->name_specific[3], VIDEO_DEC_RV_NAME);
    strcpy(stComponents[index]->name_specific[4], VIDEO_DEC_WMV_NAME);
    strcpy(stComponents[index]->name_specific[5], VIDEO_DEC_H263_NAME);
    strcpy(stComponents[index]->name_specific[6], VIDEO_DEC_MSMPEG_NAME);
    strcpy(stComponents[index]->name_specific[7], VIDEO_DEC_AVS_NAME);
    strcpy(stComponents[index]->name_specific[8], VIDEO_DEC_VP8_NAME);
    strcpy(stComponents[index]->name_specific[9], VIDEO_DEC_THO_NAME);
    strcpy(stComponents[index]->name_specific[10], VIDEO_DEC_JPG_NAME);
    strcpy(stComponents[index]->name_specific[11], VIDEO_DEC_VC1_NAME);
    strcpy(stComponents[index]->name_specific[12], VIDEO_DEC_HEVC_NAME);
    */

    strcpy(stComponents[index]->role_specific[0], VIDEO_DEC_H264_ROLE);
    strcpy(stComponents[index]->role_specific[1], VIDEO_DEC_MPEG2_ROLE);
    /*
    strcpy(stComponents[index]->role_specific[2], VIDEO_DEC_MPEG4_ROLE);
    strcpy(stComponents[index]->role_specific[3], VIDEO_DEC_RV_ROLE);
    strcpy(stComponents[index]->role_specific[4], VIDEO_DEC_WMV_ROLE);
    strcpy(stComponents[index]->role_specific[5], VIDEO_DEC_H263_ROLE);
    strcpy(stComponents[index]->role_specific[6], VIDEO_DEC_MSMPEG_ROLE);
    strcpy(stComponents[index]->role_specific[7], VIDEO_DEC_AVS_ROLE);
    strcpy(stComponents[index]->role_specific[8], VIDEO_DEC_VP8_ROLE);
    strcpy(stComponents[index]->role_specific[9], VIDEO_DEC_THO_ROLE);
    strcpy(stComponents[index]->role_specific[10], VIDEO_DEC_JPG_ROLE);
    strcpy(stComponents[index]->role_specific[11], VIDEO_DEC_VC1_ROLE);
    strcpy(stComponents[index]->role_specific[12], VIDEO_DEC_HEVC_ROLE);
    */

    index++;
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);

    return index;
#if 0
    /** component 2 - video encoder */
    stComponents[index]->componentVersion.s.nVersionMajor = 1;
    stComponents[index]->componentVersion.s.nVersionMinor = 1;
    stComponents[index]->componentVersion.s.nRevision = 1;
    stComponents[index]->componentVersion.s.nStep = 1;

    //stComponents[index]->name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
    stComponents[index]->name = malloc(OMX_MAX_STRINGNAME_SIZE);
    memset(stComponents[index]->name, 0x00, OMX_MAX_STRINGNAME_SIZE);
    if (stComponents[index]->name == NULL) {
        return OMX_ErrorInsufficientResources;
    }
    strcpy(stComponents[index]->name, VIDEO_ENC_BASE_NAME);
    stComponents[index]->name_specific_length = VIDEO_ENC_NUM;
    stComponents[index]->constructor = omx_vpuenc_component_Constructor;

    //stComponents[index]->name_specific = calloc(stComponents[index]->name_specific_length,sizeof(char *));
    //stComponents[index]->role_specific = calloc(stComponents[index]->name_specific_length,sizeof(char *));

    stComponents[index]->name_specific = malloc(stComponents[index]->name_specific_length*sizeof(char *));
    memset(stComponents[index]->name_specific, 0x00, stComponents[index]->name_specific_length*sizeof(char *));

    stComponents[index]->role_specific = malloc(stComponents[index]->name_specific_length*sizeof(char *));
    memset(stComponents[index]->role_specific, 0x00, stComponents[index]->name_specific_length*sizeof(char *));

    for(i=0; i<stComponents[index]->name_specific_length; i++) {
        //stComponents[index]->name_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
        stComponents[index]->name_specific[i] = malloc(OMX_MAX_STRINGNAME_SIZE);
        memset(stComponents[index]->name_specific[i], 0x00, OMX_MAX_STRINGNAME_SIZE);
        if (stComponents[index]->name_specific[i] == NULL) {
            return OMX_ErrorInsufficientResources;
        }
    }
    for(i=0; i<stComponents[index]->name_specific_length; i++) {
        //stComponents[index]->role_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
        stComponents[index]->role_specific[i] = malloc(OMX_MAX_STRINGNAME_SIZE);
        memset(stComponents[index]->role_specific[i], 0x00, OMX_MAX_STRINGNAME_SIZE);
        if (stComponents[index]->role_specific[i] == NULL) {
            return OMX_ErrorInsufficientResources;
        }
    }

    strcpy(stComponents[index]->name_specific[0], VIDEO_ENC_MPEG4_NAME );
    strcpy(stComponents[index]->name_specific[1], VIDEO_ENC_AVC_NAME);
    strcpy(stComponents[index]->name_specific[2], VIDEO_ENC_H263_NAME);
    strcpy(stComponents[index]->name_specific[3], VIDEO_ENC_JPG_NAME);

    strcpy(stComponents[index]->role_specific[0], VIDEO_ENC_MPEG4_ROLE);
    strcpy(stComponents[index]->role_specific[1], VIDEO_ENC_AVC_ROLE);
    strcpy(stComponents[index]->role_specific[2], VIDEO_ENC_H263_ROLE);
    strcpy(stComponents[index]->role_specific[3], VIDEO_ENC_JPG_ROLE);

    index++;

    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);

    return index;
#endif
}
