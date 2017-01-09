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

#include "common.h"

int tfa98xxRun_trace=1;
int cli_verbose=0;    /* verbose flag */
unsigned char  tfa98xxI2cSlave=TFA_I2CSLAVEBASE; // global for i2c access
int NXP_I2C_verbose=0;

char* patchFile = NULL;

/*
 *
 */
// status register errors to check for not 1
#define TFA98XX_STATUSREG_ERROR1_SET_MSK (  \
        TFA98XX_STATUSREG_OCDS  )
#define TFA98XX_STATUSREG_ERROR2_SET_MSK (  \
        TFA98XX_STATUSREG_ACS |   \
        TFA98XX_STATUSREG_WDS )


char* stateFlagsStr(int stateFlags)
{
    static char flags[10];

    flags[0] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_Activity)) ? 'A':'a';
    flags[1] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_S_Ctrl)) ? 'S':'s';
    flags[2] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_Muted)) ? 'M':'m';
    flags[3] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_X_Ctrl)) ? 'X':'x';
    flags[4] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_T_Ctrl)) ? 'T':'t';
    flags[5] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_NewModel)) ? 'L':'l';
    flags[6] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_VolumeRdy)) ? 'V':'v';
    flags[7] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_Damaged)) ? 'D':'d';
    flags[8] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_SignalClipping)) ? 'C':'c';

    flags[9] = 0;
    return flags;
}

void dump_state_info(Tfa98xx_StateInfo_t* pState)
{
  printf("state: flags %s, agcGain %2.1f\tlimGain %2.1f\tsMax %2.1f\tT %d\tX1 %2.1f\tX2 %2.1f\tRe %2.2f\tshortOnMips %d\n",
                stateFlagsStr(pState->statusFlag),
                pState->agcGain,
                pState->limGain,
                pState->sMax,
                pState->T,
                pState->X1,
                pState->X2,
                pState->Re,
                pState->shortOnMips);
}

void waitCalibration(Tfa98xx_handle_t handle, int *calibrateDone)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    int tries = 0;
    unsigned short mtp = 0;
#define WAIT_TRIES 1000

    err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
    if(err != Tfa98xx_Error_Ok) {
       TFA_LOGE("%s;%u", __func__, __LINE__);
    }

    /* in case of calibrate once wait for MTPEX */
    if ( mtp & TFA98XX_MTP_MTPOTC) {
        while (tries < WAIT_TRIES)
        {   // TODO optimise with wait estimation
            err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
            *calibrateDone = ( mtp & TFA98XX_MTP_MTPEX);    /* check MTP bit1 (MTPEX) */
            if(*calibrateDone != 0) {
                break;
            }
            usleep(TFA98XX_CALIBRATION_WAITING_TIME);
            tries++;
        }
        TFA_LOGD("waiting for TFA98XX_MTP, tries: %d", tries);
    } else /* poll xmem for calibrate always */
    {
        while (tries < WAIT_TRIES)
        {   // TODO optimise with wait estimation
            err = Tfa98xx_DspReadMem(handle, 231, 1, calibrateDone);
            if(*calibrateDone != 0) {
                break;
            }
            usleep(TFA98XX_CALIBRATION_WAITING_TIME);
            tries++;
        }
        if(tries==WAIT_TRIES)
            TFA_LOGE("calibrateDone 231 timedout\n");
    }

}

void setPatch(const char* fileName)
{
  patchFile = fileName;
}

void setConfig(Tfa98xx_handle_t handle, const char* fileName)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    Tfa98xx_Config_t config;
    int ret = 0;
    FILE* f = NULL;

    TFA_LOGD("using config %s\n", fileName);

    f=fopen(fileName, "rb");
    if (NULL == f) {
        TFA_LOGE("%s:%u:Not exist %s\n", __func__, __LINE__, fileName);
        return;
    }
    ret = fread(config, 1, sizeof(Tfa98xx_Config_t), f);
    if(ret != sizeof(Tfa98xx_Config_t)) {
         TFA_LOGE("fread for '%s' error(%d)", fileName, ret);
         goto EXIT;
    }
    err = Tfa98xx_DspWriteConfig(handle, sizeof(Tfa98xx_Config_t), config);
    if(err != Tfa98xx_Error_Ok) {
         TFA_LOGE("Tfa98xx_DspWriteConfig is error");
    }
