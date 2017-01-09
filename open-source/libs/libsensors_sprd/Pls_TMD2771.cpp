/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 Sorin P. <sorin@hypermagik.com>
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

#include "PlsSensor.h"
#include "sensors.h"

#define TMD2771_DEVICE_NAME               		"/dev/taos"

#if 0
#define LTR_IOCTL_MAGIC                         0x1C
#define LTR_IOCTL_GET_PFLAG                     _IOR(LTR_IOCTL_MAGIC, 1, int)
#define LTR_IOCTL_GET_LFLAG                     _IOR(LTR_IOCTL_MAGIC, 2, int)
#define LTR_IOCTL_SET_PFLAG                     _IOW(LTR_IOCTL_MAGIC, 3, int)
#define LTR_IOCTL_SET_LFLAG                     _IOW(LTR_IOCTL_MAGIC, 4, int)
#define LTR_IOCTL_GET_DATA                      _IOW(LTR_IOCTL_MAGIC, 5, unsigned char)
#endif

#define TAOS_IOCTL_MAGIC        	0XCF
#define TAOS_IOCTL_ALS_ON       	_IO(TAOS_IOCTL_MAGIC, 1)
#define TAOS_IOCTL_ALS_OFF      	_IO(TAOS_IOCTL_MAGIC, 2)
#define TAOS_IOCTL_ALS_DATA     	_IOR(TAOS_IOCTL_MAGIC, 3, short)
#define TAOS_IOCTL_ALS_CALIBRATE	_IO(TAOS_IOCTL_MAGIC, 4)
#define TAOS_IOCTL_CONFIG_GET   	_IOR(TAOS_IOCTL_MAGIC, 5, struct taos_cfg)
#define TAOS_IOCTL_CONFIG_SET		_IOW(TAOS_IOCTL_MAGIC, 6, struct taos_cfg)
#define TAOS_IOCTL_PROX_ON		_IO(TAOS_IOCTL_MAGIC, 7)
#define TAOS_IOCTL_PROX_OFF		_IO(TAOS_IOCTL_MAGIC, 8)
#define TAOS_IOCTL_PROX_DATA		_IOR(TAOS_IOCTL_MAGIC, 9, struct taos_prox_info)
#define TAOS_IOCTL_PROX_EVENT           _IO(TAOS_IOCTL_MAGIC, 10)
#define TAOS_IOCTL_PROX_CALIBRATE	_IO(TAOS_IOCTL_MAGIC, 11)

#define TAOS_IOCTL_SENSOR_ON	_IO(TAOS_IOCTL_MAGIC, 12)
#define TAOS_IOCTL_SENSOR_OFF	_IO(TAOS_IOCTL_MAGIC, 13)
#define TAOS_IOCTL_SENSOR_CONFIG	_IOW(TAOS_IOCTL_MAGIC, 14, struct taos_cfg)
#define TAOS_IOCTL_SENSOR_CHECK	_IO(TAOS_IOCTL_MAGIC, 15)
#define TAOS_IOCTL_SENSOR_test	_IO(TAOS_IOCTL_MAGIC, 16)

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{"TMD2771 Light sensor",
	 "TAOS",
	 1, SENSORS_LIGHT_HANDLE,
	 SENSOR_TYPE_LIGHT, 1.0f,
	 100000.0f, 0.005f, 0, {}},
	{"TMD2771 Proximity sensor",
	 "TAOS",
	 1, SENSORS_PROXIMITY_HANDLE,
	 SENSOR_TYPE_PROXIMITY, 1.0f,
	 1.0f, 0.005f, 0, {}},
};

PlsSensor::PlsSensor() :
	SensorBase(TMD2771_DEVICE_NAME, "light sensor"),
		mEnabled(0), mPendingMask(0), mInputReader(32), mHasPendingEvent(false)
{
	memset(mPendingEvents, 0, sizeof(mPendingEvents));

	mPendingEvents[Light].version = sizeof(sensors_event_t);
	mPendingEvents[Light].sensor = ID_L;
	mPendingEvents[Light].type = SENSOR_TYPE_LIGHT;

	mPendingEvents[Proximity].version = sizeof(sensors_event_t);
	mPendingEvents[Proximity].sensor = ID_P;
	mPendingEvents[Proximity].type = SENSOR_TYPE_PROXIMITY;

	for (int i = 0; i < numSensors; i++)
		mDelays[i] = 200000000;	// 200 ms by default
}

PlsSensor::~PlsSensor()
{
}

bool PlsSensor::hasPendingEvents() const
{
	return mHasPendingEvent;
}

int PlsSensor::setDelay(int32_t handle, int64_t ns)
{
	return 0;
}

