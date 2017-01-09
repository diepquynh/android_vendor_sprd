// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#ifndef _AUDIO_20121110_H__
#define _AUDIO_20121110_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_aud {
//-----------------------------------------------------------------------------

enum aud_outdev_e {
	AUD_OUTDEV_SPEAKER = 0,
	AUD_OUTDEV_EARPIECE,
	AUD_OUTDEV_HEADSET,
};

int audPlayerOpen( int outdev, int sampleRate, int stereo, int sync ); 
int audPlayerPlay( const unsigned char * data, int size );
int audPlayerStop( void );
int audPlayerClose( void );

enum aud_indev_e {
	AUD_INDEV_BUILTIN_MIC = 0,
	AUD_INDEV_HEADSET_MIC,
	AUD_INDEV_BACK_MIC,
};

int audRcrderOpen( int indev, int sampleRate );
int audRcrderRecord( unsigned char * data, int size );
int audRcrderStop( void );
int audRcrderClose( void );
unsigned char headsetPlugState( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _AUDIO_20121110_H__
