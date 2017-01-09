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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#define ST480

#include "OriSensor.h"

#define SENODIAIO                   0xA1

/* IOCTLs for hal */
#define ECS_IOCTL_APP_SET_MFLAG         _IOW(SENODIAIO, 0x10, short)
#define ECS_IOCTL_APP_GET_MFLAG         _IOR(SENODIAIO, 0x11, short)
#define ECS_IOCTL_APP_SET_DELAY         _IOW(SENODIAIO, 0x12, short)
#define ECS_IOCTL_APP_GET_DELAY         _IOR(SENODIAIO, 0x13, short)
#define ECS_IOCTL_APP_SET_MVFLAG        _IOW(SENODIAIO, 0x14, short)
#define ECS_IOCTL_APP_GET_MVFLAG        _IOR(SENODIAIO, 0x15, short)

#define ST480_FREQUENCY 38
/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{"ST480 Magnetic field sensor",
	"Senodia",
	1,
	SENSORS_MAGNETIC_FIELD_HANDLE,
	SENSOR_TYPE_MAGNETIC_FIELD, 65535.0f,
	CONVERT_M, 0.35f, 10000, 0, 0, {}},
	{"ST480 Orientation sensor",
	"Senodia",
	1, SENSORS_ORIENTATION_HANDLE,
	SENSOR_TYPE_ORIENTATION, 360.0f,
	CONVERT_O, 0.495f, 10000, 0, 0, {}},
};

OriSensor::OriSensor()
: SensorBase("/dev/st480", "compass"),
	enabled(0),
	mPendingMask(0),
	mLevel(0),
	mMaEnabled(0),
	mOrEnabled(0),
	mInputReader(32)
{

	memset(mPendingEvents, 0, sizeof(mPendingEvents));

	mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
	mPendingEvents[MagneticField].sensor = ID_M;
	mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
	mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

	mPendingEvents[Orientation  ].version = sizeof(sensors_event_t);
	mPendingEvents[Orientation  ].sensor = ID_O;
	mPendingEvents[Orientation  ].type = SENSOR_TYPE_ORIENTATION;
	mPendingEvents[Orientation  ].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

	for (int i=0 ; i<__numSensors ; i++)
	{
		mEnabled[i] = 0;
		mDelay[i] = 50000000; // 50 ms by default
	}

	// read the actual value of all sensors if they're enabled already

	open_device();

	if (!enabled) {
		close_device();
	}
}

OriSensor::~OriSensor() {
}

int OriSensor::setEnable(int32_t handle, int en)
{
	int what = -1;
	int cmd = -1;

	ALOGD("handle = %d,en = %d",handle,en);

	what = handle2id(handle);

	if (what >= __numSensors)
		return -EINVAL;

	int newState  = en ? 1 : 0;
	int err = 0;

	if ((newState<<what) != (enabled & (1<<what))) {
		if (!enabled) {
			open_device();
		}
		switch (what) {
			case MagneticField: cmd = ECS_IOCTL_APP_SET_MFLAG; mMaEnabled = en; break;
			case Orientation:   cmd = ECS_IOCTL_APP_SET_MVFLAG; mOrEnabled = en; break;
			default: return -EINVAL;
		}
		short flags = newState;
		err = ioctl(dev_fd, cmd, &flags);
		err = err<0 ? -errno : 0;
		ALOGE_IF(err, "ECS_IOCTL_APP_SET_XXX failed (%s)", strerror(-err));
		if (!err) {
			enabled &= ~(1<<what);
			enabled |= (uint32_t(flags)<<what);
		}
		if (!enabled) {
			close_device();
		}
	}
	return err;
}

int OriSensor::getEnable(int32_t handle)
{
	int id = handle2id(handle);
	if (id >= 0) {
		return enabled;
	 } else {
		return 0;
	 }
}

int64_t OriSensor::getDelay(int32_t handle)
{
	int id = handle2id(handle);
	if (id > 0) {
		return mDelay[id];
	} else {
		return 0;
	}
}


