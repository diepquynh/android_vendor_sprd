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


#define ACC_DATA_NAME    "FreescaleAccelerometer" 
#define ACC_SYSFS_PATH   "/sys/class/input"
#define ACC_SYSFS_DELAY  "poll"
#define ACC_SYSFS_ENABLE "enable"
#define ACC_EVENT_X ABS_X
#define ACC_EVENT_Y ABS_Y
#define ACC_EVENT_Z ABS_Z
#define ACC_UNIT_CONVERSION(value) (float)((float)((short)value) * (GRAVITY_EARTH / (0x4000)))

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{"Freescale Devices 3-axis Accelerometer",
     "Freescale",
     1, SENSORS_ACCELERATION_HANDLE,
     SENSOR_TYPE_ACCELEROMETER, (GRAVITY_EARTH * 4.0f),
	 (GRAVITY_EARTH * 4.0f) / 4096.0f, 0.145f, 10000, { } },
};
static char mClassPath[PATH_MAX];
static uint32_t mPendingMask;
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
		if (strcmp(buf, ACC_DATA_NAME) == 0) {
		    found = 1;
		    close(fd);
		    break;
		}

		close(fd);
		fd = -1;
	}
	closedir(dir);
	//ALOGE("the G sensor dir is %s",class_path);

	if (found) {
		return 0;
	}else {
		*class_path = '\0';
		return -1;
	}
}
int writeEnable(int isEnable) {
	char attr[PATH_MAX] = {'\0'};
	if(mClassPath[0] == '\0')
		return -1;

	strcpy(attr, mClassPath);
	strcat(attr,"/");
	strcat(attr,ACC_SYSFS_ENABLE);

	int fd = open(attr, O_RDWR);
	if (0 > fd) {
		ALOGE("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
		return -errno;
	}

	char buf[2];

	if (isEnable) {
		buf[0] = '1';
	} else {
		buf[0] = '0';
	}
	buf[1] = '\0';

	int err = 0;
	err = write(fd, buf, sizeof(buf));

	if (0 > err) {
		err = -errno;
		ALOGE("Could not write SysFs attribute \"%s\" (%s).", attr, strerror(errno));
	} else {
		err = 0;
	}

	close(fd);

	return err;
}
AccSensor::AccSensor() :
	SensorBase(NULL, ACC_DATA_NAME),
		mEnabled(0), mDelay(0), mInputReader(32), mHasPendingEvent(false),
		mSensorCoordinate()
{
    memset(&mPendingEvent, 0, sizeof(mPendingEvent));
	memset(mClassPath, '\0', sizeof(mClassPath));

	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_A;
	mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

	if(sensor_get_class_path(mClassPath))
	{
		ALOGE("Can`t find the Acc sensor!");
	}
	ALOGD("freescale sensor hal");
}

AccSensor::~AccSensor()
{
}

int AccSensor::setInitialState()
{
	return 0;
}

bool AccSensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int AccSensor::setEnable(int32_t handle, int enabled)
{	int err = 0;
    uint32_t newState = enabled;

    if (mEnabled != newState) {
        if (newState && !mEnabled)
            err = writeEnable(1);
        else if (!newState)
            err = writeEnable(0);
		ALOGI("change G sensor state \"%d -> %d\"", mEnabled, newState);
        ALOGE_IF(err, "Could not change sensor state \"%d -> %d\" (%s).", mEnabled, newState, strerror(-err));
        if (!err) {
            mEnabled = newState;
            setDelay(NULL, mDelay);
        }
    }
    return err;
}

int AccSensor::setDelay(int32_t handle, int64_t delay_ns)
{
	if (delay_ns < 0)
        return -EINVAL;
	if (mEnabled) {
		mDelay = delay_ns;
		char attr[PATH_MAX] = {'\0'};
		if(mClassPath[0] == '\0')
			return -1;

		strcpy(attr, mClassPath);
		strcat(attr,"/");
		strcat(attr,ACC_SYSFS_DELAY);

		int fd = open(attr, O_RDWR);
		if (0 > fd) {
			ALOGE("Could not open (write-only) SysFs attribute \"%s\" (%s).", attr, strerror(errno));
			return -errno;
		}
		if (mDelay > 10240000000LL) {
			mDelay = 10240000000LL; /* maximum delay in nano second. */
		}
		if (mDelay < 312500LL) {
			mDelay = 312500LL; /* minimum delay in nano second. */
		}

		char buf[80];
		sprintf(buf, "%lld", mDelay/1000/1000);
		write(fd, buf, strlen(buf)+1);
		close(fd);
    }
    return 0;
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

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if ((type == EV_ABS) || (type == EV_REL) || (type == EV_KEY)) {
            switch (event->code) {
				case ACC_EVENT_X :
				    mPendingMask = 1;
				    mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(event->value);
				    break;
				case ACC_EVENT_Y :
				    mPendingMask = 1;
				    mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(event->value);
				    break;
				case ACC_EVENT_Z :
				    mPendingMask = 1;
				    mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(event->value);
				    break;
			}
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
			if (mPendingMask) {
				mPendingMask = 0;
				mPendingEvent.timestamp = time;
				if (mEnabled) {
					mSensorCoordinate.coordinate_data_convert(
						mPendingEvent.acceleration.v, INSTALL_DIR);
					*data++ = mPendingEvent;
					count--;
					numEventReceived++;
				}
			}
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            ALOGE("FslAccSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }

    return numEventReceived;
}

int AccSensor::populateSensorList(struct sensor_t *list)
{
	memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
	return numSensors;
}
