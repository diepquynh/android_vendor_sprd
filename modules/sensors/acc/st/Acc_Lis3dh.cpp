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

#include "../AccSensor.h"


#define LIS3DH_ACC_DEV_PATH_NAME "/dev/lis3dh_acc"
#define LIS3DH_ACC_INPUT_NAME "accelerometer"

#define LIS3DH_ACC_IOCTL_BASE 77
/* The following define the IOCTL command values via the ioctl macros */
#define LIS3DH_ACC_IOCTL_SET_DELAY _IOW(LIS3DH_ACC_IOCTL_BASE, 0, int)
#define LIS3DH_ACC_IOCTL_GET_DELAY _IOR(LIS3DH_ACC_IOCTL_BASE, 1, int)
#define LIS3DH_ACC_IOCTL_SET_ENABLE _IOW(LIS3DH_ACC_IOCTL_BASE, 2, int)
#define LIS3DH_ACC_IOCTL_GET_ENABLE _IOR(LIS3DH_ACC_IOCTL_BASE, 3, int)

#define ACC_UNIT_CONVERSION(value) ((value)*GRAVITY_EARTH / (1024.0f))

#define MAX_ENABLED 32767
/*****************************************************************************/
static struct sensor_t sSensorList[] = {
    {"ST LIS3DH 3-axis Accelerometer",
     "ST",
     1,
     SENSORS_ACCELERATION_HANDLE,
     SENSOR_TYPE_ACCELEROMETER,
     (GRAVITY_EARTH * 2.0f),
     (GRAVITY_EARTH) / 1024.0f,
     0.145f,
     5000,  // fastest is 200Hz
     0,
     0,
     SENSOR_STRING_TYPE_ACCELEROMETER,
     "",
     1000000,
     SENSOR_FLAG_CONTINUOUS_MODE,
     {}},
};

AccSensor::AccSensor()
    : SensorBase(LIS3DH_ACC_DEV_PATH_NAME, LIS3DH_ACC_INPUT_NAME),
      mEnabled(0),
      mDelay(-1),
      mInputReader(32),
      mHasPendingEvent(false),
      mSensorCoordinate() {
  mPendingEvent.version = sizeof(sensors_event_t);
  mPendingEvent.sensor = ID_A;
  mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
  memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

  open_device();
}

AccSensor::~AccSensor() {
  if (mEnabled) {
    setEnable(0, 0);
  }

  close_device();
}

int AccSensor::setInitialState() {
  struct input_absinfo absinfo;
  int clockid = CLOCK_BOOTTIME;


  if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
    mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(absinfo.value);
  }
  if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
    mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(absinfo.value);
  }
  if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
    mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(absinfo.value);
  }

  if (!ioctl(data_fd, EVIOCSCLOCKID, &clockid)) {
    ALOGD("AccSensor: set EVIOCSCLOCKID = %d\n", clockid);
  } else {
    ALOGE("AccSensor: set EVIOCSCLOCKID failed \n");
  }

  return 0;
}

bool AccSensor::hasPendingEvents() const { return mHasPendingEvent; }

int AccSensor::setEnable(int32_t handle, int enabled) {
  int err = 0;

  /* handle check */
  if (handle != ID_A) {
    ALOGE("AccSensor: Invalid handle (%d)", handle);
    return -EINVAL;
  }

  if (mEnabled <= 0) {
    if (enabled) {
      setInitialState();

      err = ioctl(dev_fd, LIS3DH_ACC_IOCTL_SET_ENABLE, &enabled);
    }
  } else if (mEnabled == 1) {
    if (!enabled) {
      err = ioctl(dev_fd, LIS3DH_ACC_IOCTL_SET_ENABLE, &enabled);
    }
  }
  if (err != 0) {
    ALOGE("AccSensor: IOCTL failed (%s)", strerror(errno));
    return err;
  }


  if (enabled) {
    mEnabled++;
    if (mEnabled > MAX_ENABLED) mEnabled = MAX_ENABLED;
  } else {
    mEnabled--;
    if (mEnabled < 0) mEnabled = 0;
  }
  ALOGD("AccSensor: mEnabled = %d", mEnabled);

  return err;
}

int AccSensor::setDelay(int32_t handle, int64_t delay_ns) {
  int err = 0;
  int ms;

  /* handle check */
  if (handle != ID_A) {
    ALOGE("AccSensor: Invalid handle (%d)", handle);
    return -EINVAL;
  }
  ALOGE("AccSensor: mDelay(%lld) delay_ns(%lld)", mDelay, delay_ns);
  if (mDelay != delay_ns) {
    ms = delay_ns / 1000000;
    ALOGE("AccSensor: ms(%d) ", ms);
    if (ioctl(dev_fd, LIS3DH_ACC_IOCTL_SET_DELAY, &ms)) {
      return -errno;
    }
    mDelay = delay_ns;
  }
  return err;
}

int64_t AccSensor::getDelay(int32_t handle) {
  return (handle == ID_A) ? mDelay : 0;
}

int AccSensor::getEnable(int32_t handle) {
  return (handle == ID_A) ? mEnabled : 0;
}

int AccSensor::readEvents(sensors_event_t *data, int count) {
  if (count < 1) return -EINVAL;

  if (mHasPendingEvent) {
    mHasPendingEvent = false;
    mPendingEvent.timestamp = getTimestamp();
    *data = mPendingEvent;
    return mEnabled ? 1 : 0;
  }

  ssize_t n = mInputReader.fill(data_fd);
  if (n < 0) return n;

  int numEventReceived = 0;
  input_event const *event;
  static float acc_latest_x;
  static float acc_latest_y;
  static float acc_latest_z;

  while (count && mInputReader.readEvent(&event)) {
    int type = event->type;
    if (type == EV_ABS) {
      float value = event->value;
      if (event->code == EVENT_TYPE_ACCEL_X) {
        acc_latest_x = ACC_UNIT_CONVERSION(value);
      } else if (event->code == EVENT_TYPE_ACCEL_Y) {
        acc_latest_y = ACC_UNIT_CONVERSION(value);
      } else if (event->code == EVENT_TYPE_ACCEL_Z) {
        acc_latest_z = ACC_UNIT_CONVERSION(value);
      }
    } else if (type == EV_SYN) {
      mPendingEvent.timestamp = timevalToNano(event->time);
      if (mEnabled) {
        mPendingEvent.acceleration.x = acc_latest_x;
        mPendingEvent.acceleration.y = acc_latest_y;
        mPendingEvent.acceleration.z = acc_latest_z;
        mSensorCoordinate.coordinate_data_convert(mPendingEvent.acceleration.v,
                                                  INSTALL_DIR);
        *data++ = mPendingEvent;
        count--;
        numEventReceived++;
      }
    } else {
      ALOGE("AccSensor: unknown event (type=%d, code=%d)", type, event->code);
    }
    mInputReader.next();
  }

  return numEventReceived;
}

int AccSensor::populateSensorList(struct sensor_t *list) {
  memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
  return numSensors;
}
