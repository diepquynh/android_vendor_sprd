#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>
#include <fcntl.h>
#include "modem_control.h"
#include "packet.h"
#include "nv_read.h"
#ifdef SECURE_BOOT_ENABLE
#include "modem_verify.h"
#endif

#define BM_DEV "/dev/sprd_bm"

#define MODEM_MAGIC		"SCI1"
#define MODEM_HDR_SIZE		12 //size of a block
#define MODEM_IMG_HDR		0x1
#define MODEM_LAST_HDR		0x100
#define MODEM_SHA1_HDR		0x400
#define MODEM_SHA1_SIZE		20

typedef struct {
	uint32_t type_flags;
	uint32_t offset;
	uint32_t length;
} data_block_header_t;

enum sci_bm_cmd_index {
	BM_STATE = 0x0,
	BM_CHANNELS,
	BM_AXI_DEBUG_SET,
	BM_AHB_DEBUG_SET,
	BM_PERFORM_SET,
	BM_PERFORM_UNSET,
	BM_OCCUR,
	BM_CONTINUE_SET,
	BM_CONTINUE_UNSET,
	BM_DFS_SET,
	BM_DFS_UNSET,
	BM_PANIC_SET,
	BM_PANIC_UNSET,
	BM_BW_CNT_START,
	BM_BW_CNT_STOP,
	BM_BW_CNT_RESUME,
	BM_BW_CNT,
	BM_BW_CNT_CLR,
	BM_DBG_INT_CLR,
	BM_DBG_INT_SET,
	BM_CMD_MAX,
};

static int assert_fd = 0;
int server_modem_ctrl=-1;
int client_modemd=-1;
sem_t sem_first_boot;
static bool g_b_wake_locking = false;
static const char s_reset_cmd[2] = {0x7a, 0x0a};
static char diag_chan[256]={0}, log_chan[256]={0}, mkbuf[256]={0};

#ifdef SECURE_BOOT_ENABLE
uint8_t s_modem_puk[PUBKEY_LEN] = {0};
int     s_is_shutdown = -1;

int HexToBin(const char *hex_ptr, int length, char *bin_ptr)
{
    char *dest_ptr = bin_ptr;
    int i;
    char ch;

    if (hex_ptr == NULL || bin_ptr == NULL) {
        return -1;
    }

    for(i = 0; i < length; i += 2) {
        ch = hex_ptr[i];
        if(ch >= '0' && ch <= '9')
            *dest_ptr = (char)((ch - '0') << 4);
        else if(ch >= 'a' && ch <= 'f')
            *dest_ptr = (char)((ch - 'a' + 10) << 4);
        else if(ch >= 'A' && ch <= 'F')
            *dest_ptr = (char)((ch - 'A' + 10) << 4);
        else
            return -1;

        ch = hex_ptr[i+1];
        if(ch >= '0' && ch <= '9')
            *dest_ptr |= (char)(ch - '0');
        else if(ch >= 'a' && ch <= 'f')
            *dest_ptr |= (char)(ch - 'a' + 10);
        else if(ch >= 'A' && ch <= 'F')
            *dest_ptr |= (char)(ch - 'A' + 10);
        else
            return -1;

        dest_ptr++;
    }
    return 0;
}

static int modem_parse_puk_cmdline(uint8_t *puk_ptr)
{
    int fd = 0,ret = 0, i = 0,flag = 0;
    char cmdline[CMDLINE_LENGTH] = {0};
    char puk_str[PUBKEY_LEN*2+1] = {0};
    char* p_str = NULL;

    // Read PUK from cmdline
    fd = open("/proc/cmdline", O_RDONLY);
    if (fd < 0) {
        MODEM_LOGD("[secure]%s, /proc/cmdline open failed", __FUNCTION__);
        return 0;
    }
    ret = read(fd, cmdline, sizeof(cmdline));
    if (ret < 0) {
        MODEM_LOGD("[secure]%s,/proc/cmdline read failed", __FUNCTION__);
        close(fd);
        return 0;
    }
    MODEM_LOGD("[secure]%s,cmdline: %s\n", __FUNCTION__, cmdline);
    p_str = strstr(cmdline, CMD_PUKSTRING);
    if (p_str != NULL) {
        p_str += strlen(CMD_PUKSTRING);
        memcpy(puk_str, p_str, PUBKEY_LEN*2);
        MODEM_LOGD("[secure]%s, puk_str = %s\n", __FUNCTION__, puk_str);
        HexToBin(puk_str, PUBKEY_LEN*2, puk_ptr);
        flag = 1;
    } else {
        MODEM_LOGD("[secure]%s, parse puk failed", __FUNCTION__);
    }
    return flag;
}