EXIT:
    fclose(f);
}

void setConfigStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName)
{
    Tfa98xx_Error_t err;
    Tfa98xx_Config_t config;
    int ret;
    FILE* f;

    printf("using config %s\n", fileName);

    f=fopen(fileName, "rb");
    assert(f!=NULL);
    ret = fread(config, 1, sizeof(Tfa98xx_Config_t), f);
    assert(ret == sizeof(Tfa98xx_Config_t));
    err = Tfa98xx_DspWriteConfigMultiple(handle_cnt, handles, sizeof(Tfa98xx_Config_t), config);
    assert(err == Tfa98xx_Error_Ok);
    fclose(f);
}

int tfaRun_CheckEvents(unsigned short regval) {
    int severity=0;

    //see if following alarms are set
    if ( regval & TFA98XX_STATUSREG_ERROR1_SET_MSK ) //
        severity = 1;
    // next will overwrite if set
    if ( regval & TFA98XX_STATUSREG_ERROR2_SET_MSK )
        severity = 2;
    // check error conditions

    return severity;
}


Tfa98xx_Error_t tfaRun_PowerCycleCF(Tfa98xx_handle_t handle)
{
    Tfa98xx_Error_t err;

    err = Tfa98xx_Powerdown(handle, 1);
    assert(err == Tfa98xx_Error_Ok);
    err = Tfa98xx_Powerdown(handle, 0);
    assert(err == Tfa98xx_Error_Ok);

    return err;
}

float tCoefFromSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes)
{
    int iCoef;

    // tCoef(A) is the last parameter of the speaker
    iCoef = (speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-3]<<16) + (speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-2]<<8) + speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-1];

     return (float)iCoef/(1<<23);
}

void tCoefToSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, float tCoef)
{
    int iCoef;

    iCoef =(int)(tCoef*(1<<23));

    speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-3] = (iCoef>>16)&0xFF;
    speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-2] = (iCoef>>8)&0xFF;
    speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-1] = (iCoef)&0xFF;
}

void muteAmplifier(Tfa98xx_handle_t handle)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    unsigned short status = 0;
    int tries = 0;

    /* signal the TFA98xx to mute plop free and turn off the amplifier */
    err = Tfa98xx_SetMute(handle, Tfa98xx_Mute_Amplifier);
    assert(err == Tfa98xx_Error_Ok);

    /* now wait for the amplifier to turn off */
#if 0
    err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
    assert(err == Tfa98xx_Error_Ok);
    while ( (status & TFA98XX_STATUSREG_SWS_MSK) == TFA98XX_STATUSREG_SWS_MSK)
    {
        err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
        assert(err == Tfa98xx_Error_Ok);
    }
#else
    do {
        err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
        if (err != Tfa98xx_Error_Ok) {
            ALOGE("FUNC: %s, LINE: %s err = 0x%04x", __func__, __LINE__, err);
            break;
        }
        if ( (status & TFA98XX_STATUSREG_SWS_MSK) == TFA98XX_STATUSREG_SWS_MSK) {
            tries++;
            usleep(TFA98XX_MUTE_AMP_WAITING_TIME);
        } else {
            break;
        }
    } while (tries < TFA98XX_TRY_TIMES);
#endif
    ALOGD("%s: %u:err = 0x%08x status = 0x%04x tries = %d\n", __func__, __LINE__, err, status, tries);
}

