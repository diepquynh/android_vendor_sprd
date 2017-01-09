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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "AccSensor.h"
#include "OriSensor.h"
#include "PlsSensor.h"

/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF

#define LIGHT_SENSOR_POLLTIME    2000000000

#define SENSORS_ACCELERATION     (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD   (1<<ID_M)
#define SENSORS_ORIENTATION      (1<<ID_O)
#define SENSORS_LIGHT            (1<<ID_L)
#define SENSORS_PROXIMITY        (1<<ID_P)

#define SENSORS_ACCELERATION_HANDLE     0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_ORIENTATION_HANDLE      2
#define SENSORS_LIGHT_HANDLE            3
#define SENSORS_PROXIMITY_HANDLE        4

/*****************************************************************************/

/* The SENSORS Module */
static struct sensor_t sSensorList[AccSensor::numSensors
					+ OriSensor::numSensors +
				   PlsSensor::numSensors] = {
};

static int numSensors = 0;
static char GetChipInfo[256] = {0};

static int open_sensors(const struct hw_module_t *module, const char *id,
			struct hw_device_t **device);

static int sensors__get_sensors_list(struct sensors_module_t *module,
				     struct sensor_t const **list)
{
	*list = sSensorList;
	return numSensors;
}

static struct hw_module_methods_t sensors_module_methods = {
open:	open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
		common:{
				tag: HARDWARE_MODULE_TAG,
				version_major: 1,
				version_minor: 0,
				id: SENSORS_HARDWARE_MODULE_ID,
				name: "SPRD Sensor module",
				author: "Spreadtrum",
				methods: &sensors_module_methods,
				dso: 0,
				reserved:{},
		},
		get_sensors_list:sensors__get_sensors_list,
};

struct sensors_poll_context_t {
	struct sensors_poll_device_t device;	// must be first

	 sensors_poll_context_t();
	~sensors_poll_context_t();
	int activate(int handle, int enabled);
	int setDelay(int handle, int64_t ns);
	int setDelay_sub(int handle, int64_t ns);
	int pollEvents(sensors_event_t * data, int count);

private:
	enum {
#ifndef ACC_NULL
		acc,
#endif
#ifndef ORI_NULL
		ori,
#endif
#ifndef PLS_NULL
		pls,
#endif
		numSensorDrivers,
		numFds,
	};

	static const size_t wake = numFds - 1;
	static const char WAKE_MESSAGE = 'W';
	struct pollfd mPollFds[numFds];
	int mWritePipeFd;
	SensorBase *mSensors[numSensorDrivers];
	PlsSensor *PlsObjList[PlsChipNum];

	int handleToDriver(int handle) const {
		switch (handle) {
		case ID_A:
#ifndef ACC_NULL
			return acc;
#endif
		case ID_M:
		case ID_O:
#ifndef ORI_NULL
			return ori;
#endif
		case ID_L:case ID_P:
#ifndef PLS_NULL
			return pls;
#endif
		return 0;
		}
		return -EINVAL;
	}

};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
#ifndef ACC_NULL
	mSensors[acc] = new AccSensor();
	numSensors +=
	    mSensors[acc]->populateSensorList(sSensorList + numSensors);
	mPollFds[acc].fd = mSensors[acc]->getFd();
	mPollFds[acc].events = POLLIN;
	mPollFds[acc].revents = 0;
	ALOGD("AnumSensors=%d; %d", numSensors, AccSensor::numSensors);
#endif
#ifndef ORI_NULL
	mSensors[ori] = new OriSensor();
	numSensors +=
	    mSensors[ori]->populateSensorList(sSensorList + numSensors);
	mPollFds[ori].fd = mSensors[ori]->getFd();
	mPollFds[ori].events = POLLIN;
	mPollFds[ori].revents = 0;
	ALOGD("OnumSensors=%d; %d", numSensors, OriSensor::numSensors);
#endif
#ifndef PLS_NULL
#ifdef PLS_COMPATIBLE
	PlsObjList[LTR558ALS] = new PlsLTR558();
	PlsObjList[EPL2182] = new PlsEPL2182();
	for( int i=0; i<PlsChipNum; i++) {
		memset(GetChipInfo,0,sizeof(GetChipInfo));
		mSensors[pls] = PlsObjList[i];
		mSensors[pls]->setEnable(ID_L, 1);
		mSensors[pls]->getChipInfo(GetChipInfo);
		ALOGD("[PlsSensor] the chip information is %s\n", GetChipInfo);
		mSensors[pls]->setEnable(ID_L, 0);
		if(0 == strcmp(GetChipInfo,PlsChipInfoList[i])) {
			PlsNewSuccess = true;
			for(int j=i+1; j<PlsChipNum; j++)
				delete PlsObjList[j];
			break;
		}else
			delete PlsObjList[i];
	}
	if(false == PlsNewSuccess) {
		mSensors[pls] = new PlsSensor();
		PlsNewSuccess = true;
	}
#else
	mSensors[pls] = new PlsSensor();
	ALOGD("[PlsSensor]non compatible\n");
#endif
	numSensors +=
	    mSensors[pls]->populateSensorList(sSensorList + numSensors);
	mPollFds[pls].fd = mSensors[pls]->getFd();
	mPollFds[pls].events = POLLIN;
	mPollFds[pls].revents = 0;
	ALOGD("PnumSensors=%d; %d", numSensors, PlsSensor::numSensors);