static int modem_verify_image(char *fin, int offsetin, int size)
{
    int ret = 0;
    int fdin = -1, readsize = 0, imagesize = 0;
    uint8_t *buf = NULL;

    MODEM_LOGD("[secure]%s: enter", __FUNCTION__);
    MODEM_LOGD("[secure]%s: fin = %s, size = %d", __FUNCTION__, fin, size);
    // Read image
    fdin = open(fin, O_RDONLY, 0);
    if (fdin < 0) {
        MODEM_LOGE("[secure]%s: Failed to open %s", __FUNCTION__, fin);
        return -1;
    }
    if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
        MODEM_LOGE("[secure]failed to lseek %d in %s", offsetin, fin);
        ret = -1;
        goto leave;
    }
    if (L_TGDSP_SIZE == size) {
        imagesize = size + KEY_INFO_SIZ + VLR_INFO_SIZ;
    } else {
        imagesize = size;
    }
    MODEM_LOGD("[secure]%s: imagesize = %d", __FUNCTION__, imagesize);
    buf = malloc(imagesize);
    if (buf == 0) {
        MODEM_LOGE("[secure]%s: Malloc failed!!", __FUNCTION__);
        ret = -1;
        goto leave;
    }
    memset(buf, 0, imagesize);
    readsize = read(fdin, buf, imagesize);
    MODEM_LOGD("[secure]%s: buf readsize = %d", __FUNCTION__, readsize);
    if (readsize <= 0) {
        MODEM_LOGE("[secure]failed to read %s", fin);
        ret = -1;
        goto leave;
    }
    // Start verify
    secure_verify(buf, s_modem_puk);
    ret = 0;
leave:
    close(fdin);
    free(buf);
    return ret;
}

#endif

static uint32_t get_boot_offset(IMAGE_LOAD_S* table, uint32_t secure_offset)
{
	int fdin = -1;
	uint32_t offset = 0, hdr_offset = 0;
	uint8_t hdr_buf[MODEM_HDR_SIZE << 3] = {0};
	uint8_t read_len;
	uint8_t result = 0; // 0:OK, 1:not find, 2:some error occur
	data_block_header_t* hdr_ptr = NULL;

	if(strstr(table->path_r, "modem")) {
		fdin = open(table->path_r, O_RDONLY, 0);
		if(fdin < 0) {
			return offset;
		}

		do {
			if (lseek(fdin, hdr_offset + secure_offset, SEEK_SET) != (hdr_offset + secure_offset)) {
				result = 2;
				break;
			}

			read_len = read(fdin, hdr_buf, sizeof(hdr_buf));
			if(read_len != sizeof(hdr_buf))	{ // image size should be > (MODEM_HDR_SIZE << 3)
				result = 2;
				break;
			}

			if(!hdr_offset) {
				if(memcmp(hdr_buf, MODEM_MAGIC, sizeof(MODEM_MAGIC))) {
					result = 2;
					MODEM_LOGD("old image format!\n");
					break;
				}

				hdr_ptr = (data_block_header_t*)hdr_buf + 1;
				hdr_offset = MODEM_HDR_SIZE;
			} else {
				hdr_ptr = (data_block_header_t*)hdr_buf;
			}

			while(!(hdr_ptr->type_flags & MODEM_IMG_HDR)) {
				hdr_ptr ++;
				hdr_offset += MODEM_HDR_SIZE;
				if(read_len == ((uint8_t*)hdr_ptr - hdr_buf)) {
					result = 1;
					break;
				}

				if(hdr_ptr->type_flags & MODEM_LAST_HDR) {
					result = 2;
					MODEM_LOGE("no modem image, error image header!!!\n");
					break;
				}
			};

			if(result != 1) {
				break;
			}

		} while(1);

		if(!result) {
			offset = hdr_ptr->offset;
			if(hdr_ptr->type_flags & MODEM_SHA1_HDR) {
				offset += MODEM_SHA1_SIZE;
			}
		}

		if(fdin >= 0)
			close(fdin);
	}

	return offset;
}


int write_proc_file(char *file, int offset, char *string)
{
	int fd, stringsize, res = -1, retry = 0;

	do{
		fd = open(file, O_RDWR);
		if (fd < 0) {
			MODEM_LOGE("fd =%d: open file %s, error: %s", fd, file, strerror(errno));
			usleep(200000);
			retry++;
		}
	}while(fd < 0 && retry < 6);

	if(fd < 0) {
		MODEM_LOGE("Cant open file %s, error :%s", file, strerror(errno));
		goto leave;
	}

	if (lseek(fd, offset, SEEK_SET) != offset) {
		MODEM_LOGE("Cant lseek file %s, error :%s", file, strerror(errno));
		goto leave;
	}

	stringsize = strlen(string);
	if (write(fd, string, stringsize) != stringsize) {
		MODEM_LOGE("Could not write %s in %s, error :%s", string, file, strerror(errno));
		goto leave;
	}

	res = 0;
leave:
	if(fd >= 0) {
		close(fd);
	}
	return res;
}

void modemd_enable_busmonitor(bool bEnable)
{
	int fd;
	int param;
	int cmd;

	fd = open(BM_DEV, O_RDWR);
	if (fd < 0) {
		MODEM_LOGD("modemd_enable_busmonitor %s failed, error: %s", BM_DEV, strerror(errno));
		return;
	}

	cmd = bEnable ? BM_DBG_INT_SET : BM_DBG_INT_CLR;
	ioctl(fd, cmd, &param);

	MODEM_LOGD("modemd_enable_busmonitor bEnable = %d, cmd = %d", bEnable, cmd);
	close(fd);
}


