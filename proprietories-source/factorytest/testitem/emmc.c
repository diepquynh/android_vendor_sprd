#include "testitem.h"

int test_emmc_start(void)
{
	struct statfs fs;
	int fd;
	int ret = RL_FAIL; //fail
	int cur_row = 2;
	char  buffer[64]={0},temp[64]={0};
	int read_len = 0;
	float size = 0;
	char *endptr;

	ui_fill_locked();
	ui_show_title(MENU_TEST_EMMC);
	gr_flip();
	if (0 == access("/sys/block/mmcblk0",F_OK)){
		sprintf(temp,"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_OK);
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, temp);
		gr_flip();
		fd = open("/sys/block/mmcblk0/size",O_RDONLY);
		if(fd < 0){
			goto TEST_END;
		}
		read_len = read(fd,buffer,sizeof(buffer));
		if(read_len <= 0){
			goto TEST_END;
		}

		size = strtoul(buffer,&endptr,0);
		close(fd);
		LOGD("sys/block/mmcblk0/size value = %f, read_len = %d ", size, read_len);
		sprintf(temp, "%s %4.2f GB", TEXT_EMMC_CAPACITY,(size/2/1024/1024));
		cur_row = ui_show_text(cur_row, 0, temp);
		gr_flip();
		ret = RL_PASS;
	}else{
		sprintf(temp,"%s%s",TEXT_EMMC_STATE,TEXT_EMMC_FAIL);
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, temp);
		gr_flip();
	}

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

	save_result(CASE_TEST_EMMC,ret);
	return ret;
}
