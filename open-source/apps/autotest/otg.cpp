#include <stdlib.h>
#include <fcntl.h>
#include "otg.h"
#include <string.h>

#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF

#include "debug.h"

#define IS_SUPPORT_OTG_PATH "/sys/bus/platform/drivers/dwc_otg/is_support_otg"
#define HOST_ENABLE_PATH "/sys/devices/20200000.usb/host_enable"
#define OTG_STATUS_PATH "/sys/devices/20200000.usb/otg_status"

int otg_is_support(void)
{
	char is_support_otg[2] = {0};
	int is_support_otg_fd = 0;
	int ret = 0;

	is_support_otg_fd = open(IS_SUPPORT_OTG_PATH, O_RDONLY);
	read(is_support_otg_fd, is_support_otg, 2);
	if(strncmp(is_support_otg, "1", sizeof("1") - 1) == 0)
	{
		ret = 1;
		DBGMSG("otg is support");
	}
	else
	{
		ret = 0;
		DBGMSG("otg is not support");
	}
	return ret;
}

int otgEnable(void)
{
	char host_enable[8] = {0};
	int host_enable_fd = 0;
	int ret = 0;
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		system("echo enable > /sys/devices/20200000.usb/host_enable");

		host_enable_fd = open(HOST_ENABLE_PATH, O_RDONLY);
		read(host_enable_fd, host_enable, 8);
		if(strncmp(host_enable, "enabled", sizeof("enabled") - 1) == 0)
		{
			ret = 0;
		}
		else
		{
			ret = -1;
		}
		close(host_enable_fd);
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}

int otgDisable(void)
{
	char host_enable[8] = {0};
	int host_enable_fd = 0;
	int ret = 0;
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		system("echo disable > /sys/devices/20200000.usb/host_enable");

		host_enable_fd = open(HOST_ENABLE_PATH, O_RDONLY);
		read(host_enable_fd, host_enable, 8);
		if(strncmp(host_enable, "disabled", sizeof("disabled") - 1) == 0)
		{
			ret = 0;
		}
		else
		{
			ret = -1;
		}
		close(host_enable_fd);
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}

int otgIdStatus(void)
{
	char otg_status[8] = {0};
	int otg_status_fd = 0;
	int ret = 0;
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		otg_status_fd = open(OTG_STATUS_PATH, O_RDONLY);
		if(otg_status_fd < 0)
		{
			ret = -1;
		}
		else
		{
			memset(otg_status, 0, sizeof(otg_status));
			read(otg_status_fd, otg_status, 4);
			DBGMSG("otg_status = %s", otg_status);
			if(strncmp(otg_status, "high", sizeof("high") - 1) == 0)
			{
				ret = 1;
			}
			else if(strncmp(otg_status, "low", sizeof("low") - 1) == 0)
			{
				ret = 0;
			}
			close(otg_status_fd);
		}
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}

int otgVbusOpen(void)
{
	int ret = 0;
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		system("echo on > /sys/devices/20200000.usb/vbus_control");
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}

int otgVbusClose(void)
{
	int ret = 0;
	FUN_ENTER;
	if(otg_is_support() == 1)
	{
		system("echo off > /sys/devices/20200000.usb/vbus_control");
	}
	else
	{
		ret = -1;
	}
	FUN_EXIT;
	return ret;
}