int PlsSensor::setEnable(int32_t handle, int en)
{
	int what = -1;
	switch (handle) {
	case ID_L:
		what = Light;
		break;
	case ID_P:
		what = Proximity;
		break;
	}

	if (uint32_t(what) >= numSensors)
		return -EINVAL;

	int newState = en ? 1 : 0;
	int err = 0;

	ALOGD
	    ("PlsSensor::setEnable=0= en=%d; newState=%d; what=%d; mEnabled=%d\n",
	     en, newState, what, mEnabled);

	//if((uint32_t(newState) << what) != (mEnabled & (1 << what)))
	{
		if (!mEnabled)
			open_device();

		int flags = newState;

		switch (what) {
		case Light:
			if (newState) {
				err = ioctl(dev_fd, TAOS_IOCTL_ALS_ON, &flags);
				ALOGD
				    ("taos ---Light::TAOS_IOCTL_ALS_ON=== err=%d\n",
				     err);
			} else {
				err = ioctl(dev_fd, TAOS_IOCTL_ALS_OFF, &flags);
				ALOGD
				    ("taos ---Light::TAOS_IOCTL_ALS_OFF=== err=%d\n",
				     err);
			}
			break;

		case Proximity:
			if (newState) {
				err = ioctl(dev_fd, TAOS_IOCTL_PROX_ON, &flags);
				ALOGD
				    ("taos ---Proximity::TAOS_IOCTL_PROX_ON=== err=%d\n",
				     err);
			} else {
				err =
				    ioctl(dev_fd, TAOS_IOCTL_PROX_OFF, &flags);
				ALOGD
				    ("taos ---Proximity::TAOS_IOCTL_PROX_OFF=== err=%d\n",
				     err);
			}
			break;

		}

		if (!err) {
			mEnabled &= ~(1 << what);
			mEnabled |= (uint32_t(newState) << what);
		}

		ALOGD
		    ("PlsSensor::setEnable=1= en=%d; newState=%d; what=%d; mEnabled=%d\n",
		     en, newState, what, mEnabled);

		if (!mEnabled)
			close_device();

	}
	return err;
}

int PlsSensor::getEnable(int32_t handle)
{
	int enable = 0;
	int what = -1;
	switch (handle) {
	case ID_L:
		what = Light;
		break;
	case ID_P:
		what = Proximity;
		break;
	}

	if (uint32_t(what) >= numSensors)
		return -EINVAL;

	enable = mEnabled & (1 << what);

	if (enable > 0)
		enable = 1;

	ALOGD("PlsSensor::getEnable===mEnabled=0x%x; enable=%d\n", mEnabled,
	      enable);

	return enable;
}

int PlsSensor::readEvents(sensors_event_t * data, int count)
{

	ALOGD("PlsSensor::readEvents()======data_fd=%d,count=%d", data_fd,
	      count);

	if (count < 1)
		return -EINVAL;

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const *event;

	while (count && mInputReader.readEvent(&event)) {
		int type = event->type;
		if (type == EV_ABS) {
			processEvent(event->code, event->value);
			// mInputReader.next();
			int64_t time = timevalToNano(event->time);
			for (int j = 0; count && mPendingMask && j < numSensors;
			     j++) {
				if (mPendingMask & (1 << j)) {
					mPendingMask &= ~(1 << j);
					mPendingEvents[j].timestamp = time;
					if (mEnabled & (1 << j)) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if (!mPendingMask) {
				mInputReader.next();
			}

		} else if (type == EV_SYN) {
			int64_t time = timevalToNano(event->time);
			for (int j = 0; count && mPendingMask && j < numSensors;
			     j++) {
				if (mPendingMask & (1 << j)) {
					mPendingMask &= ~(1 << j);
					mPendingEvents[j].timestamp = time;
					if (mEnabled & (1 << j)) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if (!mPendingMask) {
				mInputReader.next();
			}
		} else {
			ALOGD
			    ("Apds9900Sensor: unknown event (type=%d, code=%d)",
			     type, event->code);
			mInputReader.next();
		}
	}

	return numEventReceived;
}

void PlsSensor::getChipInfo(char *buf){}

void PlsSensor::processEvent(int code, int value)
{

	ALOGD("PlsSensor::processEvent()=====code=%d::value=%d ", code, value);
	switch (code) {
	case EVENT_TYPE_PROXIMITY:
		mPendingMask |= 1 << Proximity;
		mPendingEvents[Proximity].distance = value;
		ALOGD("PlsSensor: mPendingEvents[Proximity].distance = %f",
		      mPendingEvents[Proximity].distance);
		break;
	case EVENT_TYPE_LIGHT:
		mPendingMask |= 1 << Light;
		mPendingEvents[Light].light = (float)value;
		ALOGD("PlsSensor: mPendingEvents[Light].light = %f",
		      mPendingEvents[Light].light);
		break;
	default:
		ALOGD("PlsSensor: default value = %d", value);
		break;
	}
}

int PlsSensor::populateSensorList(struct sensor_t *list)
{
	memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
	return numSensors;
}

/*****************************************************************************/
