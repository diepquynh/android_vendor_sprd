/******************************************************************************
** File Name:      trans_samplerate44k.h                                            
** Author:         cherry.liu                                              
** DATE:           12/26/2012                                                
** Copyright:      2012 Spreatrum, Incoporated. All Rights Reserved.         
** Description:    This file defines the  module which transforms the 
                   samplerate to 44.1kHz                    
******************************************************************************

******************************************************************************
**                        Edit History                                       
**  -----------------------------------------------------------------------  
** DATE           NAME             DESCRIPTION                               
** 12/26/2012       cherry.liu     Create.                                   
******************************************************************************/
  
#ifndef _TRANS_SAMPLERATE44K_H_
#define _TRANS_SAMPLERATE44K_H_
/**---------------------------------------------------------------------------**
**                         Dependencies                                      **
**---------------------------------------------------------------------------**/
#include "sci_types.h"
/*
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>


#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <expat.h>

#include <tinyalsa/asoundlib.h>
#include <audio_utils/resampler.h>
#include "audio_pga.h"
#include "vb_effect_if.h"
#include "vb_pga.h"

#include "eng_audio.h"
#include "aud_proc.h"
//#include "vb_control_parameters.h"
//#include "string_exchange_bin.h"

//#include "dumpdata.h"
*/
#ifdef AUDIO_MUX_PCM
#include "audio_mux_pcm.h"
#endif

/**---------------------------------------------------------------------------**
**                        Debugging Flag                                     **
**---------------------------------------------------------------------------**/

