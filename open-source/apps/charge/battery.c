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

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "common.h"


#if HAVE_ANDROID_OS
#include <linux/ioctl.h>
#endif

#include "battery.h"


#define POWER_SUPPLY_PATH "/sys/class/power_supply"

enum gFieldID gFieldIds;

struct BatteryManagerConstants {
	int statusUnknown;
	int statusCharging;
	int statusDischarging;
	int statusNotCharging;
	int statusFull;
	int healthUnknown;
	int healthGood;
	int healthOverheat;
	int healthDead;
	int healthOverVoltage;
	int healthUnspecifiedFailure;
};
static struct BatteryManagerConstants gConstants;

struct PowerSupplyPaths {
	char* acOnlinePath;
	char* usbOnlinePath;
	char* batteryStatusPath;
	char* batteryHealthPath;
	char* batteryPresentPath;
	char* batteryCapacityPath;
	char* batteryVoltagePath;
	char* batteryTemperaturePath;
	char* batteryTechnologyPath;
};

static struct PowerSupplyPaths gPaths;

int PowerSupplyStatus[mBatteryEnd];

pthread_mutex_t gBatteryMutex = PTHREAD_MUTEX_INITIALIZER;

static int gVoltageDivisor = 1;

static int getBatteryStatus(const char* status)
{
	switch (status[0]) {
		case 'C': return gConstants.statusCharging;         /* Charging */
		case 'D': return gConstants.statusDischarging;      /* Discharging */
		case 'F': return gConstants.statusFull;             /* Not charging */
		case 'N': return gConstants.statusNotCharging;      /* Full */
		case 'U': return gConstants.statusUnknown;          /* Unknown */

		default: {
					 LOGW("Unknown battery status '%s'", status);
					 return gConstants.statusUnknown;
				 }
	}
}

static int getBatteryHealth(const char* status)
{
	switch (status[0]) {
		case 'D': return gConstants.healthDead;         /* Dead */
		case 'G': return gConstants.healthGood;         /* Good */
		case 'O': {
					  if (strcmp(status, "Overheat") == 0) {
						  return gConstants.healthOverheat;
					  } else if (strcmp(status, "Over voltage") == 0) {
						  return gConstants.healthOverVoltage;
					  }
					  LOGW("Unknown battery health[1] '%s'", status);
					  return gConstants.healthUnknown;
				  }

		case 'U': {
					  if (strcmp(status, "Unspecified failure") == 0) {
						  return gConstants.healthUnspecifiedFailure;
					  } else if (strcmp(status, "Unknown") == 0) {
						  return gConstants.healthUnknown;
					  }
				  }

		default: {
					 LOGW("Unknown battery health[2] '%s'", status);
					 return gConstants.healthUnknown;
				 }
	}
}

static int readFromFile(const char* path, char* buf, size_t size)
{
	if (!path)
		return -1;
	int fd = open(path, O_RDONLY, 0);
	if (fd == -1) {
		LOGE("Could not open '%s'", path);
		return -1;
	}

	size_t count = read(fd, buf, size);
	if (count > 0) {
		count = (count < size) ? count : size - 1;
		while (count > 0 && buf[count-1] == '\n') count--;
		buf[count] = '\0';
	} else {
		buf[0] = '\0';
	} 

	close(fd);
	return count;
}

static void setBooleanField(const char* path, enum gFieldID fieldID)
{
	const int SIZE = 16;
	char buf[SIZE];

	char value = 0;
	if (readFromFile(path, buf, SIZE) > 0) {
		if (buf[0] == '1') {
			value = 1;
		}
	}
	PowerSupplyStatus[fieldID] = value;
}

static void setIntField(const char* path, enum gFieldID fieldID)
{
	const int SIZE = 128;
	char buf[SIZE];

	int value = 0;
	if (readFromFile(path, buf, SIZE) > 0) {
		value = atoi(buf);
	}
	PowerSupplyStatus[fieldID] = value;
}