int load_sipc_image(char *fin, int offsetin, char *fout, int offsetout, int size)
{
	int res = -1, fdin, fdout, bufsize, i, rsize, rrsize, wsize;
	char buf[8192];
	int buf_size = sizeof(buf);

	MODEM_LOGD("%s (%s.%d ==> %s.%d size=0x%x)\n",__func__,
			 fin, offsetin, fout, offsetout, size);
	modemd_enable_busmonitor(false);

	fdin = open(fin, O_RDONLY, 0);
	if (fdin < 0) {
	        MODEM_LOGE("failed to open %s", fin);
	        modemd_enable_busmonitor(true);
	        return -1;
	}

	fdout = open(fout, O_WRONLY, 0);
	if (fdout < 0) {
		close(fdin);
		MODEM_LOGE("failed to open %s, error: %s", fout, strerror(errno));
		modemd_enable_busmonitor(true);
		return -1;
	}

	if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
		MODEM_LOGE("failed to lseek %d in %s", offsetin, fin);
		goto leave;
	}

	if (lseek(fdout, offsetout, SEEK_SET) != offsetout) {
		MODEM_LOGE("failed to lseek %d in %s", offsetout, fout);
		goto leave;
	}

	do{
		rsize = min(size, buf_size);
		rrsize = read(fdin, buf, rsize);
		if(rrsize == 0)
			goto leave;
		if (rrsize < 0) {
			MODEM_LOGE("failed to read %s %s", fin, strerror(errno));
			goto leave;
		}
		wsize = write(fdout, buf, rrsize);
		if (wsize <= 0) {
			MODEM_LOGE("failed to write %s [wsize = %d  rsize = %d  remain = %d]",
				fout, wsize, rsize, size);
			goto leave;
		}
		size -= rrsize;
	}while(size > 0);
	res = 0;
leave:
	modemd_enable_busmonitor(true);
	close(fdin);
	close(fdout);
	return res;
}

static int boot_from_table(LOAD_TABLE_S *tables,uint load_max)
{
	IMAGE_LOAD_S* tmp_table= &(tables->modem);
	uint load_cnt=0;
#ifdef SECURE_BOOT_ENABLE
	int secure_offset = BOOT_INFO_SIZE + VLR_INFO_SIZ;
#endif
	uint32_t normal_boot_offset = 0;

	for(;load_cnt<load_max;load_cnt++){
		if(tmp_table->size != 0){
#ifdef SECURE_BOOT_ENABLE
		MODEM_LOGD("[secure]%s: table cnt = %d is_secured = %d",
                       __func__, load_cnt, tmp_table->is_secured);
			if (1 == tmp_table->is_secured) {
				MODEM_LOGD("[secure]%s: s_is_shutdown = %d ", __func__, s_is_shutdown);
				normal_boot_offset = get_boot_offset(tmp_table, secure_offset);
				if (0 == s_is_shutdown) {
					MODEM_LOGD("[secure]verify start");
					modem_verify_image(tmp_table->path_r, 0, tmp_table->size + normal_boot_offset);
					MODEM_LOGD("[secure]verify done.");
				} else
					MODEM_LOGD("[secure]1st load, no need to verify");

				MODEM_LOGD("[secure]secure_offset = %d", secure_offset);
				load_sipc_image(tmp_table->path_r,secure_offset + normal_boot_offset,tmp_table->path_w,0,tmp_table->size);
			} else {
				normal_boot_offset = get_boot_offset(tmp_table, 0);
				load_sipc_image(tmp_table->path_r,normal_boot_offset,tmp_table->path_w,0,tmp_table->size);
			}
#else
			normal_boot_offset = get_boot_offset(tmp_table, 0);
			load_sipc_image(tmp_table->path_r,normal_boot_offset,tmp_table->path_w,0,tmp_table->size);
#endif
		} else
			MODEM_LOGD("image[%d].size=0\n",load_cnt);

		tmp_table++;
	}
	MODEM_LOGD("cnt =%d, from %s to %s, size=0x%x %d",load_cnt,tmp_table->path_r,tmp_table->path_w,tmp_table->size,load_max);
	return 0;
}