/**---------------------------------------------------------------------------**
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------**/
#ifdef __cplusplus
extern   "C"
{
#endif
/**---------------------------------------------------------------------------**
 **                         MACRO Definations                                 **
 **---------------------------------------------------------------------------**/
#define PLAY44K_TRANS_MAX_LEN  3000

#define U2_TAPS83_DELAY_LEN    41
#define U2_TAPS195_DELAY_LEN   97
#define U3_TAPS195_DELAY_LEN   64
#define U3_TAPS21_DELAY_LEN    6
#define U4_TAPS163_DELAY_LEN   40
#define U49_TAPS273_DELAY_LEN  5


/**---------------------------------------------------------------------------**
 **                         Data Structures                                   **
 **---------------------------------------------------------------------------**/
typedef struct
{

    int16   *trans_u2_taps83_delay_buf_left;//delay len:41 words
    int16   *trans_u2_taps83_sig_buf_left;
    int16   *trans_u2_taps83_delay_buf_right;
    int16   *trans_u2_taps83_sig_buf_right;

    int16   *trans_u2_taps195_delay_buf_left;//delay len:97 words
    int16   *trans_u2_taps195_sig_buf_left;
    int16   *trans_u2_taps195_delay_buf_right;
    int16   *trans_u2_taps195_sig_buf_right;

    int16   *trans_u3_taps195_delay_buf_left;//delay len:64 words
    int16   *trans_u3_taps195_sig_buf_left;
    int16   *trans_u3_taps195_delay_buf_right;
    int16   *trans_u3_taps195_sig_buf_right;

    int16   *trans_u3_taps21_delay_buf_left;//delay len:6 words
    int16   *trans_u3_taps21_sig_buf_left;
    int16   *trans_u3_taps21_delay_buf_right;
    int16   *trans_u3_taps21_sig_buf_right;    

    int16   *trans_u4_taps163_delay_buf_left;//delay len:40 words
    int16   *trans_u4_taps163_sig_buf_left;
    int16   *trans_u4_taps163_delay_buf_right;
    int16   *trans_u4_taps163_sig_buf_right; 

    int16   *trans_u49_taps273_delay_buf_left;//delay len:5 words
    int16   *trans_u49_taps273_sig_buf_left;
    int16   *trans_u49_taps273_delay_buf_right;
    int16   *trans_u49_taps273_sig_buf_right; 
    uint32  trans_overleap_count ;

} TRANSAM44K_CONTEXT_T;	

/**---------------------------------------------------------------------------**
 **                         Global Variables                                  **
 **---------------------------------------------------------------------------**/
	
	
/**---------------------------------------------------------------------------**
 **                         Constant Variables                                **
 **---------------------------------------------------------------------------**/


/**---------------------------------------------------------------------------**
 **                          Function Declare                                 **
 **---------------------------------------------------------------------------**/
#if 0
/*****************************************************************************/
//  Description:    double the samplerate of input buffer 
//                  with 195 taps fir filter
//  Note:           return dest count value     
//****************************************************************************/  
uint32 TRANSSAMP_U2Taps195(
    int16* psSrcLeftData, // in ; delay+input  
    int16* psSrcRightData,// in ; delay+input  
    int16* psDestLeftData, // out ;  output  
    int16* psDestRightData,// out ;  output  
    int16  uiSrcCount     // in ; 
);

/*****************************************************************************/
//  Description:    double the samplerate of input buffer 
//                  with 83 taps fir filter
//  Note:           return dest count value     
//****************************************************************************/  
uint32 TRANSSAMP_U2Taps83(
    int16* psSrcLeftData, // in ; delay+input  
    int16* psSrcRightData,// in ; delay+input  
    int16* psDestLeftData, // out ;  output  
    int16* psDestRightData,// out ;  output  
    int16  uiSrcCount     // in ; 
);

/*****************************************************************************/
//  Description:    triple the samplerate of input buffer 
//                  with 195 taps fir filter
//  Note:           return dest count value     
//****************************************************************************/  
uint32 TRANSSAMP_U3Taps195(
    int16* psSrcLeftData, // in ; delay+input  
    int16* psSrcRightData,// in ; delay+input 
    int16* psDestLeftData, // out ; output 
    int16* psDestRightData,// out ; output
    int16  uiSrcCount     // in ; 
);

/*****************************************************************************/
//  Description:    triple the samplerate of input buffer 
//                  with 21 taps fir filter
//  Note:           return dest count value     
//****************************************************************************/  
uint32 TRANSSAMP_U3Taps21(
    int16* psSrcLeftData, // in ; delay+input  
    int16* psSrcRightData,// in ; delay+input 
    int16* psDestLeftData, // out ; output 
    int16* psDestRightData,// out ; output
    int16  uiSrcCount     // in ; 
);

/*****************************************************************************/
//  Description:    transform the samplerate of input buffer  by 4
//                  with 163 taps fir filter
//  Note:           return dest count value     
//****************************************************************************/  
uint32 TRANSSAMP_U4Taps163(
    int16* psSrcLeftData, // in ; delay+input  
    int16* psSrcRightData,// in ; delay+input 
    int16* psDestLeftData, // out ; output 
    int16* psDestRightData,// out ; output
    int16  uiSrcCount     // in ; 
);

/*****************************************************************************/
//  Description:    transform the samplerate of input buffer  by 49/dsN
//                  with 273 taps fir filter
//  Note:           return dest count value     
//****************************************************************************/  
uint32 TRANSSAMP_U49Taps273(
    int16* psSrcLeftData,   // in ; delay+input  
    int16* psSrcRightData,  // in ; delay+input 
    int16* psDestLeftData,  // out ; output 
    int16* psDestRightData, // out ; output
    int16  uiSrcCount,      // in ;
    uint32 dsN,             // in ; 
    uint32* overLeapCountPtr   // in and out
);

#endif

/*****************************************************************************/
//  Description:    MP344K_InitParam 
//                   
//  Note:           return -1 for error     
//****************************************************************************/ 
int16 MP344K_InitParam(
    TRANSAM44K_CONTEXT_T *ptTransamObj,
    uint32 curSamplerate,
    uint32  transDataMaxLen
);

/*****************************************************************************/
//  Description:    MP344K_DeInitParam 
//                   
//  Note:           return -1 for error     
//****************************************************************************/
int16 MP344K_DeInitParam(
    TRANSAM44K_CONTEXT_T *ptTransamObj
);

/*****************************************************************************/
//  Description:    transform the samplerate of input to 44100Hz 
//                   
//  Note:           return dest count value     
//****************************************************************************/ 
uint32 MP3_Play44k(//return dest count
    TRANSAM44K_CONTEXT_T *ptTransamObj,
    int16* psSrcLeftData,      // in  
    int16* psSrcRightData,     // in  
    int16  uiSrcCount,         // in   
    uint32 uiCurrentSamplerate,// in 
    int16* psDestLeftData,     // out    
    int16* psDestRightData    // out     
);


/**---------------------------------------------------------------------------**
 **                         Compiler Flag                                     **
 **---------------------------------------------------------------------------**/ 
#ifdef __cplusplus
}
#endif 

#endif 

//end of file








