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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <cutils/log.h>

#include <hardware/sensors.h>
#include <utils/Timers.h>

#define S_ON	1
#define S_OFF	0

struct sensors_poll_device_t *device;
struct sensors_module_t *module;
struct sensor_t const *list;
int count = 0;

char const *getSensorName(int type)
{
	switch (type) {
	case SENSOR_TYPE_ACCELEROMETER:
		return "Acc";
	case SENSOR_TYPE_MAGNETIC_FIELD:
		return "Mag";
	case SENSOR_TYPE_ORIENTATION:
		return "Ori";
	case SENSOR_TYPE_GYROSCOPE:
		return "Gyr";
	case SENSOR_TYPE_LIGHT:
		return "Lux";
	case SENSOR_TYPE_PRESSURE:
		return "Bar";
	case SENSOR_TYPE_TEMPERATURE:
		return "Tmp";
	case SENSOR_TYPE_PROXIMITY:
		return "Prx";
	case SENSOR_TYPE_GRAVITY:
		return "Grv";
	case SENSOR_TYPE_LINEAR_ACCELERATION:
		return "Lac";
	case SENSOR_TYPE_ROTATION_VECTOR:
		return "Rot";
	case SENSOR_TYPE_RELATIVE_HUMIDITY:
		return "Hum";
	case SENSOR_TYPE_AMBIENT_TEMPERATURE:
		return "Tam";
	}
	return "ukn";
}

static int show_sensor_info(int argc, char **argv)
{
	int opt;
	bool show = false;

	while ((opt = getopt(argc, argv, "a")) != -1) {
		switch (opt) {
		case 'a':
			show = true;
			break;
		default:
			return -EINVAL;
		}
	}
	printf("%d sensors found:\n", count);
	for (int i = 0; i < count; i++) {
		if (show) {
			printf("%s\n"
			       "\tvendor: %s\n"
			       "\tversion: %d\n"
			       "\thandle: %d\n"
			       "\ttype: %s\n"
			       "\tmaxRange: %f\n"
			       "\tresolution: %f\n"
			       "\tpower: %f mA\n",
			       list[i].name,
			       list[i].vendor,
			       list[i].version,
			       list[i].handle,
			       getSensorName(list[i].type),
			       list[i].maxRange, list[i].resolution,
			       list[i].power);
		} else {
			printf("sensor_type: %s ::  %s\n",
			       getSensorName(list[i].type), list[i].name);
		}
	}
	return 0;
}

static int activate_sensors(int id, int delay, int opt)
{
	int err;
	err = device->activate(device, list[id].handle, 0);
	if (err != 0) {
		printf("deactivate() for '%s'failed (%s)\n",
		       list[id].name, strerror(-err));
		return 0;
	}
	if (!opt) {
		return 0;
	}
	err = device->activate(device, list[id].handle, 1);
	if (err != 0) {
		printf("activate() for '%s'failed (%s)\n",
		       list[id].name, strerror(-err));
		return 0;
	}
	device->setDelay(device, list[id].handle, ms2ns(delay));
	return err;
}

static int do_read(int argc, char **argv)
{
	int err;
	int opt;
	int delay = 10;
	char *type = NULL;
	int type_num = 0;
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];

	while ((opt = getopt(argc, argv, "t:d:")) != -1) {
		switch (opt) {
		case 't':
			type = optarg;
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		default:
			return -EINVAL;
		}
	}

	if (optind < argc) {
		return -EINVAL;
	}

	for (int i = 1; i <= SENSOR_TYPE_AMBIENT_TEMPERATURE; i++) {
		if (strcmp(type, getSensorName(i)) == 0)
			type_num = i;
	}
	printf("do_read: type'%s-%d' delay (%d)\n", type, type_num, delay);

	/*********activate_sensors(ON)***************/
	for (int i = 0; i < count; i++) {
		err = activate_sensors(i, 1, S_ON);
		if (err != 0) {
			printf("activate_sensors(ON) for '%s'failed (%s)\n",
			       list[i].name, strerror(-err));
			return 0;
		}
	}
	do {
		int n = device->poll(device, buffer, numEvents);
		if (n < 0) {
			printf("poll() failed (%s)\n", strerror(-err));
			break;
		}

		/*printf("read %d events:\n", n); */
		for (int i = 0; i < n; i++) {
			const sensors_event_t & data = buffer[i];

			if (data.version != sizeof(sensors_event_t)) {
				printf
				    ("incorrect event version (version=%d, expected=%d",
				     data.version, sizeof(sensors_event_t));
				break;
			}

			if (type_num == data.type)
				printf
				    ("sensor=%s, time=%lld, value=<%5.1f,%5.1f,%5.1f>\n",
				     getSensorName(data.type), data.timestamp,
				     data.data[0], data.data[1], data.data[2]);
		}
	} while (1);

	/*********activate_sensors(OFF)***************/
	for (int i = 0; i < count; i++) {
		err = activate_sensors(i, NULL, S_OFF);
		if (err != 0) {
			printf("activate_sensors(OFF) for '%s'failed (%s)\n",
			       list[i].name, strerror(-err));
			return 0;
		}
	}
	return err;
}

static void usage(void)
{
	printf("Usage:\n");
	printf("  utest_sensor list [-a]\n");
	printf("  utest_sensor read [-t sensor_type] [-d delay]\n");
	printf
	    ("  utest_sensor verify [-t sensor_type] [-d delay] [-o verify_option]\n");
}

int main(int argc, char **argv)
{
	int err;
	char *cmd;
	int rval = -EINVAL;

	if (argc < 2) {
		usage();
		return rval;
	}

	cmd = argv[1];
	argc--;
	argv++;
	printf("--------------------------------------------------------\n");
	printf("utest_sensor -- %s\n", cmd);

	/*********load sensor.so***************/
	err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
			    (hw_module_t const **)&module);
	if (err != 0) {
		printf("hw_get_module() failed (%s)\n", strerror(-err));
		return 0;
	}
	/*********open sensor***************/
	err = sensors_open(&module->common, &device);
	if (err != 0) {
		printf("sensors_open() failed (%s)\n", strerror(-err));
		return 0;
	}
	/*********read cmd***************/
	count = module->get_sensors_list(module, &list);

	if (strcmp(cmd, "list") == 0) {
		rval = show_sensor_info(argc, argv);
	} else if (strcmp(cmd, "read") == 0) {
		rval = do_read(argc, argv);
	} else if (strcmp(cmd, "verify") == 0) {
		rval = 0;
	}
	/*********cmd worng***************/
	if (rval == -EINVAL) {
		usage();
	}

	printf("--------------------------------------------------------\n");
	/*********close sensor***************/
	err = sensors_close(device);
	if (err != 0) {
		printf("sensors_close() failed (%s)\n", strerror(-err));
	}
	return rval;
}
