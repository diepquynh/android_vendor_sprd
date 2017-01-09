#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <signal.h>
#include <fcntl.h>

#include "audio_debug.h"
#define LOG_TAG 	"audio_hw_agdsp"
#include <utils/Log.h>

#define AGDSP_DEBUG
#ifdef AGDSP_DEBUG
#define AGDSP_LOGD  LOG_I
#define AGDSP_LOGE  LOG_E
#else
#define AGDSP_LOGD(x...)    printf
#define AGDSP_LOGE(x...)    printf
#endif

#define MAX_NAME_LEN			0x20
#define MAX_PATH_LEN			0x80
#define MODEM_IMAGE_PATH_PROP	"ro.product.patitionpath"
#define AGDSP_CPROC_ROOT		"/proc/agdsp/"
#define AGDSP_CPROC_START		"/proc/agdsp/start"
#define AGDSP_CPROC_STOP		"/proc/agdsp/stop"
#ifdef BM_DEV_ENABLE
#define BM_DEV 					"/dev/sprd_bm"
#endif /* BM_DEV_ENABLE */
#define DEFAULT_IMAGE_ROOT_PATH	"/dev/block/platform/sdio_emmc/by-name/"
#define LTE_BANK_PREFIX			"l_"

#define min(A,B)				(((A) < (B)) ? (A) : (B))

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

struct load_node_info {
	char name[MAX_NAME_LEN];
	uint32_t size;
};

struct image_load_info{
	char path_w[MAX_PATH_LEN + 1];
	char path_r[MAX_PATH_LEN + 1];
	char path_r_bk[MAX_PATH_LEN + 1];
	int  offset_in;
	int  offset_out;
	uint32_t size;
};

#ifdef BM_DEV_ENABLE
void agdsp_enable_busmonitor(bool bEnable)
{
	int fd;
	int param;
	int cmd;

	fd = open(BM_DEV, O_RDWR);
	if (fd < 0) {
		AGDSP_LOGD("modemd_enable_busmonitor %s failed, error: %s", BM_DEV, strerror(errno));
		return;
	}

	cmd = bEnable ? BM_DBG_INT_SET : BM_DBG_INT_CLR;
	ioctl(fd, cmd, &param);

	AGDSP_LOGD("modemd_enable_busmonitor bEnable = %d, cmd = %d", bEnable, cmd);
	close(fd);
}
#endif /* BM_DEV_ENABLE */

int write_proc_file(char *file, int offset, char *string)
{
	int fd, stringsize, res = -1, retry = 0;

	do {
	    fd = open(file, O_WRONLY);
	    if (fd < 0) {
			AGDSP_LOGE("%s: open file %s, error: %s", file, strerror(errno));
			usleep(20000);
			retry++;
		}
	} while(fd < 0 && retry < 6);

	if (lseek(fd, offset, SEEK_SET) != offset) {
		AGDSP_LOGE("Cant lseek file %s, error :%s", file, strerror(errno));
		goto leave;
    }

	stringsize = strlen(string);
	if (write(fd, string, stringsize) != stringsize) {
		AGDSP_LOGE("Could not write %s in %s, error :%s", string, file, strerror(errno));
		goto leave;
	}

	res = 0;

	AGDSP_LOGD("%s: write %s to %s", __func__, string, file);

leave:
	close(fd);

	return res;
}

