
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

static unsigned char padding[8] = { 0 };

#define NAME_MAX_LEN 2048

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
	printf("usage: mkdbimg,make secure debug img,add secure prim debug cert & developer cert\n");
	printf("--img    the full path  which img localed\n");
	printf("--cert   the primary cert \n");
	printf("--key_path the config of mkdbimg\n");
#ifdef SoC_ID
    printf("--soc_id  the soc_id of device\n");
#else
	printf("--uid0   the uid0 of cpu\n");
	printf("--uid1   the uid1 of cpu\n");
#endif
	printf("--debug mask  the mask of reg\n");

}

int sprd_add_sb_cert(char *img, char *cert, char *key_path, char *id0, char *id1, char *mask)
{
	int fd;
	int i;

	char *primary_sb_cert_data = NULL;
	char *input_img = NULL;
	char *output_data = img;
	char *name = NULL;
	char *basec = strdup(img);
	name = basename(basec);
	char *end = NULL;
#ifdef SoC_ID
    unsigned char socid[32];
#else
	unsigned int uid0, uid1;
#endif
	if (0 != memcmp("fdl1-sign.bin", name, strlen("fdl1-sign.bin"))
	    && 0 != memcmp("u-boot-spl-16k-sign.bin", name, strlen("u-boot-spl-16k-sign.bin"))) {
		printf("error: this img do not need add debug cert\n");
		goto fail;
	}

	sprdsignedimageheader *sprd_signptr;
	sys_img_header *sprd_hdr;
	int img_len;
	int cert_len;
	developer_debugcert sprd_devcert;
	memset(&sprd_devcert, 0, sizeof(developer_debugcert));

	char *key[2];

	for (i = 0; i < 2; i++) {
		key[i] = (char *)malloc(NAME_MAX_LEN);
		if (key[i] == 0)
			goto fail;
		memset(key[i], 0, NAME_MAX_LEN);
		strcpy(key[i], key_path);
		if (key_path[strlen(key_path) - 1] != '\/')
			key[i][strlen(key_path)] = '/';
		//printf("key[%d]= %s\n", i, key[i]);
	}

	strcat(key[0], "rsa2048_devkey_pub.pem");	/* this pubk is used to  verify devcert */
	strcat(key[1], "rsa2048_devkey.pem");	/*this prikey is used to sign devcert */
	//printf("key[0] = %s\n",key[0]);
	//printf("key[1] = %s\n",key[1]);

	input_img = load_file(img, &img_len);
	if (input_img == 0) {
		printf("error:could not load img\n");
		return 0;
	}

	primary_sb_cert_data = load_file(cert, &cert_len);
	if (primary_sb_cert_data == 0) {
		printf("error:could not load sb_primarycert\n");
		return 0;
	}
	sprd_devcert.cert_type = CERTTYPE_DEVELOPDBG;
	sprd_devcert.debug_mask = strtoul(mask, &end, 16);
#ifdef SoC_ID
    int id_len = (strlen(id0)%2 == 0) ? strlen(id0)/2 : (strlen(id0)/2 + 1);
    //printf("\r\n id_len=%d,ido_len=%d\n",id_len,strlen(id0));
    if(id0[0] == '0' && TOLOWER(id0[1]) == 'x')
    {
        id0 = id0 + 2;
        id_len = id_len -1;
    }
    int idx = 0;
    for(i=0;i < id_len;i++)
    {
        idx = i * 2;
        sprd_devcert.soc_id[i] = str2Num16(id0+idx);
        //printf("%x\t",sprd_devcert.soc_id[i]);
    }
    //printf("\n");
#else
	sprd_devcert.uid0 = strtoul(id0, &end, 16);
	sprd_devcert.uid1 = strtoul(id1, &end, 16);
	printf("\n\tuid0: 0x%x | uid1: 0x%x | debug_mask: 0x%x\n\n", sprd_devcert.uid0, sprd_devcert.uid1, sprd_devcert.debug_mask);
#endif
	getpubkeyfrmPEM(&sprd_devcert.dev_pubkey, key[0]);
	calcSignature(&sprd_devcert.debug_mask, DEV_SIGN_LEN, sprd_devcert.dev_signature, key[1]);
	sprd_hdr = (sys_img_header *) input_img;
	sprd_signptr = (sprdsignedimageheader *) (input_img + sprd_hdr->mImgSize + sizeof(sys_img_header));
	sprd_signptr->cert_dbg_prim_offset = sprd_signptr->cert_offset + sizeof(sprd_keycert);
	sprd_signptr->cert_dbg_prim_size = sizeof(primary_debugcert);
	sprd_signptr->cert_dbg_developer_offset = sprd_signptr->cert_dbg_prim_offset + sizeof(primary_debugcert);
	sprd_signptr->cert_dbg_developer_size = sizeof(developer_debugcert);
	fd = open(output_data, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd == 0) {
		printf("error:could create '%s'\n", output_data);
		return 0;
	}
	if (write(fd, input_img, img_len) != img_len)
		goto fail;
	if (write(fd, primary_sb_cert_data, sizeof(primary_debugcert)) != sizeof(primary_debugcert))
		goto fail;
	if (write(fd, &sprd_devcert, sizeof(developer_debugcert)) != sizeof(developer_debugcert))
		goto fail;

	return 1;

fail:
	printf("error: fail\n");
	close(fd);
	return 0;

}

int main(int argc, char **argv)
{
#ifdef SoC_ID
	if (argc != 6)
#else
	if (argc != 7)
#endif
    {
		usage();
		return 0;
	}

	char *cmd1, *cmd2, *cmd3, *cmd4, *cmd5, *cmd6;

	cmd1 = argv[1];
	cmd2 = argv[2];
	cmd3 = argv[3];
	cmd4 = argv[4];
	cmd5 = argv[5];
	cmd6 = argv[6];
#ifdef SoC_ID
	sprd_add_sb_cert(cmd1, cmd2, cmd3, cmd4, NULL, cmd5);/*img,primary_cert,key_path,soc_id,mask*/
#else
	sprd_add_sb_cert(cmd1, cmd2, cmd3, cmd4, cmd5, cmd6);/*img,primary_cert,key_path,id0,id1,mask*/
#endif
	return 1;
}