void loadSpeakerFile(const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes)
{
    int ret = 0;
    FILE* f = NULL;

    ALOGD("using speaker %s\n", fileName);

    f = fopen(fileName, "rb");
    if (NULL == f) {
        ALOGE("%s: %u: Not exist %s\n", __func__, __LINE__, fileName);
        return;
    }

    ret = fread(speakerBytes, 1, sizeof(Tfa98xx_SpeakerParameters_t), f);
    if (ret != sizeof(Tfa98xx_SpeakerParameters_t)) {
        ALOGE("fread for '%s' error(%d)", fileName, ret);
    }
    fclose(f);
}

int dspSupporttCoef(Tfa98xx_handle_t handle)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    int bSupporttCoef = 0;

    err = Tfa98xx_DspSupporttCoef(handle, &bSupporttCoef);
    if(err != Tfa98xx_Error_Ok) {
        ALOGE("FUNC: %s, LINE: %s", __func__, __LINE__);
    }

    return bSupporttCoef;
}


/* load a speaker model from a file, as generated by the GUI, or saved from a previous execution */
void setSpeaker(Tfa98xx_handle_t handle, const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    int ret = 0;
    FILE* f = NULL;

    printf("using speaker %s\n", fileName);

    f=fopen(fileName, "rb");
    if (NULL == f) {
        printf("%s: %u: Not exist %s\n", __func__, __LINE__, fileName);
        return;
    }
    ret = fread(speakerBytes, 1, sizeof(Tfa98xx_SpeakerParameters_t), f);
    if (ret != sizeof(Tfa98xx_SpeakerParameters_t)) {
        ALOGE("fread for '%s' error(%d)", fileName, ret);
        goto EXIT;
    }
    err = Tfa98xx_DspWriteSpeakerParameters(handle, sizeof(Tfa98xx_SpeakerParameters_t), speakerBytes);
    if(err != Tfa98xx_Error_Ok) {
        TFA_LOGE("Tfa98xx_DspWriteSpeakerParameters error");
    }
EXIT:
    fclose(f);
}

/* save the current speaker model to a file, for future use */
void saveSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, const char* fileName)
{
    int ret = 0;
    FILE* f = NULL;

    f=fopen(fileName, "wb");
    if (NULL == f) {
        TFA_LOGE("%s: %u: Not exist %s\n", __func__, __LINE__, fileName);
        return;
    }
    ret = fwrite(speakerBytes, 1, sizeof(Tfa98xx_SpeakerParameters_t), f);
    if(ret != sizeof(Tfa98xx_SpeakerParameters_t)) {
        TFA_LOGE("fwrite for '%s' error(%d)", fileName, ret);
    }
    fclose(f);
}
/* load a preset from a file, as generated by the GUI, can be done at runtime */
void setPreset(Tfa98xx_handle_t handle, const char* fileName)
{
    int ret = 0;
    int presetSize = 0;
    unsigned char* buffer = NULL;
    FILE* f = NULL;
    struct stat st;
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;

    TFA_LOGD("using preset %s\n", fileName);

    f=fopen(fileName, "rb");
    if (NULL == f) {
        TFA_LOGE("Not exist %s\n", fileName);
        return;
    }
    ret = fstat(_fileno(f), &st);
    if(ret != 0) {
        TFA_LOGE("fstat for '%s' error(%d)", fileName, ret);
        goto EXIT;
    }
    presetSize = st.st_size;
    if(presetSize != TFA98XX_PRESET_LENGTH) {
        TFA_LOGE("presetSize(%d) != TFA98XX_PRESET_LENGTH", presetSize);
        goto EXIT;
    }
    buffer = (unsigned char*)malloc(presetSize);
    if(buffer == NULL) {
        TFA_LOGE("malloc failed!");
        goto EXIT;
    }
    ret = fread(buffer, 1, presetSize, f);
    if(ret != presetSize) {
        TFA_LOGE("fread for '%s' error(%d)", fileName, ret);
        goto EXIT;
    }
    err = Tfa98xx_DspWritePreset(handle, TFA98XX_PRESET_LENGTH, buffer);
    if(err != Tfa98xx_Error_Ok) {
        TFA_LOGE("Tfa98xx_DspWritePreset error");
		TFA_LOGE("Tfa98xx_DspWritePreset error = %d", err);
    }
EXIT:
    if (f) {
        fclose(f);
    }
    if (buffer) {
        free(buffer);
    }
}