int load_sipc_modem_img(int modem, int pmic_is_shutdown)
{
	char path_read[128] = {0};
	char path_write[128] = {0};
	char persist_prop[128]={0};
	char dsp_partition[128] = {0};
	char cmdline_bank[128] = {"/proc/cmdline"};
	char alive_info[20]={0};
	char prop_nvp[128]={0};
	int i, ret;

	LOAD_TABLE_S * table_ptr= &(load_value.load_table);

	memset(&load_value,0,sizeof(load_value));

	system("chmod 666 /proc/cmdline");
	system("chown system:root /proc/cmdline");

	if ( -1 == property_get("ro.product.partitionpath", path_read, "") ) {
	        MODEM_LOGD("%s: get partitionpath fail",__func__);
	        return -1;
	}

	table_ptr->arm7.size = 0;
	if(access("/proc/cppmic",F_OK) == 0) {
		strcpy(table_ptr->arm7.path_w ,"/proc/cppmic/modem");
		strcpy(table_ptr->arm7.path_r,path_read);
		if (pmic_is_shutdown) {
			table_ptr->arm7.size = 0x8000;
			strcat(table_ptr->arm7.path_r,"pm_sys");
		}
		strcpy(load_value.arm7_start ,"/proc/cppmic/start");
		strcpy(load_value.arm7_stop ,"/proc/cppmic/stop");
	}

	strcpy(table_ptr->cmdline.path_r ,"/proc/cmdline");
	table_ptr->cmdline.size = CMDLINE_SIZE;
	strcpy(persist_prop,PERSIST_MODEM_CHAR);

	switch(modem){
	case TD_MODEM:
	{
		table_ptr->modem.size = TD_MODEM_SIZE;
		table_ptr->dsp.size = TD_DSP_SIZE;
		property_get(TD_PROC_PROP, path_write, "");
		strcat(strcpy(table_ptr->dsp.path_w,path_write), DSP_BANK);
		strcat(persist_prop, "t.nvp");
		property_get(persist_prop,prop_nvp,"");
		strcat(path_read,prop_nvp);
		if(0 == strlen(path_read)){
			MODEM_LOGD("invalid ro.modem.x.nvp path_char %s\n",path_read);
			return 0;
		}
		strcat(strcpy(table_ptr->dsp.path_r,path_read),"dsp");
	}
	break;
	case W_MODEM:
	{
		table_ptr->modem.size = W_MODEM_SIZE;
		table_ptr->dsp.size= W_DSP_SIZE;
		property_get(W_PROC_PROP, path_write, "");
		strcat(strcpy(table_ptr->dsp.path_w,path_write), DSP_BANK);
		strcat(persist_prop, "w.nvp");
		property_get(persist_prop,prop_nvp,"");
		strcat(path_read,prop_nvp);
		if(0 == strlen(path_read)){
			MODEM_LOGD("invalid ro.modem.x.nvp path_char %s\n",path_read);
			return 0;
		}
		strcat(strcpy(table_ptr->dsp.path_r,path_read),"dsp");
	}
	break;
	case LF_MODEM:
	{
		table_ptr->modem.size= LF_MODEM_SIZE;
		table_ptr->gdsp.size= LF_TGDSP_SIZE;
		table_ptr->ldsp.size= LF_LDSP_SIZE;
		table_ptr->warm.size= LF_WARM_SIZE;
		property_get(LF_PROC_PROP,path_write, "");
		strcat(strcpy(table_ptr->gdsp.path_w,path_write), TGDSP_BANK);
		strcat(strcpy(table_ptr->ldsp.path_w,path_write), LDSP_BANK);
		strcat(strcpy(table_ptr->warm.path_w,path_write), WARM_BANK);
		strcat(persist_prop, "lf.nvp");
		property_get(persist_prop,prop_nvp,"");
		strcat(path_read,prop_nvp);
		if(0 == strlen(path_read)){
			MODEM_LOGD("invalid ro.modem.x.nvp path_char %s\n",path_read);
			return 0;
		}
		strcat(strcpy(table_ptr->gdsp.path_r,path_read),"gdsp");
		strcat(strcpy(table_ptr->ldsp.path_r,path_read),"ldsp");
		strcat(strcpy(table_ptr->warm.path_r,path_read),"warm");
	}
	break;
	case LTE_MODEM:
	{
		table_ptr->modem.size= L_MODEM_SIZE;
		table_ptr->gdsp.size= L_TGDSP_SIZE;
		table_ptr->ldsp.size= L_LDSP_SIZE;
		table_ptr->warm.size= L_WARM_SIZE;
		property_get(LTE_PROC_PROP,path_write, "");
		strcat(strcpy(table_ptr->gdsp.path_w,path_write), TGDSP_BANK);
		strcat(strcpy(table_ptr->ldsp.path_w,path_write), LDSP_BANK);
		strcat(strcpy(table_ptr->warm.path_w,path_write), WARM_BANK);
		strcat(persist_prop, "l.nvp");
		property_get(persist_prop,prop_nvp,"");
		strcat(path_read,prop_nvp);
		if(0 == strlen(path_read)){
			MODEM_LOGD("invalid ro.modem.x.nvp path_char %s\n",path_read);
			return 0;
		}
		strcat(strcpy(table_ptr->gdsp.path_r,path_read),"gdsp");
		strcat(strcpy(table_ptr->ldsp.path_r,path_read),"ldsp");
		strcat(strcpy(table_ptr->warm.path_r,path_read),"warm");
    	}
	break;
	case TL_MODEM:
	{
		table_ptr->modem.size = TL_MODEM_SIZE;
		table_ptr->gdsp.size = TL_TGDSP_SIZE;
		table_ptr->ldsp.size = TL_LDSP_SIZE;
		property_get(LTE_PROC_PROP,path_write, "");
		strcat(strcpy(table_ptr->gdsp.path_w,path_write), TGDSP_BANK);
		strcat(strcpy(table_ptr->ldsp.path_w,path_write), LDSP_BANK);
		strcat(persist_prop, "tl.nvp");
		property_get(persist_prop,prop_nvp,"");
		strcat(path_read,prop_nvp);
		if(0 == strlen(path_read)){
			MODEM_LOGD("invalid ro.modem.x.nvp path_char %s\n",path_read);
			return 0;
        	}
		strcat(strcpy(table_ptr->gdsp.path_r,path_read),"gdsp");
		strcat(strcpy(table_ptr->ldsp.path_r,path_read),"ldsp");
    	}
	break;
	}
	strcat(strcpy(load_value.cp_start,path_write),MODEM_START);
	strcat(strcpy(load_value.cp_stop,path_write),MODEM_STOP);
	strcat(strcpy(table_ptr->modem.path_w,path_write), MODEM_BANK);
	strcat(strcpy(table_ptr->cmdline.path_w,path_write),CMDLINE_BANK);
	strcat(strcpy(table_ptr->modem.path_r,path_read),"modem");

#ifdef SECURE_BOOT_ENABLE
	MODEM_LOGD("[secure]%s: modem type = 0x%x", __func__, modem);
	table_ptr->modem.is_secured = 1;
	table_ptr->dsp.is_secured = 1;
	table_ptr->gdsp.is_secured = 1;
	table_ptr->ldsp.is_secured = 1;
	table_ptr->warm.is_secured = 1;
	table_ptr->arm7.is_secured = 1;
	MODEM_LOGD("[secure]%s: is_shutdown = 0x%x", __func__, pmic_is_shutdown);
	s_is_shutdown = pmic_is_shutdown;
	memset(s_modem_puk, 0, sizeof(s_modem_puk));
	ret = modem_parse_puk_cmdline(s_modem_puk);
	if (ret != 1) {
		MODEM_LOGD("[secure]%s: modem_parse_puk_cmdline failed!!!", __func__);
		return -1;
	}
#endif
	MODEM_LOGD("cp_stop= %s",load_value.cp_stop);
	/* write 1 to stop*/
	if(access("/proc/cppmic",F_OK) == 0){
		MODEM_LOGD("arm7_stop= %s",load_value.arm7_stop);
		if(pmic_is_shutdown)
		    	write_proc_file(load_value.arm7_stop, 0, "1");
	}	
	write_proc_file(load_value.cp_stop,0,"1");
	
	strcat(strcpy(load_value.nv1_read,path_read),FIXNV1_BANK);
	strcat(strcpy(load_value.nv2_read,path_read),FIXNV2_BANK);
	strcat(strcpy(load_value.nv_write,path_write),"fixnv");
	MODEM_LOGD("loading FIXNV: path=%s, bak_path=%s, out=%s\n",
		load_value.nv1_read,load_value.nv2_read,load_value.nv_write);
	read_nv_partition(load_value.nv1_read,load_value.nv2_read,load_value.nv_write);	

	strcat(strcpy(load_value.nv1_read,path_read),RUNNV1_BANK);
	strcat(strcpy(load_value.nv2_read,path_read),RUNNV2_BANK);
	strcat(strcpy(load_value.nv_write,path_write),"runnv");
	MODEM_LOGD("loading RuntimeNV: path=%s, bak_path=%s, out=%s\n",
		load_value.nv1_read,load_value.nv2_read,load_value.nv_write);
	read_nv_partition(load_value.nv1_read,load_value.nv2_read,load_value.nv_write);

	boot_from_table(table_ptr,sizeof(LOAD_TABLE_S)/sizeof(IMAGE_LOAD_S));

	/* write 1 to start*/
	if(access("/proc/cppmic",F_OK) == 0){
		MODEM_LOGD("arm7_start= %s",load_value.arm7_start);
		if(pmic_is_shutdown)
			write_proc_file(load_value.arm7_start,0,"1");
	}
	MODEM_LOGD("cp_start= %s",load_value.cp_start);
        write_proc_file(load_value.cp_start,0, "1");

	return 0;
}

