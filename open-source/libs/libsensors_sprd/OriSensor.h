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

#ifndef ANDROID_ORI_SENSOR_H
#define ANDROID_ORI_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

#ifdef ST480
#include <linux/ioctl.h>
#include "RunAlgorithm.h"
#endif

#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_ORIENTATION_HANDLE      2
#define SENSORS_LIGHT_HANDLE            3
#define SENSORS_PROXIMITY_HANDLE        4
/*****************************************************************************/

struct input_event;

class OriSensor : public SensorBase {
public:
            OriSensor();
    virtual ~OriSensor();

#ifndef ORI_NULL
    enum {
		Accelerometer = 0,
        MagneticField,
        Orientation,
        __numSensors
    };
    static const int numSensors = 2;
#else
	static const int numSensors = 0;
#endif

    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int64_t getDelay(int32_t handle);
    virtual int getEnable(int32_t handle);
	int setAccel(sensors_event_t* data);
	virtual int populateSensorList(struct sensor_t *list);

private:
#ifndef ORI_NULL
    int mEnabled[__numSensors];
	int64_t mDelay[__numSensors];
    sensors_event_t mPendingEvents[__numSensors];
#endif
#ifdef ST480
    int enabled;
    unsigned char mLevel;
    bool mMaEnabled;
    bool mOrEnabled;
    _st480 st480;
#endif
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
	char input_sysfs_path[PATH_MAX];
	int input_sysfs_path_len;

	int handle2id(int32_t handle);
    void processEvent(int code, int value);
};

/*****************************************************************************/

#endif  // ANDROID_ORI_SENSOR_H