/* load a preset from a file, as generated by the GUI, can be done at runtime */
void setPresetMono(Tfa98xx_handle_t handle, const char* fileName)
{
    int ret;
    int presetSize;
    unsigned char* buffer;
    FILE* f;
    struct stat st;
    Tfa98xx_Error_t err;

    printf("using preset %s\n", fileName);

    f=fopen(fileName, "rb");
    assert(f!=NULL);
    ret = fstat(_fileno(f), &st);
    assert(ret == 0);
    presetSize = st.st_size;
    assert(presetSize == TFA98XX_PRESET_LENGTH);
    buffer = (unsigned char*)malloc(presetSize);
    assert(buffer != NULL);
    ret = fread(buffer, 1, presetSize, f);
    assert(ret == presetSize);
    err = Tfa98xx_DspWritePreset(handle, TFA98XX_PRESET_LENGTH, buffer);
    assert(err == Tfa98xx_Error_Ok);
    fclose(f);
    free(buffer);
}

/* load a preset from a file, as generated by the GUI, can be done at runtime */
void setPresetStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName)
{
    int ret;
    int presetSize;
    unsigned char* buffer;
    FILE* f;
    struct stat st;
    Tfa98xx_Error_t err;

    printf("using preset %s\n", fileName);

    f=fopen(fileName, "rb");
    assert(f!=NULL);
    ret = fstat(_fileno(f), &st);
    assert(ret == 0);
    presetSize = st.st_size;
    assert(presetSize == TFA98XX_PRESET_LENGTH);
    buffer = (unsigned char*)malloc(presetSize);
    assert(buffer != NULL);
    ret = fread(buffer, 1, presetSize, f);
    assert(ret == presetSize);
    err = Tfa98xx_DspWritePresetMultiple(handle_cnt, handles, TFA98XX_PRESET_LENGTH, buffer);
    assert(err == Tfa98xx_Error_Ok);
    fclose(f);
    free(buffer);
}



/* load a set of EQ settings from a file, as generated by the GUI, can be done at runtime */
void setEQ(Tfa98xx_handle_t handle, const char* fileName)
{
    int ret = 0;
    FILE* f = NULL;
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    int ind = 0; /* biquad index */
    float b0, b1, b2, a1, a2; /* the coefficients */
    int line = 1;
    char buffer[256];

    TFA_LOGD("using EQ %s\n", fileName);

    f=fopen(fileName, "rb");
    if (NULL == f) {
        TFA_LOGE("Not exist %s\n", fileName);
        return;
    }

    while (!feof(f))
    {
        if (NULL == fgets(buffer, sizeof(buffer)-1, f) )
        {
            break;
        }
        ret = sscanf(buffer, "%d %f %f %f %f %f", &ind, &b0, &b1, &b2, &a1, &a2);
        if (ret == 6)
        {
            if ((b0 != 1) || (b1 != 0) || (b2 != 0) || (a1 != 0) || (a2 != 0)) {
                err = Tfa98xx_DspBiquad_SetCoeff(handle, ind, b0, b1, b2, a1, a2);
                if(err != Tfa98xx_Error_Ok) {
                    TFA_LOGE("Tfa98xx_DspBiquad_SetCoeff error");
                    break;
                }
                TFA_LOGD("Loaded biquad %d\n", ind);
            } else {
                 err = Tfa98xx_DspBiquad_Disable(handle, ind);
                 if(err != Tfa98xx_Error_Ok) {
                     TFA_LOGE("Tfa98xx_DspBiquad_Disable error");
                     break;
                 }
                 TFA_LOGD("Disabled biquad %d\n", ind);
            }
        }
        else {
            TFA_LOGD("error parsing file, line %d\n", line);
            //break;
        }
        line++;
    }
    fclose(f);
}

