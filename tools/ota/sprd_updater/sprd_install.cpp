#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <selinux/label.h>
#include <selinux/selinux.h>
#include <ftw.h>
#include <sys/statfs.h>
#include <sys/capability.h>
#include <sys/xattr.h>
#include <linux/xattr.h>
#include <inttypes.h>

#include "bootloader.h"
#include "applypatch/applypatch.h"
#include "cutils/android_reboot.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "edify/expr.h"
/*#include "mincrypt/sha.h"*/
#include "minzip/DirUtil.h"
#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"

#include "install.h"
#include "updater/updater.h"
#ifdef USE_EXT4
#include "make_ext4fs.h"
#include "wipe.h"
#endif

// SPRD: add for ubi support
#include "ubiutils/ubiutils.h"
// SPRD: add for spl update
extern "C" {
#include "splmerge/splmerge.h"
}

// SPRD: add for format vfat
#include "vfat/vfat_format.h"

/* SPRD: modify for support ubi and bp update @{ */
static int mtd_partitions_scanned = 0;
static bool write_mtd_image(const char* name, Value* contents, const char* partition) {
    bool success = false;
    printf("%s: write_mtd_image to [%s]\n", name, partition);

    if (!mtd_partitions_scanned) {
        mtd_scan_partitions();
        mtd_partitions_scanned = 1;
    }

    const MtdPartition* mtd = mtd_find_partition_by_name(partition);
    if (mtd == NULL) {
        printf("%s: no mtd partition named \"%s\"\n", name, partition);
        return success;
    }

    MtdWriteContext* ctx = mtd_write_partition(mtd);
    if (ctx == NULL) {
        printf("%s: can't write mtd partition \"%s\"\n",
               name, partition);
        return success;
    }

    if (contents->type == VAL_STRING) {
        // we're given a filename as the contents
        char* filename = contents->data;
        FILE* f = fopen(filename, "rb");
        if (f == NULL) {
            printf("%s: can't open %s: %s\n",
                   name, filename, strerror(errno));
            goto done;
        }

        success = true;
        char* buffer = (char*) malloc(BUFSIZ);
        int read;
        while (success && (read = fread(buffer, 1, BUFSIZ, f)) > 0) {
            int wrote = mtd_write_data(ctx, buffer, read);
            success = success && (wrote == read);
        }
        free(buffer);
        fclose(f);
    } else {
        // we're given a blob as the contents
        ssize_t wrote = mtd_write_data(ctx, contents->data, contents->size);
        success = (wrote == contents->size);
    }
    if (!success) {
        printf("mtd_write_data to %s failed: %s\n",
               partition, strerror(errno));
    }

    if (mtd_erase_blocks(ctx, -1) == -1) {
        printf("%s: error erasing blocks of %s\n", name, partition);
    }
done:
    if (mtd_write_close(ctx) != 0) {
        printf("%s: error closing write of %s\n", name, partition);
    }
    printf("%s: return %d from write_mtd_image [%s]\n", name, success, partition);
    return success;
}

static bool read_from_mtd_partion(const char* name, const char* partition, char *buf, int len) {
    bool success = false;
    int read_len = 0;
    printf("%s: read_from_mtd_partion from [%s]\n", name, partition);

    if (!mtd_partitions_scanned) {
        mtd_scan_partitions();
        mtd_partitions_scanned = 1;
    }

    const MtdPartition* mtd = mtd_find_partition_by_name(partition);
    if (mtd == NULL) {
        printf("%s: no mtd partition named \"%s\"\n", name, partition);
        return success;
    }

    MtdReadContext* ctx = mtd_read_partition(mtd);
    if (ctx == NULL) {
        printf("%s: can't read mtd partition \"%s\"\n",
               name, partition);
        goto done;
    }

    read_len = mtd_read_data(ctx, buf, len);
    if (read_len != len) {
        printf("mtd_read_data from %s failed: %s\n",
               partition, strerror(errno));
        goto done;
    }
    success = true;

done:
    mtd_read_close(ctx);
    printf("%s: return %d from read_from_mtd_partion [%s]\n", name, success, partition);
    return success;
}

static int disk_write(int fd, const char *buf, int len, const char *devname)
{
    int write_len = 0;

    while (len) {
        int ret = write(fd, buf, len);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf("cannot write %d bytes to volume \"%s\": %s\n",
                              len, devname, strerror(errno));
            break;
        }

        if (ret == 0) {
            printf("cannot write %d bytes to volume \"%s\"\n", len, devname);
            break;
        }

        len -= ret;
        buf += ret;
        write_len += ret;
    }

    return write_len;
}

