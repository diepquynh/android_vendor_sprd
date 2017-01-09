
#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/err.h"

#include "sprdsec_header.h"
#include "pk1.h"
#include "rsa_sprd.h"
#include "sprdsha.h"
#include "sprd_verify.h"
#include "sec_string.h"

#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

static unsigned char padding[512] = { 0 };

#define NAME_MAX_LEN 2048
#define KEYCERT_VERSION 1
#define CONTENTCERT_VERSION 1
#define AES_BLOCK_SIZE 16
static void *load_file(const char *fn, unsigned *_sz)
{
	char *data;
	int sz;
	int fd;

	data = 0;
	fd = open(fn, O_RDONLY);

	if (fd < 0)
		return 0;

	sz = lseek(fd, 0, SEEK_END);

	if (sz < 0)
		goto oops;

	if (lseek(fd, 0, SEEK_SET) != 0)
		goto oops;

	data = (char *)malloc(sz);

	if (data == 0)
		goto oops;

	if (read(fd, data, sz) != sz)
		goto oops;

	close(fd);

	if (_sz)
		*_sz = sz;

	return data;

oops:
	close(fd);
	if (data != 0)
		free(data);
	return 0;
}

int write_padding(int fd, unsigned pagesize, unsigned itemsize)
{
	unsigned pagemask = pagesize - 1;
	unsigned int count;
	memset(padding, 0xff, sizeof(padding));
	if ((itemsize & pagemask) == 0) {
		return 0;
	}

	count = pagesize - (itemsize & pagemask);
	//printf("need to padding %d byte,%d,%d\n",count,itemsize%8,(itemsize & pagemask));
	if (write(fd, padding, count) != count) {
		return -1;
	} else {
		return 0;
	}
}

void usage(void)
{
	printf("usage:ssencrypt the image\n");
	printf("--keyfile,       the file which keep the aes key\n");
    printf("--file nale,     the image which to be encrypted\n");
}

/*
*  this function encrypt the image use aes cbc_256
*/

static void hex_print(const void* pv, size_t len)
{
    const unsigned char * p = (const unsigned char*)pv;
    if (NULL == pv)
        printf("NULL");
    else
    {
            size_t i = 0;
            for (; i<len;++i)
                printf("%02X ", *p++);
        }
    printf("\n");
}

