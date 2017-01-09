#include "testitem.h"

#define S_ON	1
#define S_OFF	0

struct sensors_poll_device_t *device;
struct sensors_module_t *module;
struct sensor_t const *list;
int count = 0;

int get_sensor_name(const char * name )
{
    int fd = -1;
    int i = 0;
    char devName[32] = { 0 };
    char EvtName[32] = { 0 };
    struct stat stt;

    int EvtNameLen = strlen(INPUT_EVT_NAME);
    strcpy(EvtName, INPUT_EVT_NAME);
    EvtName[EvtNameLen + 1] = 0;

    for(i = 0; i < 16; ++i ) {
        EvtName[EvtNameLen] = (char)('0' + i);
        LOGD("input evt name = %s", EvtName);

        if( stat(EvtName, &stt) != 0 ) {
            LOGE("stat '%s' error!",EvtName);
            break;
        }

        fd = open(EvtName, O_RDONLY);
        if( fd < 0 ) {
            LOGE("Failed to open %s", EvtName);
            continue;
        }

        if( ioctl(fd, EVIOCGNAME(sizeof(devName)), devName) > 0 &&  strstr(devName, name) != NULL ) {
            LOGD("open '%s' OK", devName);
            break;
        }

        LOGD("input evt name = %s, dev name = %s", EvtName, devName);
        close(fd);
        fd = -1;
    }

    if( fd >= 0 ) {
        if( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 ) {
            LOGE("fcntl: set to nonblock error!");
        }
    }

    return fd;
}

int find_input_dev(int mode, const char *event_name)
{
	int fd = -1;
	int ret = -1;
	const char *dirname = "/dev/input";
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;
	char name[128];

	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
				(de->d_name[0] == '.' && de->d_name[1] == '.'  &&
				 de->d_name[2] == '\0')) {
			/* ignore .(current) and ..(top) directory */
			continue;
		}
		strcpy(filename, de->d_name);
		fd = open(devname, mode);

		if (fd >= 0) {
			memset(name, 0, sizeof(name));
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 0) {

			} else {
				if (!strcmp(name, event_name)) {
					ret = fd; //get the sensor name from the event
					goto END;
				}
			}
			close(fd);
		}
	}
END:
	closedir(dir);

	return ret;
}

static int activate_sensors(int id, int delay, int opt)
{
	int err;
	err = device->activate(device, list[id].handle, 0);
	if (err != 0) {
		LOGD("mmitest deactivate() for '%s'failed",list[id].name);
		return 0;
	}
	if (!opt) {
		return 0;
	}
	err = device->activate(device, list[id].handle, 1);
	if (err != 0) {
		LOGE("mmitest activate() for '%s'failed ",list[id].name);
		return 0;
	}
	device->setDelay(device, list[id].handle, ms2ns(delay));
	return err;
}

int sensor_enable()
{
	int i,err;

	for ( i = 0; i < count; i++) {
		err = activate_sensors(i, 1, S_ON);
		LOGD("activate_sensors(ON) for '%s'", list[i].name);
		if (err != 0) {
			LOGE("activate_sensors(ON) for '%s'failed", list[i].name);
			return 0;
		}
	}

	return err;
}

int sensor_stop()
{
	int i,err;

	/*********activate_sensors(OFF)***************/
	for (i = 0; i < count; i++) {
		err = activate_sensors(i, 0, S_OFF);
		if (err != 0) {
			LOGE("activate_sensors(OFF) for '%s'failed", list[i].name);
			return 0;
		}
	}
	/*********close sensor***************/
	err = sensors_close(device);
	if (err != 0) {
		LOGE("mmitest sensors_close() failed");
	}
	return err;
}

int sensor_start()
{
	int err;
	/*********load sensor.so***************/
	err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
			    (hw_module_t const **)&module);
	if (err != 0) {
		LOGD("mmitest hw_get_module() failed (%s)", strerror(-err));
		return 0;
	}
	/*********open sensor***************/
	err = sensors_open(&module->common, &device);
	if (err != 0) {
		LOGD("mmitest sensors_open() failed (%s)", strerror(-err));
		return 0;
	}
	/*********read cmd***************/
	count = module->get_sensors_list(module, &list);

	return err;
}
