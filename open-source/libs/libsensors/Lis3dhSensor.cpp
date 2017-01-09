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

#include "Lis3dhSensor.h"

#define LIS3DH_ACC_DEV_PATH_NAME    "/dev/lis3dh_acc"
#define LIS3DH_ACC_INPUT_NAME  "accelerometer" 

#define	LIS3DH_ACC_IOCTL_BASE 77
/** The following define the IOCTL command values via the ioctl macros */
#define	LIS3DH_ACC_IOCTL_SET_DELAY		_IOW(LIS3DH_ACC_IOCTL_BASE, 0, int)
#define	LIS3DH_ACC_IOCTL_GET_DELAY		_IOR(LIS3DH_ACC_IOCTL_BASE, 1, int)
#define	LIS3DH_ACC_IOCTL_SET_ENABLE		_IOW(LIS3DH_ACC_IOCTL_BASE, 2, int)
#define	LIS3DH_ACC_IOCTL_GET_ENABLE		_IOR(LIS3DH_ACC_IOCTL_BASE, 3, int)

#define LIS3DH_LAYOUT_NEGX    0x01
#define LIS3DH_LAYOUT_NEGY    0x02
#define LIS3DH_LAYOUT_NEGZ    0x04
 

#define LIS3DH_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (1024.0f))

/*****************************************************************************/

Lis3dhSensor::Lis3dhSensor()
    : SensorBase(LIS3DH_ACC_DEV_PATH_NAME, LIS3DH_ACC_INPUT_NAME),
      mEnabled(0),
      mDelay(-1),
      mLayout(0),
      mInputReader(32),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    mLayout =  getLayout();
   
    open_device();
}

Lis3dhSensor::~Lis3dhSensor() {
    if (mEnabled) {
        setEnable(0, 0);
    }

    close_device();
}

int Lis3dhSensor::setInitialState() {
    struct input_absinfo absinfo;

    if (mEnabled) {
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
            mPendingEvent.acceleration.x = LIS3DH_UNIT_CONVERSION(absinfo.value);
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
            mPendingEvent.acceleration.y = LIS3DH_UNIT_CONVERSION(absinfo.value);
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
            mPendingEvent.acceleration.z = LIS3DH_UNIT_CONVERSION(absinfo.value);
        }
    }
    return 0;
}

int Lis3dhSensor::getLayout() {
    return LIS3DH_LAYOUT_NEGX|LIS3DH_LAYOUT_NEGZ;
}

bool Lis3dhSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int Lis3dhSensor::setEnable(int32_t handle, int enabled) {
    int err = 0;
    int opDone = 0;

    /* handle check */
    if (handle != ID_A) {
        ALOGE("Lis3dhSensor: Invalid handle (%d)", handle);
        return -EINVAL;
    }

    if (mEnabled <= 0) {
        if (enabled) {
            err = ioctl(dev_fd, LIS3DH_ACC_IOCTL_SET_ENABLE, &enabled);
            opDone = 1;
        }
    } else if (mEnabled == 1) {
        if (!enabled) {
            err = ioctl(dev_fd, LIS3DH_ACC_IOCTL_SET_ENABLE, &enabled);
            opDone = 1;
        }
    }
    if (err != 0) {
        ALOGE("Lis3dhSensor: IOCTL failed (%s)", strerror(errno));
        return err;
    }
    if (opDone) {
        ALOGD("Lis3dhSensor: Control set %d", enabled);
        setInitialState();
    }

    if (enabled) {
        mEnabled++;
        if (mEnabled > 32767) mEnabled = 32767;
    } else {
        mEnabled--;
        if (mEnabled < 0) mEnabled = 0;
    }
    ALOGD("Lis3dhSensor: mEnabled = %d", mEnabled);

    return err;
}

int Lis3dhSensor::setDelay(int32_t handle, int64_t delay_ns)
{
    int err = 0;
    int ms; 

    /* handle check */
    if (handle != ID_A) {
        ALOGE("Lis3dhSensor: Invalid handle (%d)", handle);
        return -EINVAL;
    }

    if (mDelay != delay_ns) {
        ms = delay_ns / 1000000;
        if (ioctl(dev_fd, LIS3DH_ACC_IOCTL_SET_DELAY, &ms)) {
            return -errno;
        }
        mDelay = delay_ns;
    }

    return err;
}

int64_t Lis3dhSensor::getDelay(int32_t handle)
{
    return (handle == ID_A) ? mDelay : 0;
}

int Lis3dhSensor::getEnable(int32_t handle)
{
    return (handle == ID_A) ? mEnabled : 0;
}

int Lis3dhSensor::readEvents(sensors_event_t* data, int count)
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
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;

        if (type == EV_ABS) {
            int   value = event->value; 
            if (event->code == EVENT_TYPE_ACCEL_X) {
                if (mLayout & LIS3DH_LAYOUT_NEGX) value = (~value) +1; // Reverse polarity of x ? 

                mPendingEvent.acceleration.x = LIS3DH_UNIT_CONVERSION(value);
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                if (mLayout & LIS3DH_LAYOUT_NEGY) value = (~value) +1; // Reverse polarity of y ? 

                mPendingEvent.acceleration.y = LIS3DH_UNIT_CONVERSION(value);
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                if (mLayout & LIS3DH_LAYOUT_NEGZ) value = (~value) +1; // Reverse polarity of z ? 

                mPendingEvent.acceleration.z = LIS3DH_UNIT_CONVERSION(value);
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            ALOGE("Lis3dhSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}