int load_sipc_image(char *fin, int offsetin, char *fout, int offsetout, int size)
{
	int res = -1, fdin, fdout, bufsize, i, rsize, rrsize, wsize;
	char buf[8192];
	int buf_size = sizeof(buf);

	if(NULL == fin || NULL == fout) {
		AGDSP_LOGE("%s: param invalid", __func__);
		return -1;
	}

	AGDSP_LOGD("%s (%s.%d ==> %s.%d size=0x%x)\n",__func__, fin, offsetin, fout, offsetout, size);
#ifdef BM_DEV_ENABLE
	agdsp_enable_busmonitor(false);
#endif /* BM_DEV_ENABLE */

	fdin = open(fin, O_RDONLY, 0);
	if (fdin < 0) {
		AGDSP_LOGE("failed to open %s", fin);
#ifdef BM_DEV_ENABLE
        agdsp_enable_busmonitor(true);
#endif /* BM_DEV_ENABLE */
        return -1;
	}

	fdout = open(fout, O_WRONLY, 0);
	if (fdout < 0) {
		close(fdin);
		AGDSP_LOGE("failed to open %s, error: %s", fout, strerror(errno));
#ifdef BM_DEV_ENABLE
		agdsp_enable_busmonitor(true);
#endif /* BM_DEV_ENABLE */
		return -1;
	}

	if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
		AGDSP_LOGE("failed to lseek %d in %s", offsetin, fin);
		goto leave;
	}

	if (lseek(fdout, offsetout, SEEK_SET) != offsetout) {
		AGDSP_LOGE("failed to lseek %d in %s", offsetout, fout);
		goto leave;
	}

	do{
		rsize = min(size, buf_size);
		rrsize = read(fdin, buf, rsize);
		if(rrsize == 0)
			goto leave;
		if (rrsize < 0) {
			AGDSP_LOGE("failed to read %s %s", fin, strerror(errno));
			goto leave;
		}
		wsize = write(fdout, buf, rrsize);
		if (wsize <= 0) {
			AGDSP_LOGE("failed to write %s [wsize = %d  rsize = %d  remain = %d]", fout, wsize, rsize, size);
			goto leave;
		}
		size -= rrsize;
	}while(size > 0);

	res = 0;

leave:
#ifdef BM_DEV_ENABLE
	agdsp_enable_busmonitor(true);
#endif /* BM_DEV_ENABLE */
	close(fdin);
	close(fdout);
	return res;
}

static int get_modem_load_info(char *file, int num, struct load_node_info *info)
{
	int fd, ret, i, nu;

	if(NULL == file || NULL == info) {
		AGDSP_LOGE("%s: invalid param", __func__);
		return -1;
	};

	fd = open(file, O_RDONLY);
	if(fd < 0) {
		AGDSP_LOGE("%s: open %s failed, error: %s", __func__, file, strerror(errno));
		return -1;
	}

	ret = read(fd, info, num);
	if(ret < 0) {
		AGDSP_LOGE("%s: read %s failed, error: %s", __func__, file, strerror(errno));
		goto leave;
	}

	AGDSP_LOGD("%s: num = 0x%x, ret = 0x%x", __func__, num/sizeof(struct load_node_info), ret/sizeof(struct load_node_info));
	for(i=0, nu=ret/sizeof(struct load_node_info); i < nu; i++, info++) {
		AGDSP_LOGD("%s: <%s, 0x%x>", __func__, info->name, info->size);
	}
leave:
	close(fd);

	return ret;
}

void agdsp_boot(void)
{
	char path[128] = AGDSP_CPROC_ROOT;
	struct load_node_info ldninfo;
	struct image_load_info ldinfo;

	memset(&ldninfo, 0, sizeof(struct load_node_info));
	memset(&ldinfo, 0, sizeof(struct image_load_info));

	strcat(path, "ldinfo");
	if(get_modem_load_info(path, sizeof(struct load_node_info), &ldninfo) < 0) {
		AGDSP_LOGE("%s: from %s get modem load info failed", __func__, path);
		return -1;
	}

	strcpy(ldinfo.path_r, DEFAULT_IMAGE_ROOT_PATH);
	strcat(ldinfo.path_r, LTE_BANK_PREFIX);
	strcat(ldinfo.path_r, ldninfo.name);

	strcpy(ldinfo.path_w, AGDSP_CPROC_ROOT);
	strcat(ldinfo.path_w, ldninfo.name);

	ldinfo.size = ldninfo.size;

	AGDSP_LOGD("ldinfo.path_w: %s", ldinfo.path_w);
	AGDSP_LOGD("ldinfo.path_r: %s", ldinfo.path_r);
	AGDSP_LOGD("ldinfo.size: 0x%x", ldinfo.size);

	write_proc_file(AGDSP_CPROC_STOP, 0, "1");
	AGDSP_LOGD("%s: start load image", __func__);
	load_sipc_image(ldinfo.path_r, 0, ldinfo.path_w, 0, ldinfo.size);
	write_proc_file(AGDSP_CPROC_START, 0, "1");
	return;
}
