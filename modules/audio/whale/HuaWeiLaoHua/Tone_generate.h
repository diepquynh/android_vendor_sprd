#ifndef tone_generate_h
#define tone_generate_h


#define int16 short
#define int32 int
#define uint16  unsigned short


typedef struct
{
    int16 freq;
    int16 volume;
    int16 len;
    int16* output_ptr;
} AUDIO_TONE_GENERATE_STRUCT_T;


void audio_tone_generator(AUDIO_TONE_GENERATE_STRUCT_T * tone_generate_struct);

#endif





