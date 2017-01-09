#include <Tfa98xx.h>
#include <Tfa98xx_Registers.h>
#include <assert.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <NXP_I2C.h>
#ifndef WIN32
// need PIN access
#include <lxScribo.h>
#endif


//#define SPDIF_AUDIO
//#define USB_AUDIO

#ifdef SPDIF_AUDIO
typedef int  (__stdcall *SetPin_type)(unsigned char pinNumber, unsigned short value);
static SetPin_type SetPin;

void InitSpdifAudio();
#endif
#ifdef USB_AUDIO
#ifdef WIN32
typedef int  (__stdcall *SetPin_type)(unsigned char pinNumber, unsigned short value);
static SetPin_type SetPin;
#endif
void InitUsbAudio();
#endif /* USB_AUDIO */