int modem_ctrl_parse_cmdline(char * cmdvalue)
{
	int fd = 0,ret=0;
	char cmdline[CMDLINE_SIZE] = {0};
	char *str = NULL;
	int val;

	if(cmdvalue == NULL){
		MODEM_LOGD("cmd_value = NULL\n");
		return -1;
	}
	fd = open("/proc/cmdline", O_RDONLY);
	if (fd >= 0) {
		if ((ret = read(fd, cmdline, sizeof(cmdline)-1))> 0){
			cmdline[ret] = '\0';
			MODEM_LOGD("modem_ctrl: cmdline %s\n",cmdline);
			str = strstr(cmdline, "modem=");
			if ( str != NULL){
				str += (sizeof("modem=")-1);
				*(strchr(str,' ')) = '\0';
			} else {
				MODEM_LOGE("cmdline 'modem=' is not exist\n");
				goto ERROR;
			}
			MODEM_LOGD("cmdline len = %d, str=%s\n",strlen(str),str);
			if(!strcmp(cmdvalue,str))
				val=1;
			else
				val=0;	
			close(fd);
			return val;
		} else {
			MODEM_LOGE("cmdline modem=NULL");
			goto ERROR;
		}
        } else {
		MODEM_LOGE("/proc/cmdline open error:%s\n",strerror(errno));
		return 0;
	}
ERROR:
	MODEM_LOGD("modem_ctrl: 1");
	close(fd);
	return 0;
}

int open_modem_dev(char *path)
{
	int  fd = -1;

retry:
	fd = open(path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		if (errno == EINTR || errno == EAGAIN)
			goto retry;
        	else
			return -1;
	}
	return fd;
}

