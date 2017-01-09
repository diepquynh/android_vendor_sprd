#include "nvmerge.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>


static NVITEM_CFG nv_cfg[BACKUP_NUM_MAX];
static uint32 item_num = 0;
static uint32 fixnv_size = 0;
typedef struct _NV_HEADER {
    uint32 magic;
    uint32 len;
    uint32 checksum;
    uint32 version;
} nv_header_t;
#define NV_HEAD_MAGIC   0x00004e56
#define NV_VERSION      101
#define SECTOR_SIZE     512

static unsigned short ___calc_checksum(unsigned char *dat, unsigned long len)
{
    unsigned short num = 0;
    unsigned long chkSum = 0;
    while (len > 1) {
        num = (unsigned short)(*dat);
        dat++;
        num |= (((unsigned short)(*dat)) << 8);
        dat++;
        chkSum += (unsigned long)num;
        len -= 2;
    }
    if (len) {
        chkSum += *dat;
    }
    chkSum = (chkSum >> 16) + (chkSum & 0xffff);
    chkSum += (chkSum >> 16);
    return (unsigned short)(~chkSum);
}

static BOOLEAN _chkEcc(uint8 * buf, uint32 size, uint16 checksum)
{
    uint16 crc, crcOri;

    crc = ___calc_checksum(buf, size);
    NVMERGE_TRACE("NVMERGE:crc = 0x%x,checksum=0x%x\n", crc, checksum);

    return (crc == checksum);
}

static BOOLEAN _get_cfg(char *cfg_p)
{
    char line[512];
    uint32 i = 0;
    FILE *cfg_fd;

    if (!(cfg_fd = fopen(cfg_p, "r"))) {
        NVMERGE_TRACE("NVMERGE:open cfg file error\n");
        return FALSE;
    }
    memset(nv_cfg, 0, sizeof(nv_cfg));
    item_num = 0;
    NVMERGE_TRACE("NVMERGE:\tname\tid\t\n");
    while (fgets(line, sizeof(line), cfg_fd)) {
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        if (-1 == sscanf(line, "%s %x", nv_cfg[i].name, &nv_cfg[i].id)
            ) {
            continue;
        }
        NVMERGE_TRACE("NVMERGE:\t%s\t0x%x\t\n", nv_cfg[i].name, nv_cfg[i].id);
        i++;
        item_num++;
        if (BACKUP_NUM_MAX <= i) {
            NVMERGE_TRACE("NVMERGE: Max support %d item, this config has too many item!!!\n", BACKUP_NUM_MAX);
            fclose(cfg_fd);
            return FALSE;
        }
    }
    fclose(cfg_fd);
    return TRUE;
}

/*
    id:
    nvBuf:        nv data
    nvLength:    nv size
    itemPos:    item pos
    itemSize:    item size
*/
static BOOLEAN ___findItem( /*IN*/ uint32 id, /*IN*/ uint8 * nvBuf, /*IN*/ uint32 nvLength, /*OUT*/ uint32 * itemSize, /*OUT*/ uint32 * itemPos)
{
    uint32 offset = 4;
    uint16 tmp[2];
    while (1) {
        if (*(uint16 *) (nvBuf + offset) == INVALID_ID) {
            NVMERGE_TRACE("NVMERGE:___findItem find the tail\n");
            break;
        }
        if (offset + sizeof(tmp) > nvLength) {
            NVMERGE_TRACE("NVMERGE: ___findItem Surpass the boundary of the part\r\n");
            break;
        }
        memcpy(tmp, nvBuf + offset, sizeof(tmp));
        offset += sizeof(tmp);
        if (id == (uint32) tmp[0]) {
            *itemSize = (uint32) tmp[1];
            *itemPos = offset;
            NVMERGE_TRACE("NVMERGE:___findItem id = 0x%x\n", id);
            return TRUE;
        }
        offset += tmp[1];
        offset = (offset + 3) & 0xFFFFFFFC;
    }
    return FALSE;
}

