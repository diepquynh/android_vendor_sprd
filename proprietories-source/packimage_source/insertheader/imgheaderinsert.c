#include "imgheaderinsert.h"

#include "mincrypt/sha256.h"

void do_sha256(uint8_t *data,int bytes_num,unsigned char *hash)
{
    SHA256_CTX ctx;
    const uint8_t* sha;

    SHA256_init(&ctx);
    SHA256_update(&ctx, data, bytes_num);
    sha = SHA256_final(&ctx);

    memcpy(hash,sha,SHA256_DIGEST_SIZE);
}

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;
    int psz;//raw size + padding

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    psz = ((sz + 15)/16)*16;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(psz);
    if(data == 0) goto oops;

    memset(data, 0, psz);
    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = psz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}


static void usage(void)
{
    printf("======================================================== \n");
    printf("Usage: \n");
    printf("$./imgheaderinsert <filename> <add_payloadhash> \n");
    printf("-------------------------------------------------------- \n");
    printf("-filename              --the image to be inserted with sys_img_header \n");
    printf("-------------------------------------------------------- \n");
    printf("-add_payloadhash = 1   --add payload hash when secure boot is disabled \n");
    printf("                 = 0   --payload hash isn't needed when secure boot is enabled\n");
    printf("======================================================== \n");
}

#if 0
static unsigned char padding[PADDING_MAX_SIZE] = { 0, };

int write_padding(int fd, int count)
{
    if(count == 0) {
        return 0;
    } else if(count < 0) {
        return 1;
    }

    if(write(fd, padding, count) != count) {
        return 1;
    } else {
        return 0;
    }
}
#endif

int main(int argc, char* argv[])
{

    sys_img_header img_h;
    char filename[FILE_NAME_SIZE] = "0";
    char tmpname[FILE_NAME_SIZE] = "0";
    char imagename[FILE_NAME_SIZE] = "0";
    char suffix[10] = "0";
    char flag = '.';
    char *namesuffix = "-sign";
    void *payload = NULL;
    char *start = NULL;
    char *end = NULL;
    char *ptr = NULL;
    int fd,fd_org;
    uint32_t imgpadsize = 0;//raw size + padding

    if (argc != 3) {
        usage();
        return 1;
    }

    memset(&img_h, 0, sizeof(img_h));
    memset(filename, 0, sizeof(filename));
    memset(imagename, 0, sizeof(imagename));

    strcpy(filename,argv[1]);
    strcpy(imagename,argv[1]);
    int addPayloadHash = atoi(argv[2]);

    start = imagename;
    end = strrchr(start,flag);
    if (end == NULL) {
        return 1;
    }
    memcpy(suffix,end,strlen(end)+1);
    imagename[end-start] = '\0';
    strcat(imagename,namesuffix);
    strcat(imagename,suffix);

    img_h.mVersion=1;
    img_h.mMagicNum=0x42544844;

    payload = load_file(filename, &imgpadsize);
    if(payload == NULL) {
        printf("error: could not load %s \n", filename);
        return 1;
    }

    img_h.mImgSize = imgpadsize;

    if (addPayloadHash == 1) {
        do_sha256(payload, img_h.mImgSize, img_h.mPayloadHash);
    }

    fd = open(imagename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd < 0) {
        printf("error: could not create '%s'\n", imagename);
        goto fail;
    }

    fd_org = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd_org < 0) {
        printf("error: could not create '%s'\n", filename);
        goto fail;
    }
    if((uint32_t)write(fd_org, payload, imgpadsize) != imgpadsize) goto fail;

    if(write(fd, &img_h, sizeof(img_h)) != sizeof(img_h)) goto fail;

    if((uint32_t)write(fd, payload, imgpadsize) != imgpadsize) goto fail;

    free(payload);
    close(fd);
    close(fd_org);
    return 0;

fail:
    unlink(imagename);
    close(fd);
    close(fd_org);
    free(payload);
    printf("error: failed writing '%s'\n", imagename);
    return 1;
}

