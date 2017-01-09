#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
//SPRD: add header
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mount.h>
#include "memory.h"
#include <sys/vfs.h>
#include "crc32.h"
#include "gptdata.h"
#include "roots.h"
#include <cutils/log.h>
#include <dlfcn.h>

void InitializeGPTData(GPTData *pData)
{
    printf("InitializeGPTData enter!\n");
    pData->blockSize = SECTOR_SIZE;
    pData->diskSize = 0;
    pData->partitions = NULL;
    memset(pData->device, 0, sizeof(pData->device));
    pData->fd = 0;
    pData->mainCrcOk = 0;
    pData->secondCrcOk = 0;
    pData->mainPartsCrcOk = 0;
    pData->secondPartsCrcOk = 0;
    pData->mainHeader.numParts = 0;
    pData->numParts = 0;
    //SetGPTSize(NUM_GPT_ENTRIES);
    pData->backup_dir = NULL;
    pData->count = 0;
    pData->gpttab.import_part = malloc(sizeof(Partition) * IMPORT_PART_NUM);
    memset(pData->gpttab.import_part, 0, sizeof(Partition) * IMPORT_PART_NUM);
    strcpy(pData->gpttab.import_part[0].part_name, "uboot");
    strcpy(pData->gpttab.import_part[1].part_name, "sml");
    strcpy(pData->gpttab.import_part[2].part_name, "trustos");
    strcpy(pData->gpttab.import_part[3].part_name, "misc");
    strcpy(pData->gpttab.import_part[4].part_name, "recovery");
    strcpy(pData->gpttab.import_part[5].part_name, "miscdata");
    // Initialize CRC functions...
    load_volume_table();
    chksum_crc32gentab();
    return;
}

