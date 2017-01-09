
#include "common.h"
#include <linux/android_alarm.h>

void self_inspection_apr_enabled()
{
	char value[PROPERTY_VALUE_MAX];
	char *default_value = (char*)"unknown";
	property_get("persist.sys.apr.enabled", value, default_value);
	if (!strcmp(value, "0")) {
		exit(0);
	} else if (!strcmp(value, "unknown")) {
//		property_set("persist.sys.apr.enabled", "1");
	}
}

int64_t getdate(char* strbuf, size_t max, const char* format)
{
	struct tm tm;
	time_t t;

	tzset();

	time(&t);
	localtime_r(&t, &tm);
	strftime(strbuf, max, format, &tm);

	return t;
}

int64_t uptime(char* strbuf)
{
	struct timespec ts;
	int fd, result;

	fd = open("/dev/alarm", O_RDONLY);
	if (fd < 0)
		return fd;

	result = ioctl(fd, ANDROID_ALARM_GET_TIME(ANDROID_ALARM_ELAPSED_REALTIME), &ts);
	close(fd);

	if (result == 0) {
		if (strbuf) {
			sprintf(strbuf, "%d", ts.tv_sec);
		}
		return ts.tv_sec;
	}

	return -1;
}