int wait_for_modem_alive(int modem)
{
	char buf[256] = {0};
	int ret=0, suc = 0;
	int count = 0;
	int fd =0;
	char tty_dev[256] = {0}, path[256]={0};
	struct timeval timeout;
	fd_set rfds;

	if(modem == TD_MODEM) {
		property_get(TD_TTY_DEV_PROP, tty_dev, "");
	} else if(modem == W_MODEM) {
		property_get(W_TTY_DEV_PROP, tty_dev, "");
	} else if(modem == TL_MODEM) {
		property_get(TL_TTY_DEV_PROP, tty_dev, "");
	} else if(modem == LF_MODEM) {
		property_get(LF_TTY_DEV_PROP, tty_dev, "");
	} else if(modem == LTE_MODEM) {
		property_get(LTE_TTY_DEV_PROP, tty_dev, "");
	} else {
		 MODEM_LOGE("input wrong modem type!");
		return ret;
	}

	sprintf(path, "%s0", tty_dev);
	while(1){
		fd = open_modem_dev(path);
		if(fd < 0){
			MODEM_LOGD("failed to open tty dev: %s, fd = %d",path, fd);
			usleep(1000*1000);
		}
		else break;
	}
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	MODEM_LOGD("wait for alive info from : %d", fd);

	for(;;){
		timeout.tv_sec=8;
		timeout.tv_usec=0;
		do {
			ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		} while(ret == -1 && errno == EINTR);
		if(ret < 0){
			MODEM_LOGE("select error: %s", strerror(errno));
			continue;
		} else if(ret == 0){
			MODEM_LOGE(" wait alive select timeout,exit!");
			//system("echo \"c\" > /proc/sysrq-trigger");
			close(fd);
			return ret;
		}else{
			count = read(fd, buf, sizeof(buf));
			if (count <= 0) {
				MODEM_LOGE("read %d return %d, error: %s", fd, count, strerror(errno));
					continue;
			}
			MODEM_LOGD("read response %s from %d", buf,fd);
			if (strstr(buf, "Alive")) {
				suc = 1;
				break;
			}
		}
	}

	close(fd);
	return suc;
}

static void modemd_read_empty_log(int fd)
{
   char buf[2048] = {0};
   int ret = 0;
   int count = 0;
   struct timeval timeout;
   fd_set rfds;

   FD_ZERO(&rfds);
   FD_SET(fd, &rfds);

   for(;;){
           timeout.tv_sec = 1;
           timeout.tv_usec=0;
       do {
           ret = select(fd+1, &rfds, NULL, NULL, &timeout);
       } while(ret == -1 && errno == EINTR);
       if(ret < 0){
           //MODEMD_LOGE("select error: %s", strerror(errno));
           continue;
       }else if(ret == 0){
           MODEM_LOGD(" time out, read log over!");
           break;
       }else{
            //MODEMD_LOGD("one time read log start");
            do
            {
                count = read(fd, buf, sizeof(buf));
                //MODEMD_LOGD("read log count = %d",count);
            }
            while(count > 0);
            continue;
       }
   }
}

static void* prepare_reset_modem(int modem)
{
	int w_cnt,log_fd,diag_fd;

	MODEM_LOGD("open diag_chan = %s, log_chan = %s!", diag_chan,log_chan);
	if(strlen(diag_chan)==0 || strlen(log_chan)==0)
		return NULL;

	log_fd= open(log_chan, O_RDWR | O_NONBLOCK);
	if(log_fd < 0){
		log_fd= open(diag_chan, O_RDWR | O_NONBLOCK);
		MODEM_LOGD("open log_chan = %s\n", diag_chan);
	} else  MODEM_LOGD("open log_chan = %s\n", log_chan);

	if(log_fd >= 0) {
		modemd_read_empty_log(log_fd);
		MODEM_LOGD("read log over %s!\n", log_chan);
		close(log_fd);
	}
	//than write 'z' to cp
	diag_fd= open(diag_chan, O_RDWR | O_NONBLOCK);
	if(diag_fd < 0) {
		MODEM_LOGD("MODEM cannot open %s\n", diag_chan);
		return NULL;
	} else {
		MODEM_LOGD("ready write diag cmd = %s!", s_reset_cmd);
		w_cnt = write(diag_fd, s_reset_cmd, sizeof(s_reset_cmd));
		MODEM_LOGD("MODEM write diag_chan:%d ,%s\n", w_cnt, strerror(errno));
		close(diag_fd);
		return NULL;
	}

}