/* load a DSP ROM code patch from file */
void dspPatch(Tfa98xx_handle_t handle, const char* fileName)
{
    int ret = 0;
    int fileSize = 0;
    unsigned char* buffer = NULL;
    FILE* f = NULL;
    struct stat st;
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;

    TFA_LOGD("Loading patch file %s\n", fileName);

    f=fopen(fileName, "rb");
    if ( NULL == f ) {
        TFA_LOGE("Error loading patch file %s\n", fileName);
        return;
    }
    ret = fstat(_fileno(f), &st);
    if(ret != 0) {
        TFA_LOGE("fstat for '%s' error(%d)", fileName, ret);
        goto EXIT;
    }
    fileSize = st.st_size;
    buffer = malloc(fileSize);
    if(NULL == buffer) {
        TFA_LOGE("malloc failed!");
        goto EXIT;
    }
    ret = fread(buffer, 1, fileSize, f);
    if(ret != fileSize) {
        TFA_LOGE("fread for '%s' error(%d)", fileName, ret);
        goto EXIT;
    }
    err = Tfa98xx_DspPatch(handle, fileSize, buffer);
    if(err != Tfa98xx_Error_Ok) {
        TFA_LOGE("Tfa98xx_DspPatch error %d", err);
    }
EXIT:
    if (f) {
        fclose(f);
    }

    if (buffer) {
        free(buffer);
    }
}

/* load a set of EQ settings from a file, as generated by the GUI, can be done at runtime */
void setEQStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName)
{
    int ret;
    FILE* f;
    Tfa98xx_Error_t err;
    int ind; /* biquad index */
    float b0, b1, b2, a1, a2; /* the coefficients */
    int line = 1;
    char buffer[256];

    printf("using EQ %s\n", fileName);

    f=fopen(fileName, "rb");
    assert(f!=NULL);

    while (!feof(f))
    {
        if (NULL == fgets(buffer, sizeof(buffer)-1, f) )
        {
            break;
        }
        ret = sscanf(buffer, "%d %f %f %f %f %f", &ind, &b0, &b1, &b2, &a1, &a2);
        if (ret == 6)
        {
            if ((b0 != 1) || (b1 != 0) || (b2 != 0) || (a1 != 0) || (a2 != 0)) {
                err = Tfa98xx_DspBiquad_SetCoeffMultiple(handle_cnt, handles, ind, b0, b1, b2, a1, a2);
                assert(err == Tfa98xx_Error_Ok);
                printf("Loaded biquad %d\n", ind);
      } else {
        err = Tfa98xx_DspBiquad_DisableMultiple(handle_cnt, handles, ind);
                assert(err == Tfa98xx_Error_Ok);
                printf("Disabled biquad %d\n", ind);
            }
        }
        else {
            printf("error parsing file, line %d\n", line);
            //break;
        }
        line++;
    }
    fclose(f);
}


/* load a speaker model from a file, as generated by the GUI, or saved from a previous execution */
void setSpeakerStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes)
{
    Tfa98xx_Error_t err;
    int ret;
    FILE* f;

    printf("using speaker %s\n", fileName);

    f=fopen(fileName, "rb");
    assert(f!=NULL);
    ret = fread(speakerBytes, 1, sizeof(Tfa98xx_SpeakerParameters_t), f);
    assert(ret == sizeof(Tfa98xx_SpeakerParameters_t));
    err = Tfa98xx_DspWriteSpeakerParametersMultiple(handle_cnt, handles, sizeof(Tfa98xx_SpeakerParameters_t), speakerBytes);
    assert(err == Tfa98xx_Error_Ok);
    fclose(f);
}


