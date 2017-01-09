/*
 *  iqfeed.cpp - The main function of WCDMA I/Q data feed.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-9-22 Zhang Ziyi
 *  Initial version.
 */
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "iqfeed.h"
#include "wiq_drv_api.h"

static void wait_ext_sd_mount(int sec,const android::String8& dir)
{
	int n = 0;

	while (n < sec) {
		if (get_sd_state(dir)) {
			info_log("external SD card mounted");
			break;
		}
		sleep(2);
		n += 2;
	}
}

static bool get_wiq_file_dir(android::String8& dir)
{
	bool ret = get_sd_root(dir);

	if (ret) {
		dir += "/";
	}

	return ret;
}

static bool get_iq_file_path(android::String8& fpath,
			     const android::String8& dir,
			     const char* name,
			     size_t len)
{
	// Check the file name
	size_t i;

	for (i = 0; i < len; ++i) {
		if ('\0' == name[i]) {
			break;
		}
	}
	if (len == i) {
		return false;
	}

	fpath = dir;
	fpath.append(name, i);

	return true;
}

static void print_buf(const uint8_t* buf, size_t len)
{
	if (len > 16) {
		len = 16;
	}

	info_log("Data at %p", buf);

	char data_str[64];
	char* data_start = data_str;
	for (size_t i = 0; i < len; ++i) {
		int n = sprintf(data_start, " %02X", buf[i]);
		data_start += n;
	}
	info_log("%s", data_str);
}

static int feed_iq(int iq_file, iq_pb_data_header& dhdr,
		   int iqdev, void* buf, size_t len)
{
	int ret = -1;
	size_t copy_len;
	struct stat iq_stat;

	ret = fstat(iq_file, &iq_stat);
	if (-1 == ret) {
		err_log("get I/Q file size error");
		return -1;
	}

	// Big file
	if (iq_stat.st_size > len - WCDMA_IQ_BUF_HEADER_LEN) {
		ret = lseek(iq_file, dhdr.iqdata_offset, SEEK_SET);
		if (!ret) {
			copy_len = dhdr.iqdata_length;
		} else {
			err_log("set I/Q file offset error");
		}
	} else {
		// Feed all data in the file
		copy_len = iq_stat.st_size;
	}

	// Copy data
	if (!ret) {
		size_t copied = 0;
		size_t remain;
		uint8_t* buf_start = reinterpret_cast<uint8_t*>(buf);

		// Skip the header
		buf_start += WCDMA_IQ_BUF_HEADER_LEN;
		if (copy_len + WCDMA_IQ_BUF_HEADER_LEN > len) {
			copy_len = len - WCDMA_IQ_BUF_HEADER_LEN;
		}
		remain = copy_len;
		while (copied < copy_len) {
			ssize_t n = read(iq_file, buf_start + copied,
					 remain);
			if (n <= 0) {
				ret = -1;
				break;
			}
			info_log("read I/Q file: %u",
				 static_cast<unsigned>(n));
			copied += n;
			remain -= n;
		}

		if (!ret) {
			dhdr.data_status = DATA_AP_MOVE_FINISH;
			dhdr.iqdata_length = copy_len;
			ret = ioctl(iqdev, CMD_SET_IQ_MOVE_FINISHED,
				    &dhdr);
		}
	}

	return ret;
}

static int map_iq_buffer(int iqdev, void*& iq_buf, size_t& iq_len)
{
	int ret = -1;

	iq_buf = mmap(0, WCDMA_IQ_BUF_SIZE, PROT_READ | PROT_WRITE,
		      MAP_SHARED, iqdev, 0);
	if (MAP_FAILED != iq_buf) {
		iq_len = WCDMA_IQ_BUF_SIZE;
		ret = 0;
	}

	return ret;
}

int main(int /*argc*/, char** /*argv*/)
{
	info_log("UID=%u/%u, GID=%u/%u",
		 static_cast<unsigned>(getuid()),
		 static_cast<unsigned>(geteuid()),
		 static_cast<unsigned>(getgid()),
		 static_cast<unsigned>(getegid()));

	// Get WCDMA I/Q file directory
	android::String8 wiq_file_dir;

	if (!get_wiq_file_dir(wiq_file_dir)) {
		err_log("get WCDMA I/Q file directory error");
		return 3;
	}

	void* iq_buf;
	size_t iq_len;
	int iqdev = open("/dev/iq_mem", O_RDWR);

	if (-1 == iqdev) {
		err_log("can not open /dev/iq_mem");
		return 1;
	}

	// Map the shared memory
	if (map_iq_buffer(iqdev, iq_buf, iq_len) < 0) {
		close(iqdev);
		err_log("map I/Q buffer error");
		return 2;
	}
	info_log("I/Q shared buffer %p/%u", iq_buf,
		 static_cast<unsigned>(iq_len));

	// Set WCDMA I/Q playback mode
	int mode = PLAY_BACK_MODE;
	if (-1 == ioctl(iqdev, CMD_SET_IQ_CH_TYPE, mode)) {
		close(iqdev);
		err_log("set WCDMA I/Q playback mode error");
		return 4;
	}

	// Wait for external SD card to be mounted
	wait_ext_sd_mount(30, wiq_file_dir);

	android::String8 iq_file_path;
	struct pollfd pol_iq;

	pol_iq.fd = iqdev;
	pol_iq.events = POLLIN;
	while (true) {
		// Wait for the file name
		pol_iq.revents = 0;
		int n = poll(&pol_iq, 1, -1);
		if (n <= 0) {
			err_log("poll error");
			continue;
		}
		// Get request parameters
		iq_pb_data_header pb_param;
		if (-1 == ioctl(iqdev, CMD_GET_IQ_PB_INFO, &pb_param)) {
			err_log("get WCDMA I/Q playback parameters error");
			continue;
		}
		info_log("I/Q request: %s, %u, %u",
			 pb_param.iqdata_filename,
			 static_cast<unsigned>(pb_param.iqdata_offset),
			 static_cast<unsigned>(pb_param.iqdata_length));
		// Get file path name
		if (!get_iq_file_path(iq_file_path, wiq_file_dir,
				      pb_param.iqdata_filename,
				      MAX_CHAR_NUM)) {
			err_log("get I/Q file path error");
			continue;
		}
		// Open I/Q file
		int iq_file = open(iq_file_path.string(), O_RDONLY);
		if (-1 == iq_file) {
			err_log("open I/Q file %s error",
				iq_file_path.string());
			continue;
		}

		info_log("I/Q file %s opened",
			 iq_file_path.string());
		if (feed_iq(iq_file, pb_param, iqdev, iq_buf, iq_len) < 0) {
			err_log("feed I/Q error");
		}

		close(iq_file);
	}

	close(iqdev);

	return 0;
}
