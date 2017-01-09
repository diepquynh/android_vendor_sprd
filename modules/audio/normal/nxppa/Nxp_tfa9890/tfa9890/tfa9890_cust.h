/*
 * tfa9890_cust.h
 *
 *
 *  Created on: May 8, 2014
 *  Author: Customer, according to the Platform
 */

#ifndef _TFA9890_CUST_H_
#define _TFA9890_CUST_H_

//#define TFA_I2CDEVICE        "/dev/smartpa_i2c"        // Panda = 4 device driver interface special file
#define TFA_I2CDEVICE "/dev/smartpa1"  /*Sprd*/
#define TFA_I2CSLAVEBASE        (0x34)              // tfa device slave address of 1st (=left) device
#define I2C_ADDRESS             (TFA_I2CSLAVEBASE<<1)
//#define I2C_ADDRESS             (0x6c)
#define SAMPLE_RATE             (48000)

#define SPEAKER_FILENAME "X2_0630.speaker"
#define CONFIG_FILENAME "TFA9890_N1B12_N1C3_v2.config"
#define PATCH_FILENAME "TFA9890_N1C3_2_1_1.patch"
#define MUSIC_PRESET_FILENAME  "HQ0630_0_0_X2_0630.preset"
#define MUSIC_EQ_FILENAME      "HQ0630_0_0_X2_0630.eq"
#define SPEECH_PRESET_FILENAME "Speech0701-Flat2_0_0_X2_0630.preset"
#define SPEECH_EQ_FILENAME     "Speech0701-Flat2_0_0_X2_0630.eq"

#if 0
#define TFA98XX_NOMINAL_IMPEDANCE         (6)
#define TFA98XX_NOMINAL_IMPEDANCE_MIN     ((float)TFA98XX_NOMINAL_IMPEDANCE*0.8)
#define TFA98XX_NOMINAL_IMPEDANCE_MAX     ((float)TFA98XX_NOMINAL_IMPEDANCE*1.4)
#endif

#define TFA98XX_NOMINAL_IMPEDANCE         (5)
#define TFA98XX_NOMINAL_IMPEDANCE_MIN     ((float)TFA98XX_NOMINAL_IMPEDANCE*0.8)
#define TFA98XX_NOMINAL_IMPEDANCE_MAX     ((float)TFA98XX_NOMINAL_IMPEDANCE*1.6)


#define TFA98XX_TCOEF_MIN     (0.02)
#define TFA98XX_TCOEF_MAX     (0.04)

#ifdef WIN32
// cwd = dir where vcxproj is
#define LOCATION_FILES "../../../../settings/"
#else
// cwd = linux dir
#define LOCATION_FILES "/etc/smartpa_params/"
#endif

#endif /* _TFA9890_CUST_H_ */