void resetMtpEx(Tfa98xx_handle_t handle)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    unsigned short mtp = 0;
    unsigned short status = 0;
    int tries = 0;

    /* reset MTPEX bit because calibration happened with wrong tCoefA */
    err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
    assert(err == Tfa98xx_Error_Ok);
    /* all settings loaded, signal the DSP to start calibration, only needed once after cold boot */

    /* reset MTPEX bit if needed */
    if ( (mtp & TFA98XX_MTP_MTPOTC) && (mtp & TFA98XX_MTP_MTPEX))
    {
        err = Tfa98xx_WriteRegister16(handle, 0x0B, 0x5A); /* unlock key2 */
        assert(err == Tfa98xx_Error_Ok);

        err = Tfa98xx_WriteRegister16(handle, TFA98XX_MTP, 1); /* MTPOTC=1, MTPEX=0 */
        assert(err == Tfa98xx_Error_Ok);
        err = Tfa98xx_WriteRegister16(handle, 0x62, 1<<11); /* CIMTP=1 */
        assert(err == Tfa98xx_Error_Ok);
    }

    do
    {
        err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
        if((status & TFA98XX_STATUSREG_MTPB_MSK) == TFA98XX_STATUSREG_MTPB_MSK) {
            tries++;
            Sleep(10);
        } else {
            break;
        }
    } while (tries < TFA98XX_TRY_TIMES);
    if ( (status & TFA98XX_STATUSREG_MTPB_MSK) != 0) {
        ALOGE("Waiting for TFA98XX_STATUSREG_MTPB timeouts!");
    }
}

