


/******************************************************************************
 ** File Name:      aud_record_nr.h                                                *
 ** Author:         Cherry.Liu                                             *
 ** DATE:           05/29/2015                                               *
 ** Copyright:      2015 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 05/29/2015       Cherry.Liu       Create.                                  *
 ******************************************************************************/
#ifndef audio_record_nr_h
#define audio_record_nr_h
typedef short int16;
void audio_record_nr_init(int16 *audio_recorder_nr_nv_ptr);/*init param length request 8 words*/
void audio_record_nr_set(int16 *audio_recorder_nr_nv_ptr);/*set dynamically param length request 8 words*/
void audio_record_nr(int16* data_buf); /*data length shouble be 480 words!!!*/
void audio_record_nr_sterno(int16* in_buf, int16* in_buf_r);/*data length shouble be 480 words!!!!*/
#endif
