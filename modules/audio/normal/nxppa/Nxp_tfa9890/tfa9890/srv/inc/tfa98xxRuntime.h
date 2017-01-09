/*
 * tfa98xxRuntime.h
 *
 *  Created on: Feb 5, 2013
 *      Author: wim
 */

#ifndef TFA98XXRUNTIME_H_
#define TFA98XXRUNTIME_H_

#include "nxpTfa98xx.h"

/*
 * accounting globals
 */
extern int gTfaRun_timingVerbose; // in tfaRuntime
extern int gTfaRun_useconds;
extern int gTfRun_i2c_writes, gTfRun_i2c_reads;
extern int cli_verbose; 

Tfa98xx_Error_t tfaRun_Coldboot(Tfa98xx_handle_t handle, int state);
Tfa98xx_Error_t tfaRun_MuteAmplifier(Tfa98xx_handle_t handle);
int tfaRun_IsCold(Tfa98xx_handle_t handle);

Tfa98xx_Error_t tfa98xxCalWaitCalibration(Tfa98xx_handle_t handle, int *calibrateDone);

/*
 * start the maximus speakerboost algorithm
 *  this implies a full system startup when the system was not already started
 *
 */
Tfa98xx_Error_t tfaRun_SpeakerBoost(Tfa98xx_handle_t handle, int force);
/*
 *  this will load the patch witch will implicitly start the DSP
 *   if no patch is available the DPS is started immediately
 */
Tfa98xx_Error_t tfaRun_StartDSP(Tfa98xx_handle_t handle);
/*
 * start the clocks and wait until the AMP is switching
 *  on return the DSP sub system will be ready for loading
 */
Tfa98xx_Error_t tfaRun_Startup(Tfa98xx_handle_t handle);

int tfaRun_StatusWait(Tfa98xx_handle_t handle, unsigned short bitmask,
			int value, int maxTries);

/*
 * startup all devices. all step until patch loading is handled 
 */
int tfaRunStartupAll(Tfa98xx_handle_t *handles);
/*
 * shutdown the clocks and power down single or all devices
 */
int tfaRunShutDown(Tfa98xx_handle_t handle);
int tfaRunShutDownAll(Tfa98xx_handle_t *handles);
/*
 * run the startup/init sequence and set ACS bit
 */
Tfa98xx_Error_t tfaRun_ColdStartup(Tfa98xx_handle_t handle);
void tfaRun_Sleepus(int us);
void tfaRun_SleepusTrace(int us, char *file, int line);
#define tfaRun_Sleepus(t) tfaRun_SleepusTrace(t, __FILE__, __LINE__);
/*
 * validate the parameter cache
 *  if not loaded fix defaults
 */
void tfaRun_ValidateParams( nxpTfa98xxParameters_t *tfaParams, int defaultprofile);
int tfaRun_NameToParam( char *fullname, nxpTfa98xxParamsType_t type, nxpTfa98xxParameters_t *parms);
/*
 * first test implementation for handling OCP events
 */
int tfaRun_CheckEvents(unsigned short regval);
void tfaRun_StatusCheck(Tfa98xx_handle_t handle);
/*
 * check for algorythm corruption
 */
int tfaRun_CheckAlgo(Tfa98xx_handle_t handle);
Tfa98xx_Error_t tfaRun_PowerCycleCF(Tfa98xx_handle_t handle);
#endif /* TFA98XXRUNTIME_H_ */