int OriSensor::setDelay(int32_t handle, int64_t ns)
{
	int what = -1;

	what = handle2id(handle);

	if (what >= __numSensors)
		return -EINVAL;

	if (ns < 0)
		return -EINVAL;

	mDelay[what] = ns;

	if (enabled) {
		uint64_t wanted = -1LLU;
		for (int i=0 ; i<__numSensors ; i++) {
			if (enabled & (1<<i)) {
				uint64_t ns = mDelay[i];
				wanted = wanted < ns ? wanted : ns;
			}
		}
		short delay = int64_t(wanted) / 1000000;

		if(delay < 26)
		{
			mLevel = 1;
		}
		else if( (delay >=26) && (delay <= 52) )
		{
			mLevel = 2;
		}
		else if( (delay > 52) && (delay <= 100) )
		{
			mLevel = 3;
		}
		else if((delay > 100) && (delay <= 200))
		{
			mLevel = 4;
		}
		else
		{
			mLevel = 5;
		}

		delay = (int)(1000/ST480_FREQUENCY);

		if (ioctl(dev_fd, ECS_IOCTL_APP_SET_DELAY, &delay)) {
			return -errno;
		}
	}

	return 0;
}

int OriSensor::readEvents(sensors_event_t* data, int count)
{
	if (count < 1)
		return -EINVAL;

	ssize_t n = mInputReader.fill(data_fd);

	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const* event;
	static int64_t prev_time;

	while (count && mInputReader.readEvent(&event))
	{
		int type = event->type;
		if (type == EV_ABS) {
			processEvent(event->code, event->value);
			mInputReader.next();
		} else if (type == EV_SYN) {
			int64_t time = timevalToNano(event->time);
			if(mMaEnabled || mOrEnabled)
			{
				int time_diff_ms = 0;
				if(prev_time > 0)
					time_diff_ms = (int)((time - prev_time) / 1000);
				prev_time = time;

				//Run Algorithm
				run_library(st480, mLevel, time_diff_ms);

				float angles[3] = {0};
				float values[3] = {0};

				if(mMaEnabled)
				{
					get_magnetic_values(values);
					mPendingEvents[MagneticField].magnetic.x = values[0];
					mPendingEvents[MagneticField].magnetic.y = values[1];
					mPendingEvents[MagneticField].magnetic.z = values[2];
				}

				if(mOrEnabled)
				{
					get_orientation_angles(angles);
					mPendingEvents[Orientation].orientation.azimuth = angles[0];
					mPendingEvents[Orientation].orientation.pitch = angles[1];
					mPendingEvents[Orientation].orientation.roll = angles[2];
					mPendingMask |= 1<<Orientation;
				}
			}

			for (int j=0 ; count && mPendingMask && j<__numSensors ; j++) {
				if (mPendingMask & (1<<j)) {
					mPendingMask &= ~(1<<j);
					mPendingEvents[j].timestamp = time;
					if (enabled & (1<<j)) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if (!mPendingMask) {
				mInputReader.next();
			}
		} else
		{
			ALOGE("ST480Sensor: unknown event (type=%d, code=%d)",
                    		type, event->code);
			mInputReader.next();
		}
	}

	return numEventReceived;
}

int OriSensor::setAccel(sensors_event_t* data)
{
	int err;

	err = 0;
	st480.acc_x = data->acceleration.x;
	st480.acc_y = data->acceleration.y;
	st480.acc_z = data->acceleration.z;
	return err;
}

int OriSensor::handle2id(int32_t handle)
{
	switch (handle) {
	case ID_A:
		return Accelerometer;
	case ID_M:
		return MagneticField;
	case ID_O:
		return Orientation;
	default:
		ALOGE("OriSensor: unknown handle (%d)", handle);
		return -EINVAL;
        }
}

void OriSensor::processEvent(int code, int value)
{
	switch (code) {
		case EVENT_TYPE_MAGV_X:
			mPendingMask |= 1<<MagneticField;
			st480.mag_x = value;
			break;
		case EVENT_TYPE_MAGV_Y:
			mPendingMask |= 1<<MagneticField;
			st480.mag_y = value;
			break;
		case EVENT_TYPE_MAGV_Z:
			mPendingMask |= 1<<MagneticField;
			st480.mag_z = value;
			break;
	}
}

int OriSensor::populateSensorList(struct sensor_t *list)
{
        memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
        return numSensors;
}
