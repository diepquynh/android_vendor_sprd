
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

int usage(void)
{

	printf("usage:create primary cert,pls input the debug_mask key_path\n");
	printf("--debug_mask                                the mask of reg\n");
	printf("--key_path                            the config dir of key\n");
	return 0;
}

int sprd_create_primary_cert(char *mask, char *key_path)
{
	int fd;
	int i;

	char *end = NULL;
	char *cert_name = "primary_debug.cert";
	sprd_rsapubkey dev_sb_pubk;

	char *key[3];

	for (i = 0; i < 3; i++) {
		key[i] = (char *)malloc(NAME_MAX_LEN);
		if (key[i] == 0)
			goto fail;
		memset(key[i], 0, NAME_MAX_LEN);
		strcpy(key[i], key_path);
		if (key_path[strlen(key_path) - 1] != '\/')
			key[i][strlen(key_path)] = '/';
		//printf("key[%d]= %s\n", i, key[i]);
	}

	strcat(key[0], "rsa2048_1_pub.pem");	/* this pubk for verify prim_cert */
	strcat(key[1], "rsa2048_devkey_pub.pem");	/*this pubk is provieded by customer,here cal this pubk's hash and fill the primary cert->devkey_hash */
	strcat(key[2], "rsa2048_1.pem");	/* this prikey is for sign the prim cert */

	fd = open(cert_name, O_CREAT | O_TRUNC | O_WRONLY, 0644);

	if (fd == 0) {
		printf("error:could create '%s'\n", cert_name);
		goto fail;
		return 0;
	}

	primary_debugcert sprd_primary_cert;

	memset(&sprd_primary_cert, 0, sizeof(primary_debugcert));

	sprd_primary_cert.cert_type = CERTTYPE_PRIMDBG;
	sprd_primary_cert.debug_mask = strtoul(mask, &end, 16);
	sprd_primary_cert.reserved = 0;

	printf("intput mask is: 0x%x \n", sprd_primary_cert.debug_mask);

	getpubkeyfrmPEM(&sprd_primary_cert.pubkey, key[0]);	/*fill primary_cert -> pubk, this key is used for verify primary */
	getpubkeyfrmPEM(&dev_sb_pubk, key[1]);	/*get dev pubk for customer,this pubk is provided by customer */
	cal_sha256(&dev_sb_pubk, SPRD_RSAPUBKLEN, sprd_primary_cert.devkey_debug_hash_data);	/*cal devpubk's hash and keep in primary cert */
	calcSignature(sprd_primary_cert.devkey_debug_hash_data, PRIMARY_SIGN_LEN, sprd_primary_cert.devkey_debug_signature,
		      key[2]);

	if (write(fd, &sprd_primary_cert, sizeof(primary_debugcert)) != sizeof(primary_debugcert))
		goto fail;

	for (i = 0; i < 3; i++) {
		if (key[i] != 0)
			free(key[i]);
	}
	return 1;

fail:
	for (i = 0; i < 3; i++) {
		if (key[i] != 0)
			free(key[i]);
	}
	return 0;

}

int main(int argc, char **argv)
{
	if (argc != 3) {
		usage();
		return 0;
	}

	char *cmd1 = argv[1];	/*debug mask */
	char *cmd2 = argv[2];	/*the path of config directory */

	sprd_create_primary_cert(cmd1, cmd2);

}