static int exec_cmd(const char** cmd) {
    char buffer[20];
    int status;
    int i = 0;
    int run_program_status;
    chmod(cmd[0], 0755);
    pid_t child = fork();
    if (child == 0) {
        printf("about to run execv");
        while(cmd[i]) {
            printf(" ");
            printf("%s", cmd[i++]);
        }
        printf("!\n");
        execv(cmd[0], (char* const*)cmd);
        printf("run_program: execv %s failed: %s\n", cmd[0], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    waitpid(child, &status, 0);
    // child process exit normally
    if (WIFEXITED(status)) {
        // get child process exit signal
        if ((run_program_status = WEXITSTATUS(status)) != 0) {
            printf("run_program: child exited with status %d\n",
                    WEXITSTATUS(status));
        }
    }
    // child process terminated with signal
    else if (WIFSIGNALED(status)) {
        printf("run_program: child terminated by signal %d\n",
                WTERMSIG(status));
        return -1;
    }
    // child process terminated, run program failed
    else {
        return -1;
    }
    return run_program_status;
}

int RepairFS(const char *mount_point)
{
    int status;
    int run_status = 0;
    Volume* v = volume_for_path(mount_point);
    if (v == NULL) {
        printf("unknown volume for path [%s]\n", mount_point);
        return -1;
    }
    if (strcmp(v->fs_type, "ext4") != 0){
        printf("file system is %s, but not ext4\n", v->fs_type);
        return 0;
    }

    chmod("/tmp/linker", 0755);
    chmod("/tmp/linker64", 0755);

    char * cmd[] = { "/tmp/e2fsck", "-fy", v->blk_device, '\0'};
    run_status = exec_cmd(cmd);

    if (0 == run_status || 1 == run_status) {
        run_status = 0;
    }
    return run_status;
}

void char16tochar(uint16_t *partname, char *buffer)
{
    int i;
    //printf("char16tochar entry!\n");
    for(i = 0; i < 36; i++) {
        buffer[i] = (char )partname[i];
    }
    //printf("char16tochar :partition name %s\n", buffer);
}

int RepartFlagFromBackdir(const char* back_dir, int mode, int *repart_flag) {
    char file_path[256];
    char buffer[64];
    FILE * fp;
    int ret_val = 0;
    char repart_flag_string[64];

    if (!repart_flag) {
        printf("GetRepartFlagFromBackdir : repart_flag = NULL\n");
        return 0;
    }
    strcpy(file_path, back_dir);
    strcat(file_path, "/");
    strcat(file_path, REPART_FLAG_FILE);
    printf("GetRepartFlagFromBackdir: repart_flag_file = %s\n", file_path);
    printf("mode=%d\n", mode);
    switch(mode) {
        case SET_FLAG:
        case CLEAR_FLAG:
            {
                sprintf(repart_flag_string, "repart_flag=%d\n", mode);
                printf("fputs %s\n", repart_flag_string);
                if( NULL == (fp = fopen(file_path,"w+"))) {
                    printf("can't open or create file %s: %s!\n", file_path, strerror(errno));
                    return 0;
                }
                if (EOF != fputs(repart_flag_string, fp)) {
                    ret_val = 1;
                }
            }
            break;
        case GET_FLAG:
            {
                if( NULL == (fp = fopen(file_path,"r"))) {
                    printf("can't open file %s: [%d] %s!\n", file_path, errno, strerror(errno));
                    if (errno == 2) {
                        *repart_flag = 0;
                        return 1;
                    } else {
                        return 0;
                    }
                }
                if ( NULL != fgets(buffer, sizeof(buffer), fp)) {
                    buffer[strlen(buffer) - 1] = '\0';
                    printf("fgets buffer = %s\n", buffer);
                    if (strstr(buffer, "repart_flag=1")) {
                        *repart_flag = 1;
                    }  else {
                        *repart_flag = 0;
                    }
                    ret_val = 1;
                }
            }
            break;
        default:
            ret_val = 0;
    }
    if (fp)
        fclose(fp);
    return ret_val;
}

int Seek(uint64_t sector, int fd) {
    uint64_t seekTo;
    uint64_t sought;
    int ret = SUCCESS;

    seekTo = sector * GetBlockSize(fd);
    sought = lseek64(fd, seekTo, SEEK_SET);
    if (sought != seekTo)
    {
        ret = FAILER;
    }

    return ret;
}


int OpenForRead(GPTData *pData, char *name)
{
    printf("OpenForRead enter!\n");

    pData->fd = open(name, O_RDWR);

    if(pData->fd < 0)
    {
        printf("can't open %s, %s\n", name, strerror(errno));
        return FAILER;
    }
    return SUCCESS;
}

int LoadPartitions(GPTData *pData, char *name)
{
    int ret = 1;
    uint32_t i;
    printf("LoadPartitions enter!\n");

    ret = OpenForRead(pData, name);
    if(FAILER == ret)
    {
        printf("LoadPartitions :: OpenForRead failer!\n");
        return RUN_ERROR;
    }
    pData->diskSize = GetDiskSize(pData->fd);
    pData->blockSize = GetBlockSize(pData->fd);

    printf("disksize = %llu, blocksize = %d\n", pData->diskSize, pData->blockSize);

    ret = ForceLoadGPTData(pData);
    if(ret == 0) {
        return ret;
    }
    for (i = 0; i<pData->gpttab.partition_num; i++) {
        pData->gpttab.partitions[i].part_size /= pData->blockSize;
    }

    return 1;
}

int GetBlockSize(int fd)
{
    int err = -1;
    int blockSize = 0;

    err = ioctl(fd, BLKSSZGET, &blockSize);
    if(err == -1)
    {
        printf("GetBlockSize error! errno = %s\n", strerror(errno));
        blockSize = SECTOR_SIZE;
    }

    return blockSize;
}

long long GetDiskSize(int fd)
{
    int err;
    long long sectors = 0;
    long long bytes = 0;
    struct stat64 st;

    long long sz;
    long long b;

    printf("GetDiskSize enter!\n");

    err = ioctl(fd, BLKGETSIZE64, &b);
    if(err <0)
    {
        printf("GetDiskSize error\n");
    }

    printf("b = %lld\n",b);
    sectors = b;
#if 0
    err = ioctl(fd, BLKGETSIZE, &sz);
    if(err)
    {
        printf("get blksize error, error = %s\n", strerror(errno));
        sectors = sz = 0;
    }
printf("GetDiskSize111 = %d\n", sz);
    if((!err) || (errno == EFBIG))
    {
        err = ioctl(fd, BLKGETSIZE64, &b);
        if (err || b == 0 || b == sz)
        {
            sectors = sz;
            printf("GetDiskSize222 = %d\n", sectors);
        }
        else
        {
            printf("GetDiskSize333 = %d\n", b);
            sectors = (b >> 9);
            printf("GetDiskSize444 = %ld\n", sectors);
        }
    }
#endif
    // Unintuitively, the above returns values in 512-byte blocks, no
    // matter what the underlying device's block size. Correct for this....
    sectors /= GetBlockSize(fd);

    // The above methods have failed, so let's assume it's a regular
    // file (a QEMU image, dd backup, or what have you) and see what
    // fstat() gives us....
    if((sectors == 0) || (err == -1))
    {
        printf("GetDiskSize all failed!\n");
        if (fstat64(fd, &st) == 0)
        {
            bytes = st.st_size;
            if (bytes % 512 != 0)
                printf("Warning: File size is not a multiple of 512 bytes! Misbehavior is likely!\n");
            sectors = bytes / 512;
        }
     }
    printf("sectors = %lld\n", sectors);
    return sectors;
}

int CheckTable(GPTData* pData, int *loadedTable)
{
    int ret = 0;
    printf("CheckTable: mainHeader.partitionEntriesLBA=%llu\n", pData->mainHeader.partitionEntriesLBA);
    printf("CheckTable: secondHeader.partitionEntriesLBA=%llu\n", pData->secondHeader.partitionEntriesLBA);
    pData->mainPartsCrcOk = LoadPartitionTable(pData, &(pData->mainHeader), pData->fd);
    if (pData->mainPartsCrcOk) {
        printf("LoadPartitions success from mainParts\n");
        *loadedTable = 1;
        return 1;
    } else {
        pData->secondPartsCrcOk = LoadPartitionTable(pData, &(pData->secondHeader), pData->fd);
        if (pData->secondPartsCrcOk) {
            printf("LoadPartitions success from secondParts\n");
            *loadedTable = 2;
            return 1;
        }
    }
    return ret;
}

int ForceLoadGPTData(GPTData *pData)
{
    int allOK;
    int validHeaders;
    int loadedTable = 1;
    int ret = 0;

    printf("ForceLoadGPTData enter!\n");
    printf("ForceLoadGPTData :: load main header!\n");
    allOK = LoadHeader(pData, &(pData->mainHeader), pData->fd, 1, &(pData->mainCrcOk));
    if(pData->mainCrcOk && (pData->mainHeader.backupLBA < pData->diskSize))
    {
        printf("ForceLoadGPTData :: pData->mainCrcOk, load second header, pData->mainHeader.backupLBA = %llu\n", pData->mainHeader.backupLBA);
        allOK = LoadHeader(pData, &(pData->secondHeader), pData->fd, pData->mainHeader.backupLBA, &(pData->secondCrcOk)) && allOK;
        printf("ForceLoadGPTData :: pData->secondCrcOk = %d\n", pData->secondCrcOk);
    }
    else
    {
        printf("ForceLoadGPTData :: load second header\n");
        allOK = LoadHeader(pData, &(pData->secondHeader), pData->fd, pData->diskSize - 1, &(pData->secondCrcOk)) && allOK;
        printf("ForceLoadGPTData :: pData->secondCrcOk = %d\n", pData->secondCrcOk);
        if(pData->mainCrcOk && (pData->mainHeader.backupLBA >= pData->diskSize))
        {
            printf("Warning! Disk size is smaller than the main header indicates!\n");
        }
    }
    if (!allOK)
    {
        printf("ForceLoadGPTData :: GPT invalid!\n");
            return allOK;
    }
    validHeaders = CheckHeaderValidity(pData);
    if (validHeaders > 0)
    {
        // if at least one header is OK....
        // GPT appears to be valid....
        //state = gpt_valid;
        if(validHeaders == 1)
        {
            // valid main header, invalid backup header
            printf("Caution: invalid backup GPT header, but valid main header, regenerating backup header from main header.\n");
            RebuildSecondHeader(pData);
            pData->secondCrcOk = pData->mainCrcOk; // Since regenerated, use CRC validity of main
        }
        else if(validHeaders == 2)
        {
            // valid backup header, invalid main header
            printf("Caution: invalid main GPT header, but valid backup, regenerating main header from second header.\n");
            RebuildMainHeader(pData);
            pData->mainCrcOk = pData->secondCrcOk; // Since regenerated, use CRC validity of second
        }
        if(CheckTable(pData, &loadedTable)){
            printf("load parts success! loadedTable = %d\n", loadedTable);
            if (loadedTable == 1) {  // mainHeader map to mainParts, need update secondHeader
                pData->numParts = pData->mainHeader.numParts;
                RebuildSecondHeader(pData);
            }
            else if(loadedTable == 2) { // secondHeader map to SecondParts, need update mainHeader
                pData->numParts = pData->secondHeader.numParts;
                RebuildMainHeader(pData);
            }
            return 1;
        }
        else {
            printf("load parts failed!\n");
            return 0;
        }
    }
    else
    {
          printf("mainHeader and backupHeader CRC check failed!\n");
          return 0;
    }
    return allOK;
}

int Packdata(const char* mount_point, const char *file_path, char ** cmd, int is_pack)
{
    int status;
    int run_status = 0;

    if (is_pack) {
        if (ensure_path_mounted(mount_point) != 0) {
            printf("PackData :: mount %s failed!\n", mount_point);
            return -1;
        }
    }
    else {
        if (format_volume(mount_point) != 0) {
            printf("UnPackData :: format %s failed!\n", mount_point);
            return -1;
        }
        if (ensure_path_mounted(mount_point) != 0) {
            printf("UnPackData :: mount %s failed!\n", mount_point);
            return -1;
        }
    }
    run_status = exec_cmd(cmd);
    if (0 != run_status) {
        printf("Packdata: exec_cmd failed!\n");
        return -1;
    }
    if (is_pack) {
        if (ensure_path_unmounted(mount_point) != 0) {
            printf("PackData :: unmount %s failed!\n", mount_point);
            return -1;
        }
    } else {
        if (0 != remove(file_path)) {
            printf("delete %s failed: %s", file_path, strerror(errno));
            return -1;
        }
    }
    return 0;
}

uint64_t get_system_df_free(char * disk)
{
    int ret;
    if(disk == NULL) {
        return -1;
    }
    struct statfs diskInfo;
    ret = statfs(disk,&diskInfo);
    if(ret < 0) {
        printf("statfs error: %s\n", strerror(errno));
        return -1;
    }
    unsigned long long totalBlocks = diskInfo.f_bsize;
    unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;

    printf("get_system_df_free:  disktotalBlocks %lld\n", totalBlocks);
    printf("get_system_df_free:  diskfree %lld\n", freeDisk);

    return freeDisk;
}

int BackupPartition(GPTData *pData, int partNum, long long partsize)
{
    int fd, ret;
    int readed;
    int writed;
    long long freeDisk;
    int dirlen;
    char filepath[256];
    char temppath[256];
    char buffer[36];
    char tmp[4096];

    dirlen = strlen(pData->backup_dir);
    if (pData->backup_dir[dirlen -1] != '/') {
        pData->backup_dir[dirlen] = '/';
        pData->backup_dir[dirlen +1] = 0;
    }
    strcpy(filepath, pData->backup_dir);
    char16tochar(pData->partitions[partNum].name, buffer);
    sprintf(tmp, ".bak.%s.img", buffer);
    strcat(filepath, tmp);
    printf("BackupPartition:: final filepath = %s    partsize = %lld Bytes\n", filepath, partsize);

    sprintf(temppath, "%s.bak.tmp.img",pData->backup_dir);
    if(0 == strcmp(buffer, "system") && pData->gpttab.block_based == 0) {
        char * cmd[] = {"/tmp/pack", "pack", "-s", "/system", "-p", temppath, "-S", "-v", "-b", '\0'};
        if (0 != Packdata("/system", temppath, cmd, 1)) {
            return -1;
        } else {
            ret = rename(temppath, filepath);
            if(ret < 0) {
                printf("rename %s to %s failed! : %s\n", temppath, filepath, strerror(errno));
                return -1;
            }
            return 1;
        }
    }

    fd = open(temppath,O_RDWR | O_CREAT | O_TRUNC, 0644);
    if ( fd < -1) {
        perror("BackupPartition::open file");
        return -1;
    }
    Seek(pData->partitions[partNum].firstLBA, pData->fd);

    int  readlen;
    while(partsize > 0) {
        if(partsize < 4096)
            readlen = partsize;
        else
            readlen = 4096;
        readed = read(pData->fd, tmp, readlen);
        if( (readed < 0) || (readed != readlen)) {
            printf("Backuppartition:: can't read form /dev/block/mmcblk0\n");
            return -1;
        }
        writed = write(fd, tmp, readed);
        if((writed < 0) || (readed != writed)) {
            printf("Backuppartition:: can't write to %s, %s\n", temppath, strerror(errno));
            return -1;
        }
        partsize -= 4096;
    }
    ret = rename(temppath, filepath);
    if(ret < 0) {
        printf("rename %s to %s failed! : %s\n", temppath, filepath, strerror(errno));
        perror("rename:");
        return -1;
    }
    uint64_t t = 1;  //if this partition have been backed up to SDcard, set the flag.
    pData->partitions[partNum].attributes |= (t << BACKUP_FLAG);
    printf("BackupPartition :: partition->attribute %llu\n", pData->partitions[partNum].attributes);
    close(fd);
    return 1;
}

void BackupCheck(GPTData *pData, int change_pos)
{
    uint32_t i, j;
    char buffer[36];
    uint64_t new_size = 0;
    uint64_t orig_size = 0;
    int z = 0;
    printf("BackupCheck::entry!\n");
    for(i = 0; i < pData->numParts; i++) {
        char16tochar(pData->partitions[i].name, buffer);
        for(j = 0; j < pData->gpttab.partition_num; j++) {
            if(strcmp(buffer, pData->gpttab.partitions[j].part_name) == 0) {
                if (change_pos < 0) {
                    pData->gpttab.partitions[j].backup_flag = 0;
                    break;
                }
                if(j < change_pos) {
                    pData->gpttab.partitions[j].backup_flag = 0;
                }
                else {
                    if(pData->gpttab.partitions[j].backup_flag == 1) {
                        if (0 != strcmp(buffer, "userdata")) {
                            orig_size = pData->partitions[i].lastLBA - pData->partitions[i].firstLBA + 1;
                            new_size = pData->gpttab.partitions[j].part_size;
                            printf("update option block_based = %d.\n", pData->gpttab.block_based);
			    if (0 == strcmp(buffer, "system") && pData->gpttab.block_based && (new_size < orig_size)) {
                                printf("BackupCheck: partition %s new_size[%llu blocks] < origsize[%llu blocks ] , update new partition size !\n", buffer, new_size, orig_size);
			    }else if(new_size < orig_size) {
                                pData->gpttab.partitions[j].part_size = orig_size;
                                printf("BackupCheck: partition %s new_size[%llu blocks] < origsize[%llu blocks ] ,used old size to replace config!\n", buffer, new_size, orig_size);
                            }
                        }
                        pData->gpttab.partitions[j].recovery_flag = 1;
                        for (z = 0; z < IMPORT_PART_NUM; z++) {
                            if (0 == strcmp(buffer, pData->gpttab.import_part[z].part_name)) {
                                pData->gpttab.partitions[j].recovery_flag = 0;
                                pData->gpttab.import_part[z].recovery_flag = 1;
                                pData->gpttab.import_part[z].part_size = pData->gpttab.partitions[j].part_size;
                                pData->gpttab.import_part[z].real_pos = j;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

int Resize2FS(GPTData* pData, const char *mount_point, int resize_mode, uint64_t* min_size)
{
    int status;
    int run_status = 0;
    char part_size[64];
    int pipefd[2];

    int i = 0;
    Volume* v = volume_for_path(mount_point);
    if (v == NULL) {
        printf("unknown volume for path [%s]\n", mount_point);
        return -1;
    }
    if (strcmp(v->fs_type, "ext4") != 0){
        printf("file system is %s, but not ext4\n", v->fs_type);
        return 0;
    }

    switch (resize_mode) {
        case RESIZEFS_MIN:
            {
                // uinit is 512B
                sprintf(part_size, "%llu", *min_size / FOURK_BLOCK);
                printf("Resize2FS convert partsize : %s 4K blcoks\n", part_size);
                char* cmd[] = {"/tmp/resize2fs", "-e", "-f", v->blk_device, part_size, '\0'};
                run_status = exec_cmd(cmd);
            }
            break;
        case RESIZEFS_FULL:
            {
                char* cmd[] = {"/tmp/resize2fs", "-e", "-f", v->blk_device, '\0'};
                run_status = exec_cmd(cmd);
            }
            break;
        case RESIZEFS_GETSIZE:
            {
                pipe(pipefd);
                char* temp = (char*)malloc(10);
                sprintf(temp, "%d", pipefd[1]);
                /*close(pipefd[0]);*/
                char* cmd[] = {"/tmp/resize2fs", "-c", temp, "-e", "-f", "-M", v->blk_device, '\0'};
                run_status = exec_cmd(cmd);
                if (run_status != 0)
                    return -1;
                close(pipefd[1]);
                FILE* from_child = fdopen(pipefd[0], "r");
                char buffer[1024];
                fgets(buffer, sizeof(buffer), from_child);
                printf("min_size = %s\n", buffer);
                fclose(from_child);
                if (min_size) {
                    *min_size = atoll(buffer);
                } else {
                    printf("Resize2FS: min_size = NULL");
                    return -1;
                }
            }
            break;
        default:
            return -1;
    }
    return run_status;
}

void MountCheck(const char* mount_point)
{
    int ret;
    int i;
    ret = ensure_path_mounted(mount_point);
    if (!ret) {
        for (i = 0; i < 5; i++) {
            // Try to umount 5 times before continuing on.
            // Should we try rebooting if all attempts fail?
            if (!ensure_path_unmounted(mount_point)) {
                printf("unmount %s success!\n", mount_point);
                break;
            }
            printf("CheckFS: umount(%s) failed!: %s\n", mount_point, strerror(errno));
            sleep(1);
        }
    }
}

int CheckFS(GPTData* pData, const char* mount_point)
{
    Volume* v = volume_for_path(mount_point);
    if(!strcmp(v->fs_type, "ext4")) {
        MountCheck("/data");
        if (RepairFS(mount_point)) {
            printf("CheckFS: RepairFS %s failed!\n", mount_point);
            return 0;
        }
        if (Resize2FS(pData, mount_point, RESIZEFS_FULL, NULL)) {
            printf("CheckFS: Resize2FS %s failed!\n", mount_point);
            return 0;
        }
    }
    return 1;
}

uint64_t CacluteDataSize(GPTData* pData)
{
    uint32_t  i;
    uint64_t  total_size = 0;
    uint64_t  new_data_size = 0;
    uint64_t  part_total_size = 0;
    printf("CacluteDataSize entry\n");
    //512B unit!!
    for (i = 0; i < pData->gpttab.partition_num - 1; i++) {
        total_size += pData->gpttab.partitions[i].part_size;
    }
    if (0 != strcmp(pData->gpttab.partitions[i].part_name, "userdata")) {
        return RUN_ERROR;
    }
    part_total_size = pData->mainHeader.lastUsableLBA - pData->mainHeader.firstUsableLBA + 1;
    new_data_size = part_total_size - total_size;
    printf("CacluteDataSize: part_total_size = %llu, total_size = %llu, new_data_size = %llu\n",
                part_total_size, total_size, new_data_size);
    return new_data_size;
}

int CacluteUsedSize(GPTData* pData, char* mount_point)
{
    struct statfs sf;
    uint64_t used_size = 0;
    uint64_t new_data_size;
    printf("CacluteUsedSize %s\n", mount_point);
    if (0 == ( new_data_size = CacluteDataSize(pData) ) ){
        printf("CacluteDataSize failed!\n");
        return RUN_ERROR;
    }
#if 0
    if (ensure_path_mounted(mount_point) != 0) {
        printf("CheckFreeSpaceEnough: mount userdata failed!");
        return RUN_ERROR;
    }
    if (statfs(mount_point, &sf) != 0) {
        printf("failed to statfs %s: %s\n", mount_point, strerror(errno));
        return RUN_ERROR;
    }
    used_size = sf.f_bsize * (sf.f_blocks - sf.f_bavail) + 100 * 1024 * 1024;
    // Byte unit
    if (new_data_size * pData->blockSize - used_size < USERDATA_TOLETATE_SIZE ) {
        printf("data partition if changed, can't store backup data!\n");
        return RUN_ERROR;
    }
    if(ensure_path_unmounted(mount_point) != 0) {
        printf("CacluteUsedSize: unmount %s failed\n", mount_point);
        return RUN_ERROR;
    }
#endif

#if 0
    if (0 != RepairFS(mount_point)) {
        printf("RepairFS %s failed!\n", mount_point);
        return RUN_ERROR;
    }
#endif
    if (0 == CheckFS(pData, mount_point)) {
        printf("CacluteUsedSize: CheckFS failed!\n");
        return RUN_ERROR;
    }
    if (0 != Resize2FS(pData, mount_point, RESIZEFS_GETSIZE, &used_size)) {
        printf("CacluteUsedSize Resize2FS get min_size failed!\n");
        return RUN_ERROR;
    }
    // Byte unit
    used_size *= FOURK_BLOCK;  //4K convert to Byte
    if (new_data_size * pData->blockSize - used_size < USERDATA_TOLETATE_SIZE ) {
        printf("data partition if changed, can't store backup data!\n");
        return RUN_ERROR;
    }
    return used_size;
}

int CheckFreeSpaceEnough(GPTData *pData)
{
    uint32_t i, j;
    struct statfs sf;
    uint64_t used_size = 0;
    uint64_t total_used_size = 0;
    uint64_t avail_size = 0;
    char buffer[36];
    printf("CheckFreeSpaceEnough, entry.\n");
    for(i = 0; i < pData->numParts; i++) {
        char16tochar(pData->partitions[i].name, buffer);
        for(j = 0; j < pData->gpttab.partition_num; j++) {
            if(strcmp(buffer, pData->gpttab.partitions[j].part_name) == 0) {
                if(pData->gpttab.partitions[j].backup_flag) {
                    if(strcmp(buffer, "userdata") == 0) {
                        if(0 == (used_size = CacluteUsedSize(pData, "/data"))) {
                            return RUN_ERROR;
                        }
                        //logic data uint is B
                        pData->gpttab.partitions[j].part_size = used_size;
                        printf("used_size = %llu\n", used_size);
                        total_used_size += used_size;
                    }
                    else {
                        total_used_size += (pData->partitions[i].lastLBA - pData->partitions[i].firstLBA + 1) * pData->blockSize;
                    }
                }
            }
        }
    }
    if (statfs(pData->backup_dir, &sf) != 0) {
        printf("failed to statfs %s: %s\n", pData->backup_dir, strerror(errno));
        return RUN_ERROR;
    }
    avail_size = sf.f_bsize * sf.f_bavail;
    printf("CheckFreeSpaceEnough: avail_size = %llu, total_used_size = %llu\n", avail_size, total_used_size);
    if (avail_size < total_used_size) {
        printf("BackupPartition failed,cause not enough space in backup_dir\n");
        printf("CheckFreeSpaceEnough: There has no enough size in backup dir\n");
        return RUN_NO_SPACE;
    }
    return RUN_SUCCESS;
}

int CheckExistBackup(GPTData *pData)
{
    uint32_t  i, ret;
    char buf[36];
    char filepath[256]={0};
    int dirlen = 0;
    int z = 0;

    dirlen = strlen(pData->backup_dir);
    if (pData->backup_dir[dirlen -1] != '/') {
        pData->backup_dir[dirlen] = '/';
        pData->backup_dir[dirlen +1] = 0;
    }
    for(i = 0; i < pData->gpttab.partition_num; i++) {
        strcpy(filepath, pData->backup_dir);
        sprintf(buf, ".bak.%s.img", pData->gpttab.partitions[i].part_name);
        printf("CheckExistBackup :: buf :%s\n", buf);
        strcat(filepath, buf);
        printf("BackupPartition:: final filepath =%s\n", filepath);
        ret = access(filepath, F_OK);
        if(ret == 0) {
            if (pData->repart_flag == 1) {
                if (pData->gpttab.partitions[i].backup_flag)
                    pData->gpttab.partitions[i].backup_flag = 0;
                pData->gpttab.partitions[i].recovery_flag = 1;
                for (z = 0; z < IMPORT_PART_NUM; z++) {
                    if (0 == strcmp(pData->gpttab.partitions[i].part_name, pData->gpttab.import_part[z].part_name)) {
                        pData->gpttab.partitions[i].recovery_flag = 0;
                        pData->gpttab.import_part[z].recovery_flag = 1;
                        pData->gpttab.import_part[z].part_size = pData->gpttab.partitions[i].part_size;
                        pData->gpttab.import_part[z].real_pos = i;
                        break;
                    }
                }
            } else {
                printf("CheckExistBackup:: repart_flag false, file exist, need to delete file %s\n", filepath);
                if (0 != remove(filepath)){
                    printf("delete %s failed: %s", filepath, strerror(errno));
                    return RUN_ERROR;
                }
                printf("delete %s success!\n", filepath);
            }
        }
    }
    return RUN_SUCCESS;
}

int BackupPartData2File(GPTData *pData, int change_pos)
{
    uint32_t  i, j ,z;
    char buffer[36];
    int retval = 1;
    int exist_backup;
    long long partsize = 0;
    //check if some partitioins will be changed, if so ,we shall back up them first.this is what we will do below.
    BackupCheck(pData, change_pos);
    if(RUN_ERROR == CheckExistBackup(pData)){
        return RUN_ERROR;
    }
    if(RUN_SUCCESS != (retval = CheckFreeSpaceEnough(pData))) {
        return retval;
    }
    printf("BackupPartData2File:: entry!\n");
    for(i = 0; i < pData->numParts; i++) {
        char16tochar(pData->partitions[i].name, buffer);
        for(j = 0; j < pData->gpttab.partition_num; j++) {
            if(strcmp(buffer, pData->gpttab.partitions[j].part_name) == 0) {
                if (pData->gpttab.partitions[j].backup_flag == 1) {
                    printf("backuppartdata2File:: backup %s part.\n", buffer);
                    if (0 == strcmp(buffer, "userdata")) {
#if 0
                        if (0 != RepairFS("/data")) {
                            printf("RepairFS %s failed!\n", "userdata");
                            return RUN_ERROR;
                        }
#endif
                        partsize = pData->gpttab.partitions[j].part_size;

                        if (0 != Resize2FS(pData, "/data", RESIZEFS_MIN , &partsize)) {
                            printf("Resize2fs %s failed!\n", "userdata");
                            return RUN_ERROR;
                        }
                    }else if (0 == strcmp(buffer, "system")) {
			partsize = pData->blockSize * pData->gpttab.partitions[j].part_size;
			printf("BackupPartData2File:: %16s backup size is %16llu Bytes!\n", buffer, partsize);
		    }
                    else {
                        //system partition never allow shrink!!
                        partsize = pData->blockSize * (pData->partitions[i].lastLBA - pData->partitions[i].firstLBA + 1);
			printf("BackupPartData2File:: %16s backup size is %16llu Bytes!\n", buffer, partsize);
                    }
                    retval = BackupPartition(pData, i, partsize);
                    if(retval == -1) {
                        printf("backup partition data failed! \n");
                        return RUN_ERROR;
                    }
                }
                else {
                    printf("backupPartdata2FIle:: partition %s need not backup!\n", pData->gpttab.partitions[j].part_name);
                }
            }
        }
    }
    return RUN_SUCCESS;
}

void chartochar16(char *partname, uint16_t *buffer)
{
    uint32_t i;
    for(i = 0; i < strlen(partname); i++) {
        buffer[i] = partname[i];
    }
}


int ChangePartition(GPTData *pData, int change_pos)
{
    int retval = RUN_SUCCESS; // assume there'll be no problems
    uint16_t buffer[36];
    memset(buffer, 0, 36);
    int new_pos = change_pos;
    GPTPart * parts ;
    uint64_t startSector, endSector;

    if(change_pos < 0 ) {
        return RUN_SUCCESS;
    }
    parts = malloc( sizeof(GPTPart) * pData->gpttab.partition_num);
    memset(parts, 0, sizeof(GPTPart) * pData->gpttab.partition_num);
    memcpy(parts, pData->partitions, sizeof(GPTPart) * change_pos);
    if (change_pos == 0) {
        startSector = pData->partitions[0].firstLBA;
    } else {
        startSector = pData->partitions[change_pos - 1].lastLBA + 1;
    }
    for (new_pos = change_pos; new_pos < pData->gpttab.partition_num ; new_pos++) {
        memcpy(parts[new_pos].PartType, pData->partitions[change_pos].PartType, 16);
        memcpy(parts[new_pos].GUIDData, pData->partitions[change_pos].GUIDData, 16);
        parts[new_pos].GUIDData[15] += 1;
        memcpy(&parts[new_pos].attributes, &pData->partitions[change_pos].attributes, sizeof(uint64_t));
        memset(buffer, 0, 36);
        chartochar16(pData->gpttab.partitions[new_pos].part_name, buffer);
        memcpy(parts[new_pos].name, buffer, 36);
        parts[new_pos].firstLBA = startSector;
        if (new_pos != pData->gpttab.partition_num -1) {
            endSector = startSector + pData->gpttab.partitions[new_pos].part_size -1;
            parts[new_pos].lastLBA = endSector;
            startSector = endSector + 1;
        }
    }
    new_pos -= 1;
    // for userdata partition
    if (0 != strcmp(pData->gpttab.partitions[new_pos].part_name, "userdata")) {
        printf("ChangePartition:: Partition change failed, the last partition is %s, should be userdata !!\n",
                                                pData->gpttab.partitions[new_pos].part_name);
        return RUN_ERROR;
    }
    endSector = pData->mainHeader.lastUsableLBA;
    parts[new_pos].lastLBA = endSector;

    free(pData->partitions);
    pData->partitions = parts;
    pData->numParts = pData->gpttab.partition_num;
    pData->mainHeader.numParts = pData->numParts;
    pData->secondHeader.numParts = pData->numParts;
    return RUN_SUCCESS;
}

int CheckPartitionChange(GPTData *pData)
{
    uint32_t i;
    uint32_t j;
    uint64_t orig_size = 0;
    uint64_t new_size = 0;
    char buffer[36];

    printf("CheckPartitionChange:: entry!\n");
    for(i = 0; i< pData->numParts; i++) {
        char16tochar(pData->partitions[i].name, buffer);
        orig_size = pData->partitions[i].lastLBA - pData->partitions[i].firstLBA + 1;
        if (pData->partitions[i].lastLBA == 0)
            continue;
        if (i >= pData->gpttab.partition_num) {
            return i;
        }
        new_size = pData->gpttab.partitions[i].part_size;
        printf("name = %16s, orig_size = %10llu, new_size = %10llu\n", buffer, orig_size, new_size);
        if(strcmp(buffer, "userdata") == 0)
            continue;
        else if(strcmp(buffer, "system") == 0) {
            printf("update option block_based = %d.\n", pData->gpttab.block_based);
            if(pData->gpttab.block_based){
		    //system partition name if not same, return changed
		    if(strcmp(pData->gpttab.partitions[i].part_name, buffer) != 0) {
                        for(j=i;j<pData->numParts;j++){
                            if(strcmp(pData->gpttab.partitions[i].part_name, buffer) == 0)
                                 break;
                        }
                        pData->gpttab.partitions[j].part_size = new_size;
		        printf("system partion new size [%llu] orinal size[%llu] partition order changed!.\n", new_size, orig_size);
		        return i;
		    }
		    else {
                        //system partition size changed
			if ( new_size != orig_size) {
			    printf("system partion new size [%llu]is is not equal to orinal size[%llu].\n", new_size, orig_size);
			    pData->gpttab.partitions[i].part_size = new_size;
			    return i;
			}
		    }
            }else {
		    //system partition name if not same, return changed
		    if(strcmp(pData->gpttab.partitions[i].part_name, buffer) != 0) {
			return i;
		    }
		    else {
			//system_tora_size  M unit
			//partsize 512B unit
			if ( new_size < orig_size) {
			    printf("system partion new size [%llu]is less than orinal size[%llu].\n", new_size, orig_size);
			    continue;
			}
			else {
			    if( new_size - orig_size >= pData->gpttab.system_tora_size * 2048) {
				printf("system partition new size is bigger than orignalsize plus torlerate_size.\n");
				pData->gpttab.partitions[i].part_size = new_size;
				return i;
			    }
			}
		    }
            }
        }
        else if (orig_size < new_size) {
            return i;
        }
        if(strcmp(pData->gpttab.partitions[i].part_name, buffer) != 0) {
            return i;
        }
    }
    return -1;
}

void dumpHeaders(GPTData* pData, GPTHeader* header)
{
    if (header == &pData->mainHeader) {
        printf("dumpHeaders: ==================load main header!\n");
    }
    else {
        printf("dumpHeaders: ==================load backup header!\n");
    }
    printf("Header.signature= %llu\n",header->signature);
    printf("Header.revision= %u\n",header->revision);
    printf("Header.headerCRC= %u\n",header->headerCRC);
    printf("Header.reserved= %u\n",header->reserved);
    printf("Header.headerSize= %u\n",header->headerSize);
    printf("Header.currentLBA= %llu\n",header->currentLBA);
    printf("Header.backupLBA= %llu\n",header->backupLBA);
    printf("Header.firstUsableLBA= %llu \n",header->firstUsableLBA);
    printf("Header.lastUsableLBA= %llu\n",header->lastUsableLBA);
    printf("Header.partitionEntriesLBA= %llu\n",header->partitionEntriesLBA);
    printf("Header.numParts= %u\n",header->numParts);
    printf("Header.sizeOfPartitionEntries= %u\n",header->sizeOfPartitionEntries);
    printf("Header.partitionEntriesCRC= %u\n",header->partitionEntriesCRC);
}

int RefreshDevPath(){
    char* wrapper_cmd[] = {"/sbin/path_wrapper", '\0'};
    int run_status = 0;
    DIR *dirp;
    struct dirent *dir;
    struct stat buf;

    if (access(wrapper_cmd[0], R_OK))
      return -1;

    if((dirp=opendir(FIX_PATH))==NULL)
      return -1;

    if(chdir(FIX_PATH)==-1)
      return -1;

    while(dir=readdir(dirp))
    {
        if((strcmp(dir->d_name,".")==0) || (strcmp(dir->d_name,".."))==0)
            continue;
        if(stat(dir->d_name,&buf)==-1)
            printf("stat error\n");
        if(remove(dir->d_name)==-1)
            printf("%s", dir->d_name);
    }

    run_status = exec_cmd(wrapper_cmd);
    if (0 != run_status) {
        printf("/sbin/path_wrapper: exec_cmd failed!\n");
        return -1;
    }
    return 1;
}


int SaveGPTData(GPTData *pData, int change_pos)
{
    int allOK = 1;
    if (change_pos < 0) {
        return allOK;
    }
    printf("SaveGPTData enter!\n");
    //MoveSecondHeaderToEnd(pData);
    RebuildSecondHeader(pData);
    RecomputeCRCs(pData);

    // As per UEFI specs, write the secondary table and GPT first....
    allOK = allOK && SavePartitionTable(pData, pData->fd, pData->secondHeader.partitionEntriesLBA);

    // Now write the secondary GPT header...
    allOK = allOK && SaveHeader(&(pData->secondHeader), pData->fd, pData->secondHeader.currentLBA);
    sync();
    // Now write the main partition tables...

    allOK = allOK && SavePartitionTable(pData, pData->fd, pData->mainHeader.partitionEntriesLBA);

    // Now write the main GPT header...
    allOK = allOK && SaveHeader(&(pData->mainHeader), pData->fd, pData->mainHeader.currentLBA);

    DiskSync(pData->fd);
    RefreshDevPath();
    return allOK;
}

void Close(int fd)
{
    printf("Close enter!\n");

    if(close(fd) < 0)
    {
        printf("Close device error : %s", strerror(errno));
    }
}


int DiskSync(int fd)
{
    int i;
    int ret = 0;

    printf("DiskSync enter!\n");

    sync();
    usleep(500000); // Theoretically unnecessary, but ioctl() fails sometimes if omitted....
    fsync(fd);

    i = ioctl(fd, BLKRRPART);
    if(i < 0)
    {
        printf("Warning: The kernel is still using the old partition table. The new table will be used at the next reboot.\n");
    }
    else
    {
        ret = 1;
    }

    sync();
	usleep(500000); // must add to avoid format_volume error !
	fsync(fd);
    return ret;
}

int SaveHeader(GPTHeader *header, int fd, uint64_t sector)
{
    int allOK = 1;
    int blockSize;

    printf("SaveHeader enter!\n");
    blockSize = GetBlockSize(fd);
    if(Seek(sector, fd))
    {
        printf("SaveHeader :: seek ok!\n");
        if(Write(header, blockSize, fd) == -1)
        {
            allOK = 0;
            printf("SaveHeader :: Write failer!\n");
        }
    }
    else
    {
        allOK = 0;
        printf("SaveHeader :: seek failer!\n");
    }

    return allOK;
}


int SavePartitionTable(GPTData *pData, int fd, uint64_t sector)
{
    int allOK = 1;

    printf("SavePartitionTable enter!\n");

    if(Seek(sector, fd))
    {
        printf("SavePartitionTable :: seek ok!\n");
        if(Write(pData->partitions, pData->mainHeader.sizeOfPartitionEntries * pData->numParts, fd) == -1)
        {
            printf("SavePartitionTable :: write failer!\n");
            allOK = 0;
        }
    }
    else
    {
        allOK = 0;
        printf("SavePartitionTable :: seek failer!\n");
    }

    return allOK;
}

int Write(void* buffer, int numBytes, int fd)
{
    int i;
    int numBlocks;
    char* tempSpace;
    int ret = 0;
    int blockSize = 512;

    blockSize = GetBlockSize(fd);
    if(numBytes <= blockSize)
    {
        numBlocks = 1;
        tempSpace = (char *)malloc(sizeof(char) * blockSize);
    }
    else
    {
        numBlocks = numBytes / blockSize;
        if((numBytes % blockSize) != 0)
        {
            numBlocks++;
        }
        tempSpace = (char *)malloc(sizeof(char) * numBlocks * blockSize);
    }

    memcpy(tempSpace, buffer, numBytes);

    for(i = numBytes; i < numBlocks * blockSize; i++)
    {
        tempSpace[i] = 0;
    }

    ret = write(fd, tempSpace, numBlocks * blockSize);
    if(ret < 0) {
        perror("write error:");
    }

    // Adjust the return value, if necessary....
    if(((numBlocks * blockSize) != numBytes) && (ret > 0))
    {
        ret = numBytes;
    }

    return ret;
}

int Read(void* buffer, int numBytes, int fd)
{
    int blockSize;
    int numBlocks;
    char* tempSpace;
    int ret = SUCCESS;

    printf("Read enter!\n");

    blockSize = GetBlockSize(fd);
    if(numBytes <= blockSize)
    {
        numBlocks = 1;
        printf("Read :: numBlocks = 1\n");
        tempSpace = (char* )malloc(blockSize);
    }
    else
    {
        numBlocks = numBytes / blockSize;
        if((numBytes % blockSize) != 0)
        {
            numBlocks++;
        }
        tempSpace = (char *)malloc(numBlocks * blockSize);
    }

    if(tempSpace == NULL)
    {
         printf("Unable to allocate memory in Read()!\n");
         return FAILER;
    }

    // Read the data into temporary space, then copy it to buffer
    ret = read(fd, tempSpace, numBlocks * blockSize);
    memcpy(buffer, tempSpace, numBytes);

    // Adjust the return value, if necessary....
    if(((numBlocks * blockSize) != numBytes) && (ret > 0))
    {
        ret = numBytes;
    }

    free(tempSpace);
    tempSpace = NULL;

    return ret;
}



void RecomputeCRCs(GPTData *pData)
{
    uint32_t crc;
    uint32_t hSize;

    printf("RecomputeCRCs enter!\n");

    if(pData->mainHeader.headerSize > sizeof(pData->mainHeader))
    {
        hSize = pData->secondHeader.headerSize = pData->mainHeader.headerSize = HEADER_SIZE;
    }
    else
    {
        hSize = pData->secondHeader.headerSize = pData->mainHeader.headerSize;
    }

    // Compute CRC of partition tables & store in main and secondary headers
    crc = chksum_crc32((unsigned char*)pData->partitions, pData->numParts * GPT_SIZE);
    pData->mainHeader.partitionEntriesCRC = crc;
    pData->secondHeader.partitionEntriesCRC = crc;

    pData->mainHeader.headerCRC = 0;
    pData->secondHeader.headerCRC = 0;
    crc = chksum_crc32((unsigned char*)&(pData->mainHeader), hSize);
    pData->mainHeader.headerCRC = crc;
    crc = chksum_crc32((unsigned char*)&(pData->secondHeader), hSize);
    pData->secondHeader.headerCRC = crc;

    printf("RecomputeCRCs :: partitionEntriesCRC = %d, pData->mainHeader.headerCRC = %d, pData->secondHeader.headerCRC = %d\n",
              pData->mainHeader.partitionEntriesCRC, pData->mainHeader.headerCRC, pData->secondHeader.headerCRC);

    return;
}

void DisplayGPTData(GPTData *pData)
{
    uint32_t i;
    int j;
    char buffer[36];
    printf("DisplayGPTData enter!\n");

    pData->partitionSize = (uint64_t *)malloc(sizeof(uint64_t) * pData->numParts);

    for(i = 0; i < pData->numParts; i++)
    {
        char16tochar(pData->partitions[i].name, buffer);
        pData->partitionSize[i] = (pData->partitions[i].lastLBA - pData->partitions[i].firstLBA + 1) * GetBlockSize(pData->fd);
        printf("pData->partitions[%3d]    name %16s    firstLBA:%10llu     lastLBA:%10llu    partitionSize:%16llu Bytes = %8llu MB \n", 
            i, buffer, pData->partitions[i].firstLBA, pData->partitions[i].lastLBA ,
            pData->partitionSize[i], pData->partitionSize[i] / (1024 * 1024));
    }
}



int LoadPartitionTable(GPTData *pData, GPTHeader *header, int fd)
{
    int ret;
    int sizeOfParts;
    uint32_t newCRC;

    printf("LoadPartitionTable enter! header->partitionEntriesLBA = %llu\n", header->partitionEntriesLBA);

    ret = Seek(header->partitionEntriesLBA, fd);
    if(ret == 1)
    {
        sizeOfParts = header->numParts * header->sizeOfPartitionEntries;
        printf("LoadPartitionTable :: header->numParts = %d, header->sizeOfPartitionEntries = %d\n",
            header->numParts, header->sizeOfPartitionEntries);

        if(Read(pData->partitions, sizeOfParts, fd) != sizeOfParts)
        {
            printf("Warning! Read error, Misbehavior now likely!\n");
            return 0;
        }
        newCRC = chksum_crc32((unsigned char*)pData->partitions, sizeOfParts);
        printf("LoadPartitionTable :: newCRC = %u, header->partitionEntriesCRC = %u\n", newCRC, header->partitionEntriesCRC);
        return (newCRC == header->partitionEntriesCRC);
    }
    return ret;
}



void RebuildSecondHeader(GPTData *pData)
{
    pData->secondHeader.signature = GPT_SIGNATURE;
    pData->secondHeader.revision = pData->mainHeader.revision;
    pData->secondHeader.headerSize = pData->mainHeader.headerSize;
    pData->secondHeader.headerCRC = 0;
    pData->secondHeader.reserved = pData->mainHeader.reserved;
    pData->secondHeader.currentLBA = pData->mainHeader.backupLBA;
    pData->secondHeader.backupLBA = pData->mainHeader.currentLBA;
    pData->secondHeader.firstUsableLBA = pData->mainHeader.firstUsableLBA;
    pData->secondHeader.lastUsableLBA = pData->mainHeader.lastUsableLBA;
    memcpy(&pData->secondHeader.diskGUID , &pData->mainHeader.diskGUID, 16);
    pData->secondHeader.partitionEntriesLBA = pData->secondHeader.lastUsableLBA + 1;
    pData->secondHeader.numParts = pData->mainHeader.numParts;
    pData->secondHeader.sizeOfPartitionEntries = pData->mainHeader.sizeOfPartitionEntries;
    pData->secondHeader.partitionEntriesCRC = pData->mainHeader.partitionEntriesCRC;
    memcpy(pData->secondHeader.reserved2, pData->mainHeader.reserved2, sizeof(pData->secondHeader.reserved2));
    pData->secondCrcOk = pData->mainCrcOk;
    //SetGPTSize(pData->secondHeader.numParts, 0);
}

void RebuildMainHeader(GPTData *pData)
{
    pData->mainHeader.signature = GPT_SIGNATURE;
    pData->mainHeader.revision = pData->secondHeader.revision;
    pData->mainHeader.headerSize = pData->secondHeader.headerSize;
    pData->mainHeader.headerCRC = 0;
    pData->mainHeader.reserved = pData->secondHeader.reserved;
    pData->mainHeader.currentLBA = pData->secondHeader.backupLBA;
    pData->mainHeader.backupLBA = pData->secondHeader.currentLBA;
    pData->mainHeader.firstUsableLBA = pData->secondHeader.firstUsableLBA;
    pData->mainHeader.lastUsableLBA = pData->secondHeader.lastUsableLBA;
    memcpy(&pData->mainHeader.diskGUID , &pData->secondHeader.diskGUID, 16);
    pData->mainHeader.partitionEntriesLBA = 2;
    pData->mainHeader.numParts = pData->secondHeader.numParts;
    pData->mainHeader.sizeOfPartitionEntries = pData->secondHeader.sizeOfPartitionEntries;
    pData->mainHeader.partitionEntriesCRC = pData->secondHeader.partitionEntriesCRC;
    memcpy(pData->mainHeader.reserved2, pData->secondHeader.reserved2, sizeof(pData->mainHeader.reserved2));
    pData->mainCrcOk = pData->secondCrcOk;
    //SetGPTSize(pData->mainHeader.numParts, 0);
}

int CheckHeaderValidity(GPTData *pData)
{
    int valid = 3;

    printf("pData->mainHeader.signature = %x\n", pData->mainHeader.signature);
    printf("pData->mainHeader.revision = %x\n", pData->mainHeader.revision);
    printf("pData->secondHeader.signature = %x\n", pData->secondHeader.signature);
    printf("pData->secondHeader.revision = %x\n", pData->secondHeader.revision);

    if((pData->mainHeader.signature != GPT_SIGNATURE) || !pData->mainCrcOk)
    {
        valid -= 1;
        printf("CheckHeaderValidity :: check main header crc failer!\n");
        printf("check mainHeader signature %d\n", pData->mainHeader.signature == GPT_SIGNATURE);
        dumpHeaders(pData, &pData->mainHeader);
    }
    else if(((pData->mainHeader).revision != 0x00010000) && valid)
    {
        valid -= 1;
        printf("CheckHeaderValidity :: Unsupported GPT version in main header");
    }

    if((pData->secondHeader.signature != GPT_SIGNATURE) || !pData->secondCrcOk)
    {
        valid -= 2;
        printf("CheckHeaderValidity :: check second header crc failer!\n");
        printf("CheckHeaderValidity check secondHeader dumpHeaders.!\n");
        dumpHeaders(pData, &pData->secondHeader);
    }
    else if((pData->secondHeader.revision != 0x00010000) && valid)
    {
        valid -= 2;
        printf("CheckHeaderValidity :: Unsupported GPT version in second header");
    }

    printf("CheckHeaderValidity :: valid = %d\n", valid);

    return valid;
}

int LoadHeader(GPTData *pData, GPTHeader *header, int fd, uint64_t sector, int *crcOk)
{
    int allOK = 1;
    GPTHeader tempHeader;
    int blockSize;

    blockSize = GetBlockSize(fd);
    Seek(sector, fd);
    if(Read(&tempHeader, blockSize, fd) != blockSize)
    {
      printf("Warning! Read error; strange behavior now likely!\n");
      allOK = 0;
    }

    // Reverse byte order, if necessary
    //if (IsLittleEndian() == 0) {
    //  ReverseHeaderBytes(&tempHeader);
    //}

    *crcOk = CheckHeaderCRC(&tempHeader, fd);
    printf("LoadHeader :: crcOk = %d, tempHeader.numParts = %d\n", *crcOk, tempHeader.numParts);
    *header = tempHeader;
    printf("LoadHeader: dumpHeaders!\n");
    dumpHeaders(pData, header);
    if(allOK && (pData->numParts != tempHeader.numParts) && *crcOk)
    {
        printf("LoadHeader :: SetGPTSize\n");
        allOK = SetGPTSize(pData, tempHeader.numParts, 0);
    }
    pData->secondHeader.partitionEntriesLBA = pData->mainHeader.lastUsableLBA + 1;
    return allOK;
}

int SetGPTSize(GPTData *pData, uint32_t numEntries, int fillGPTSectors)
{
    int allOK = 1;
    GPTPart* newParts;
    uint32_t entriesPerSector;

    printf("SetGPTSize enter!\n");

    // First, adjust numEntries upward, if necessary, to get a number
    // that fills the allocated sectors
    entriesPerSector = pData->blockSize / GPT_SIZE;
    if(fillGPTSectors && ((numEntries % entriesPerSector) != 0))
    {
      printf("Adjusting GPT size\n");
      numEntries = ((numEntries / entriesPerSector) + 1) * entriesPerSector;
    }

    if(((numEntries != pData->numParts) || (pData->partitions == NULL)) && (numEntries > 0))
    {
        printf("SetGPTSize :: malloc memory for GPTPart!\n");

        newParts = (GPTPart *)malloc(sizeof(GPTPart) * numEntries);
        if(NULL != newParts)
        {
            pData->partitions = newParts;
        }
        pData->numParts = numEntries;
        numEntries = GPT_ENTRY_SIZE;
        pData->mainHeader.firstUsableLBA = ((numEntries * GPT_SIZE) / pData->blockSize) +
                                  (((numEntries * GPT_SIZE) % pData->blockSize) != 0) + 2 ;
        pData->secondHeader.firstUsableLBA = pData->mainHeader.firstUsableLBA;
        MoveSecondHeaderToEnd(pData);
    }

    printf("SetGPTSize :: pData->mainHeader.numParts = %d\n", pData->mainHeader.numParts);

    return allOK;
}

void MoveSecondHeaderToEnd(GPTData *pData)
{
    printf("MoveSecondHeaderToEnd enter!\n");

    pData->mainHeader.backupLBA = pData->secondHeader.currentLBA = pData->diskSize - 1;
    pData->mainHeader.lastUsableLBA = pData->secondHeader.lastUsableLBA = pData->diskSize - pData->mainHeader.firstUsableLBA;
    pData->secondHeader.partitionEntriesLBA = pData->secondHeader.lastUsableLBA + 1;
}

int CheckHeaderCRC(GPTHeader* header, int fd)
{
    uint32_t oldCRC;
    uint32_t newCRC;
    int hSize;
    uint8_t *temp;

    printf("CheckHeaderCRC enter!\n");

    // Back up old header CRC and then blank it, since it must be 0 for
    // computation to be valid
    oldCRC = header->headerCRC;
    header->headerCRC = 0;

    hSize = header->headerSize;

    printf("CheckHeaderCRC :: hSize = %d\n", hSize);

    //if (IsLittleEndian() == 0)
    //  ReverseHeaderBytes(header);

    if((hSize > GetBlockSize(fd)) || (hSize < HEADER_SIZE))
    {
        hSize = HEADER_SIZE;
        printf("Warning! Header size is invalid. Setting the header size for CRC computation to HEADER_SIZE\n");
    }
    else if((hSize > sizeof(GPTHeader)))
    {
        printf("Caution! Header size for CRC check is greater than sizeof(GPTHeader)\n");
    }

    printf("CheckHeaderCRC :: hSize = %d\n", hSize);

    temp = (uint8_t *)malloc(hSize);
    memset(temp, 0, hSize);
    if(hSize < sizeof(GPTHeader))
    {
        memcpy(temp, header, hSize);
    }
    else
    {
        memcpy(temp, header, sizeof(GPTHeader));
    }

    newCRC = chksum_crc32((unsigned char*)temp, hSize);
    free(temp);
    temp = NULL;

    //if (IsLittleEndian() == 0)
    //  ReverseHeaderBytes(header);

    header->headerCRC = oldCRC;
    printf("oldCRC = %d, newCRC = %d\n", oldCRC, newCRC);

    return (oldCRC == newCRC);
}

int RecoveryAPartition(GPTData *pData, int tabnum)
{
    int i ,ret = 1;
    int fd;
    long long partsize;
    long long filesize;
    int readed, writed;
    char file[100];
    char filepath[256];
    char tmp[4096];
    struct stat	 filestat;

    printf("\nRecoveryAPartition::entry\n");

    strcpy(filepath, pData->backup_dir);
    sprintf(file, ".bak.%s.img", pData->gpttab.partitions[tabnum].part_name);
    strcat(filepath, file);

    partsize = pData->blockSize * (pData->partitions[tabnum].lastLBA - pData->partitions[tabnum].firstLBA + 1);
    printf("RecoveryAPartition:: partsize %llu Bytes\n", partsize);
    if(0 == strcmp(pData->gpttab.partitions[tabnum].part_name, "system") && pData->gpttab.block_based == 0) {
        char * cmd[] = {"/tmp/unpack", "unpack", "-p", filepath, "-S", "-v", '\0'};
        if (0 != Packdata("/system", filepath, cmd, 0)) {
            return -1;
        } else {
            return 1;
        }
    }

    fd = open(filepath, O_RDONLY);
    if(fd < 0) {
        perror("RecoveryAPartition::");
        return -1;
    }

    ret = fstat(fd, &filestat);
    if(ret < 0) {
        perror("fstat::");
        return -1;
    }
    filesize = filestat.st_size;
    printf("RecoveryAPartition:: filepath  %s  filesize is %lld Bytes in sdcard!\n", filepath , filesize);
    if(filesize > partsize) {
        printf("RecoveryAPartition::filesize > partsize!\n");
        return -1;
    }

    printf("RecoveryAPartition:: seek to %llu blocks\n", pData->partitions[tabnum].firstLBA);
    Seek(pData->partitions[tabnum].firstLBA, pData->fd);

    int  readlen;
    while(filesize > 0) {
        if(filesize < 4096) {
            readlen = filesize;
        }
        else {
            readlen = 4096;
        }
        readed = read(fd, tmp, readlen);
        if((readed < 0) || (readed != readlen)){
            printf("Backuppartition:: can't read, error = %s\n", strerror(errno));
            return -1;
        }
        writed = write(pData->fd, tmp, readed);
        if((writed < 0) || (readed != writed)) {
            printf("Backuppartition:: can't write, error = %s\n", strerror(errno));
            return -1;
        }
        filesize -= writed;
    }

    printf("RecoveryAPartition:: recovery write success filesize = %lld \n", filesize);
    //SaveGPTData(pData);
    sync();
    fsync(pData->fd);

    uint64_t t = 1;  //if this partition data have been recoveryed to GPT, clear the flag.
    printf("RecoveryAPartition :: partition[%s]->artributes %llu\n", file , pData->partitions[tabnum].attributes);
    pData->partitions[tabnum].attributes &= ~(t << BACKUP_FLAG);
    printf("RecoveryAPartition :: partition[%s]->artributes %llu\n", file , pData->partitions[tabnum].attributes);
    close(fd);

    if (0 != remove(filepath)) {
        printf("delete %s failed: %s", filepath, strerror(errno));
    }
    return 1;
}

int RecoveryPartition(GPTData *pData)
{
    uint32_t  i, j;
    int ret;
    char buffer[36];
    printf("\nRecoveryPartition:: entry!\n");
    if(NULL == pData->backup_dir) {
        printf("RecoveryPartiton::backup_dir is NULL!\n");
        return 1;
    }
    for(i = 0; i < IMPORT_PART_NUM; i++) {
        if(pData->gpttab.import_part[i].recovery_flag == 1) {
            ret = RecoveryAPartition(pData, pData->gpttab.import_part[i].real_pos);
            if(ret < 0) {
                printf("RecoveryPartition:: recovery %s partition fail!\n", pData->gpttab.import_part[i].part_name);
                return 0;
            }
        }
    }
    for(i = 0; i < pData->gpttab.partition_num; i++) {
        char16tochar(pData->partitions[i].name, buffer);
        if(pData->gpttab.partitions[i].recovery_flag == 1) {
#if 1
            if (0 == strcmp(buffer, "userdata")) {
                if (format_volume("/data") != 0) {
                    printf("WipeData :: wipe data failed!\n");
                    return 0;
                }
                printf("FormatData :: format data success!\n");
            }
#endif
            ret = RecoveryAPartition(pData, i);
            if(ret < 0) {
                printf("RecoveryPartition:: recovery %s partition fail!\n", pData->gpttab.partitions[i].part_name);
                return 0;
            }
            if (0 == strcmp(buffer, "userdata")) {
#if 0
                if (0 != RepairFS("/data") ) {
                    printf("RepairFS %s failed!\n", "userdata");
                    return 0;
                }

                if (0 != Resize2FS(pData, "/data", RESIZEFS_FULL, NULL)) {
                    printf("Resize2fs %s failed!\n", "userdata");
                    return 0;
                }
#endif
                if (0 == CheckFS(pData, "/data")) {
                    printf("CheckFS failed!\n");
                    return RUN_ERROR;
                }
            }
        }
    }
    return 1;
}

int IfNeedFormatPartition(GPTData * pData, const int change_pos, const char* part_name, const char* mount_point)
{
    char buffer[36];
    int pos = 0;
    printf("IfNeedFormatPartition:: entry!\n");
    for(pos = 0; pos < pData->numParts; pos ++) {
        char16tochar(pData->partitions[pos].name, buffer);
        if(0 == strcmp(buffer, part_name)) {
            if(0 == strcmp(buffer, "userdata")) {
                if (pos != pData->numParts - 1) {
                    printf("FormatData failed:: The last partition [%d] should be userdata!\n", pos);
                    return 0;
                }
            }
            if(pData->gpttab.partitions[pos].recovery_flag == 0 &&
                ( (pos >= change_pos && change_pos >= 0) || pData->repart_flag) ) {
                printf("Dont't backup %s partition, need to format %s partition!\n", buffer, buffer);
                if (format_volume(mount_point) != 0) {
                    printf("IfNeedFormatPartition :: format %s failed!\n", buffer);
                    return 0;
                }
                printf("IfNeedFormatPartition :: format %s success!\n", buffer);
            }
        }
    }
    return 1;
}

int FinalDestory(GPTData *pData)
{
    Close(pData->fd);
    return 1;
}

static int ParseFileLine(GPTData *pData, FILE *fp)
{
    char *p;
    char buffer[256]={0};

    printf("ParseFileLine :: entry!\n");

    while(fgets(buffer, 256, fp) != NULL) {
        buffer[strlen(buffer) - 1] = '\0';
        p = strtok(buffer, " ");

        if (strcmp(p, "partition:") == 0) {
            pData->gpttab.partition_num++;
        }
    }

    if (pData->gpttab.partition_num <= 0) {
        printf("load config file failed\n!");
        return 0;
    }

    pData->gpttab.partitions = malloc( sizeof(Partition) * pData->gpttab.partition_num);
    memset(pData->gpttab.partitions, 0, sizeof(Partition) * pData->gpttab.partition_num);
    if (pData->gpttab.partitions == NULL) {
        printf("malloc failed!\n");
        perror("parse partitions from config file");
        return 0;
    }

    printf("ParseFileLine:: partition_num:%d\n",  pData->gpttab.partition_num);
    return 1;
}

int ParseConfigFile(GPTData *pData, char *cfg_name)
{
    printf("ParseConfigFile : entry!\n");

    int i, j;
    uint32_t  m,n,q,t, r;
    int ret;
    FILE *fp;
    char *p;
    char buffer[256] = {0};

    fp = fopen(cfg_name, "r");
    if(!fp)
    {
        printf("Cannot open file %s %s\n", cfg_name, strerror(errno));
        return FAILER;
    }

    ret = ParseFileLine(pData, fp);
    if(ret == 0) {
        return 0;
    }
    fseek(fp, 0, SEEK_SET);

    m = 0;
    while(fgets(buffer, 256, fp) != NULL) {
        buffer[strlen(buffer) - 1] = '\0';
        p = strtok(buffer, " ");

        if (strcmp(p, "config_version") == 0) {
            p = strtok(NULL, " ");
            printf("ParseConfigFile: configfile version : %s\n", p);
            if(p == NULL) {
                printf("ParseConfigFile: error options!\n");
                return 0;
            }
        }
        else if (strcmp(p, "partition:") == 0) {
            if(m < pData->gpttab.partition_num) {
                p = strtok(NULL, " ");
                if(p == NULL) {
                    printf("ParseConfigFile: error options!\n");
                    return 0;
                }
                strcpy(pData->gpttab.partitions[m].part_name, p);
                p = strtok(NULL, " ");
                pData->gpttab.partitions[m].part_size = atoll(p) * 1024 * 1024;
                p = strtok(NULL, " ");
                pData->gpttab.partitions[m].backup_flag = atoll(p);
                printf("ParseConfigFile: partname: %16s , partsize: %10llu , backupflag: %d\n", 
                    pData->gpttab.partitions[m].part_name, 
                    pData->gpttab.partitions[m].part_size,
                    pData->gpttab.partitions[m].backup_flag);
                m++;
            }
        }
        else if (strcmp(p, "system_torlarate:") == 0) {
            p = strtok(NULL, " ");
            pData->gpttab.system_tora_size = atoll(p);
            printf("ParseConfigFile: system_torlarate : %lld\n", pData->gpttab.system_tora_size);
        }
        else if (strcmp(p, "block_based:") == 0) {
            p = strtok(NULL, " ");
            pData->gpttab.block_based = atoi(p);
            printf("ParseConfigFile: block_based : %d\n", pData->gpttab.block_based);
        }
        else {
            printf("ParseConfigFile: invalid command line!\n");
        }
    }
    fclose(fp);
    return 1;
}

void DisplayConfigFile(GPTData * pData)
{
    uint32_t  i;

    printf("~~~~~~~~~~~~DisplayConfigFile~~~~~~~~~~~~~~\n");
    for (i = 0; i < pData->gpttab.partition_num; i++) {
        printf("partition item:%16s     part_size:%16llu     backup_flag:%d \n"
            , pData->gpttab.partitions[i].part_name
            , pData->gpttab.partitions[i].part_size
            , pData->gpttab.partitions[i].backup_flag);
    }
    printf("~~~~~~~~~~~DisplayConfigFile~~~~~~~~~~~~~~~\n");
}


