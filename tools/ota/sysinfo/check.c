
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "check.h"
#include "cutils/hashmap.h"
#include "openssl/sha.h"

static Hashmap* app_to_sha1_ori = NULL;
static Hashmap* app_to_sha1_ori_new = NULL;/*for the case ota update failure*/

#define BUFFER_SIZE 4096
static int do_sha1(const char *path, char *sha1_string)
{
    int fd;
    SHA_CTX sha1_ctx;
    int i = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    SHA1_Init(&sha1_ctx);
    while (1) {
        char buf[BUFFER_SIZE];
        ssize_t rlen;
        rlen = read(fd, buf, sizeof(buf));
        if (rlen == 0)
            break;
        else if (rlen < 0) {
            (void)close(fd);
            return -1;
        }
        SHA1_Update(&sha1_ctx, buf, rlen);
    }

    if (close(fd)) {
        return -1;
    }

    /*const char* sha1 = (char *)SHA_final(&sha1_ctx);*/
    char sha1[SHA_DIGEST_LENGTH];
    SHA1_Final(sha1, &sha1_ctx);

    for (i = 0; i < SHA_DIGEST_LENGTH; i++){
        sprintf((sha1_string+2*i),"%02x",sha1[i]);
    }
    *(sha1_string + 2*SHA_DIGEST_LENGTH) = '\0';
    return 0;
}

/*this func is used to make hash table*/
static int str_hash(void* key)
{
    return hashmapHash(key, strlen((char*)key));
}

static bool str_icase_equals(void *KeyA, void *KeyB)
{
    return strcasecmp((char*)KeyA, (char*)KeyB) == 0;
}

static int read_ori_system_sha1(Hashmap** sha1_hash,FILE* file)
{

    char  buf[1024];

    *sha1_hash = hashmapCreate(2048, str_hash, str_icase_equals);
    if(!*sha1_hash){
        printf("hash map create fail\n");
        return -1;
    }

    while (fgets(buf,sizeof(buf),file) != NULL) {
        char app_name[512];
        char sha1_value[41];

        if (sscanf(buf,"%s %s",sha1_value,app_name) == 2) {
            char* app_name_dup = strdup(app_name);
            sha1_value[40] = '\0';
            char* sha1_value_dup = strdup(sha1_value);
            hashmapPut(*sha1_hash,app_name_dup,(void*)sha1_value_dup);
        }
    }

out:
    return hashmapSize(*sha1_hash);
}

/*
 * this func is used to read all of files stored in system partition recursively
 * all of the items is stored in hashmap new
 * the parameter is the path here is "/system/"
 *
 * the return value is the total num of items
*/
int read_dir(char *pathname, int* err_num)
{
    DIR * dir;
    struct dirent * ptr;
    char path[1024];
    char sys_info_name[1024];
    int count = 0;
    char buf[41];
    char* buf_fake="0000000000000000000000000000000000000000";
    char* buf_ori;
    int ret = -1;
    int check_ok_num = 0;
    bool need_new_checkbin = false;

    dir =opendir(pathname);
    if (!dir) {
        printf("open dir fail %s\n",pathname);
        goto out;
    }

    while((ptr = readdir(dir))!=NULL) {

        if (ptr->d_type == DT_DIR) {
            if (0 == strcmp(".",ptr->d_name) || 0 == strcmp("..",ptr->d_name)) {
                continue;
            }
            sprintf(path,"%s%s/",pathname,ptr->d_name);
            count += read_dir(path, err_num);
        }
        else {
            sprintf(sys_info_name,"%s%s",pathname,ptr->d_name);
            ret = do_sha1(sys_info_name, buf);
            need_new_checkbin = false;
            if(0 == strcmp(ptr->d_name, "recovery-resource.dat")) {
                count++;
                printf("%s omit sha1 check!!! \n",ptr->d_name);
                continue;
            }
            if(0 == strcmp(ptr->d_name, "Camera2.apk")){
                continue;
            }

            if (hashmapContainsKey(app_to_sha1_ori, (void*)sys_info_name) ||
                (need_new_checkbin = true,app_to_sha1_ori_new && hashmapContainsKey(app_to_sha1_ori_new,(void*)sys_info_name))) {
                //printf(" containeded %s ",ptr->d_name);
                if (!need_new_checkbin){
                    buf_ori = (char *)hashmapGet(app_to_sha1_ori,(void*)sys_info_name);
                }
                else {
                    buf_ori = (char *)hashmapGet(app_to_sha1_ori_new,(void*)sys_info_name);
                }

                if (strcasecmp(buf_ori,buf) == 0 || (ret && (strcasecmp(buf_ori,buf_fake) == 0))){
                //printf("md5 value is  equaled!! ");
                //printf(" check_ok_num is %d\n",++check_ok_num);
                    count++;
                    continue;
                }
                else {
                    if (app_to_sha1_ori_new && !need_new_checkbin) {
                        buf_ori = (char *)hashmapGet(app_to_sha1_ori_new,(void*)sys_info_name);
                        if (strcasecmp(buf_ori,buf) == 0 || (ret && (strcasecmp(buf_ori,buf_fake) == 0))) {
                            count++;
                            continue;
                        }
                    }

                    /* ignore some link errors begin */
                    if (strcasecmp(buf_ori,buf_fake) != 0) {
                        printf("system check failed ori is %s ,now is %s ,sys name is %s!!! \n",buf_ori,buf,sys_info_name);
                        (*err_num)++;
                    }
                    else {
                        count++;
                        continue;
                    }
                    /* ignore some link errors end */
                }
            }
            else {
                printf("system check failed ori is %s ,now is %s ,sys name is %s!!!!!! \n",app_to_sha1_ori, app_to_sha1_ori_new,sys_info_name);
                (*err_num)++;
            }
            count++;
        }
    }
    closedir(dir);
out:
    return count;
}

int check_system_root()
{
    DIR* dir;
    int total_num = 0;
    int ori_total_num = 0;
    int ori_total_num_new = 0;
    int err_total_num = 0;
    struct dirent *ptr;
    char sha1_string[41];
    int ret;
    FILE* file;
    
    file = fopen("/systeminfo/check.bin","r");
    if (!file) {
        printf("failed to open check.bin check failed %s \n", strerror(errno));
        goto out;
    } else {
        printf("success to open check.bin \n");
    }

    ori_total_num = read_ori_system_sha1(&app_to_sha1_ori, file);//create the hash table
    fclose(file);
    if (-1 == ori_total_num) {
        goto out;
    }

    file = fopen("/systeminfo/check.bin.new","r");
    if (file) {
        printf("success to open check.bin.new \n");
        ori_total_num_new = read_ori_system_sha1(&app_to_sha1_ori_new,file);//create the hash table
        fclose(file);
    } else {
        printf("failed to open check.bin.new check failed %s \n", strerror(errno));
    }

    printf("\nchecking NOW, please waiting...\n");
    total_num = read_dir("/system/",&err_total_num);

    if ((0 == err_total_num) && ((ori_total_num == total_num) || (ori_total_num_new == total_num))) {
        return 0;
    } else if (err_total_num) {
        printf("\nsystem should have %d files, but now %d files.\n", ori_total_num, total_num);
    } else if (ori_total_num != total_num &&  ori_total_num_new != total_num) {
        printf("\nsystem check failed the num of file under system should be %d  but now it is %d files.\n",ori_total_num,total_num);
    }
out:
    return 1;
}
