
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

//used to parse new packed modem image(modem bin+symbol)
#define MODEM_MAGIC        "SCI1"
#define MODEM_HDR_SIZE     12
#define MODEM_IMG_HDR      0x1
#define MODEM_LAST_HDR     0x100
#define MODEM_SHA1_HDR     0x400
#define MODEM_SHA1_SIZE    20

typedef struct {
	unsigned int type_flags;
	unsigned int offset;
	unsigned int length;
} data_block_header_t;
//end modem parse vars

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

/*
 *  this function compare the first four bytes in image and return 1 if equals to
 *  MODEM_MAGIC
 */
static int is_packed_modem_image(char *data)
{
	if (memcmp(data, MODEM_MAGIC, sizeof(MODEM_MAGIC)))
		return 0;
	return 1;
}

/*
 *  this function parse new packed modem image and return modem code offset and length
 */
static void get_modem_info(unsigned char *data, unsigned int *code_offset, unsigned int *code_len)
{
	unsigned int offset = 0, hdr_offset = 0, length = 0;
	unsigned char hdr_buf[MODEM_HDR_SIZE << 3] = {0};
	unsigned char read_len;
	unsigned char result = 0; // 0:OK, 1:not find, 2:some error occur
	data_block_header_t *hdr_ptr = NULL;

	read_len = sizeof(hdr_buf);
	memcpy(hdr_buf, data, read_len);

		do {
			if(!hdr_offset) {
				hdr_ptr = (data_block_header_t*)hdr_buf + 1;
				hdr_offset = MODEM_HDR_SIZE;
			} else {
				hdr_ptr = (data_block_header_t*)hdr_buf;
			}

			while(!(hdr_ptr->type_flags & MODEM_IMG_HDR)) {
				hdr_ptr ++;
				hdr_offset += MODEM_HDR_SIZE;
				if(read_len == ((unsigned char*)hdr_ptr - hdr_buf)) {
					result = 1;
					break;
				}

				if(hdr_ptr->type_flags & MODEM_LAST_HDR) {
					result = 2;
					printf("no modem image, error image header!!!\n");
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
			length = hdr_ptr->length;
		}

	*code_offset = offset;
	*code_len = length;
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
	printf("usage:sign the image\n");
	printf("--filename,      the image which to signed \n");
	printf("--config_path,   the config path keep the config file for signture \n");

}

/*
*  this function only sign the img
*/

int sprd_signimg(char *img, char *key_path)
{

	int i, j;
	int fd;
	int img_len;
	char *key[6] = { 0 };
	unsigned pagesize = 512;
	char *input_data = NULL;
	char *output_data = NULL;

	char *img_name = NULL;
	char *payload_addr = NULL;

	unsigned int modem_offset = 0;
	unsigned int modem_len = 0;

	output_data = img;
	char *basec = strdup(img);
	img_name = basename(basec);

	printf("input image name is:%s\n",img);
	for (i = 0; i < 6; i++) {
		key[i] = (char *)malloc(NAME_MAX_LEN);
		if (key[i] == 0)
			goto fail;
		memset(key[i], 0, NAME_MAX_LEN);
		strcpy(key[i], key_path);
		if (key_path[strlen(key_path) - 1] != '\/')
			key[i][strlen(key_path)] = '/';
		//printf("key[%d]= %s\n", i, key[i]);

	}

	strcat(key[0], "rsa2048_0_pub.pem");
	strcat(key[1], "rsa2048_1_pub.pem");
	strcat(key[2], "rsa2048_2_pub.pem");
	strcat(key[3], "rsa2048_0.pem");
	strcat(key[4], "rsa2048_1.pem");
	strcat(key[5], "rsa2048_2.pem");

	sprdsignedimageheader sign_hdr;
	sprd_keycert keycert;
	sprd_contentcert contentcert;
	memset(&sign_hdr, 0, sizeof(sprdsignedimageheader));
	memset(&keycert, 0, sizeof(sprd_keycert));
	memset(&contentcert, 0, sizeof(sprd_contentcert));

	input_data = load_file(img, &img_len);
	if (input_data == 0) {
		printf("error:could not load img\n");
		return 0;
	}
	printf("img_len = %d\n", img_len);

	payload_addr = input_data + sizeof(sys_img_header);

	if (is_packed_modem_image(payload_addr)) {
		printf("new packed modem image is found!\n");
		get_modem_info(payload_addr, &modem_offset, &modem_len);
		payload_addr += modem_offset;
		sign_hdr.payload_size = modem_len;
		printf("modem offset is %d \n", modem_offset);
		printf("modem size is %d \n", modem_len);
	} else {
		sign_hdr.payload_size = img_len - sizeof(sys_img_header);
	}
	sign_hdr.payload_offset = sizeof(sys_img_header);
	sign_hdr.cert_offset = img_len + sizeof(sprdsignedimageheader);

	sprd_rsapubkey nextpubk;

	fd = open(output_data, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd == 0) {
		printf("error:could create '%s'\n", output_data);
		return 0;
	}

	if (write(fd, input_data, img_len) != img_len)
		goto fail;

	if ((0 == memcmp("fdl1-sign.bin", img_name, strlen("fdl1-sign.bin")))
	    || (0 == memcmp("u-boot-spl-16k-sign.bin", img_name, strlen("u-boot-spl-16k-sign.bin")))) {
		printf("sign fdl1/spl: %s\n", img_name);
		keycert.certtype = CERTTYPE_KEY;
		keycert.version = 0x1;
		keycert.type = 0x1;
		sign_hdr.cert_size = sizeof(sprd_keycert);
		getpubkeyfrmPEM(&keycert.pubkey, key[0]);	/*pubk0 */
		getpubkeyfrmPEM(&nextpubk, key[1]);	/*pubk1 */
		printf("current pubk is: %s\n", key[0]);
		printf("nextpubk is: %s\n", key[1]);
		//dumpHex("payload:",payload_addr,512);
		cal_sha256(payload_addr, sign_hdr.payload_size, keycert.hash_data);
		cal_sha256(&nextpubk, SPRD_RSAPUBKLEN, keycert.hash_key);
		calcSignature(keycert.hash_data, ((HASH_BYTE_LEN << 1) + 8), keycert.signature, key[3]);
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &keycert, sizeof(sprd_keycert)) != sizeof(sprd_keycert))
			goto fail;

	} else if ((0 == memcmp("fdl2-sign.bin", img_name, strlen("fdl2-sign.bin")))
		   || (0 == memcmp("u-boot-sign.bin", img_name, strlen("u-boot-sign.bin"))) \
           || (0 == memcmp("u-boot-dtb-sign.bin",img_name,strlen("u-boot-dtb-sign.bin")))) {
		printf("sign fdl2/uboot: %s\n", img_name);
		keycert.certtype = CERTTYPE_KEY;
		keycert.version = KEYCERT_VERSION;
		printf("keycert version is: %d\n", keycert.version);
		sign_hdr.cert_size = sizeof(sprd_keycert);
		getpubkeyfrmPEM(&keycert.pubkey, key[1]);	/*pubk1 */
		getpubkeyfrmPEM(&nextpubk, key[2]);	/*pubk2 */
		printf("current pubk is: %s\n", key[1]);
		printf("next pubk is: %s\n", key[2]);
		cal_sha256(payload_addr, sign_hdr.payload_size, keycert.hash_data);
		cal_sha256(&nextpubk, SPRD_RSAPUBKLEN, keycert.hash_key);
		calcSignature(keycert.hash_data, ((HASH_BYTE_LEN << 1) + 8), keycert.signature, key[4]);
		/*
		if(write_padding(fd,pagesize,img_len))
			goto fail;
		*/
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &keycert, sizeof(sprd_keycert)) != sizeof(sprd_keycert))
			goto fail;

	} else if ((0 == memcmp("tos-sign.bin", img_name, strlen("tos-sign.bin"))) \
            || (0 == memcmp("sml-sign.bin", img_name, strlen("sml-sign.bin"))) \
            || (0 == memcmp("mobilevisor-sign.bin",img_name,strlen("mobilevisor-sign.bin"))) \
            || (0 == memcmp("secvm-sign.bin",img_name,strlen("secvm-sign.bin"))) \
            || (0 == memcmp("mvconfig-sign.bin",img_name,strlen("mvconfig-sign.bin")))) {
		printf("sign tos/sml: %s\n", img_name);
		contentcert.certtype = CERTTYPE_CONTENT;
		sign_hdr.cert_size = sizeof(sprd_contentcert);
		getpubkeyfrmPEM(&contentcert.pubkey, key[1]);	/*pubk1 */
		printf("current pubk is: %s\n", key[1]);
		cal_sha256(payload_addr, sign_hdr.payload_size, contentcert.hash_data);
		calcSignature(contentcert.hash_data, (HASH_BYTE_LEN + 8), contentcert.signature, key[4]);
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &contentcert, sizeof(sprd_contentcert)) != sizeof(sprd_contentcert))
			goto fail;
	} else {
		printf("sign boot/modem: %s\n", img_name);
		contentcert.certtype = CERTTYPE_CONTENT;
		contentcert.version = CONTENTCERT_VERSION;
		printf("contentcert version is: %d\n", contentcert.version);
		sign_hdr.cert_size = sizeof(sprd_contentcert);
		getpubkeyfrmPEM(&contentcert.pubkey, key[2]);	/*pubk2 */
		printf("current pubk is: %s\n", key[2]);
		cal_sha256(payload_addr, sign_hdr.payload_size, contentcert.hash_data);
		calcSignature(contentcert.hash_data, (HASH_BYTE_LEN + 8), contentcert.signature, key[5]);
		if (write(fd, &sign_hdr, sizeof(sprdsignedimageheader)) != sizeof(sprdsignedimageheader))
			goto fail;
		if (write(fd, &contentcert, sizeof(sprd_contentcert)) != sizeof(sprd_contentcert))
			goto fail;

	}

	return 1;

fail:
	printf("sign failed!!!\n");
	unlink(output_data);
	close(fd);
	for (i = 0; i < 5; i++) {
		if (key[i] != 0)
			free(key[i]);
	}

	if (basec != 0)
		free(basec);
	return 0;

}

int main(int argc, char **argv)
{
	if (argc != 3) {
		usage();
		return 0;
	}
	char *cmd1 = argv[1];	// img name
	char *cmd2 = argv[2];	//key documount
	sprd_signimg(cmd1, cmd2);

}
