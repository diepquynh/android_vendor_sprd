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
 
#ifndef ANDROID_AL3006_SENSOR_H
#define ANDROID_AL3006_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class SensorAL3006 : public SensorBase
{
public:
	SensorAL3006();
    virtual ~SensorAL3006();

    enum
    {
        Light   = 0,
        Proximity   = 1,
        numSensors
    };

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual bool hasPendingEvents() const;
    virtual int readEvents(sensors_event_t* data, int count);
	virtual int getEnable(int32_t handle);
    void processEvent(int code, int value);
private:
    int update_delay();
    uint32_t mEnabled;
    bool mHasPendingEvent;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    uint64_t mDelays[numSensors];
};

/*****************************************************************************/

#endif  // ANDROID_AL3006_SENSOR_H