static void setInt(enum gFieldID fieldID, int value)
{
	PowerSupplyStatus[fieldID] = value;
}
static void setVoltageField(const char* path, enum gFieldID fieldID)
{
	const int SIZE = 128;
	char buf[SIZE];

	int value = 0;
	if (readFromFile(path, buf, SIZE) > 0) {
		value = atoi(buf);
		value /= gVoltageDivisor;
	}
	PowerSupplyStatus[fieldID] = value;
}

extern int is_exit;

void * battery_status_update(void * cookie)
{
	const int SIZE = 128;
	char buf[SIZE];
	int i;

	for(;!is_exit;){
		pthread_mutex_lock(&gBatteryMutex);

		setBooleanField(gPaths.acOnlinePath, mAcOnline);
		setBooleanField(gPaths.usbOnlinePath, mUsbOnline);
		setBooleanField(gPaths.batteryPresentPath, mBatteryPresent);

		setIntField(gPaths.batteryCapacityPath, mBatteryLevel);
		setVoltageField(gPaths.batteryVoltagePath, mBatteryVoltage);
		setIntField(gPaths.batteryTemperaturePath, mBatteryTemperature);

		if (readFromFile(gPaths.batteryStatusPath, buf, SIZE) > 0)
			setInt(mBatteryStatus, getBatteryStatus(buf));
		else
			setInt(mBatteryStatus,
					gConstants.statusUnknown);

		if (readFromFile(gPaths.batteryHealthPath, buf, SIZE) > 0)
			setInt(mBatteryHealth, getBatteryHealth(buf));

		pthread_mutex_unlock(&gBatteryMutex);
		usleep(100000/ PROGRESSBAR_INDETERMINATE_FPS);
		if(cookie)
			break;
	}
	return NULL;
}

