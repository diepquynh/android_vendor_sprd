#include <cutils/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "splmerge.h"

#ifdef WIN32
#define SPLMERGE_TRACE  printf
#else
#define LOG_TAG "SPLMERGE"
#define SPLMERGE_TRACE  printf
#endif

#define  TRUE   1
#define  FALSE  0
#define BOOTLOADER_HEADER_OFFSET 0x20
typedef struct{
    unsigned  int version;
    unsigned  int magicData;
    unsigned  int checkSum;
    unsigned  int hashLen;
}EMMC_BootHeader;

static unsigned short eMMCCheckSum(const unsigned int *src, int len)
{
    unsigned int   sum = 0;
    unsigned short *src_short_ptr = NULL;

    while (len > 3){
        sum += *src++;
        len -= 4;
    }
    src_short_ptr = (unsigned short *) src;

    if (0 != (len&0x2)){
        sum += * (src_short_ptr);
        src_short_ptr++;
    }
    if (0 != (len&0x1)){
        sum += * ( (unsigned char *) (src_short_ptr));
    }
    sum  = (sum >> 16) + (sum & 0x0FFFF);
    sum += (sum >> 16);
    return (unsigned short) (~sum);
}

static unsigned int get_pad_data(const unsigned int *src, int len, int offset, unsigned short sum)
{
    unsigned int sum_tmp;
    unsigned int sum1 = 0;
    unsigned int pad_data;
    unsigned int i;
    sum = ~sum;
    sum_tmp = sum & 0xffff;
    sum1 = 0;
    for(i = 0; i < offset; i++) {
        sum1 += src[i];
    }
    for(i = (offset + 1); i < len; i++) {
        sum1 += src[i];
    }
    pad_data = sum_tmp - sum1;
    return pad_data;
}

static void splFillCheckData(unsigned int * splBuf, int checksum_len)
{
    #define MAGIC_DATA    0xAA55A5A5
    #define CHECKSUM_START_OFFSET    0x28
    #define MAGIC_DATA_SAVE_OFFSET    (0x20/4)
    #define CHECKSUM_SAVE_OFFSET    (0x24/4)

    EMMC_BootHeader *header;
    unsigned int pad_data;
    unsigned int w_len;
    unsigned int w_offset;
    w_len = (checksum_len-(BOOTLOADER_HEADER_OFFSET+sizeof(*header))) / 4;
    w_offset = w_len - 1;
    //pad the data inorder to make check sum to 0
    pad_data = (unsigned int)get_pad_data((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET+sizeof(*header), w_len, w_offset, 0);
    *(volatile unsigned int *)((unsigned char*)splBuf+checksum_len - 4) = pad_data;
    header = (EMMC_BootHeader *)((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET);
    header->version  = 0;
    header->magicData= MAGIC_DATA;
    header->checkSum = (unsigned int)eMMCCheckSum((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET+sizeof(*header), checksum_len-(BOOTLOADER_HEADER_OFFSET+sizeof(*header)));
#ifdef SECURE_BOOT_ENABLE
    header->hashLen  = CONFIG_SPL_HASH_LEN>>2;
#else
    header->hashLen  = 0;
#endif
}

/*
 * merge_type: input
 * old_header: input
 * new_buf:    input
 *       can not change this input data
 * merged_buf: output
 *
 * must ensure merged_buf_len >= new_buf_len;
*/
int spl_merge(int merge_type, char *old_header, char *new_buf, int new_buf_len,
              char *merged_buf, int merged_buf_len, int checksum_len)
{
    if (new_buf_len > merged_buf_len) {
        SPLMERGE_TRACE("spl_merge: Error! merged buf too small\n");
        return 1;
    }

    //SPLMERGE_TRACE("spl_merge: merge_type is %d \n", merge_type);
    /* input data new_buf can not be changed, so copy to merged_buf */
    memcpy(merged_buf, new_buf, new_buf_len);
    if (new_buf_len < merged_buf_len)
        memset(merged_buf+new_buf_len, 0xff, merged_buf_len - new_buf_len);
    if (MERGE_TYPE_MTD == merge_type) {
        memcpy(merged_buf, old_header, SPL_HEADER_LEN);
    } else if (MERGE_TYPE_EMMC == merge_type) {
        splFillCheckData((unsigned int *)merged_buf, checksum_len);
    /*} else if (MERGE_TYPE_UBI == merge_type) {*/
    } else {
        SPLMERGE_TRACE("spl_merge: Error! not support merge type(%d).\n", merge_type);
        return 1;
    }

    return 0;
}

#ifdef MAKE_EXECUTABLE

int main(int argc,char* argv[])
{
    int   fd,out_fd;
    char  *oldSpl,*newSpl, *outSpl;
    char *buf;
    int len,write_len;
    int checksum_len = SPL_CHECKSUM_LEN;
#ifdef CONFIG_NAND
    char  header_buf[SPL_HEADER_LEN];
    int   header_len;
#endif

    if(4 != argc) {
        SPLMERGE_TRACE("Usage:\n");
        SPLMERGE_TRACE("\tSPLMERGE oldSpl new_spl out_spl\n");
        return 1;//param error
    }
    SPLMERGE_TRACE("%s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
    oldSpl = argv[1];
    newSpl = argv[2];
    outSpl = argv[3];
    buf = malloc(SPL_CONTENT_MAX_LEN);
    memset(buf,0xff,SPL_CONTENT_MAX_LEN);
    if(!buf) {
        SPLMERGE_TRACE("SPLMERGE: malloc 32k oldbuf failed!\n");
        return 1;
    }
    fd = open(newSpl, O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
    if(fd < 0){
        SPLMERGE_TRACE("SPLMERGE: open old path failed\n");
        free(buf);
        return 1;
    }
    len = read(fd, buf, SPL_CONTENT_MAX_LEN);
    SPLMERGE_TRACE("SPLMERGE: len 0x%x\n",len);
    close(fd);
#ifdef CONFIG_NAND
    fd = open(oldSpl, O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
    if(fd < 0){
        SPLMERGE_TRACE("SPLMERGE: open old path failed\n");
        free(buf);
        return 1;
    }
    header_len = read(fd, header_buf, SPL_HEADER_LEN);
    SPLMERGE_TRACE("SPLMERGE: header_len 0x%x\n",header_len);
    memcpy(buf,header_buf,SPL_HEADER_LEN);
    close(fd);
#else
    if (len > SPL_CHECKSUM_LEN) {
        checksum_len = SPL_CONTENT_MAX_LEN;
    }
    splFillCheckData((unsigned int *)buf, checksum_len);
#endif

    fd = open(outSpl, O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0){
        SPLMERGE_TRACE("SPLMERGE: open out path failed\n");
        free(buf);
        return 1;
    }
    if (len < SPL_CHECKSUM_LEN)
        len = SPL_CHECKSUM_LEN;
    write_len = write(fd, buf, len);
    SPLMERGE_TRACE("SPLMERGE: write_len 0x%x\n",write_len);
    close(fd);
    free(buf);

    SPLMERGE_TRACE("SPLMERGE: spl merge success\n");
    return 0;
}

#endif /* MAKE_EXECUTABLE */