void coldStartup(Tfa98xx_handle_t handle, const int rate, const char* fileName)
{
   Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
   unsigned short status = -1;
   int ready = 0;
   int tries = 0;

   /* load the optimal TFA98xx in HW settings */
   err = Tfa98xx_Init(handle);
   if(err != Tfa98xx_Error_Ok) {
       TFA_LOGE("Tfa98xx_Init error(%d)", err);
   }

   /* Set sample rate to example*/
   err = Tfa98xx_SetSampleRate(handle, rate);
   if(err != Tfa98xx_Error_Ok) {
       TFA_LOGE("Tfa98xx_SetSampleRate error(%d)", err);
   }

    if (TFA98XX_NOMINAL_IMPEDANCE != 8) {
        err = Tfa98xx_ReadRegister16(handle, TFA98XX_I2S_SEL_REG, &status);
        assert(err == Tfa98xx_Error_Ok);
        status &= ~(TFA98XX_I2S_SEL_REG_SPKR_MSK);
        switch (TFA98XX_NOMINAL_IMPEDANCE) {
            case 6:
                status |= 0x400; /* Set impedance as 6ohm */
                break;
            case 4:
                status |= 0x200; /* Set impedance as 4ohm */
                break;
        }
        Tfa98xx_WriteRegister16(handle, TFA98XX_I2S_SEL_REG, status);
    }
#if 0   
    /*add for hw ask for limit current to 2.4A for FPC not support 3.8A*/
    /*DCMCC(register 07 bit5~bit3)
                bit5~bit3:
                         000  0.5A
                         001  1.0A
                         010  1.4A
                         011  1.9A
                         100  2.4A
                         101  2.9A
                         110  3.3A
                         111  3.8A(NXP default)
        */
    unsigned short current_24a;
    err = Tfa98xx_ReadRegister16(handle,TFA98XX_DCDCBOOST,&current_24a);
    TFA_LOGD("current_24a =%x",current_24a);
    current_24a |= (TFA98XX_DCDCBOOST_DCMCC & 0x27);
    TFA_LOGD("current_24a =%x",current_24a);
    err = Tfa98xx_WriteRegister16( handle, TFA98XX_DCDCBOOST, current_24a);
   /*add end */
#endif

   /* Power On the device by setting bit 0 to 0 of register 9*/
   err = Tfa98xx_Powerdown(handle, 0);
   if(err != Tfa98xx_Error_Ok) {
       TFA_LOGE("Tfa98xx_Powerdown to 0 error(%d)", err);
   }

   TFA_LOGD("Waiting for IC to start up\n");
   
    /*  powered on
     *    - now it is allowed to access DSP specifics
     *    - stall DSP by setting reset
     * */
    err = Tfa98xx_DspReset(handle, 1);
    if(err != Tfa98xx_Error_Ok) {
        TFA_LOGD("Tfa98xx_DspReset to 1 error(%d)", err);
    }

   status = -1;

   /* Check the PLL is powered up from status register 0*/
#if 0
   err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
   assert(err == Tfa98xx_Error_Ok);
   while ( (status & TFA98XX_STATUSREG_AREFS_MSK) == 0)
   {
      /* not ok yet */
      err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
      assert(err == Tfa98xx_Error_Ok);
   }
#else
   while (tries < TFA98XX_TRY_TIMES) {
      /* not ok yet */
      err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
      if((status & TFA98XX_STATUSREG_AREFS_MSK) == 0) {
          tries++;
          usleep(TFA98XX_COLD_STARTUP_WAITING_TIME);
      } else {
          break;
      }
   }
#endif
   TFA_LOGD("status: 0x%04x, tries: %d", status, tries);

    /*  wait until the DSP subsystem hardware is ready
     *    note that the DSP CPU is not running yet (RST=1)
     * */
     tries = 0;
     while (tries < TFA98XX_TRY_TIMES)
     {
          /* are we ready? */
          err = Tfa98xx_DspSystemStable(handle, &ready);
          if(ready == 0) {
              tries++;
              usleep(TFA98XX_COLD_STARTUP_WAITING_TIME);
          } else {
              break;
          }
     }
     TFA_LOGD("ready: 0x%04x, tries: %d, err: %d", ready, tries, err);

     /* Load cold-boot patch for the first time to force cold start-up.
      *  use the patchload only to write the internal register
      * */
     dspPatch(handle, fileName);

     err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
     if(err != Tfa98xx_Error_Ok) {
         TFA_LOGE("Tfa98xx_ReadRegister16 TFA98XX_STATUSREG error(%d)", err);
     }

     if(!(status & TFA98XX_STATUSREG_ACS_MSK)) {/* ensure cold booted */
         TFA_LOGE("not cold booted yet");
     }

   /* cold boot, need to load all parameters and patches */
   if ( patchFile ) {
       /* patch the ROM code */
       dspPatch(handle, patchFile);
   }
   else {
       TFA_LOGD("No patchfile.\n");
       /* in this case start the DSP */
       err = Tfa98xx_DspReset(handle, 0);
       if(err != Tfa98xx_Error_Ok) {
           TFA_LOGE("Tfa98xx_DspReset to 0 error(%d)", err);
       }
   }

}


void statusCheck(Tfa98xx_handle_t handle)
{
    Tfa98xx_Error_t err;
    unsigned short status;

   /* Check status from register 0*/
    err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
   if (status & TFA98XX_STATUSREG_WDS_MSK)
   {
      printf("DSP watchDog triggerd");
      return;
   }
}

