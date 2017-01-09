#define LOG_TAG     "MODEMD"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include "modemd.h"

//#define TEST_MODEM

#define VLX_ASSERT_DEV    "/dev/vbpipe2"
#define VLX_RESET_DEV    "/dev/vbpipe0"

#ifdef CONFIG_EMMC
/* see g_sprd_emmc_partition_cfg[] in u-boot/nand_fdl/fdl-2/src/fdl_partition.c */
#define PARTITION_MODEM        "/dev/block/mmcblk0p2"
#define PARTITION_DSP          "/dev/block/mmcblk0p3"
#define PARTITION_FIX_NV1      "/dev/block/mmcblk0p4"
#define PARTITION_FIX_NV2      "/dev/block/mmcblk0p5"
#define PARTITION_RUNTIME_NV1  "/dev/block/mmcblk0p6"
#define PARTITION_RUNTIME_NV2  "/dev/block/mmcblk0p7"
#define PARTITION_PROD_INFO1   "/dev/block/mmcblk0p8"
#define PARTITION_PROD_INFO2   "/dev/block/mmcblk0p9"
#define EMMC_MODEM_SIZE        (8 * 1024 * 1024)
#else
#define PARTITION_MODEM        "modem"
#define PARTITION_DSP          "dsp"
#define F2R1_MODEM_SIZE        (3500 * 1024)
#define F4R2_MODEM_SIZE        (8 * 1024 * 1024)
#endif

#define DATA_BUF_LEN    (2048 * 10)
#define DATA_BUF_SIZE (128)

#define MODEM_IMAGE_OFFSET  0
#ifdef MODEM_BANK
#undef MODEM_BANK
#endif
#define MODEM_BANK          "guestOS_2_bank"
#define DSP_IMAGE_OFFSET    0
#ifdef DSP_BANK
#undef DSP_BANK
#endif
#define DSP_BANK            "dsp_bank"

#ifndef CONFIG_EMMC
int modem_offset=0, dsp_offset=0;
#endif
char modem_mtd[256], dsp_mtd[256];
int mcp_size = 512, modem_image_len = 0;
unsigned char data[DATA_BUF_LEN];

static int is_assert = 0;

static pthread_mutex_t         reset_mutex;
static pthread_cond_t          reset_cond;
static pthread_mutex_t         read_mutex;
static pthread_cond_t          read_cond;

/******************************************************
 *
 ** vlx interface begain
 *
 *****************************************************/
int write_proc_file(char *file, int offset, char *string)
{
    int fd, stringsize, res = -1;

    fd = open(file, O_RDWR);
    if (fd < 0) {
        MODEMD_LOGE("Unknown file %s, error: %s", file, strerror(errno));
        return 0;
    }

    if (lseek(fd, offset, SEEK_SET) != offset) {
        MODEMD_LOGE("Cant lseek file %s, error :%s", file, strerror(errno));
        goto leave;
    }

    stringsize = strlen(string);
    if (write(fd, string, stringsize) != stringsize) {
        MODEMD_LOGE("Could not write %s in %s, error :%s", string, file, strerror(errno));
        goto leave;
    }

    res = 0;
leave:
    close(fd);

    return res;
}

char *loadOpenFile(int fd, int *fileSize)
{
    char *buf;

    *fileSize = lseek(fd, 0, SEEK_END);
    if (*fileSize < 0) {
        MODEMD_LOGE("loadOpenFile error");
        return 0;
    }

    buf = (char *)malloc((*fileSize) + 1);
    if (buf == 0) {
        MODEMD_LOGE("Malloc failed");
        return 0;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        free(buf);
        MODEMD_LOGE("lseek error");
        return 0;
    }

    MODEMD_LOGE("file size %d", *fileSize);

    if (read(fd, buf, *fileSize) != *fileSize) {
        free(buf);
        buf = 0;
    } else
        buf[*fileSize] = 0;

    return buf;
}

