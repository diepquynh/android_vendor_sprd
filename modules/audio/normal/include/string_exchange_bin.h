/**********************code following is for temp,need to modify,by wz*********************/


#ifndef __STRING_EXCHANGE_BIN_H__
#define __STRING_EXCHANGE_BIN_H__


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "eng_audio.h"
#include "vb_control_parameters.h"


aud_mode_t *adev_get_audiomode(void);

int adev_get_audiomodenum4eng(void);

void adev_free_audmode(void);


//AUDIO_TOTAL_T audio_para_total_hw[AUDIO_PARA_COUNT];//wz

void stringfile2nvstruct(char *filename, void *para_ptr, int lenbytes);

void  nvstruct2stringfile(char* filename,void *para_ptr, int lenbytes);


#endif