static void* modem_ctrl_listen_thread(void *para)
{
	char buf[128]={0};
	int cnt=0,i;
	int modem;

	MODEM_LOGD(">>>>>> start modem_ctrl first load ......\n");

	if(para == NULL){
		MODEM_LOGE("modem is NULL\n");
		return NULL;
	} else {
		modem = *(int*)para;
		MODEM_LOGD("modem=0x%x\n",modem);
	}

	if(modem_ctrl_parse_cmdline("shutdown")){
		load_sipc_modem_img(modem,1);
		if(!wait_for_modem_alive(modem))
			return NULL;
	}
	MODEM_LOGD("send sem_first_boot\n");
	sem_post(&sem_first_boot);
	while(client_modemd<0){
		if(++cnt<20){
			usleep(10*1000);
			MODEM_LOGD("wait modemd connect\n");
		} else {
			MODEM_LOGE("modemd not start,exit modem_ctrl_listen_thread");
			return NULL;
		}
	}
	while(1){
		cnt=read(client_modemd,buf,sizeof(buf));
		if(cnt <= 0){
			MODEM_LOGE("%s,read cnt errno:[%d] %s\n", __FUNCTION__,errno, strerror(errno));
			break;
		}
		MODEM_LOGD("%s,read cnt= %d, str= %s",__FUNCTION__,cnt,buf);

		if(!strcmp(buf,MODEM_RESET)){
			load_sipc_modem_img(modem,0);
			if(!wait_for_modem_alive(modem))
				return NULL;
			if(i=write(client_modemd,MODEM_ALIVE,strlen(MODEM_ALIVE))<=0){
				MODEM_LOGE("%s,write modem_alive errno:[%d] %s\n", __FUNCTION__,errno, strerror(errno));
				system("echo load_modem_img >/sys/power/wake_unlock");
				g_b_wake_locking = false;
				return NULL;
			} else {
				MODEM_LOGD("write to modemd len = %d,str=Modem Alive\n",i);
				system("echo load_modem_img >/sys/power/wake_unlock");
				g_b_wake_locking = false;
			}
		}
		if(!strcmp(buf,PREPARE_RESET))
			prepare_reset_modem(modem);
	}
	return NULL;
}

