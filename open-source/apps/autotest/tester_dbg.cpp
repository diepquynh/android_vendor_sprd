// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#include <stdlib.h>

#include "type.h"
#include "audio.h"
#include "battery.h"
#include "bt.h"
#include "camera.h"
#include "cmmb.h"
#include "diag.h"
#include "driver.h"
#include "fm.h"
#include "gps.h"
#include "input.h"
#include "lcd.h"
#include "light.h"
#include "sensor.h"
#include "sim.h"
#include "tcard.h"
#include "tester.h"
#include "ver.h"
#include "vibrator.h"
#include "wifi.h"
#include "autotest.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_tester {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//#define TST_ENB_AUDIO
//#define TST_ENB_BAT
//#define TST_ENB_BT
//#define TST_ENB_CAM
//#define TST_ENB_CMMB
//#define TST_ENB_DRV
//#define TST_ENB_FM
//#define TST_ENB_GPS
//#define TST_ENB_INPUT
//#define TST_ENB_LIGHT
//#define TST_ENB_LCD
//#define TST_ENB_SENSOR
//#define TST_ENB_SIM
#define TST_ENB_HeadSet
//#define TST_ENB_TCARD
//#define TST_ENB_VIB
//#define TST_ENB_WIFI

//------------------------------------------------------------------------------
// for developer debug test
int autotest_dbgtest( int argc, char *argv[] )
{
	if( test_Init() < 0 ) {
		test_Deinit();
		return -1;
	}

	int cnt, idx, oka;

#ifdef TST_ENB_AUDIO
	const uchar * pcm_mono;
	const uchar * pcm_left;
	const uchar * pcm_right;
	int     mono_len, left_len, right_len;
	test_GetMonoPcm( &pcm_mono, &mono_len );
	test_GetLeftPcm( &pcm_left, &left_len );
	test_GetRightPcm( &pcm_right, &right_len );

	if( audRcrderOpen(AUD_INDEV_BUILTIN_MIC, 16000) >= 0 ) {
		//usleep(20 * 1000);

		#define RCRD_CNT 160000
		uchar * rcrd = new uchar[RCRD_CNT];
		audRcrderRecord(rcrd, RCRD_CNT);
		DBGMSG("record data: %02x %02x %02x %02x %02x %02x %02x %02x\n",
			*rcrd, *(rcrd+1), *(rcrd+2), *(rcrd+3), *(rcrd+4), *(rcrd+5), *(rcrd+6), *(rcrd+7));
		audRcrderStop();
		audRcrderClose();
		FILE * pf = fopen("/data/rcrd.pcm", "wb");
		fwrite(rcrd, 1, RCRD_CNT, pf);
		fclose(pf);
		delete []rcrd;
	}

#if 0
	audPlayerOpen(AUD_OUTDEV_SPEAKER, 44100, 0, 0);
	audPlayerPlay(pcm_mono, mono_len);
	sleep(2);
	audPlayerClose();

	audPlayerOpen(AUD_OUTDEV_EARPIECE, 44100, 0, 0);
	audPlayerPlay(pcm_mono, mono_len);
	sleep(10);
	audPlayerClose();
	sleep(2);

	audPlayerOpen(AUD_OUTDEV_HEADSET, 44100, 1, 0);
	audPlayerPlay(pcm_left, left_len);
	sleep(6);
	audPlayerClose();
	sleep(2);

	audPlayerOpen(AUD_OUTDEV_HEADSET, 44100, 1, 0);
	audPlayerPlay(pcm_right, right_len);
	sleep(6);
	audPlayerClose();
#endif
#endif

#ifdef TST_ENB_BAT
	batOpen();
	batEnableCharger(1);
	batIsCharging();
	batClose();
#endif // TST_ENB_BAT

#ifdef TST_ENB_BT
	oka = 0;
	cnt = 10;
	for( idx = 0; idx < cnt; ++idx ) {
		DBGMSG("---------------------- bt test index %d --------------\n", idx);
		struct bdremote_t bds[10];

		if( btOpen() >= 0 && btInquire(bds, 1) > 0 ) {
			oka++;
		}

		btClose();
		sleep(1);
	}
	INFMSG("============================================\n");
	INFMSG("TestResult: bt[oka = %d, cnt = %d]\n", oka, cnt);
	INFMSG("============================================\n");
#endif

#ifdef TST_ENB_CAM
	#ifndef CAM_IDX
	#define CAM_IDX       5
    #define CAM_W         240 //(320 * 2)
    #define CAM_H         320 //(240 * 2)
    #define CAM_PIX_SIZE  2
    #define CAM_DAT_SIZE  (240 * CAM_PIX_SIZE)
	#endif // CAM_IDX

	if( camOpen(CAM_IDX, CAM_W, CAM_H) >= 0 ) {
		uchar buffer[CAM_DAT_SIZE];

		oka = 0;
		cnt = 10;
		for( idx = 0; idx < cnt; ++idx ) {
			DBGMSG("-------------- camera test index %d --------------\n", idx);
			if( camStart() < 0 ) {
				DBGMSG("!!!! cam start fail !!!!\n");
			} else {
				usleep(100 * 1000);
				if( camGetData(buffer, CAM_DAT_SIZE) >= 0 ) {
					char msg[CAM_DAT_SIZE * 3 + (CAM_DAT_SIZE / 16 + 1)];
					char * pmsg = msg;
					for( int i = 0; i < CAM_DAT_SIZE; ++i ) {
						snprintf(pmsg, 4, "%02X ", buffer[i]);
						pmsg += 3;
						if( i > 0 && (15 == (i % 16)) ) {
							*pmsg++ = '\n';
						}
					}
					*pmsg = 0;
					oka++;

					INFMSG("-------- cam data --------\n");
					INFMSG("%s", msg);
					INFMSG("--------------------------\n");
				}
			}

			camStop();
			usleep(100 * 1000);
		}

		INFMSG("============================================\n");
		INFMSG("TestResult: camera[oka = %d, cnt = %d]\n", oka, cnt);
		INFMSG("============================================\n");
	}
	camClose();
#endif

#ifdef TST_ENB_CMMB
	if( cmmbOpen() >= 0 ) {
		uchar buffer[CAM_DAT_SIZE];

		oka = 0;
		cnt = 1;
		for( idx = 0; idx < cnt; ++idx ) {
			DBGMSG("-------------- cmmb test index %d --------------\n", idx);
			if( cmmbStart() < 0 ) {
				DBGMSG("!!!! cmmb play fail !!!!\n");
			}

			sleep(4);

			cmmbStop();
			usleep(100 * 1000);
		}

		INFMSG("============================================\n");
		INFMSG("TestResult: cmmb[oka = %d, cnt = %d]\n", oka, cnt);
		INFMSG("============================================\n");
	}
	cmmbClose();
#endif // TST_ENB_CMMB

#ifdef TST_ENB_DRV
	ushort gpio;
	uchar  ucval;

	gpio = 59; // 50 tp reset pin
	drvGIODir(gpio, 0, GPIO_DIR_OUT);
	INFMSG("gpio %d output high\n", gpio);
	drvGIOSet(gpio, 1);
	sleep(4);
	INFMSG("gpio %d output low\n", gpio);
	drvGIOSet(gpio, 0);

	gpio  = 60; // 60 tp intr pin
	ucval = 0xff;
	drvGIODir(gpio, 1, GPIO_DIR_IN);
	drvGIOGet(gpio, &ucval);
	INFMSG("gpio %d input val = %d\n", gpio, ucval);

	uchar i2cbus  = 2;
	uchar i2caddr = 0x78;
	uchar i2creg  = 0x80;

	ucval = 0xff;
	drvI2CRead(i2cbus, i2caddr, i2creg, &ucval);
	INFMSG("i2c[%d]: addr = %d, reg[%d] = %d\n", i2cbus, i2caddr, i2creg, ucval);

	INFMSG("============================================\n");
	INFMSG("TestResult: driver interface done.\n");
	INFMSG("============================================\n");
#endif // TST_ENB_DRV

#ifdef TST_ENB_FM
	fmOpen();

	fmPlay(879); // 1077
	sleep(10);

	fmStop();
	fmClose();
#endif

#ifdef TST_ENB_GPS
	gpsOpen();
	gpsStart();

	cnt = 3 * 60;

	for( idx = 0; idx < cnt; ++idx ) {
		if( gpsGetSVNum() > 3 ) {
			break;
		}
		//printf(".");
		sleep(1);
	}
	printf("\n");

	INFMSG("============================================\n");
	INFMSG("TestResult: GPS[sv num = %d, time out = %d s]\n",
			gpsGetSVNum(), idx + 1);
	INFMSG("============================================\n");

	gpsStop();
	gpsClose();
#endif

#ifdef TST_ENB_INPUT
	inputOpen();
	int num = 0;
	struct kpd_info_t kpd_info;
	while( num++ < 10 ) {
		inputKPDWaitKeyPress(&kpd_info, 2000);
	}
	inputClose();
#endif

#ifdef TST_ENB_LIGHT
	lightOpen();

	lightSetLCD(50);
	sleep(2);
	lightSetLCD(0);

	lightSetKeypad(100);
	sleep(2);
	lightSetKeypad(0);

	lightClose();
#endif

#ifdef TST_ENB_LCD
	drvOpen();
	drvLcdSendData(0x555555);
	drvClose();
#endif //

#ifdef TST_ENB_SENSOR
	int snsrs[] = {
			SENSOR_TYPE_ACCELEROMETER,
			SENSOR_TYPE_MAGNETIC_FIELD,
			SENSOR_TYPE_GYROSCOPE,
			SENSOR_TYPE_LIGHT,
			SENSOR_TYPE_PROXIMITY,
	};
	int snsr_exists[] = {
		1, // SENSOR_TYPE_ACCELEROMETER
		1, // SENSOR_TYPE_MAGNETIC_FIELD
		0, // SENSOR_TYPE_GYROSCOPE
		1, // SENSOR_TYPE_LIGHT
		1, // SENSOR_TYPE_PROXIMITY
	};
	const char * snsr_txt[] = {
			"Accelerometer",
			"Magnetic",
			"Gyroscope",
			"Light",
			"Proximity",
	};

	oka = 0;
	cnt = 10;
	for( idx = 0; idx < cnt; ++idx ) {
		DBGMSG("-------------- sensor test index %d --------------\n", idx);
		sensorOpen();

		int fail = 0;
		for( uint si = 0; si < sizeof(snsrs)/sizeof(snsrs[0]); ++si ) {
			if( sensorActivate(snsrs[si]) >= 0 ) {
				DBGMSG("----> sensor '%s' exist.\n", snsr_txt[si]);
				if( !snsr_exists[si] ) {
					fail = 1;
				}
			} else {
				ERRMSG("====> sensor '%s' not exist!\n", snsr_txt[si]);
				if( snsr_exists[si] ) {
					fail = 1;
				}
			}
		}

		if( !fail ) {
			oka++;
		}

		sensorClose();
		sleep(2);
	}
	INFMSG("============================================\n");
	INFMSG("TestResult: sensor[oka = %d, cnt = %d]\n", oka, cnt);
	INFMSG("============================================\n");
#endif

#ifdef TST_ENB_SIM
	oka = 0;
	cnt = 10;
	for( idx = 0; idx < cnt; ++idx ) {
		DBGMSG("-------------- sim test index %d --------------\n", idx);

		simOpen();
		if( simCheck(0) >= 0 && simCheck(1) >= 0 ) {
			oka++;
		}
		simClose();

		sleep(1);
	}

	INFMSG("============================================\n");
	INFMSG("TestResult: sim[oka = %d, cnt = %d]\n", oka, cnt);
	INFMSG("============================================\n");
#endif

#ifdef TST_ENB_HeadSet
	oka = 0;
	cnt = 10;
	for( idx = 0; idx < cnt; ++idx ) {
		DBGMSG("-------------- HeadSet test index %d --------------\n", idx);
		if (headsetPlugState() > 0){
			oka++;
		}
	}

	INFMSG("============================================\n");
	INFMSG("TestResult: headset[oka = %d, cnt = %d]\n", oka, cnt);
	INFMSG("============================================\n");
#endif

#ifdef TST_ENB_TCARD
	oka = 0;
	cnt = 10;
	for( idx = 0; idx < cnt; ++idx ) {
		DBGMSG("-------------- tcard test index %d --------------\n", idx);

		if( tcardOpen() >= 0 && tcardIsPresent() >= 0 ) {
			oka++;
		}
		tcardClose();

		sleep(1);
	}

	INFMSG("============================================\n");
	INFMSG("TestResult: tcard[oka = %d, cnt = %d]\n", oka, cnt);
	INFMSG("============================================\n");
#endif

#ifdef TST_ENB_VIB
	vibOpen();
	vibTurnOn(1);
	sleep(2);
	vibTurnOff();
	vibClose();
#endif

#ifdef TST_ENB_WIFI

	oka = 0;
	cnt = 10;
	for( idx = 0; idx < cnt; ++idx ) {
		DBGMSG("-------------- wifi test index %d --------------\n", idx);

		wifiOpen();

		struct wifi_ap_t aps[10];
		if( wifiSyncScanAP(aps, 1) > 0 ) {
			oka++;
		}

		int rssi = 0;
		wifiGetRssi(&rssi);
		INFMSG("wifi rssi = %d\n", rssi);

		wifiClose();
		sleep(1);
	}

	INFMSG("============================================\n");
	INFMSG("TestResult: wifi[oka = %d, cnt = %d]\n", oka, cnt);
	INFMSG("============================================\n");
#endif

	test_Deinit();
	return 0;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
