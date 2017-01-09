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

#include "audioif.h"


#ifdef SPDIF_AUDIO
#ifdef WIN32
typedef int  (__stdcall *SetPin_type)(unsigned char pinNumber, unsigned short value);
static SetPin_type SetPin;

void InitSpdifAudio()
{
  HMODULE hDll;
    unsigned char i2c_UDAmode2[] = {0x34, 0x00, 0x28, 0x2E};
    unsigned char i2c_UDAI2SSpdif[] = {0x34, 0x50, 0x01, 0x51};

    /* copied the relevant code from C:\Program Files\NXP\I2C\C\CrdI2c.c
     */
    hDll = LoadLibrary(L"Scribo.dll");
    if (hDll == 0) {
        fprintf(stderr, "Could not open Scribo.dll\n");
        return ;
    }

    SetPin = (SetPin_type) GetProcAddress(hDll, "SetPin");
    if (SetPin == NULL) {
        FreeLibrary(hDll);
        return; // function not found in library
    }

    SetPin(4, 0x1); /* Weak pull up on PA4 powers up the UDA1355 */
    NXP_I2C_Write(sizeof(i2c_UDAmode2), i2c_UDAmode2);
    NXP_I2C_Write(sizeof(i2c_UDAI2SSpdif), i2c_UDAI2SSpdif);
}
#endif
#endif
#ifdef USB_AUDIO
#ifdef WIN32
typedef int  (__stdcall *SetPin_type)(unsigned char pinNumber, unsigned short value);
static SetPin_type SetPin;

void InitUsbAudio()
{
    HMODULE hDll;
    int ret;

    /* copied the relevant code from C:\Program Files\NXP\I2C\C\CrdI2c.c
     */
    hDll = LoadLibrary(L"Scribo.dll");
    if (hDll == 0) {
        fprintf(stderr, "Could not open Scribo.dll\n");
        return ;
    }

    SetPin = (SetPin_type) GetProcAddress(hDll, "SetPin");
    if (SetPin == NULL) {
        FreeLibrary(hDll);
        return; // function not found in library
    }

    ret = SetPin(4, 0x8000); /* Active low on PA4 switches off UDA1355. */
}
#else
void InitUsbAudio()
{
    int fd;
    fd = lxScriboGetFd();
    lxScriboSetPin(fd, 4, 0x8000);
}
#endif
#endif /* USB_AUDIO */


