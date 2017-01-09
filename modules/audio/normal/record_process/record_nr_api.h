#ifndef _AUD_RECORD_NR_H
#define _AUD_RECORD_NR_H

#include<stdio.h>
#include"aud_proc.h"
#include <utils/Log.h>
#include"audio_record_nr.h"
#include "eng_audio.h"

typedef void* record_nr_handle;

typedef int (*CallBack)(void *stream, void* buffer, size_t bytes);

record_nr_handle AudioRecordNr_Init(int16 *pNvPara, CallBack get_data_func , void *data_read_handle, int request_channel);
int AudioRecordNr_Proc(record_nr_handle nr_hande, void* buffer, size_t bytes);
void AudioRecordNr_Deinit(record_nr_handle nr_hande);

#endif