char *loadFile(char *pathName, int *fileSize)
{
    int fd;
    char *buf;

    MODEMD_LOGD("pathName = %s", pathName);
    fd = open(pathName, O_RDONLY);
    if (fd < 0) {
        MODEMD_LOGE("open file %s failed", pathName);
        return 0;
    }

    buf = loadOpenFile(fd, fileSize);
    close(fd);

    return buf;
}

int check_proc_file(char *file, char *string)
{
    int filesize;
    char *buf;

    buf = loadFile(file, &filesize);
    if (!buf) {
        MODEMD_LOGE("failed to load %s", file);
        return -1;
    }

    MODEMD_LOGD("check proc file buf = %s", buf);
    if (strstr(buf, string)) {
        free(buf);
        MODEMD_LOGD("String found <%s> in <%s>", string, file);
        return 0;
    }
    free(buf);
    MODEMD_LOGD("failed to find %s in %s", string, file);
    return 1;
}

int loadimage(char *fin, int offsetin, char *fout, int offsetout, int size)
{
    int res = -1, fdin, fdout, bufsize, i, rsize, rrsize, wsize;
    char buf[8192];
    int buf_size = sizeof(buf);
#ifdef TEST_MODEM
    int fd_modem_dsp;
#endif
#ifndef CONFIG_EMMC
    mtd_info_t      meminfo;
    int             bad_info = 1;
    int             n;
    unsigned long long      offset = 0;
    unsigned long long      start_pos = 0;
    unsigned long long      bpos=0;
#endif
    MODEMD_LOGD("Loading %s in bank %s:%d %d", fin, fout, offsetout, size);

    fdin = open(fin, O_RDONLY, 0);
    fdout = open(fout, O_RDWR, 0);
    if (fdin < 0) {
        if (fdout >= 0)
            close(fdout);
        MODEMD_LOGE("failed to open %s", fin);
        return -1;
    }
    if (fdout < 0) {
        close(fdin);
        MODEMD_LOGE("failed to open %s", fout);
        return -1;
    }

#ifndef CONFIG_EMMC
    if (ioctl(fdin, MEMGETINFO, &meminfo) != 0) {
        MODEMD_LOGD("get MEMGETINFO error !\r\n");
        goto leave;
    }
    MODEMD_LOGD("meminfo: erasesize = 0x%x,writesize = 0x%x\r\n",meminfo.erasesize,meminfo.writesize);

#endif

#ifdef TEST_MODEM
    if (size == modem_image_len)
        fd_modem_dsp = open("/data/local/tmp/modem.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    else
        fd_modem_dsp = open("/data/local/tmp/dsp.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
        MODEMD_LOGE("failed to lseek %d in %s", offsetin, fin);
        goto leave;
    }

    if (lseek(fdout, offsetout, SEEK_SET) != offsetout) {
        MODEMD_LOGE("failed to lseek %d in %s", offsetout, fout);
        goto leave;
    }

#ifndef CONFIG_EMMC
    start_pos = offsetin/*+modem_offset*/;
    bpos = start_pos;
    MODEMD_LOGD("loadimage: offsetin=0x%x, modem_offset=0x%x, dsp_ofset=0x%x\n", offsetin, modem_offset, dsp_offset);
#endif

    for (i = 0; size > 0; i += min(size, buf_size)) {
#ifndef CONFIG_EMMC
        while((bpos % meminfo.erasesize)==0x0) {
            bad_info =ioctl(fdin, MEMGETBADBLOCK, &bpos) ;
            if(bad_info > 0 ) {
                MODEMD_LOGD("skip bad block 0x%x \r\n", bpos&(~(meminfo.erasesize - 1)));
                //bpos = (bpos + meminfo.erasesize)&(~(meminfo.erasesize - 1));
                bpos += meminfo.erasesize;
                if (lseek(fdin, bpos, SEEK_SET) != bpos) {
                    MODEMD_LOGD("bpos=0x%x \r\n", bpos);
                    goto leave;
                }
            } else
                break;
        }
#endif
        rsize = min(size, buf_size);
        rrsize = read(fdin, buf, rsize);
        if (rrsize != rsize) {
            MODEMD_LOGE("failed to read %s", fin);
            goto leave;
        }
        wsize = write(fdout, buf, rsize);
#ifdef TEST_MODEM
        wsize = write(fd_modem_dsp, buf, rsize);
#endif
        if (wsize != rsize) {
            MODEMD_LOGE("failed to write %s [wsize = %d  rsize = %d  remain = %d]",
                    fout, wsize, rsize, size);
            goto leave;
        }
        size -= rsize;
#ifndef CONFIG_EMMC
        bpos = lseek(fdin, 0, SEEK_CUR);
        if (bpos == (off_t) -1) {
            goto leave;
        }
#endif
    }

    res = 0;
leave:
    close(fdin);
    close(fdout);
#ifdef TEST_MODEM
    fsync(fd_modem_dsp);
    close(fd_modem_dsp);
#endif
    return res;
}

int copyfile(const char *src, const char *dst)
{
    int ret, length, srcfd, dstfd;
    struct stat statbuf;

    if ((src == NULL) || (dst == NULL))
        return -1;

    if (strcmp(src, dst) == 0)
        return -1;

    if (strcmp(src, "/proc/mtd") == 0) {
        ret = DATA_BUF_LEN;
        length = ret;
    } else {
        ret = stat(src, &statbuf);
        if (ret == ENOENT) {
            MODEMD_LOGE("src file is not exist\n");
            return -1;
        }
        length = statbuf.st_size;
    }

    if (length == 0) {
        MODEMD_LOGE("src file length is 0\n");
        return -1;
    }

    memset(data, 0, DATA_BUF_LEN);
    srcfd = open(src, O_RDONLY);
    if (srcfd < 0) {
        MODEMD_LOGE("open %s error\n", src);
        return -1;
    }

    ret = read(srcfd, data, length);
    close(srcfd);

    if (ret <= 0) {
        MODEMD_LOGE("read error length = %d  ret = %d\n", length, ret);
        return -1;
    }

    if (access(dst, 0) == 0) {
        ret = remove(dst);
        if (ret < 0) {
            MODEMD_LOGE("remove %s failed\n", dst);
        }
    }

    dstfd = open(dst, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (dstfd < 0) {
        MODEMD_LOGE("create %s error\n", dst);
        return -1;
    }

    ret = write(dstfd, data, length);
    close(dstfd);
    if (ret != length) {
        MODEMD_LOGE("write error length = %d  ret = %d\n", length, ret);
        return -1;
    }

    return length;
}

/* cat /proc/mtd */
char *get_proc_mtd(void)
{
    int fd, fileSize;
    char *buf;

    copyfile("/proc/mtd", "/data/local/tmp/mtd.txt");
    fd = open("/data/local/tmp/mtd.txt", O_RDONLY);
    if (fd < 0) {
        MODEMD_LOGE("can not open /data/local/tmp/mtd.txt\n");
        return 0;
    }

    fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize < 0) {
        close(fd);
        MODEMD_LOGE("fileSize is error\n");
        return 0;
    }
    buf = (char *)malloc((fileSize) + 1);
    if (buf == 0) {
        close(fd);
        MODEMD_LOGE("Malloc buffer failed\n");
        return 0;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        close(fd);
        free(buf);
        MODEMD_LOGE("lseek header error\n");
        return 0;
    }
    if (read(fd, buf, fileSize) != fileSize) {
        free(buf);
        buf = 0;
    } else
        buf[fileSize] = 0;
    close(fd);
    return buf;
}

int get_mcp_size(char *buf)
{
    char *pos;
    int pos_len;
    char number[8];
    int nandsize;

    if (buf == 0)
        return 0;

    pos = strstr(buf, "MiB");
    if (pos == 0) {
        MODEMD_LOGE("failed to find MiB\n");
        return 0;
    }
    pos_len = 0;
    while (*pos != ' ') {
        pos--;
        pos_len++;
    }
    pos++;
    pos_len--;
    if(pos_len>=(int)sizeof(number)) {
        MODEMD_LOGE("error to find MiB\n");
        return 0;
    }
    memset(number, 0, 8);
    strncpy(number, pos, pos_len);
    nandsize = atoi(number);

    return nandsize;
}

void get_mtd_partition(char *buf, char *name, char *mtdpath)
{
    char *pos;
    int pos_len;

    if(buf == NULL) {
        return;
    }

    pos = strstr(buf, name);
    if (pos == 0) {
        MODEMD_LOGE("failed to find %s\n", name);
        return;
    }

    while (*pos != ':') {
        pos--;
    }
    pos_len = 0;
    while (*pos != 'm') {
        pos--;
        pos_len++;
    }
    strcpy(mtdpath, "/dev/mtd/");
    strncat(mtdpath, pos, pos_len);
}

int vlx_reboot_init(void)
{
    char *mtdbuf = NULL;
    int ret;

    ret = pthread_mutex_init(&reset_mutex, NULL);
    if (ret) {
        MODEMD_LOGE("Failed to init reset_mutex : %d", ret);
        exit_modemd();
    }
    pthread_cond_init(&reset_cond, NULL);

    ret = pthread_mutex_init(&read_mutex, NULL);
    if (ret) {
        MODEMD_LOGE("Failed to init read_mutex : %d", ret);
        exit_modemd();
    }
    pthread_cond_init(&read_cond, NULL);


#ifdef CONFIG_EMMC
    modem_image_len = EMMC_MODEM_SIZE;
    MODEMD_LOGD("modem length : %d", modem_image_len);

    memset(modem_mtd, 0, 256);
    strcpy(modem_mtd, PARTITION_MODEM);
    MODEMD_LOGD("modem emmc dev : %s", modem_mtd);

    memset(dsp_mtd, 0, 256);
    strcpy(dsp_mtd, PARTITION_DSP);
    MODEMD_LOGD("dsp emmc dev : %s\n", dsp_mtd);
#else //#ifdef CONFIG_EMMC
    mtdbuf = get_proc_mtd();
    mcp_size = get_mcp_size(mtdbuf);
    if (mcp_size >= 512)
        modem_image_len = F4R2_MODEM_SIZE;
    else
        modem_image_len = F2R1_MODEM_SIZE;

    MODEMD_LOGD("mcp size : %d  modem length : %d", mcp_size, modem_image_len);

    memset(modem_mtd, 0, 256);
    get_mtd_partition(mtdbuf, PARTITION_MODEM, modem_mtd);
    MODEMD_LOGD("modem mtd dev : %s", modem_mtd);

    memset(dsp_mtd, 0, 256);
    get_mtd_partition(mtdbuf, PARTITION_DSP, dsp_mtd);
    MODEMD_LOGD("dsp mtd dev : %s", dsp_mtd);

    if (mtdbuf != 0)
        free(mtdbuf);
#endif //#ifdef CONFIG_EMMC

    return 0;
}

int load_vlx_modem_img(char * modem, int is_modem_assert)
{
    char guest_dir[256] = "/proc/nk/guest-02";
    char buf[256] = {0};
    int i, ret;

    MODEMD_LOGD("Check and load vlx modem img");

    sprintf(buf, "%s/status", guest_dir);
    MODEMD_LOGD("check reboot buf = %s", buf);

    if (check_proc_file(buf, "not started") == 0) {

        /* load modem */
        memset(buf, 0, 256);
        sprintf(buf, "%s/%s", guest_dir, MODEM_BANK);
        MODEMD_LOGD("modem bank buf = %s", buf);
        /* gillies set modem_image_len 0x600000 */
        loadimage(modem_mtd, MODEM_IMAGE_OFFSET, buf, 0x1000, modem_image_len);

        /* load dsp */
        memset(buf, 0, 256);
        sprintf(buf, "%s/%s", guest_dir, DSP_BANK);
        MODEMD_LOGD("dsp bank buf = %s", buf);

        /* dsp is only 3968KB, 0x420000 is too big */
        loadimage(dsp_mtd, DSP_IMAGE_OFFSET, buf, 0x20000, (3968 * 1024));

        /* stop eng to release vbpipe0 */
        stop_service(modem, 1);

        write_proc_file("/proc/nk/restart", 0, "2");
        write_proc_file("/proc/nk/resume", 0, "2");

        MODEMD_LOGD("wait for 20s\n");
        sleep(20);

        pthread_mutex_lock(&reset_mutex);
        pthread_cond_signal(&reset_cond);
        pthread_mutex_unlock(&reset_mutex);

        if(is_modem_assert) {
            /* info socket clients that modem is reset */
            for(i = 0; i < MAX_CLIENT_NUM; i++) {
                MODEMD_LOGE("client_fd[%d]=%d\n",i, client_fd[i]);
                if(client_fd[i] >= 0) {
                    ret = write(client_fd[i], "Modem Alive",strlen("Modem Alive"));
                    MODEMD_LOGE("write %d bytes to client_fd[%d]:%d to info modem is alive",
                            ret, i, client_fd[i]);
                    if(ret < 0) {
                        MODEMD_LOGE("reset client_fd[%d]=-1",i);
                        close(client_fd[i]);
                        client_fd[i] = -1;
                    }
                }
            }
        }

        start_service(modem, 1, 1);

        return 1;
    } else {
        MODEMD_LOGD("guest seems started \n");
    }

    return 0;
}

void *vlx_reset_thread(void *par)
{
    int retry_times;
    int reset_fd, numWrite;
    char cmdrst[2]={'z',0x0a};
    char prop[256] = {0};
    int is_reset = 0;

    memset(prop, 0, sizeof(prop));
    property_get(MODEMRESET_PROPERTY, prop, "0");
    is_reset = atoi(prop);
    if(!is_reset) {
        MODEMD_LOGD("%s:modem reset is not enabled, just exit!", __func__);
        return NULL;
    }

    reset_fd = open(VLX_RESET_DEV, O_WRONLY);
    if (reset_fd < 0) {
        MODEMD_LOGE("%s<%d>: open vpipe0 failed, error: %s",
                __func__, __LINE__, strerror(errno));
        exit_modemd();
    }

    for(;;) {
        MODEMD_LOGD("%s:wait for assert event...", __func__);

        pthread_mutex_lock(&read_mutex);
        pthread_cond_wait(&read_cond, &read_mutex);
        pthread_mutex_unlock(&read_mutex);

        if(!is_assert) {
            close(reset_fd);
            MODEMD_LOGD("%s: modem reset, wait for reload modem event...", __func__);
            pthread_mutex_lock(&reset_mutex);
            pthread_cond_wait(&reset_cond, &reset_mutex);
            pthread_mutex_unlock(&reset_mutex);
            MODEMD_LOGD("%s: wakeup, reopen vpipe0", __func__);
            reset_fd = open(VLX_RESET_DEV, O_WRONLY);
            if(reset_fd < 0) {
                MODEMD_LOGE("%s<%d>: open vpipe0 failed, error: %s",
                        __func__, __LINE__, strerror(errno));
                return NULL;
            }
            continue;
        }

        MODEMD_LOGD("%s:assert event occur", __func__);

        retry_times = 0;

write_again:
        numWrite = write(reset_fd, cmdrst, sizeof(cmdrst));
        if (numWrite == 0) {
            goto write_again;
        } else if (numWrite < 0) {
            MODEMD_LOGE("%s: write vbpipe0 return %d, errno=%d(%s)",
                    __func__, numWrite, errno, strerror(errno));
            if (errno == EPIPE) {
                MODEMD_LOGE("peer side of vbpipe0 is down, reopen it");
                close(reset_fd);
                sleep(10);
                reset_fd = open(VLX_RESET_DEV, O_WRONLY);
                if(reset_fd < 0) {
                    MODEMD_LOGE("%s<%d>: open vpipe0 failed, error: %s",
                            __func__, __LINE__, strerror(errno));
                    return NULL;
                }
            }
            sleep(1);
            retry_times++;
            if (retry_times > 5) {
                MODEMD_LOGE("%s:retry fail", __func__);
                continue;
            } else {
                goto write_again;
            }
        } else {
            close(reset_fd);
            MODEMD_LOGD("%s: write ^Z done, wait for reload modem event...", __func__);
            pthread_mutex_lock(&reset_mutex);
            pthread_cond_wait(&reset_cond, &reset_mutex);
            pthread_mutex_unlock(&reset_mutex);
            MODEMD_LOGD("%s: wakeup, reopen vpipe0", __func__);
            reset_fd = open(VLX_RESET_DEV, O_WRONLY);
            if(reset_fd < 0) {
                MODEMD_LOGE("%s<%d>: open vpipe0 failed, error: %s",
                        __func__, __LINE__, strerror(errno));
                return NULL;
            }
        }
    }
    close(reset_fd);
}

/* loop detect vlx modem state */
int detect_vlx_modem(char * modem)
{
    pthread_t tid;
    int ret, assert_fd, i;
    int numRead;
    char buf[DATA_BUF_SIZE] = {0};

    pthread_create(&tid, NULL, vlx_reset_thread, buf);

    assert_fd = open(VLX_ASSERT_DEV, O_RDONLY);
    if (assert_fd < 0) {
        MODEMD_LOGE("open %s failed, error: %s", VLX_ASSERT_DEV, strerror(errno));
        exit_modemd();
    }

    for(;;) {
        memset(buf, 0, sizeof(buf));
        numRead = read(assert_fd, buf, DATA_BUF_SIZE);
        if (numRead == 0) {
            MODEMD_LOGE("modem assert finished\n");

            if(!is_assert) {
                pthread_mutex_lock(&read_mutex);
                pthread_cond_signal(&read_cond);
                pthread_mutex_unlock(&read_mutex);
            }

            /* close */
            close(assert_fd);

            /* reload modem */
            load_vlx_modem_img(modem, is_assert);

            is_assert = 0;

            /* reopen */
            assert_fd = open(VLX_ASSERT_DEV, O_RDONLY);
            if (assert_fd < 0){
                MODEMD_LOGE("open %s failed, error: %s", VLX_ASSERT_DEV, strerror(errno));
                exit_modemd();
            }
            continue;
        } else if (numRead < 0) {
            MODEMD_LOGE("numRead %d is lower than 0\n", numRead);
            sleep(1);
            continue;
        } else {
            if(strstr(buf, "Assert") != NULL) {
                MODEMD_LOGE("modem assert happen, buf=%s", buf);

                is_assert = 1;

                /* info socket client that modem is assert */
                /* include info slog to release vbpipe0 */
                for(i = 0; i < MAX_CLIENT_NUM; i++) {
                    MODEMD_LOGE("client_fd[%d]=%d\n",i, client_fd[i]);
                    if(client_fd[i] >= 0) {
                        ret = write(client_fd[i], buf, numRead);
                        MODEMD_LOGE("write %d bytes to client_fd[%d]:%d to info modem is assert", ret, i, client_fd[i]);
                        if(ret < 0) {
                            MODEMD_LOGE("reset client_fd[%d]=-1",i);
                            close(client_fd[i]);
                            client_fd[i] = -1;
                        }
                    }
                }

                /* make sure vbpipe0 is released by other user */
                sleep(2);

                pthread_mutex_lock(&read_mutex);
                pthread_cond_signal(&read_cond);
                pthread_mutex_unlock(&read_mutex);
            }
        }
    }
    close(assert_fd);
}
/******************************************************
 *
 ** vlx interface end
 *
 *****************************************************/
