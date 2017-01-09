#include "testitem.h"

static int sdcard_rw(void)
{
	int fd;
	int ret = -1;
	int i = 0;
	char *p;
	unsigned char w_buf[RW_LEN];
	unsigned char r_buf[RW_LEN];
	char external_path[MAX_NAME_LEN];
	char sdcard_testfile[MAX_NAME_LEN];

	memset(external_path, 0, MAX_NAME_LEN);
	p = getenv("SECONDARY_STORAGE");
	if(p == NULL)
		p = getenv("EXTERNAL_STORAGE");
	if(p == NULL){
		LOGE("mmitest Can't find the external storage environment");
	}
	strncpy(external_path, p, MAX_NAME_LEN-1);
	sprintf(sdcard_testfile, "%s/test.txt",external_path);
	LOGD("mmitest the sdcard_testfile : %s", sdcard_testfile);

	for(i = 0; i < RW_LEN; i++) {
		w_buf[i] = 0xff & i;
	}

	fd = open(sdcard_testfile, O_CREAT|O_RDWR, 0666);
	if(fd < 0){
		LOGE("create %s failed", sdcard_testfile);
		goto RW_END;
	}

	if(write(fd, w_buf, RW_LEN) != RW_LEN){
		LOGE("write data failed");
		goto RW_END;
	}

	lseek(fd, 0, SEEK_SET);
	memset(r_buf, 0, sizeof(r_buf));

	read(fd, r_buf, RW_LEN);
	if(memcmp(r_buf, w_buf, RW_LEN) != 0) {
		LOGE("read data failed");
		goto RW_END;
	}

	ret = 0;
RW_END:
	if(fd > 0) close(fd);
	return ret;
}

int test_sdcard_pretest(void)
{
	int fd;
	int ret;
	system(SPRD_MOUNT_DEV);
	fd = open(SPRD_SD_DEV, O_RDWR);
	if(fd < 0) {
		ret= RL_FAIL;
	} else {
		close(fd);
		ret= RL_PASS;
	}

	save_result(CASE_TEST_SDCARD,ret);
	return ret;
}

int test_sdcard_start(void)
{
	int fd_dev = -1,fd1_size = -1,fd_mmc = -1;
	int ret = RL_FAIL; //fail
	int cur_row = 2;
	int wait_cnt = 0;
	char temp[64],buffer[64];
	int read_len = 0;
	unsigned long value=0;
	char *endptr;

	ui_fill_locked();
	ui_show_title(MENU_TEST_SDCARD);
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SD_START);
	gr_flip();
	fd_dev = open(SPRD_SD_DEV, O_RDWR);
	if(fd_dev < 0) {
		LOGE("open %s failed",SPRD_SD_DEV);
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_OPEN_FAIL);
		gr_flip();
		goto TEST_END;
	}
	ui_set_color(CL_GREEN);
	cur_row = ui_show_text(cur_row, 0, TEXT_SD_OPEN_OK);
	gr_flip();
	if( mount(SPRD_SD_DEV, "/storage/sdcard0", "vfat", 0, NULL) < 0 )
            LOGE("%s mount failed",SPRD_SD_DEV);

	fd1_size = open(SPRD_SD_DEV_SIZE,O_RDONLY);
	if(fd1_size < 0) {
		LOGE("open %s failed",SPRD_SD_DEV_SIZE);
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_STATE_FAIL);
		gr_flip();
		goto TEST_END;
	}
	read_len = read(fd1_size,buffer,sizeof(buffer));
	if(read_len <= 0){
		LOGE("read %s failed,read_len=%d",SPRD_SD_DEV_SIZE,read_len);
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_STATE_FAIL);
		gr_flip();
		goto TEST_END;
	}

	value = strtoul(buffer,&endptr,0);
	LOGD("%s value = %lu, read_len = %d",SPRD_SD_DEV_SIZE, value, read_len);
	ui_set_color(CL_GREEN);
	cur_row = ui_show_text(cur_row, 0, TEXT_SD_STATE_OK);
	sprintf(temp, "%ld MB", (value/2/1024));
	cur_row = ui_show_text(cur_row, 0, temp);
	gr_flip();
	if(sdcard_rw()< 0) {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_RW_FAIL);
		gr_flip();
		goto TEST_END;
	} else {
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_RW_OK);
		gr_flip();
	}

	ret = RL_PASS;
TEST_END:
	if(ret == RL_PASS) {
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, TEXT_TEST_PASS);
	} else {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_TEST_FAIL);
	}
	gr_flip();
	sleep(1);
	if(fd_dev >= 0) close(fd_dev);
	if(fd1_size >= 0) close(fd1_size);
	if(fd_mmc >= 0) close(fd_mmc);
	save_result(CASE_TEST_SDCARD,ret);
	return ret;
}