static bool write_data_to_disk(const char* name, Value* contents, int fd,
                               const char *partition, const char* part_type) {
    bool success = false;

    if (contents->type == VAL_STRING) {
        char* filename = contents->data;
        // we're given a filename as the contents
        FILE* f = fopen(filename, "rb");
        if (f == NULL) {
            printf("%s: can't open %s: %s\n",
                   name, filename, strerror(errno));
            goto done;
        }

        if (!strcmp(part_type, "UBI")) {
            struct stat temp;
            stat(filename, &temp);
            printf("%s:  update size is %ld \n", partition, temp.st_size);
            ubi_fupdate(fd, temp.st_size);
        }

        success = true;
        char* buffer = (char*) malloc(BUFSIZ);
        int read;
        while (success && (read = fread(buffer, 1, BUFSIZ, f)) > 0) {
            int wrote = disk_write(fd, buffer, read, partition);
            success = success && (wrote == read);
        }
        free(buffer);
        fclose(f);
    } else {
        // we're given a blob as the contents
        if (!strcmp(part_type, "UBI")) {
            ubi_fupdate(fd, contents->size);
        }
        ssize_t wrote = disk_write(fd, contents->data, contents->size, partition);
        success = (wrote == contents->size);
    }

    if (!success) {
        printf("write_data_to_disk to %s failed: %s\n",
               partition, strerror(errno));
    }
done:
    return success;
}

static bool write_emmc_or_ubi_image(const char* name, Value* contents,
                                    const char* partition, const char *part_type) {
    bool success = false;
    printf("%s: write_emmc_or_ubi_image <%s> to [%s]\n", name, part_type, partition);
    int fd = -1;
    if (!strcmp(part_type, "EMMC")) {
        fd = open(partition, O_RDWR | O_SYNC);
    } else if (!strcmp(part_type, "UBI")) {
        fd = ubi_open(partition, O_RDWR | O_SYNC);
    }
    if (fd < 0) {
        printf("%s: failed to open %s: %s\n", name, partition, strerror(errno));
        return success;
    }

    success = write_data_to_disk(name, contents, fd, partition, part_type);

    if (close(fd) != 0) {
        printf("%s: error closing %s (%s)\n", name, partition, strerror(errno));
        return success;
    }
    fsync(fd);
    // hack: sync and sleep after closing in hopes of getting
    // the data actually onto flash.
    printf("sleeping after close\n");
    sync();
    sleep(5);
    printf("%s: return %d from write_emmc_or_ubi_image <%s> to [%s]\n", name, success, part_type, partition);
    return success;
}
/* @} */

bool do_merge_spl(const char* name, Value* new_contents, const char* partition, const char *part_type) {
    bool success = false;
    int res = 0;
    Value* merged_contents;
    char *old_spl_header = NULL, *new_spl_buf, *merged_spl_buf;
    int spl_checksum_len = SPL_CHECKSUM_LEN;

    merged_contents = (Value*) malloc(sizeof(Value));
    merged_contents->type = VAL_BLOB;
    merged_contents->size = SPL_CONTENT_MAX_LEN;
    merged_contents->data = (char*) malloc(SPL_CONTENT_MAX_LEN);

    if (new_contents->type == VAL_STRING) {
        char* filename = new_contents->data;
        // we're given a filename as the contents
        FILE* f = fopen(filename, "rb");
        if (f == NULL) {
            printf("%s: can't open %s: %s\n",
                   name, filename, strerror(errno));
            goto done;
        }

        new_spl_buf = (char*) malloc(SPL_CONTENT_MAX_LEN);

        int read = fread(new_spl_buf, 1, SPL_CONTENT_MAX_LEN, f);
        success = (read > 0);
        fclose(f);
        if (!success) {
            printf("%s: read file %s return %d, error: %s\n",
                   name, filename, read, strerror(errno));
            free(new_spl_buf);
            goto done;
        }
        free(new_contents->data);
        new_contents->type = VAL_BLOB;
        new_contents->size = read;
        new_contents->data = new_spl_buf;
    } else {
        // we're given a blob as the contents
        new_spl_buf = new_contents->data;
    }

    if (new_contents->size > SPL_CHECKSUM_LEN) {
        spl_checksum_len = SPL_CONTENT_MAX_LEN;
    }
    printf("%s: new version spl len is %d\n", name, new_contents->size);
    printf("%s: spl_checksum_len is %d\n", name, spl_checksum_len);
    merged_spl_buf = (char*) merged_contents->data;
    if (!strcmp(part_type, "EMMC")) {
        res = spl_merge(MERGE_TYPE_EMMC, NULL, new_spl_buf, new_contents->size,
                        merged_spl_buf, merged_contents->size, spl_checksum_len);
        success = res == 0;
    /*} else if (!strcmp(part_type, "UBI")) {
        res = spl_merge(MERGE_TYPE_UBI, old_spl_header, new_spl_buf, merged_spl_buf);*/
    } else if (!strcmp(part_type, "MTD")) {
        old_spl_header = (char*) malloc(SPL_HEADER_LEN);
        memset(old_spl_header, 0xff, SPL_HEADER_LEN);
        success = read_from_mtd_partion(name, partition, old_spl_header, SPL_HEADER_LEN);
        if (!success) {
            goto done;
        }
        res = spl_merge(MERGE_TYPE_MTD, old_spl_header, new_spl_buf, new_contents->size,
                        merged_spl_buf, merged_contents->size, spl_checksum_len);
        success = res == 0;
    } else {
        printf("unknow partition type %s\n", part_type);
        goto done;
    }
    if (res) {
        printf("spl_merge return %d\n", res);
        goto done;
    }

    if (!strcmp(part_type, "EMMC")/* ||
        !strcmp(part_type, "UBI")*/) {
        success = write_emmc_or_ubi_image(name, merged_contents, partition, part_type);
    } else if (!strcmp(part_type, "MTD")) {
        success = write_mtd_image(name, merged_contents, partition);
    }

done:
    if (!success)
        printf("%s: do_merge_spl %s.\n", name, success?"success":"failed");
    FreeValue(merged_contents);
    return success;
}


