/******************************************************************************
 ** File Name:      eng_audio_ext.h                                           *
 ** Author:                                                                   *
 ** DATE:           09/16/2015                                                *
 ** Copyright:      2015 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    This file defined audio application interface             *
 **                                                                           *
 **                                                                           *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 09/16/2015                      Create.                                   *
 ******************************************************************************/

#ifndef __ENG_AUDIO_EXT_H__
#define __ENG_AUDIO_EXT_H__

extern int adev_get_audiomodenum4eng(void);

extern void stringfile2nvstruct(char *filename, void *para_ptr, int lenbytes);

extern void nvstruct2stringfile(char *filename, void *para_ptr, int lenbytes);

#endif
