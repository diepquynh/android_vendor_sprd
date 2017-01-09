/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_PLS_SENSOR_H
#define ANDROID_PLS_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"


#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_ORIENTATION_HANDLE      2
#define SENSORS_LIGHT_HANDLE            3
#define SENSORS_PROXIMITY_HANDLE        4
/*****************************************************************************/
#define LTR558_DEVICE_NAME               		"/dev/ltr_558als"
#define AL3006_DEVICE_NAME               		"/dev/al3006_pls"
#define TMD2771_DEVICE_NAME                            "/dev/taos"
#define ELAN_DEVICE_NAME               		"/dev/epl2182_pls"
static char const *PlsChipInfoList[] = {
	"LTR558ALS",
	"EPL2182",
};
enum {
	LTR558ALS,
	EPL2182,
	PlsChipNum
};
//static char PlsChipNum = sizeof(PlsChipInfoList)/sizeof(PlsChipInfoList[1]);
static bool PlsNewSuccess = false;

struct input_event;

class PlsSensor : virtual public SensorBase {
public:
			PlsSensor();
    virtual ~PlsSensor();

#ifndef PLS_NULL
    enum {
        Light   = 0,
        Proximity   = 1,
        numSensors
    };
#else
	static const int numSensors = 0;
#endif
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual bool hasPendingEvents() const;
    virtual int readEvents(sensors_event_t* data, int count);
	virtual int getEnable(int32_t handle);
    void processEvent(int code, int value);
    virtual int populateSensorList(struct sensor_t *list);
    virtual void getChipInfo(char *buf);

private:
    int update_delay();
    uint32_t mEnabled;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    bool mHasPendingEvent;
    sensors_event_t mPendingEvents[numSensors];
    uint64_t mDelays[numSensors];
};

class PlsLTR558 : public PlsSensor {
public:
    PlsLTR558():SensorBase(LTR558_DEVICE_NAME, "alps_pxy"){};
    virtual  ~PlsLTR558(){};
    virtual int populateSensorList(struct sensor_t *list);
};

class PlsAL3006 : public PlsSensor {
public:
    PlsAL3006():SensorBase(AL3006_DEVICE_NAME, "proximity"){};
    virtual  ~PlsAL3006(){};
    virtual int populateSensorList(struct sensor_t *list);
};

class PlsTMD2771 : public PlsSensor {
public:
    PlsTMD2771():SensorBase(TMD2771_DEVICE_NAME, "light sensor"){};
    virtual  ~PlsTMD2771(){};
    virtual int populateSensorList(struct sensor_t *list);
};

class PlsEPL2182 : public PlsSensor {
public:
    PlsEPL2182():SensorBase(ELAN_DEVICE_NAME, "proximity"){};
    virtual  ~PlsEPL2182(){};
    virtual int populateSensorList(struct sensor_t *list);
};

/*****************************************************************************/

#endif  // ANDROID_PLS_SENSOR_H