#endif
	int wakeFds[2];
	int result = pipe(wakeFds);
	ALOGE_IF(result < 0, "error creating wake pipe (%s)", strerror(errno));
	fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
	fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
	mWritePipeFd = wakeFds[1];

	mPollFds[wake].fd = wakeFds[0];
	mPollFds[wake].events = POLLIN;
	mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t()
{
	for (int i = 0; i < numSensorDrivers; i++) {
		delete mSensors[i];
	}
	close(mPollFds[wake].fd);
	close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
	int drv = handleToDriver(handle);
	int err;

	switch (handle) {
	case ID_A:
	case ID_M:
	case ID_L:
	case ID_P:
		/* No dependencies */
		break;

	case ID_O:
		/* These sensors depend on ID_A and ID_M */
		mSensors[handleToDriver(ID_A)]->setEnable(ID_A, enabled);
		mSensors[handleToDriver(ID_M)]->setEnable(ID_M, enabled);
		break;

	default:
		return -EINVAL;
	}

	ALOGD("activate handle=%d; drv=%d", handle, drv);

	err = mSensors[drv]->setEnable(handle, enabled);

	if (enabled && !err) {
		const char wakeMessage(WAKE_MESSAGE);
		int result = write(mWritePipeFd, &wakeMessage, 1);
		ALOGE_IF(result < 0, "error sending wake message (%s)",
			 strerror(errno));
	}

	return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
	switch (handle) {
	case ID_A:
	case ID_M:
	case ID_L:
	case ID_P:
		/* No dependencies */
		break;

	case ID_O:
		/* These sensors depend on ID_A and ID_M */
		setDelay_sub(ID_A, ns);
		setDelay_sub(ID_M, ns);
		break;

	default:
		return -EINVAL;
	}
	return setDelay_sub(handle, ns);
}

int sensors_poll_context_t::setDelay_sub(int handle, int64_t ns)
{
	int drv = handleToDriver(handle);
	if (drv < 0) {
		ALOGE("setDelay_sub():Negative array index\n");
		return -EINVAL;
	}
	int en = mSensors[drv]->getEnable(handle);
	int64_t cur = mSensors[drv]->getDelay(handle);
	int err = 0;

	if (en <= 1) {
		/* no dependencies */
		if (cur != ns) {
			err = mSensors[drv]->setDelay(handle, ns);
		}
	} else {
		/* has dependencies, choose shorter interval */
		if (cur > ns) {
			err = mSensors[drv]->setDelay(handle, ns);
		}
	}
	return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t * data, int count)
{
	int nbEvents = 0;
	int n = 0;
	int polltime = -1;

	do {
		// see if we have some leftover from the last poll()
		for (int i = 0; count && i < numSensorDrivers; i++) {
			SensorBase *const sensor(mSensors[i]);
			if ((mPollFds[i].revents & POLLIN)
			    || (sensor->hasPendingEvents())) {
				int nb = sensor->readEvents(data, count);
				if (nb < count) {
					// no more data for this sensor
					mPollFds[i].revents = 0;
				}
#ifndef ORI_NULL
				if ((0 != nb) && (acc == i)) {
					((OriSensor *) (mSensors[ori]))->
					    setAccel(&data[nb - 1]);
				}
#endif
				count -= nb;
				nbEvents += nb;
				data += nb;
			}
		}

		if (count) {
			// we still have some room, so try to see if we can get
			// some events immediately or just wait if we don't have
			// anything to return
			do {
				n = poll(mPollFds, numFds,
					 nbEvents ? 0 : polltime);
			} while (n < 0 && errno == EINTR);
			if (n < 0) {
				ALOGE("poll() failed (%s)", strerror(errno));
				return -errno;
			}
			if (mPollFds[wake].revents & POLLIN) {
				char msg;
				int result = read(mPollFds[wake].fd, &msg, 1);
				ALOGE_IF(result < 0,
					 "error reading from wake pipe (%s)",
					 strerror(errno));
				ALOGE_IF(msg != WAKE_MESSAGE,
					 "unknown message on wake queue (0x%02x)",
					 int (msg));
				mPollFds[wake].revents = 0;
			}
		}
		// if we have events and space, go read them
	} while (n && count);

	return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
	sensors_poll_context_t *ctx = (sensors_poll_context_t *) dev;
	if (ctx) {
		delete ctx;
	}
	return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
			  int handle, int enabled)
{
	sensors_poll_context_t *ctx = (sensors_poll_context_t *) dev;
	return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
			  int handle, int64_t ns)
{
	sensors_poll_context_t *ctx = (sensors_poll_context_t *) dev;
	return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
		      sensors_event_t * data, int count)
{
	sensors_poll_context_t *ctx = (sensors_poll_context_t *) dev;
	return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t *module, const char *id,
			struct hw_device_t **device)
{
	int status = -EINVAL;
	sensors_poll_context_t *dev = new sensors_poll_context_t();

	memset(&dev->device, 0, sizeof(sensors_poll_device_t));

	dev->device.common.tag = HARDWARE_DEVICE_TAG;
	dev->device.common.version = 0;
	dev->device.common.module = const_cast < hw_module_t * >(module);
	dev->device.common.close = poll__close;
	dev->device.activate = poll__activate;
	dev->device.setDelay = poll__setDelay;
	dev->device.poll = poll__poll;

	*device = &dev->device.common;
	status = 0;

	return status;
}