// SPRD: add for spl update in recovery
// merge_spl(filename_or_blob, partition, partition_type)
Value* MergeSplFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* result = NULL;
    int res = 0;
    char* part_type = NULL;

    Value* part_type_value;
    Value* partition_value;
    Value* new_contents;

    if (ReadValueArgs(state, argv, 3, &new_contents, &partition_value, &part_type_value) < 0) {
        return NULL;
    }

    char* partition = NULL;
    if (partition_value->type != VAL_STRING) {
        ErrorAbort(state, "partition argument to %s must be string", name);
        goto done;
    }
    partition = partition_value->data;
    if (strlen(partition) == 0) {
        ErrorAbort(state, "partition argument to %s can't be empty", name);
        goto done;
    }
    if (new_contents->type == VAL_STRING && strlen((char*) new_contents->data) == 0) {
        ErrorAbort(state, "file argument to %s can't be empty", name);
        goto done;
    }
    if (part_type_value->type != VAL_STRING) {
        ErrorAbort(state, "part_type argument to %s must be string", name);
        goto done;
    }
    part_type = part_type_value->data;
    if (strlen(part_type) == 0) {
        ErrorAbort(state, "part_type argument to %s can't be empty", name);
        goto done;
    }
    printf("%s: MergeSplFn (%s, %s) \n", name, partition, part_type);

    res = do_merge_spl(name, new_contents, partition, part_type);

    if(res){
        result = strdup("t");
    }else{
        result = NULL;
    }
    printf("merge spl result = %s\n", result);

done:
    // SPRD: add for support ubi and bp update
    if (part_type_value) FreeValue(part_type_value);
    if (partition_value) FreeValue(partition_value);
    FreeValue(new_contents);
    return StringValue(result);
}

Value* WriteEmmcImageFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* result = NULL;

    char* partition = NULL;
    char* file_name = NULL;
    Value* contents;

    if (ReadArgs(state, argv, 2, &file_name, &partition) < 0)
        return NULL;

    contents = StringValue(file_name);
    bool success = write_emmc_or_ubi_image(name, contents, partition, "EMMC");

    printf("%s %s EMMC partition\n",
           success ? "wrote" : "failed to write", partition);

    result = strdup(success ? "t" : "");

done:
    if (partition)
        free(partition);
    FreeValue(contents);
    return StringValue(result);
}

/* SPRD: add for sysinfo partition @{ */
//open_file_contexts(path)
Value* OpenFileContextsFn(const char* name, State* state, int argc, Expr* argv[]){
    char * path;

    if (ReadArgs(state, argv, 1, &path) < 0) {
        return NULL;
    }

    struct selinux_opt seopts[] = {
      { SELABEL_OPT_PATH, path }
    };

    printf("file contexts path is %s\n", path);

    selabel_close(sehandle);

    sehandle = selabel_open(SELABEL_CTX_FILE, seopts, 1);

    if (!sehandle) {
        printf("open file contexts failed\n");
    }
    return StringValue(strdup("t"));
}
/* @} */

