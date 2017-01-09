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

#define ACC_SYSFS_PATH		"/sys/class/input"
#define ACC_INPUT_NAME		"accelerometer_sensor"


/** The following define the IOCTL command values via the ioctl macros */
#define	ACC_SYSFS_DELAY		"poll_delay"
#define	ACC_SYSFS_ENABLE	"enable"

#define ACC_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (0x4000))

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{
		"STM K2hh Accelerometer",
	 	"STM",
	 	1,
	 	SENSORS_ACCELERATION_HANDLE,
	 	SENSOR_TYPE_ACCELEROMETER,
	 	(GRAVITY_EARTH * 2.0f),
	 	(GRAVITY_EARTH) / 1024.0f,
	 	0.145f,
	 	10000,
	 	0,
	 	0,
	 	SENSOR_STRING_TYPE_ACCELEROMETER,
		"",
		1000000,
		SENSOR_FLAG_CONTINUOUS_MODE,
	 	{}
	},
};

static char mSensorPath[PATH_MAX];

int sensor_get_class_path(char *class_path)
{
	char dirname[] = ACC_SYSFS_PATH;
	char buf[256];
	int res;
	DIR *dir;
	struct dirent *de;
	int fd = -1;
	int found = 0;

	dir = opendir(dirname);
	if (dir == NULL)
		return -1;

	while((de = readdir(dir))) {
		if (strncmp(de->d_name, "input", strlen("input")) != 0) {
		    continue;
        	}

		sprintf(class_path, "%s/%s", dirname, de->d_name);
		snprintf(buf, sizeof(buf), "%s/name", class_path);

		fd = open(buf, O_RDONLY);
		if (fd < 0) {
		    continue;
		}
		if ((res = read(fd, buf, sizeof(buf))) < 0) {
		    close(fd);
		    continue;
		}
		buf[res - 1] = '\0';
		if (strcmp(buf, ACC_INPUT_NAME) == 0) {
		    found = 1;
		    close(fd);
		    break;
		}

		close(fd);
		fd = -1;
	}

	closedir(dir);
	ALOGD("AccSensor:Sensor dir is %s",class_path);

	if (found) {
		return 0;
	}else {
		*class_path = '\0';
		return -1;
	}
}

AccSensor::AccSensor() :
	SensorBase(NULL, ACC_INPUT_NAME), //(DIR_DEV, INPUT_NAME_ACC)
		mEnabled(0), mDelay(-1), mInputReader(32), mHasPendingEvent(false),
		mSensorCoordinate()
{
	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_A;
	mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

	if(sensor_get_class_path(mSensorPath))
	{
		ALOGE("Can`t find the Acc sensor!");
	}
	open_device();
}

AccSensor::~AccSensor()
{
	if (mEnabled) {
		setEnable(0, 0);
	}

	close_device();
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

bool AccSensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int AccSensor::setEnable(int32_t handle, int enabled)
{
	int  err = 0;
	char buffer[2];
	char attr[PATH_MAX] = {'\0'};

	if (mSensorPath[0] == '\0') {
		ALOGE("AccSensor: Invalid sensor node path !");
		return -EINVAL;
	}

	/* handle check */
	if (handle != ID_A) {
		ALOGE("AccSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}

        buffer[0] = '\0';
        buffer[1] = '\0';

        if (mEnabled <= 0) {
                if (enabled)
                        buffer[0] = '1';
        } else if (mEnabled == 1) {
                if (!enabled)
                        buffer[0] = '0';
        }

        if (buffer[0] != '\0') {
		strcpy(attr, mSensorPath);
        	strcat(attr,"/");
	        strcat(attr,ACC_SYSFS_ENABLE);

                err = write_sys_attribute(attr, buffer, 1);
                if (err != 0) {
			ALOGE("AccSensor: Write failed (%s), node:%s ", strerror(errno), attr);
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
        char attr[PATH_MAX] = {'\0'};

        if (mSensorPath[0] == '\0') {
                ALOGE("AccSensor: Invalid sensor node path !");
                return -EINVAL;
        }
	if (handle != ID_A) {
		ALOGE("AccSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}
	if (delay_ns < 0){
		ALOGE("AccSensor: Invalid delay ns (%lld)", delay_ns);
		return -EINVAL;
	}

	if (mEnabled && mDelay!=delay_ns) {
		mDelay = delay_ns;
		strcpy(attr, mSensorPath);
		strcat(attr,"/");
		strcat(attr,ACC_SYSFS_DELAY);

		int fd = open(attr, O_RDWR);
		if (fd < 0) {
			ALOGE("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
			return -errno;
		}

		char buf[80]={0};
		sprintf(buf, "%lld", mDelay);
		write(fd, buf, strlen(buf)+1);
		ALOGE("AccSensor: set delay ns (%lld)", mDelay);
		close(fd);
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
	static float acc_latest_x;
	static float acc_latest_y;
	static float acc_latest_z;

	while (count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if (type == EV_ABS  || type==EV_REL || type==EV_KEY) {
			float value = event->value;
			if (event->code == EVENT_TYPE_ACCEL_X) {
				//mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(value);
				acc_latest_x = ACC_UNIT_CONVERSION(value);
			} else if (event->code == EVENT_TYPE_ACCEL_Y) {
				//mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(value);
				acc_latest_y = ACC_UNIT_CONVERSION(value);
			} else if (event->code == EVENT_TYPE_ACCEL_Z) {
				//mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(value);
				acc_latest_z = ACC_UNIT_CONVERSION(value);
			}
		} else if (type == EV_SYN) {
			mPendingEvent.timestamp = timevalToNano(event->time);
			if (mEnabled) {
				mPendingEvent.acceleration.x = acc_latest_x;
				mPendingEvent.acceleration.y = acc_latest_y;
				mPendingEvent.acceleration.z = acc_latest_z;
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
