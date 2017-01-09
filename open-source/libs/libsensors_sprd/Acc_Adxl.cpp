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

#include "AccSensor.h"

#define ADXL_DATA_NAME				"ADXL34x accelerometer"
#define ADXL_MAX_SAMPLE_RATE_VAL	11	/* 200 Hz */

#define ACC_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (256.0f))

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{"Analog Devices ADXL345/6 3-axis Accelerometer",
	 "ADI",
	 1, SENSORS_ACCELERATION_HANDLE,
	 SENSOR_TYPE_ACCELEROMETER, (GRAVITY_EARTH * 16.0f),
	 (GRAVITY_EARTH * 16.0f) / 4096.0f, 0.145f, 10000, {}},
};

AccSensor::AccSensor() :
	SensorBase(NULL, ADXL_DATA_NAME),
		mEnabled(0), mDelay(-1), mInputReader(4), mHasPendingEvent(false),
		mSensorCoordinate()
{
	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_A;
	mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

	if (data_fd >= 0) {
		strcpy(input_sysfs_path, "/sys/class/input/");
		strcat(input_sysfs_path, input_name);
		strcat(input_sysfs_path, "/device/device/");
		input_sysfs_path_len = strlen(input_sysfs_path);
		ALOGD("AccSensor: sysfs_path=%s", input_sysfs_path);
	} else {
		input_sysfs_path[0] = '\0';
		input_sysfs_path_len = 0;
	}
}

AccSensor::~AccSensor()
{
	if (mEnabled) {
		setEnable(0, 0);
	}
}

int AccSensor::setInitialState()
{
	struct input_absinfo absinfo;

	if (mEnabled) {
		if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
			mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(absinfo.value);
		}
		if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
			mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(absinfo.value);
		}
		if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
			mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(absinfo.value);
		}
	}
	return 0;
}

bool AccSensor::hasPendingEvents() const {
	return mHasPendingEvent;
}
int AccSensor::setEnable(int32_t handle, int enabled)
{
	int err = 0;
	char buffer[2];

	/* handle check */
	if (handle != ID_A) {
		ALOGE("AccSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}

	buffer[0] = '\0';
	buffer[1] = '\0';

	if (mEnabled <= 0) {
		if (enabled)
			buffer[0] = '0';
	} else if (mEnabled == 1) {
		if (!enabled)
			buffer[0] = '1';
	}
	if (buffer[0] != '\0') {
		strcpy(&input_sysfs_path[input_sysfs_path_len], "disable");
		err = write_sys_attribute(input_sysfs_path, buffer, 1);
		if (err != 0) {
			return err;
		}
		ALOGD("AccSensor: Control set %s", buffer);
		setInitialState();
	}

	if (enabled) {
		mEnabled++;
		if (mEnabled > 32767)
			mEnabled = 32767;
	} else {
		mEnabled--;
		if (mEnabled < 0)
			mEnabled = 0;
	}
	ALOGD("AccSensor: mEnabled = %d", mEnabled);

	return err;
}

int AccSensor::setDelay(int32_t handle, int64_t delay_ns)
{
	int err = 0;
	int rate_val;
	int32_t us;
	char buffer[16];
	int bytes;

	/* handle check */
	if (handle != ID_A) {
		ALOGE("AccSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}

	if (mDelay != delay_ns) {
		/*
		 * The ADXL34x Supports 16 sample rates ranging from 3200Hz-0.098Hz
		 * Calculate best fit and limit to max 200Hz (rate_val 11)
		 */

		us = (int32_t) (delay_ns / 1000);
		for (rate_val = 0; rate_val < 16; rate_val++)
			if (us >= ((10000000) >> rate_val))
				break;

		if (rate_val > ADXL_MAX_SAMPLE_RATE_VAL) {
			rate_val = ADXL_MAX_SAMPLE_RATE_VAL;
		}

		strcpy(&input_sysfs_path[input_sysfs_path_len], "rate");
		bytes = sprintf(buffer, "%d", rate_val);
		err = write_sys_attribute(input_sysfs_path, buffer, bytes);
		if (err == 0) {
			mDelay = delay_ns;
			ALOGD
			    ("AccSensor: Control set delay %f ms requetsed, using %f ms",
			     delay_ns / 1000000.0f,
			     1e6 / (3200000 >> (15 - rate_val)));
		}
	}

	return err;
}

int64_t AccSensor::getDelay(int32_t handle)
{
	return (handle == ID_A) ? mDelay : 0;
}

int AccSensor::getEnable(int32_t handle)
{
	return (handle == ID_A) ? mEnabled : 0;
}

int AccSensor::readEvents(sensors_event_t * data, int count)
{
	if (count < 1)
		return -EINVAL;

	if (mHasPendingEvent) {
		mHasPendingEvent = false;
		mPendingEvent.timestamp = getTimestamp();
		*data = mPendingEvent;
		return mEnabled ? 1 : 0;
	}

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const *event;

	while (count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if (type == EV_ABS) {
			float value = event->value;
			if (event->code == EVENT_TYPE_ACCEL_X) {
				mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(value);
			} else if (event->code == EVENT_TYPE_ACCEL_Y) {
				mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(value);
			} else if (event->code == EVENT_TYPE_ACCEL_Z) {
				mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(value);
			}
		} else if (type == EV_SYN) {
			mPendingEvent.timestamp = timevalToNano(event->time);
			if (mEnabled) {
				mSensorCoordinate.coordinate_data_convert(
						mPendingEvent.acceleration.v, INSTALL_DIR);
				*data++ = mPendingEvent;
				count--;
				numEventReceived++;
			}
		} else {
			ALOGE("AccSensor: unknown event (type=%d, code=%d)",
			      type, event->code);
		}
		mInputReader.next();
	}

	return numEventReceived;
}

int AccSensor::populateSensorList(struct sensor_t *list)
{
	memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
	return numSensors;
}