static BOOLEAN __mergeItem(uint8 * oldBuf, uint32 oldNVlength, uint8 * newBuf, uint32 newNVlength)
{
    uint32 i;
    uint32 oldSize, oldPos;
    uint32 newSize, newPos;
    for (i = 0; i < item_num; i++) {
        if (!___findItem(nv_cfg[i].id, oldBuf, oldNVlength, &oldSize, &oldPos)) {
            continue;
        }
        if (!___findItem(nv_cfg[i].id, newBuf, newNVlength, &newSize, &newPos)) {
            continue;
        }
        NVMERGE_TRACE("NVMERGE:__mergeItem oldSize 0x%x newSize 0x%x\n", oldSize, newSize);
        if (oldSize == newSize) {
            memcpy(newBuf + newPos, oldBuf + oldPos, newSize);
            NVMERGE_TRACE("NVMERGE:__mergeItem success id = 0x%x\n", nv_cfg[i].id);
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

static char *__checkNVPartition(char *ori_path, char *bak_path, uint32 * nv_len_ptr)
{
    int ori_fd = 0, bak_fd = 0;
    uint8 *ori_buf, *bak_buf;
    nv_header_t *ori_header_ptr, *bak_header_ptr;
    uint32 ori_len, bak_len;
    int status = 0;
    char *old_buf = NULL;
    uint16 ori_ecc, bak_ecc;

    do {
        ori_header_ptr = malloc(SECTOR_SIZE);
        if (!ori_header_ptr)
            break;
        ori_fd = open(ori_path, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        if (ori_fd < 0) {
            NVMERGE_TRACE("__checkNVPartition open ori_path failed\n");
            break;
        }
        if (SECTOR_SIZE != read(ori_fd, ori_header_ptr, SECTOR_SIZE)) {
            NVMERGE_TRACE("__checkNVPartition read ori header failed\n");
            break;
        }
        ori_ecc = ori_header_ptr->checksum;
        ori_len = ori_header_ptr->len;
        ori_buf = malloc(ori_len);
        if (!ori_buf) {
            break;
        }
        NVMERGE_TRACE("__checkNVPartition oribuf ecc\n");
        if (ori_len == read(ori_fd, ori_buf, ori_len)) {
            if (!_chkEcc(ori_buf, ori_len, ori_ecc)) {
                NVMERGE_TRACE("__checkNVPartition oribuf ecc error\n");
                break;
            }
            status += 1;
        }
    } while (0);
    do {
        bak_header_ptr = malloc(SECTOR_SIZE);
        if (!bak_header_ptr)
            break;
        bak_fd = open(bak_path, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        if (bak_fd < 0) {
            NVMERGE_TRACE("__checkNVPartition open bak_path failed\n");
            break;
        }
        if (SECTOR_SIZE != read(bak_fd, bak_header_ptr, SECTOR_SIZE)) {
            NVMERGE_TRACE("__checkNVPartition read bakpath header failed\n");
            break;
        }
        bak_ecc = bak_header_ptr->checksum;
        bak_len = bak_header_ptr->len;
        bak_buf = malloc(bak_len);
        if (!bak_buf) {
            break;
        }
        NVMERGE_TRACE("__checkNVPartition bakbuf ecc\n");
        if (bak_len == read(bak_fd, bak_buf, bak_len)) {
            if (!_chkEcc(bak_buf, bak_len, bak_ecc)) {
                NVMERGE_TRACE("__checkNVPartition bak ecc error\n");
                break;
            }
            status += 1 << 1;
        }
    } while (0);
    switch (status) {
    case 0:
        NVMERGE_TRACE("both org and bak partition are damaged!\n");
        *nv_len_ptr = 0;
        memset(ori_header_ptr, 0, SECTOR_SIZE);
        memset(ori_buf, 0, ori_len);
        //write 0 to original path
        lseek(ori_fd, 0, SEEK_SET);
        write(ori_fd, ori_header_ptr, SECTOR_SIZE);
        write(ori_fd, ori_buf, ori_len);
        fsync(ori_fd);
        //write 0 to backup path
        lseek(bak_fd, 0, SEEK_SET);
        write(bak_fd, ori_header_ptr, SECTOR_SIZE);
        write(bak_fd, ori_buf, ori_len);
        fsync(bak_fd);
        //free mem
        free(ori_buf);
        free(bak_buf);
        break;
    case 1:
        NVMERGE_TRACE("bak partition is damaged!\n");
        old_buf = ori_buf;    //free at _merge
        *nv_len_ptr = ori_len;
        free(bak_buf);
        if (bak_fd < 0)
            break;
        lseek(bak_fd, 0, SEEK_SET);
        if (SECTOR_SIZE != write(bak_fd, ori_header_ptr, SECTOR_SIZE)
            || ori_len != write(bak_fd, ori_buf, ori_len)) {
            NVMERGE_TRACE("write backup partition error\n");
        }
        fsync(bak_fd);
        break;
    case 2:
        NVMERGE_TRACE("org partition is damaged!\n!");
        //ret_path = bak_path;
        old_buf = bak_buf;    //free at _merge
        *nv_len_ptr = bak_len;
        free(ori_buf);
        if (ori_fd < 0)
            break;
        lseek(ori_fd, 0, SEEK_SET);
        if (SECTOR_SIZE != write(ori_fd, bak_header_ptr, SECTOR_SIZE)
            || bak_len != write(ori_fd, bak_buf, bak_len)) {
            NVMERGE_TRACE("write org partition error\n");
        }
        fsync(ori_fd);
        break;
    case 3:
        NVMERGE_TRACE("both org and bak partition are ok!\n");
        //ret_path = ori_path;
        old_buf = ori_buf;    //free at _merge
        *nv_len_ptr = ori_len;
        free(bak_buf);
        break;
    default:
        free(ori_buf);
        free(bak_buf);
        NVMERGE_TRACE("%s: status error!\n", __FUNCTION__);
        break;
    }
    free(ori_header_ptr);
    free(bak_header_ptr);
    if (ori_fd > 0)
        close(ori_fd);
    if (bak_fd > 0)
        close(bak_fd);
    return old_buf;
}

static BOOLEAN _merge(char *old_p1, char *old_p2, char *new_p, char *out_p)
{
    char *old_buf = NULL;
    char *out_buf = NULL;
    int ret = 0;
    int old_fd = 0, out_fd = 0;
    BOOLEAN merge_result = TRUE;
    nv_header_t *out_header_ptr = NULL;
    nv_header_t *old_header_ptr = NULL;
    char header[SECTOR_SIZE];
    uint32 old_nvlength = 0;
    uint32 ota_update_time;

// check nv partition and fill in old_buf
    old_buf = __checkNVPartition(old_p1, old_p2, &old_nvlength);

//---
//out buffer
    do {
        out_fd = open(new_p, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
        if (out_fd < 0) {
            NVMERGE_TRACE("NVMERGE:findItemfromFile open out file error out_fd =%d\n", out_fd);
            break;
        }

        if (0 == fixnv_size) {
            fixnv_size = old_nvlength;
        }
        out_buf = malloc(fixnv_size + 4);
        if (NULL == out_buf) {
            break;
        }
        memset(out_buf, 0xFF, fixnv_size + 4);
        ret = read(out_fd, out_buf, fixnv_size);
        close(out_fd);
    } while (0);

//---
//      write timestamp
    if (NULL != old_buf) {
        memcpy(&ota_update_time, old_buf, 4);
        NVMERGE_TRACE("NVMERGE:get real timestamp = 0x%4x\n", ota_update_time);
        ota_update_time++;
        memcpy(out_buf, &ota_update_time, 4);
        //---
        //  begin merge
        merge_result = __mergeItem((uint8 *) old_buf, old_nvlength, (uint8 *) out_buf, fixnv_size);
    }
    //if(merge_result){
    //}
    out_header_ptr = (nv_header_t *) header;
    memset(out_header_ptr, 0x00, SECTOR_SIZE);
    out_header_ptr->magic = NV_HEAD_MAGIC;
    out_header_ptr->len = fixnv_size;
    out_header_ptr->version = NV_VERSION;
    out_header_ptr->checksum = (uint32) ___calc_checksum((unsigned char *)out_buf, fixnv_size);
    out_fd = open(out_p, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (out_fd < 0) {
        NVMERGE_TRACE("NVMERGE:Open output file fail\n");
        free(old_buf);
        free(out_buf);
        return FALSE;
    }
    ret = lseek(out_fd, 0, SEEK_SET);
    if (SECTOR_SIZE != write(out_fd, out_header_ptr, SECTOR_SIZE)
        || fixnv_size != write(out_fd, out_buf, fixnv_size))
        merge_result = FALSE;

    fsync(out_fd);
    free(old_buf);
    free(out_buf);
    close(out_fd);
    return merge_result;
}

int main(int argc, char *argv[])
{
#ifdef WIN32
    _get_cfg(cfg_path);
    _merge(old_path, new_path, out_path);
#else
    if (7 != argc) {
        NVMERGE_TRACE("Usage:\n");
        NVMERGE_TRACE("\tNVMERGE cfg_path old_path1 old_path2 new_path out_path fixnv_size\n");
        return 1;    //param error
    }
    NVMERGE_TRACE("%s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
    if (!_get_cfg(argv[1])) {
        NVMERGE_TRACE("NVMERGE:Get cfg file error !\n");
        return 1;    //error
    }

    fixnv_size = strtol(argv[6], 0, 16);
    NVMERGE_TRACE("NVMERGE:fixnv_size = 0x%x !\n", fixnv_size);
    if (!_merge(argv[2], argv[3], argv[4], argv[5])) {
        NVMERGE_TRACE("NVMERGE:_mergeItem error !\n");
        return 1;
    }
    NVMERGE_TRACE("NVMERGE:merge nv item success!\n");
    return 0;        //merge success
#endif
}