int battery_status_init(void)
{
	char    path[PATH_MAX];
	struct dirent* entry;

	DIR* dir = opendir(POWER_SUPPLY_PATH);
	if (dir == NULL) {
		LOGE("Could not open %s\n", POWER_SUPPLY_PATH);
		return -1;
	}
	while ((entry = readdir(dir))) {
		const char* name = entry->d_name;

		// ignore "." and ".."
		if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
			continue;
		}

		char buf[20];
		// Look for "type" file in each subdirectory
		snprintf(path, sizeof(path), "%s/%s/type", POWER_SUPPLY_PATH, name);
		int length = readFromFile(path, buf, sizeof(buf));
		if (length > 0) {
			if (buf[length - 1] == '\n')
				buf[length - 1] = 0;

			if (strcmp(buf, "Mains") == 0) {
				snprintf(path, sizeof(path), "%s/%s/online", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.acOnlinePath = strdup(path);
			}
			else if (strcmp(buf, "USB") == 0) {
				snprintf(path, sizeof(path), "%s/%s/online", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.usbOnlinePath = strdup(path);
			}
			else if (strcmp(buf, "Battery") == 0) {
				snprintf(path, sizeof(path), "%s/%s/status", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.batteryStatusPath = strdup(path);
				snprintf(path, sizeof(path), "%s/%s/health", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.batteryHealthPath = strdup(path);
				snprintf(path, sizeof(path), "%s/%s/present", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.batteryPresentPath = strdup(path);
				snprintf(path, sizeof(path), "%s/%s/capacity", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.batteryCapacityPath = strdup(path);

				snprintf(path, sizeof(path), "%s/%s/voltage_now", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0) {
					gPaths.batteryVoltagePath = strdup(path);
					// voltage_now is in microvolts, not millivolts
					gVoltageDivisor = 1000;
				} else {
					snprintf(path, sizeof(path), "%s/%s/batt_vol", POWER_SUPPLY_PATH, name);
					if (access(path, R_OK) == 0)
						gPaths.batteryVoltagePath = strdup(path);
				}

				snprintf(path, sizeof(path), "%s/%s/temp", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0) {
					gPaths.batteryTemperaturePath = strdup(path);
				} else {
					snprintf(path, sizeof(path), "%s/%s/batt_temp", POWER_SUPPLY_PATH, name);
					if (access(path, R_OK) == 0)
						gPaths.batteryTemperaturePath = strdup(path);
				}

				snprintf(path, sizeof(path), "%s/%s/technology", POWER_SUPPLY_PATH, name);
				if (access(path, R_OK) == 0)
					gPaths.batteryTechnologyPath = strdup(path);
			}
		}
	}
	closedir(dir);

	if (!gPaths.acOnlinePath)
		LOGE("acOnlinePath not found");
	if (!gPaths.usbOnlinePath)
		LOGE("usbOnlinePath not found");
	if (!gPaths.batteryStatusPath)
		LOGE("batteryStatusPath not found");
	if (!gPaths.batteryHealthPath)
		LOGE("batteryHealthPath not found");
	if (!gPaths.batteryPresentPath)
		LOGE("batteryPresentPath not found");
	if (!gPaths.batteryCapacityPath)
		LOGE("batteryCapacityPath not found");
	if (!gPaths.batteryVoltagePath)
		LOGE("batteryVoltagePath not found");
	if (!gPaths.batteryTemperaturePath)
		LOGE("batteryTemperaturePath not found");
	if (!gPaths.batteryTechnologyPath)
		LOGE("batteryTechnologyPath not found");

	gConstants.statusUnknown = BATTERY_STATUS_UNKNOWN;
	gConstants.statusCharging = BATTERY_STATUS_CHARGING;
	gConstants.statusDischarging = BATTERY_STATUS_DISCHARGING;
	gConstants.statusNotCharging = BATTERY_STATUS_NOT_CHARGING;
	gConstants.statusFull = BATTERY_STATUS_FULL;
	gConstants.healthUnknown = BATTERY_HEALTH_UNKNOWN;
	gConstants.healthGood = BATTERY_HEALTH_GOOD;
	gConstants.healthOverheat = BATTERY_HEALTH_OVERHEAT;
	gConstants.healthDead = BATTERY_HEALTH_DEAD;
	gConstants.healthOverVoltage = BATTERY_HEALTH_OVER_VOLTAGE;
	gConstants.healthUnspecifiedFailure = BATTERY_HEALTH_UNSPECIFIED_FAILURE;

	int temp;
	battery_status_update((void *)&temp);
	return 0;
}

int battery_ac_online(void)
{
	int ret;

	pthread_mutex_lock(&gBatteryMutex);
	setBooleanField(gPaths.acOnlinePath, mAcOnline);
	ret = PowerSupplyStatus[mAcOnline];
	pthread_mutex_unlock(&gBatteryMutex);
	return ret;
}
int battery_usb_online(void)
{
	int ret;

	pthread_mutex_lock(&gBatteryMutex);
	setBooleanField(gPaths.usbOnlinePath, mUsbOnline);
	ret = PowerSupplyStatus[mUsbOnline];
	pthread_mutex_unlock(&gBatteryMutex);
	return ret;
}
int battery_capacity(void)
{
	int ret;

	pthread_mutex_lock(&gBatteryMutex);

	setIntField(gPaths.batteryCapacityPath, mBatteryLevel);
	ret = PowerSupplyStatus[mBatteryLevel];
	pthread_mutex_unlock(&gBatteryMutex);
	return ret;
}

int battery_status(void)
{
	int ret;
	const int SIZE = 128;
	char buf[SIZE];

	pthread_mutex_lock(&gBatteryMutex);
	if (readFromFile(gPaths.batteryStatusPath, buf, SIZE) > 0)
		setInt(mBatteryStatus, getBatteryStatus(buf));
	else
		setInt(mBatteryStatus, gConstants.statusUnknown);

	ret = PowerSupplyStatus[mBatteryStatus];
	pthread_mutex_unlock(&gBatteryMutex);
	return ret;
}
