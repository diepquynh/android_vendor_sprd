
#ifndef _MP3_API_H
#define	_MP3_API_H

//#include "t_types.h"

#ifdef   __cplusplus
extern   "C" 
{
#endif

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
	
typedef enum MP3_ARM_DEC_ERROR
{
	MP3_ARM_DEC_ERROR_NONE			  = 0x0000,	/* no error */
	MP3_ARM_DEC_ERROR_DECODING		  	  = 0x0001,	
	MP3_ARM_DEC_ERROR_INPUT_PARAM      	  = 0x0002,	
	MP3_ARM_DEC_ERROR_INPUT_BUFPTR	 	  = 0x0003,
	MP3_ARM_DEC_ERROR_OUTPUT_BUFPTR	  	  = 0x0004,
	MP3_ARM_DEC_ERROR_FRAMELEN		  	  = 0x0005,
	MP3_ARM_DEC_ERROR_NEXT_BEGIN	  	  = 0x0006,
	MP3_ARM_DEC_ERROR_BITRATE		  	  = 0x0007
}MP3_ARM_DEC_EC_E;
	


typedef struct _FrameDec  //Input param
{
	uint8_t * frame_buf_ptr;  // one audio frame buffer start address
	uint16_t frame_len;   // one audio frame length
	uint16_t next_begin;
	uint16_t bitrate;
}FRAME_DEC_T;

typedef struct _OutputFrame  //Output param
{
	uint16_t* pcm_data_l_ptr;  // left channel pcm data
	uint16_t* pcm_data_r_ptr;	  // right channel pcm data
	uint16_t pcm_bytes;  // frame sample counts
	uint16_t channel_num; // channel number	
}OUTPUT_FRAME_T;
	
int MP3_ARM_DEC_Construct(void **h_decoder_ptr);
int MP3_ARM_DEC_Deconstruct(void const **h_decoder_ptr);
void MP3_ARM_DEC_InitDecoder(void *);
void MP3_ARM_DEC_DecodeFrame( 
							 void *,
						FRAME_DEC_T *frame_dec_buf_ptr,  // [Input]
						 OUTPUT_FRAME_T *output_frame_ptr, // [Output]
						 uint32_t *decode_result	// [Output]
						 );

typedef int (*FT_MP3_ARM_DEC_Construct)(void **h_decoder_ptr);
typedef int (*FT_MP3_ARM_DEC_Deconstruct)(void const **h_decoder_ptr);
typedef void (*FT_MP3_ARM_DEC_InitDecoder)(void *);
typedef void (*FT_MP3_ARM_DEC_DecodeFrame)( 
							 void *,
						FRAME_DEC_T *frame_dec_buf_ptr,  // [Input]
						 OUTPUT_FRAME_T *output_frame_ptr, // [Output]
						 uint32_t *decode_result	// [Output]
						 );

#ifdef   __cplusplus
}
#endif

#endif  // _MP3_API_H