void setOtc(Tfa98xx_handle_t handle, int otcOn)
{
    Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
    unsigned short mtp = 0;
    unsigned short status = 0;
    int mtpChanged = 0;
    int tries = 0;

    err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
    assert(err == Tfa98xx_Error_Ok);

    assert((otcOn == 0) || (otcOn == 1) );

    /* set reset MTPEX bit if needed */
    if ( (mtp & TFA98XX_MTP_MTPOTC) != otcOn)
    {
        /* need to change the OTC bit, set MTPEX=0 in any case */
        err = Tfa98xx_WriteRegister16(handle, 0x0B, 0x5A); /* unlock key2 */
        assert(err == Tfa98xx_Error_Ok);

        err = Tfa98xx_WriteRegister16(handle, TFA98XX_MTP, (unsigned short)otcOn); /* MTPOTC=otcOn, MTPEX=0 */
        assert(err == Tfa98xx_Error_Ok);
        err = Tfa98xx_WriteRegister16(handle, 0x62, 1<<11); /* CIMTP=1 */
        assert(err == Tfa98xx_Error_Ok);

        mtpChanged =1;

    }
    //Sleep(13*16); /* need to wait until all parameters are copied into MTP */
    do
    {
        err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
        if((status & TFA98XX_STATUSREG_MTPB_MSK) == TFA98XX_STATUSREG_MTPB_MSK) {
            tries++;
            Sleep(10);
        } else {
            break;
        }
    } while (tries < TFA98XX_TRY_TIMES);
    if( (status & TFA98XX_STATUSREG_MTPB_MSK) != 0) {
        TFA_LOGE("status: 0x%x, tries: %d", status, tries);
    }
}

int checkMTPEX(Tfa98xx_handle_t handle)
{
    unsigned short mtp;
    Tfa98xx_Error_t err;
    err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
    assert(err == Tfa98xx_Error_Ok);

    if ( mtp & (1<<1))  /* check MTP bit1 (MTPEX) */
        return 1;                   /* MTPEX is 1, calibration is done */
    else
        return 0;                   /* MTPEX is 0, calibration is not done yet */
}

#if 0
int GetF0(Tfa98xx_handle_t handle, int *f0, int *fInit)
{
   Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
   unsigned char bytes[3 * 141];
   int24 data[141];
   int i = 0;
   SPKRBST_SpkrModel_t record;

   if ((NULL == f0) || (NULL == fInit)) {
       error = Tfa98xx_Error_Bad_Parameter;
       goto EXIT;
   }

   error = Tfa98xx_DspGetParam(handle, 1,
                                PARAM_GET_LSMODELW, 423, bytes);
   assert(error == Tfa98xx_Error_Ok);

   Tfa98xx_ConvertBytes2Data(sizeof(bytes), bytes, data);

   for (i = 0; i < 128; i++)
   {
      record.pFIR[i] = (double)data[i] / (1 << 22);
   }

   record.Shift_FIR = data[i++];   ///< Exponent of HX data
   record.leakageFactor = (float)data[i++] / (1 << (23));  ///< Excursion model integration leakage
   record.ReCorrection = (float)data[i++] / (1 << (23));   ///< Correction factor for Re
   record.xInitMargin = (float)data[i++] / (1 << (23 - 2));        ///< (can change) Margin on excursion model during startup
   record.xDamageMargin = (float)data[i++] / (1 << (23 - 2));      ///< Margin on excursion modelwhen damage has been detected
   record.xMargin = (float)data[i++] / (1 << (23 - 2));    ///< Margin on excursion model activated when LookaHead is 0
   record.Bl = (float)data[i++] / (1 << (23 - 2)); ///< Loudspeaker force factor
   record.fRes = data[i++];        ///< (can change) Estimated Speaker Resonance Compensation Filter cutoff frequency
   record.fResInit = data[i++];    ///< Initial Speaker Resonance Compensation Filter cutoff frequency
   record.Qt = (float)data[i++] / (1 << (23 - 6)); ///< Speaker Resonance Compensation Filter Q-factor
   record.xMax = (float)data[i++] / (1 << (23 - 7));       ///< Maximum excursion of the speaker membrane
   record.tMax = (float)data[i++] / (1 << (23 - 9));       ///< Maximum Temperature of the speaker coil
   record.tCoefA = (float)data[i++] / (1 << 23);   ///< (can change) Temperature coefficient

   *f0 = record.fRes;
   *fInit = record.fResInit;

EXIT:
   return error;
}
#endif