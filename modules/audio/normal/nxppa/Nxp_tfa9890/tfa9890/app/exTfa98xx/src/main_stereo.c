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

#include "common.h"
#include "audioif.h"

#define I2C_ADDRESS  0x68
#define SAMPLE_RATE 48000

#ifdef WIN32
// cwd = dir where vcxproj is
#define LOCATION_FILES "../../../../settings/"
#else
// cwd = linux dir
#define LOCATION_FILES "../../../settings/"
#endif


/* the base speaker file, containing tCoef */
/* Dumbo speaker*/
#define SPEAKER_FILENAME "KS_13X18_DUMBO.speaker"
#define PRESET_FILENAME "Example_0_0.KS_13X18_DUMBO.preset"
#define EQ_FILENAME "Example_0_0.KS_13X18_DUMBO.eq"
/* Sambo speaker
#define SPEAKER_FILENAME "KS_Sambo_13X18_Seltech-90DemoBox-V2R1.speaker"
#define PRESET_FILENAME "HQ90_0_0_KS_Sambo_13X18_Seltech-90DemoBox_V2R1.preset"
#define EQ_FILENAME "HQ90_0_0_KS_Sambo_13X18_Seltech-90DemoBox_V2R1.eq"*/
#define CONFIG_FILENAME "TFA9890_N1B12_N1C3_v2.config"

#define PATCH_FILENAME "TFA9890_N1C3_2_1_1.patch"
// nor B12 #define PATCH_FILENAME "TFA9890_N1B12_3_2_3.patch"

extern regdef_t regdefs[];
extern unsigned char  tfa98xxI2cSlave; // global for i2c access


/* *INDENT-OFF* */
regdef_t regdefs[] = {
        { 0x00, 0x081d, 0xfeff, "statusreg"}, //ignore MTP busy bit
        { 0x01, 0x0, 0x0, "batteryvoltage"},
        { 0x02, 0x0, 0x0, "temperature"},
        { 0x03, 0x0012, 0xffff, "revisionnumber"},
        { 0x04, 0x888b, 0xffff, "i2sreg"},
        { 0x05, 0x13aa, 0xffff, "bat_prot"},
        { 0x06, 0x001f, 0xffff, "audio_ctr"},
        { 0x07, 0x0fe6, 0xffff, "dcdcboost"},
        { 0x08, 0x0800, 0x3fff, "spkr_calibration"}, //NOTE: this is a software fix to 0xcoo
        { 0x09, 0x041d, 0xffff, "sys_ctrl"},
        { 0x0a, 0x3ec3, 0x7fff, "i2s_sel_reg"},
        { 0x40, 0x0, 0x00ff, "hide_unhide_key"},
        { 0x41, 0x0, 0x0, "pwm_control"},
        { 0x46, 0x0, 0x0, "currentsense1"},
        { 0x47, 0x0, 0x0, "currentsense2"},
        { 0x48, 0x0, 0x0, "currentsense3"},
        { 0x49, 0x0, 0x0, "currentsense4"},
        { 0x4c, 0x0, 0xffff, "abisttest"},
        { 0x62, 0x0, 0, "mtp_copy"},
        { 0x70, 0x0, 0xffff, "cf_controls"},
        { 0x71, 0x0, 0, "cf_mad"},
        { 0x72, 0x0, 0, "cf_mem"},
        { 0x73, 0x00ff, 0xffff, "cf_status"},
        { 0x80, 0x0, 0, "mtp"},
        { 0x83, 0x0, 0, "mtp_re0"},
        { 0xff, 0,0, NULL}
};
/* *INDENT-ON* */


