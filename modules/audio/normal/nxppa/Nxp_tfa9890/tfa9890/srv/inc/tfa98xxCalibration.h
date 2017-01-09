/*
 * tfa98xxCalibration.h
 *
 *  Created on: Feb 5, 2013
 *      Author: wim
 */

#ifndef TFA98XXCALIBRATION_H_
#define TFA98XXCALIBRATION_H_

/*
 * run the calibration sequence
 *
 * @param device handles
 * @param device index
 * @param once=1 or always=0
 * @return Tfa98xx Errorcode
 *
 */
Tfa98xx_Error_t tfa98xxCalibration(Tfa98xx_handle_t *handlesIn, int idx, int once );
/*
 *
 * @param device handle
 *
 */
void tfa98xxCalColdStartup(Tfa98xx_handle_t handle);
/*
 *
 * @param device handle
 *
 */
int tfa98xxCalCheckMTPEX(Tfa98xx_handle_t handle);
/*
 * return current tCoef , set new value if arg!=0
 */
Tfa98xx_Error_t tfa98xxCalWaitCalibration(Tfa98xx_handle_t handle, int *calibrateDone);
int tfa98xxCalCheckMTPEX(Tfa98xx_handle_t handle);
float tfa98xxCaltCoefFromSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes);
void tfa98xxCaltCoefToSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, float tCoef);
float tfa98xxCalGetTcoefA(Tfa98xx_handle_t *handlesIn);
float tfa98xxCalGetTcoef(Tfa98xx_handle_t handlesIn);

Tfa98xx_Error_t tfa98xxCalSetCalibrateOnce(Tfa98xx_handle_t handle);
Tfa98xx_Error_t tfa98xxCalSetCalibrationAlways(Tfa98xx_handle_t handle);
Tfa98xx_Error_t tfa98xxCalComputeSpeakertCoefA(  Tfa98xx_handle_t handle,
                                         Tfa98xx_SpeakerParameters_t loadedSpeaker,
                                         float tCoef );
int tfa98xxCalSetConfigured(Tfa98xx_handle_t *handlesIn );

int tfa98xxCalDspSupporttCoef(Tfa98xx_handle_t handle);

#endif /* TFA98XXCALIBRATION_H_ */