/* SPRD: add for support check_sd_avail_space_enough @{ */
size_t AvailSpaceForFile(const char* filename) {
    struct statfs sf;
    if (statfs(filename, &sf) != 0) {
        printf("failed to statfs %s: %s\n", filename, strerror(errno));
        return -1;
    }
    return sf.f_bsize * sf.f_bavail / 1024 / 1024;
}

size_t UsedSpaceForFile(const char* filename) {
    struct statfs sf;
    if (statfs(filename, &sf) != 0) {
        printf("failed to statfs %s: %s\n", filename, strerror(errno));
        return 0;
    }
    return sf.f_bsize * (sf.f_blocks - sf.f_bfree) / 1024 / 1024 ;
}

Value* CheckPathSpaceEnoughFn(const char* name, State* state, int argc, Expr* argv[]){
    size_t sd_avail_size = 0;
    size_t total_back_size = 0;
    size_t back_size = 0;
    int index_backup_item = 0;

    if (argc < 2) {
        return ErrorAbort(state, "%s() expects at least 2 arg", name);
    }

    char** args = ReadVarArgs(state, argc, argv);
    if (args == NULL) {
        return NULL;
    }

    sd_avail_size = AvailSpaceForFile(args[0]);
    free(args[0]);

    for(index_backup_item = 1;index_backup_item < argc; index_backup_item++) {
        back_size = UsedSpaceForFile(args[index_backup_item]);
        printf("back_size for %s = %dM\n", args[index_backup_item], back_size);
        free(args[index_backup_item]);
        total_back_size += back_size;
    }

    printf("sd_avail_size = %dM, total_back_size = %dM\n", sd_avail_size, total_back_size);
    return StringValue(strdup(sd_avail_size > total_back_size ? "t" : ""));
}
/* @} */

/* SPRD: add for check system space if enough @{ */
Value* CheckSystemSpaceEnoughFn(const char* name, State* state, int argc, Expr* argv[]){
    size_t need_size = 0;
    char * bytes_str;
    char * endptr;
    char * is_full_ota;
    size_t system_size = 0;
    int    success = 1;

    if (argc != 2) {
        return ErrorAbort(state, "%s() expects 2 args, got %d", name, argc);
    }
    if (ReadArgs(state, argv, 2, &bytes_str, &is_full_ota) < 0) {
        return ErrorAbort(state, "CheckSystemSpaceEnoughFn read args failed!\n");
    }

    need_size = strtol(bytes_str, &endptr, 10);
    if (need_size == 0 && endptr == bytes_str) {
        success = 0;
        goto done;
    }
    need_size = need_size / 1024 / 1024;
    system_size = AvailSpaceForFile("/system");
    if (0 == strcmp(is_full_ota,"full_ota")) {
        system_size += UsedSpaceForFile("/system");
    }
    printf("system_size = %dM, need_size = %dM\n", system_size, need_size);
done:
    free(bytes_str);
    free(is_full_ota);
    free(endptr);
    if(success)
        return StringValue(strdup(system_size > need_size ? "t" : ""));
    else
        return ErrorAbort(state, " can't parse bytes_str as byte count!\n");
}
/* @} */

/* SPRD: add for support copy files @{ */
int copy_file(const char *src_file, const char *dst_file) {
    FILE *fp_src, *fp_dst;
    const int buf_size = 1024;
    char buf[buf_size];
    memset(buf, 0, buf_size);
    int size = 0;
    if((fp_src = fopen(src_file, "rb")) == NULL) {
        printf("source file can't open!\n");
        return -1;
    }
    if((fp_dst = fopen(dst_file, "wb+")) == NULL) {
        printf("target file can't open!\n");
        fclose(fp_src);
        return -1;
    }
    size = fread(buf, 1, buf_size, fp_src);
    while(size != 0) {
        fwrite(buf, 1, buf_size, fp_dst);
        size = fread(buf, 1, buf_size, fp_src);
    }
    fclose(fp_src);
    fclose(fp_dst);
    return 0;
}