int main(/* int argc, char* argv[] */)
{
	Tfa98xx_Error_t err;
	Tfa98xx_handle_t handles[3];
	unsigned char h;
	Tfa98xx_SpeakerParameters_t loadedSpeaker;
	Tfa98xx_StateInfo_t stateInfo;
   unsigned short status = 0;
	float re25;
   int calibrateDone = 0;
	int i;

#ifdef 	PATCH_FILENAME
  setPatch(LOCATION_FILES PATCH_FILENAME);
#endif
 
	/* use the generic slave address for optimizations */

	//NXP_I2C_EnableLogging(1);
   for (h=0; h<2; h++)
   {
      /* create handle */
      err = Tfa98xx_Open(I2C_ADDRESS+2*h, &handles[h]);
	   assert(err == Tfa98xx_Error_Ok);
      
      coldStartup(handles[h], SAMPLE_RATE, LOCATION_FILES "coldboot.patch");
   }

   for (h=0; h<2; h++)
   {
      /* load current speaker file to structure */
      loadSpeakerFile(LOCATION_FILES SPEAKER_FILENAME, loadedSpeaker);

      /*Set to calibration once*/
      /* Only needed for really first time calibration */


      setOtc(handles[h], 1);

      /* Check if MTPEX bit is set for calibration once mode */
      if(checkMTPEX(handles[h]) == 0)
      {
         printf("DSP not yet calibrated. Calibration will start.\n");
       
         /* ensure no audio during special calibration */
         err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Digital);
         assert(err == Tfa98xx_Error_Ok);
      }
      else
      {
         printf("DSP already calibrated. Calibration skipped and previous calibration results loaded from MTP.\n");
      }
   }
	/* load predefined, or fullmodel from file */
	setSpeakerStereo(2, handles, LOCATION_FILES SPEAKER_FILENAME, loadedSpeaker);
	/* load the settings */
	setConfigStereo(2, handles, LOCATION_FILES CONFIG_FILENAME);
	/* load a preset */
	setPresetStereo(2, handles, LOCATION_FILES PRESET_FILENAME);
	/* set the equalizer to Rock mode */
	setEQStereo(2, handles, LOCATION_FILES EQ_FILENAME);

	err = Tfa98xx_SelectChannel(handles[0], Tfa98xx_Channel_L);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectChannel(handles[1], Tfa98xx_Channel_R);
	assert(err == Tfa98xx_Error_Ok);

	/* ensure stereo routing is correct: in this example we use
	 * gain is on L channel from 1->2
	 * gain is on R channel from 2->1
	 * on the other channel of DATAO we put Isense
	 */

	err = Tfa98xx_SelectI2SOutputLeft(handles[0], Tfa98xx_I2SOutputSel_DSP_Gain);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectStereoGainChannel(handles[1], Tfa98xx_StereoGainSel_Left);
	assert(err == Tfa98xx_Error_Ok);

	err = Tfa98xx_SelectI2SOutputRight(handles[1], Tfa98xx_I2SOutputSel_DSP_Gain);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectStereoGainChannel(handles[0], Tfa98xx_StereoGainSel_Right);
	assert(err == Tfa98xx_Error_Ok);

	err = Tfa98xx_SelectI2SOutputRight(handles[0], Tfa98xx_I2SOutputSel_CurrentSense);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectI2SOutputLeft(handles[1], Tfa98xx_I2SOutputSel_CurrentSense);
	assert(err == Tfa98xx_Error_Ok);

	/* all settings loaded, signal the DSP to start calibration */
	err = Tfa98xx_SetConfigured(handles[0]);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SetConfigured(handles[1]);
	assert(err == Tfa98xx_Error_Ok);
   
   for (h=0; h<2; h++)
	{
		waitCalibration(handles[h], &calibrateDone);
      if (calibrateDone)
      {
         Tfa98xx_DspGetCalibrationImpedance(handles[h],&re25);
      }
      else
      {
         re25 = 0;
         err = Tfa98xx_Powerdown(handles[h], 1);
         printf("Calibration failed, power down and return\n");
         return;
      }

      statusCheck(handles[h]);

      err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Off);
      assert(err == Tfa98xx_Error_Ok);

	}
   
	for(i=0;i<50;i++)
	{
		for (h=0; h<2; h++)
		{
		   err = Tfa98xx_DspGetStateInfo(handles[h], &stateInfo);
           assert(err == Tfa98xx_Error_Ok);
           dump_state_info(&stateInfo);
           Sleep(1000);
		   err = Tfa98xx_ReadRegister16(handles[h], TFA98XX_STATUSREG, &status);
		   switch(tfaRun_CheckEvents(status)) 
		   {//idx=0
			   case 1: //printf(">>>>>>>>>>repower CF\n");
				   tfaRun_PowerCycleCF(handles[h]);
			   break;
			   case 2:
				   printf(">>>>>>>>>>full reset required!!\n");
			   break;
			   default:	//nop
			   break;
		   }
		}
		Sleep(1000);
   }

   /* playing with the volume: first down, then up again */
   for (i=2; i<5; i++)
   {
      for (h=0; h<2; h++)
      { 
         float vol;

         printf("Setting volume to %3.1f dB\n", -6.0f*i);
         err = Tfa98xx_SetVolume(handles[h], -6.0f*i);
         assert(err == Tfa98xx_Error_Ok);
         err = Tfa98xx_GetVolume(handles[h], &vol);
         assert(err == Tfa98xx_Error_Ok);
         assert( fabs(-6.0f*i - vol) < 0.001) ;
         err = Tfa98xx_DspGetStateInfo(handles[h], &stateInfo);
         assert(err == Tfa98xx_Error_Ok);
         dump_state_info(&stateInfo);
         Sleep(1000);
      }
   }
   for (i=5; i>=0; i--)
   {
      for (h=0; h<2; h++)
      { 
         printf("Setting volume to %3.1f dB\n", -6.0f*i);
         err = Tfa98xx_SetVolume(handles[h], -6.0f*i);
         assert(err == Tfa98xx_Error_Ok);
         dump_state_info(&stateInfo);
         Sleep(1000);
      }
   }

   for (h=0; h<2; h++)
   {
      err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Amplifier);
      assert(err == Tfa98xx_Error_Ok);

      err = Tfa98xx_Powerdown(handles[h], 1);
      assert(err == Tfa98xx_Error_Ok);

      err = Tfa98xx_Close(handles[h]);
      assert(err == Tfa98xx_Error_Ok);
   }  

	return 0;
}