int main (int argc,char *argv[])
{
  /* Set up the key and iv. Do I need to say to not hard code these in a
   * real application? :-)
   */
    /*
     * first we need to read the aeskey from the file named aeskey*/
    if(argc != 3)
    {
        usage();
        return 0;
    }
    char *aeskey = argv[1];

    char *aesk = strdup(aeskey);
    char *key_name = basename(aesk);
    //printf("img:%s\n",aeskey);
    //printf("img_name:%s\n",key_name);
    FILE *fp;
    if((fp = fopen(aeskey,"r")) == NULL)
    {
        printf("can not open file\n");
        exit (-1);
    }

    unsigned char *key;
    fseek(fp,0,SEEK_END);
    int len = ftell(fp);
    key = malloc(len+1);
    rewind(fp);
    fread(key,1,len,fp);
    key[len] = 0;

    fclose(fp);
    //printf("aes_key:\t");
    //hex_print(key,len);
    /*end of read encrypt key from document*/

    /*
     * load the plaintext file to get the iv value and the plaintext
     * */
    unsigned char *iv;
    unsigned char *plaintext;

    int fd;
    int img_len = 0;
    char *input_data = NULL;

    char *img_name = NULL;
    char *payload_addr = NULL;

    char *img = argv[2];
    char *basec = strdup(img);
    img_name = basename(basec);
    //printf("img:%s\n",img);
    //printf("img_name:%s\n",img_name);

    char imagename[NAME_MAX_LEN] = "0";
    strcpy(imagename,argv[2]);
    char *start = NULL;
    char *end = NULL;
    char flag = '.';
    char *namesuffix = "-cipher";
    char suffix[10] = "0";

    start = imagename;
    end = strrchr(start,flag);
    if(end == NULL)
    {
        printf("aes end error\n");
        return 1;
    }
    memcpy(suffix,end,strlen(end)+1);
    imagename[end-start] = '\0';
    strcat(imagename,namesuffix);
    strcat(imagename,suffix);
    //printf("\n\timagename :%s\n",imagename);



    FILE *fplaintext;
    FILE *fciphertext;

    unsigned char *header_buf = NULL;
    header_buf = (unsigned char*)malloc(sizeof(sys_img_header));
    memset(header_buf,0,sizeof(sys_img_header));

    fplaintext = fopen(img,"r");
    if(fplaintext == NULL)
    {
        printf("open plaintext error\n");
        exit(-1);
    }
    rewind(fplaintext);
    fseek(fplaintext,0,SEEK_SET);
    fread(header_buf,1,sizeof(sys_img_header),fplaintext);/*get the magic and mImgSize from sys_img_header*/
    //dumpHex("header_buf:",header_buf,0x40);


    sys_img_header *img_h = NULL;
    img_h = (sys_img_header *)header_buf;
    //printf("plaintext len:\t0x%x\n",img_h->mImgSize);/*plaintext is the payload block*/

    /*
     * init vector*/
    unsigned char *iv_enc = (unsigned char *)malloc(sizeof(unsigned char) *AES_BLOCK_SIZE);
    RAND_bytes(iv_enc,AES_BLOCK_SIZE);
    memcpy(img_h->reserved,iv_enc,AES_BLOCK_SIZE);
    //printf("rand iv:");
    //hex_print(iv_enc,AES_BLOCK_SIZE);
    /*
     * ready for iv, the rand128bit|(magic<<32) |(mImgsize )*/
    uint32_t imgsize = img_h->mImgSize;
    if(!(imgsize%16))
    {
        printf("do not need to padding to 16bytes\n");
    }else
    {
        printf("0 size:0x%x\n",img_h->mImgSize);
        imgsize = ((imgsize+15)/AES_BLOCK_SIZE)*AES_BLOCK_SIZE;
        printf("yuanshi:0x%x\n",img_h->mImgSize);
        img_h->mImgSize = imgsize;
        printf("padding to 16bytes\n");
        printf("new:0x%x\n",imgsize);
    }
    uint32_t magic = img_h->mMagicNum;
    *(uint32_t*)&iv_enc[8] = (*(uint32_t*)&iv_enc[8])^magic;
    *(uint32_t*)&iv_enc[12] = (*(uint32_t*)&iv_enc[12])^imgsize;
    //printf("new iv:\t");
    //hex_print(iv_enc,AES_BLOCK_SIZE);
    iv = iv_enc;/*use iv just for aes_encrypt function*/
    //printf("pass iv:");
    //hex_print(iv,AES_BLOCK_SIZE);

    /*
     * ready for plaintext*/
    unsigned char *ciphertext_buf = NULL;
    unsigned char *plaintext_buf = NULL;
    ciphertext_buf = (unsigned char*)malloc(1024);/*keep ciphertext context*/
    plaintext_buf = (unsigned char*)malloc(1024); /*keep plaintext context*/

    unsigned int ciphertext_len,plaintext_len;
    EVP_CIPHER_CTX ctx;/*EVP context*/
    if((fciphertext = fopen(imagename,"w")) == NULL)
            {
                printf("open ciphertext error\n");
                fclose(fplaintext);
                exit(-1);
            }

    EVP_CIPHER_CTX_init(&ctx);/*init ctx*/

    if(1 != EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key,iv))
    {
        printf("encryptinit error\n");
        exit(-1);
    }

    EVP_CIPHER_CTX_set_padding(&ctx,0);/*set no padding*/

    //dumpHex("new header:",header_buf,0x50);
    fwrite(header_buf,1,sizeof(sys_img_header),fciphertext);/*write the header to ciphertext file first*/

    rewind(fplaintext);
    fseek(fplaintext,sizeof(sys_img_header),SEEK_SET);
    //fread(plaintext_buf,1,1024,fplaintext);
    //dumpHex("plaintext:",plaintext_buf,32);
    static unsigned int count;
    while(1)
    {
        plaintext_len = fread(plaintext_buf,1,16,fplaintext);
        if((plaintext_len > 0) &&(plaintext_len < 16))
            plaintext_len = 16;
        if(plaintext_len <= 0)/*read plaintext file finish*/
            break;
        if(1 != EVP_EncryptUpdate(&ctx,ciphertext_buf,&ciphertext_len,plaintext_buf,plaintext_len))
        {
            printf("evp_encryptupdate error\n");
            fclose(fplaintext);
            fclose(fciphertext);
            EVP_CIPHER_CTX_cleanup(&ctx);
            exit(-1);
        }
        count ++;
        fwrite(ciphertext_buf,1,ciphertext_len,fciphertext);
    }
    //printf("count:\t%d,plen:\t%d\n",count,plaintext_len);
#if 1
    if(1 != EVP_EncryptFinal_ex(&ctx,ciphertext_buf,&ciphertext_len))
    {
        printf("evp encryptfinal error\n");
        fclose(fciphertext);
        fclose(fplaintext);
        EVP_CIPHER_CTX_cleanup(&ctx);
        exit(-1);
    }
    //printf("final len:\t%d\n",ciphertext_len);
#endif
    fwrite(ciphertext_buf,1,ciphertext_len,fciphertext);
    fclose(fciphertext);
    fclose(fplaintext);
    EVP_CIPHER_CTX_cleanup(&ctx);
    printf("finish encrypt!\n");

    return 0;
}