Value* CopyFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* src_name;
    char* dst_name;
    int success = -1;
    if (argc != 2) {
        return ErrorAbort(state, "%s() expects 2 args, got %d", name, argc);
    }
    if (ReadArgs(state, argv, 2, &src_name, &dst_name) < 0) {
        return NULL;
    }
    if (strlen(src_name) == 0) {
        ErrorAbort(state, "src_name argument to %s() can't be empty", name);
        goto done;
    }
    if (strlen(dst_name) == 0) {
        ErrorAbort(state, "dst_name argument to %s() can't be empty", name);
        goto done;
    }
    if ((success = copy_file(src_name, dst_name)) != 0) {
        ErrorAbort(state, "copy of %s to %s failed, error %s",
          src_name, dst_name, strerror(errno));
    }
done:
    free(src_name);
    free(dst_name);
    return StringValue(strdup(success ? "": "t"));
}
/* @} */

/* SPRD: get freespace of sdcard @{ */
//apply_disk_space(bytes)
Value* ApplyDiskSpaceFn(const char* name, State* state, int argc, Expr* argv[]){
    char * bytes_str;
    char * path;
    if (ReadArgs(state, argv, 2, &path, &bytes_str) < 0) {
        return NULL;
    }
    char * endptr;
    size_t bytes = strtol(bytes_str, &endptr, 10);
    if (bytes == 0 && endptr == bytes_str) {
        ErrorAbort(state, "%s(): can't parse \"%s\" as byte count\n\n",
                   name, bytes_str);
        free(bytes_str);
        return NULL;
    }
    long sdsize = FreeSpaceForFile(path);
    return StringValue(strdup(sdsize > bytes ? "t" : ""));
}
/* @} */

/* SPRD: add for support backup and resume data @{ */
// if file exist return "True" else return "False".
Value* ExistFn(const char* name, State* state, int argc, Expr* argv[]) {
    char* result = NULL;
    struct stat st;
    if (argc != 1) {
        return ErrorAbort(state, "%s() expects 1 arg, got %d", name, argc);
    }
    char* path;
    if (ReadArgs(state, argv, 1, &path) < 0) return NULL;

    if (stat(path, &st) != 0) {
        printf("failed to stat \"%s\": %s\n", path, strerror(errno));
        result = strdup("False");
    } else {
        result = strdup("True");
    }

    free(path);
    return StringValue(result);
}
/* @} */

// SPRD: add for update from 4.4 to 5.0
static int run_program_status = 0;

Value* RunProgramExFn(const char* name, State* state, int argc, Expr* argv[]) {
    if (argc < 1) {
        return ErrorAbort(state, "%s() expects at least 1 arg", name);
    }
    char** args = ReadVarArgs(state, argc, argv);
    if (args == NULL) {
        return NULL;
    }

    char** args2 = (char **)malloc(sizeof(char*) * (argc+1));
    memcpy(args2, args, sizeof(char*) * argc);
    args2[argc] = NULL;

    printf("about to run program [%s] with %d args\n", args2[0], argc);

    chmod(args2[0], 0755);

    pid_t child = fork();
    if (child == 0) {
        execv(args2[0], args2);
        printf("run_program: execv failed: %s\n", strerror(errno));
        _exit(1);
    }
    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            printf("run_program: child exited with status %d\n",
                    WEXITSTATUS(status));
            run_program_status = WEXITSTATUS(status);
        }
    } else if (WIFSIGNALED(status)) {
        printf("run_program: child terminated by signal %d\n",
                WTERMSIG(status));
        run_program_status = 0;
    }

    int i;
    for (i = 0; i < argc; ++i) {
        free(args[i]);
    }
    free(args);
    free(args2);

    char buffer[20];
    sprintf(buffer, "%d", status);

    return StringValue(strdup(buffer));
}

/* SPRD: add for update from 4.4 to 5.0 @{ */
Value* LastRunStatusFn(const char* name, State* state, int argc, Expr* argv[]) {
    char buffer[20] = {0};
    printf("LastRunStatusFn enter, run_program_status=%d!\n",run_program_status);

    sprintf(buffer, "%d", run_program_status);

    return StringValue(strdup(buffer));
}
/* @} */

void Register_libsprd_updater(){
    RegisterFunction("merge_spl", MergeSplFn);
    RegisterFunction("write_emmc_image", WriteEmmcImageFn);
    RegisterFunction("apply_disk_space", ApplyDiskSpaceFn);
    RegisterFunction("exist", ExistFn);
    RegisterFunction("copy_file", CopyFn);
    RegisterFunction("check_path_space_enough", CheckPathSpaceEnoughFn); 
    RegisterFunction("open_file_contexts", OpenFileContextsFn);
    RegisterFunction("check_system_space_enough", CheckSystemSpaceEnoughFn);
    RegisterFunction("last_run_status", LastRunStatusFn);
    RegisterFunction("run_programex", RunProgramExFn);
}