/* loop detect sipc modem state */
void* detect_sipc_modem(void *param)
{
	char assert_dev[256] = {0};
	char watchdog_dev[256] = {0};
	int i, ret, watchdog_fd, max_fd,fd = -1;
	fd_set rfds;
	int modem = -1;
	char buf[256], prop[256];
	int numRead,numWrite;
	int is_assert = 0;
	char modem_enable[PROPERTY_VALUE_MAX];
	pthread_t t1;

	MODEM_LOGD(">>>>>> start modem_ctrl_thread program ......\n");
	sem_init(&sem_first_boot, 0, 0);
	/*get modem mode from property*/

	strcpy(prop, TD_MODEM_ENABLE_PROP);
	property_get(prop, modem_enable, "");
	if(!strcmp(modem_enable, "1")){
		modem = TD_MODEM;
		MODEM_LOGD("TD_MODEM enable");
	}
	strcpy(prop, W_MODEM_ENABLE_PROP);
	property_get(prop, modem_enable, "");
	if(!strcmp(modem_enable, "1")){
		modem = W_MODEM;
		MODEM_LOGD("W_MODEM enable");
	}
	strcpy(prop, LTE_MODEM_ENABLE_PROP);
	property_get(prop, modem_enable, "");
	if(!strcmp(modem_enable, "1")){
		modem = LTE_MODEM;
		MODEM_LOGD("LTE_MODEM enable");
	}
	strcpy(prop, LF_MODEM_ENABLE_PROP);
	property_get(prop, modem_enable, "");
	if(!strcmp(modem_enable, "1")){
		modem = LF_MODEM;
		MODEM_LOGD("LF_MODEM enable");
	}
	strcpy(prop, TL_MODEM_ENABLE_PROP);
	property_get(prop, modem_enable, "");
	if(!strcmp(modem_enable, "1")){
		modem = TL_MODEM;
		MODEM_LOGD("TL_MODEM enable");
	}
	if(modem<0){
		MODEM_LOGD("ALL_MODEM disable");
		return NULL;
	}
/*get log/diag chan according modem mode*/
	if(modem == TD_MODEM) {
		property_get(TD_ASSERT_PROP, assert_dev, DEFAULT_TD_ASSERT_DEV);
		snprintf(watchdog_dev, sizeof(watchdog_dev), "%s", TD_WATCHDOG_DEV);
		property_get("ro.modem.t.diag", diag_chan, "not_find");
		MODEM_LOGD("%s diag_chan:%s", __FUNCTION__,diag_chan);
		property_get("ro.modem.t.log", log_chan, "not_find");
		MODEM_LOGD("%s log_chan:%s", __FUNCTION__,log_chan);
	} else if(modem == W_MODEM) {
		property_get(W_ASSERT_PROP, assert_dev, DEFAULT_W_ASSERT_DEV);
		snprintf(watchdog_dev, sizeof(watchdog_dev), "%s", W_WATCHDOG_DEV);
		property_get("ro.modem.w.diag", diag_chan, "not_find");
		MODEM_LOGD("%s diag_chan:%s", __FUNCTION__,diag_chan);
		property_get("ro.modem.w.log", log_chan, "not_find");
		MODEM_LOGD("%s log_chan:%s", __FUNCTION__,log_chan);
	} else if(modem == TL_MODEM) {
		property_get(TL_ASSERT_PROP, assert_dev, DEFAULT_TL_ASSERT_DEV);
		snprintf(watchdog_dev, sizeof(watchdog_dev), "%s", TL_WATCHDOG_DEV);
		property_get("ro.modem.tl.diag", diag_chan, "not_find");
		MODEM_LOGD("%s diag_chan:%s", __FUNCTION__,diag_chan);
		property_get("ro.modem.tl.log", log_chan, "not_find");
		MODEM_LOGD("%s log_chan:%s", __FUNCTION__,log_chan);
	} else if(modem == LF_MODEM) {
		property_get(LF_ASSERT_PROP, assert_dev, DEFAULT_LF_ASSERT_DEV);
		snprintf(watchdog_dev, sizeof(watchdog_dev), "%s", LF_WATCHDOG_DEV);
		property_get("ro.modem.lf.diag", diag_chan, "not_find");
		MODEM_LOGD("%s diag_chan:%s", __FUNCTION__,diag_chan);
		property_get("ro.modem.lf.log", log_chan, "not_find");
		MODEM_LOGD("%s log_chan:%s", __FUNCTION__,log_chan);
	} else if(modem == LTE_MODEM) {
		property_get(L_ASSERT_PROP, assert_dev, DEFAULT_L_ASSERT_DEV);
		snprintf(watchdog_dev, sizeof(watchdog_dev), "%s", L_WATCHDOG_DEV);
		property_get("ro.modem.l.diag", diag_chan, "not_find");
		MODEM_LOGD("%s diag_chan:%s", __FUNCTION__,diag_chan);
		property_get("ro.modem.l.log", log_chan, "not_find");
		MODEM_LOGD("%s log_chan:%s", __FUNCTION__,log_chan);
	} else {
		MODEM_LOGE("%s: input wrong modem type!", __func__);
		return NULL;
	}

	/*set up socket connection to modemd*/	
	server_modem_ctrl = socket_local_server(SOCKET_NAME_MODEM_CTL,
		0,/*ANDROID_SOCKET_NAMESPACE_RESERVED,*/ SOCK_STREAM);
	if (server_modem_ctrl < 0) {
		MODEM_LOGE("[kai]modemctrl_listenaccept_thread: cannot create local socket server\n");
		return NULL;
	}
	if(0 != pthread_create(&t1, NULL, (void*)modem_ctrl_listen_thread, &modem)) {
                MODEM_LOGE(" modem_ctrl_ipc_thread create error!\n");
        }
	/*wait modemd connect*/
	MODEM_LOGD(" wait accept!\n");
	client_modemd= accept(server_modem_ctrl, NULL, NULL);
	if (client_modemd < 0 ) {
		MODEM_LOGE("Error on accept() errno:[%d] %s\n", errno, strerror(errno));
		sleep(1);
		return NULL;
	}
	MODEM_LOGD(" accept client_modemd =%d\n",client_modemd);

	/*wait first time modem load OK*/
	sem_wait(&sem_first_boot);
	while((assert_fd = open(assert_dev, O_RDWR))<0){
		MODEM_LOGE("open %s failed, error: %s",assert_dev, strerror(errno));
		sleep(1);
	}
	MODEM_LOGD("%s: open assert dev: %s, fd = %d", __func__, assert_dev, assert_fd);
	watchdog_fd = open(watchdog_dev, O_RDONLY);
	MODEM_LOGD("%s: open watchdog dev: %s, fd = %d", __func__, watchdog_dev, watchdog_fd);
	if (watchdog_fd < 0) {
		close(assert_fd);
		MODEM_LOGE("open %s failed, error: %s", watchdog_dev, strerror(errno));
		return NULL;
	}
	/*inform modemd cp is running*/
	while(i=write(client_modemd,MODEM_ALIVE,strlen(MODEM_ALIVE))<=0){
		MODEM_LOGE("%s,write modem_alive errno:[%d] %s\n", __FUNCTION__,errno, strerror(errno));
		sleep(1);
	}
	MODEM_LOGD("write to modemd len = %d\n",i);
	max_fd = watchdog_fd > assert_fd ? watchdog_fd : assert_fd;

	FD_ZERO(&rfds);
	FD_SET(assert_fd, &rfds);
	FD_SET(watchdog_fd, &rfds);
	/*listen assert and WDG event*/
	for (;;) {
		MODEM_LOGD("%s: wait for modem assert/hangup event ...", __func__);
		do {
        		ret = select(max_fd + 1, &rfds, NULL, NULL, 0);
		} while(ret == -1 && errno == EINTR);
		if (ret > 0) {
			if (FD_ISSET(assert_fd, &rfds)) 
				fd = assert_fd;
			else if (FD_ISSET(watchdog_fd, &rfds)) 
				fd = watchdog_fd;
			else {
				MODEM_LOGE("none of assert and watchdog fd is readalbe");
				sleep(1);
				continue;
			}
	
			system("echo load_modem_img >/sys/power/wake_lock");
			g_b_wake_locking = true;
			memset(buf, 0, sizeof(buf));
			MODEM_LOGD("enter read ...");
			numRead = read(fd, buf, sizeof(buf));
			if (numRead < 0) {
				MODEM_LOGE("read %d return %d, errno = %s", fd , numRead, strerror(errno));
				sleep(1);
				continue;
			}
			MODEM_LOGD("buf=%s", buf);
			if ((numWrite = write(client_modemd,buf,strlen(buf)))<=0)
				MODEM_LOGE("write %d return %d, errno = %s", fd , numWrite, strerror(errno));
			else
				MODEM_LOGD("write to modemd len = %d\n",i);
		}
	}
	return NULL;
}
