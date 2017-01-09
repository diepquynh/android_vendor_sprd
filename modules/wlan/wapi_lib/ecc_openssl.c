/*
 * $Copyright Broadcom Corporation$
 *
 * OpenSSL implementation for WAPI ECC routines.
 *
 * Use OpenSSL 0.9.8 or better.
 *
 * Switches:
 *    - define ECC_NEED_NID_X9_62_PRIME192V4 if OpenSSL does not support
 *      OID NID_X9_62_prime192v4
 *    - define ECC_NO_ECC192_ECDH to remove the ecc192_ecdh definition
 *
*/

#include <string.h> /* memset */

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/ecdsa.h>
#include <openssl/ecdh.h>

/*
 * private declarations
*/

/* similar to OpenSSL's EC_CURVE_DATA structure*/
struct curve_data {
	const char *p;
	const char *a;
	const char *b;
	const char *x;
	const char *y;
	const char *order;
};

/*
 * private definitions
*/

static int
eckey_privkey2bin(const EC_KEY *eckey, unsigned char *key, size_t key_len)
{
	int err;
	const BIGNUM *bn;

	if (NULL == eckey || NULL == key || 0 == key_len) {
		err = -1;
		goto DONE;
	}
	
	if (NULL == (bn = EC_KEY_get0_private_key(eckey))) {
		err = -2;
		goto DONE;
	}
	   
	if (BN_num_bytes(bn) > key_len) {
		err = -3;
		goto DONE;
	}
	
	if (0 == BN_bn2bin(bn, key)) {
		err = -4;
		goto DONE;
	}

	err = 0;
	
DONE:
	return err;
}
				  
static int
eckey_pubkey2bin(const EC_KEY *eckey, unsigned char *key, size_t key_len)
{
	int err;
	const EC_GROUP *ecgroup;
	const EC_POINT *ecpoint;
	size_t bytes;

	if (NULL == eckey || NULL == key || 0 == key_len) {
		err = -1;
		goto DONE;
	}

	if (NULL == (ecgroup = EC_KEY_get0_group(eckey))) {
		err = -2;
		goto DONE;
	}

	if (NULL == (ecpoint = EC_KEY_get0_public_key(eckey))) {
		err = -3;
		goto DONE;
	}

#ifdef CONFIG_OPENSSL
	bytes = EC_POINT_point2oct(ecgroup, ecpoint,
							   EC_GROUP_get_point_conversion_form(ecgroup),
							   key, key_len, NULL);
#else
	bytes = EC_POINT_point2oct(ecgroup, ecpoint,
							   EC_KEY_get_conv_form(eckey),
							   key, key_len, NULL);
#endif
	if (0 == bytes) {
		err = -4;
		goto DONE;
	}

	err = 0;

DONE:
	return err;
}

static int
eckey_bin2privkey(EC_KEY *eckey, const unsigned char *key, size_t key_len)
{
	int err;
	BIGNUM bn;

	BN_init(&bn);
	
	if (NULL == eckey || NULL == key || 0 == key_len) {
		err = -1;
		goto DONE;
	}

	if (!BN_bin2bn(key, key_len, &bn)) {
		err = -3;
		goto DONE;
	}

	if (!EC_KEY_set_private_key(eckey, &bn)) {
		err = -4;
		goto DONE;
	}

	err = 0;
	
DONE:
	BN_free(&bn);

	return err;
}

static int
eckey_bin2pubkey(EC_KEY *eckey, const unsigned char *key, size_t key_len)
{
	int err;
	const EC_GROUP *ecgroup;
	EC_POINT *ecpoint=NULL;

	if (NULL == eckey || NULL == key || 0 == key_len) {
		err = -1;
		goto DONE;
	}

	if (NULL == (ecgroup = EC_KEY_get0_group(eckey))) {
		err = -2;
		goto DONE;
	}
	
	if (NULL == (ecpoint = EC_POINT_new(ecgroup))) {
		err = -3;
		goto DONE;
	}

	if (!EC_POINT_oct2point(ecgroup, ecpoint, key, key_len, NULL)) {
		err = -4;
		goto DONE;
	}

	if (!EC_KEY_set_public_key(eckey, ecpoint)) {
		err = -5;
		goto DONE;
	}

	err = 0;

DONE:
	if (NULL != ecpoint)
		EC_POINT_free(ecpoint);

	return err;
}

static int
sha256_digest(const unsigned char *m, size_t m_len,
			  unsigned char (*out_md)[EVP_MAX_MD_SIZE], size_t *out_md_len)
{
	int err;
	EVP_MD_CTX mdctx;
	const EVP_MD *evpmd;
	unsigned int len;

	if (NULL == m || 0 == m_len || NULL == out_md || 0 == out_md_len) {
		err = -1;
		goto DONE;
	}

	EVP_MD_CTX_init(&mdctx);

	if (NULL == (evpmd = EVP_sha256())) {
		err = -2;
		goto DONE;
	}
	
	if (0 == EVP_DigestInit_ex(&mdctx, evpmd, NULL)) {
		err = -3;
		goto DONE;
	}
	
	if (0 == EVP_DigestUpdate(&mdctx, m, m_len)) {
		err = -4;
		goto DONE;
	}
	
	if (0 == EVP_DigestFinal_ex(&mdctx, (unsigned char *)out_md, &len)) {
		err = -5;
		goto DONE;
	}

	*out_md_len = len;
	err = 0;

DONE:
	EVP_MD_CTX_cleanup(&mdctx);

	return err;
}

static int
eckey_sha256_reduce(const EC_KEY *eckey,
					unsigned char (*out_md)[EVP_MAX_MD_SIZE],
					size_t *out_md_len)
{
	int err;
	BIGNUM bn, order;
	const EC_GROUP *ecgroup;
	BN_CTX *bnctx=NULL;

	BN_init(&bn);
	BN_init(&order);

	if (NULL == eckey || NULL == out_md || 0 == out_md_len) {
		err = -1;
		goto DONE;
	}

	if (NULL == (bnctx = BN_CTX_new()))
	{
		err = -2;
		goto DONE;
	}

	if (NULL == (ecgroup = EC_KEY_get0_group(eckey))) {
		err = -2;
		goto DONE;
	}

	if (!EC_GROUP_get_order(ecgroup, &order, bnctx)) {
		err = -3;
		goto DONE;
	}
	
	if (NULL == BN_bin2bn((unsigned char *)out_md, *out_md_len, &bn)) {
		err = -4;
		goto DONE;
	}

	/* modular reduce */
	if (!BN_mod(&bn, &bn, &order, bnctx)) {
		err = -5;
		goto DONE;
	}

	*out_md_len = BN_bn2bin(&bn, (unsigned char *)out_md);
	err = 0;

DONE:
	if (NULL != bnctx)
		BN_CTX_free(bnctx);
	BN_free(&bn);
	BN_free(&order);

	return err;
}

static int
eckey_calculate_digest(const EC_KEY *eckey, const unsigned char *m,
					   size_t m_len, unsigned char (*out_md)[EVP_MAX_MD_SIZE],
					   size_t* out_md_len)
{
	int err;

	if (   NULL == eckey
		|| NULL == m
		|| 0 == m_len
		|| NULL == out_md
		|| 0 == out_md_len)
	{
		err = -1;
		goto DONE;
	}

	/* calculate SHA-256 message digest */
	if ((err = sha256_digest(m, m_len, out_md, out_md_len))) {
		err = -1;
		goto DONE;
	}

	/* Satify OpenSSL's ECDSA implementation by truncating the digest, which
	 * is limited to 192-bits.
	 * Truncation is accomplished via reducing the hash modulo the curve
	 * order.  This taken from the ECC implementation provided in the IWNCOMM
	 * WAPI client deliverable.
	*/
	if ((err = eckey_sha256_reduce(eckey, out_md, out_md_len))) {
		err = -3;
		goto DONE;
	}

	err = 0;
	
DONE:		
	return err;
}

/* This curve comes from the ECC implementation provided through IWNCOMM:
 * see src/wapi/asue/ECC2.2-2008/ecc.c:ECC_Init.
 * A patch to OpenSSL, also provided in the IWNCOMM AP release package, includes
 * a specification of NID_X9_62_prime192v4.  As of OpenSSL-1.0.0-beta3, only
 * NID_X9_62_prime192v3 is available.
*/
static const struct curve_data *
get_curve_x9_62_prime_192v4(void)
{
	static const struct curve_data data = 
		{ "BDB6F4FE3E8B1D9E0DA8C0D46F4C318CEFE4AFE3B6B8551F"
		, "BB8E5E8FBC115E139FE6A814FE48AAA6F0ADA1AA5DF91985"
		, "1854BEBDC31B21B7AEFC80AB0ECD10D5B1B3308E6DBF11C1"
		, "4AD5F7048DE709AD51236DE65E4D4B482C836DC6E4106640"
		, "02BB3A02D4AAADACAE24817A4CA3A1B014B5270432DB27D2"
		, "BDB6F4FE3E8B1D9E0DA8C0D40FC962195DFAE76F56564677"
		};

	return &data;
}

/* adapted from openssl/crypto/ec/ec_curve.c:ec_group_new_from_data */
static EC_GROUP *
curve_get_ecgroup(const struct curve_data *data)
{
	EC_GROUP *group=NULL;
	EC_POINT *P=NULL;
	BN_CTX	 *ctx=NULL;
	BIGNUM 	 *p=NULL, *a=NULL, *b=NULL, *x=NULL, *y=NULL, *order=NULL;
	int	 ok=0;
	
	if (NULL == (ctx = BN_CTX_new()))
		goto DONE;

	if (   NULL == (p = BN_new()) || NULL == (a = BN_new())
		|| NULL == (b = BN_new()) || NULL == (x = BN_new())
		|| NULL == (y = BN_new()) || NULL == (order = BN_new()))
	{
		goto DONE;
	}
	
	if (   !BN_hex2bn(&p, data->p)
		|| !BN_hex2bn(&a, data->a)
		|| !BN_hex2bn(&b, data->b))
	{
		goto DONE;
	}
	
	/* NID_X9_62_prime_field */
	if (NULL == (group = EC_GROUP_new_curve_GFp(p, a, b, ctx)))
		goto DONE;

	if (NULL == (P = EC_POINT_new(group)))
		goto DONE;
	
	if (!BN_hex2bn(&x, data->x) || !BN_hex2bn(&y, data->y))
		goto DONE;
		
#ifdef CONFIG_OPENSSL
	if (!EC_POINT_set_affine_coordinates_GF2m(group, P, x, y, ctx))
		goto DONE;
#else
	if (!EC_POINT_set_affine_coordinates_GFp(group, P, x, y, ctx))
		goto DONE;
#endif
		
	if (!BN_hex2bn(&order, data->order) || !BN_set_word(x, 1 /* cofactor */))
		goto DONE;

	if (!EC_GROUP_set_generator(group, P, order, x))
		goto DONE;
		
	ok=1;

DONE:
	if (!ok) {
		EC_GROUP_free(group);
		group = NULL;
	}
	if (P)
		EC_POINT_free(P);
	if (ctx)
		BN_CTX_free(ctx);
	if (p)
		BN_free(p);
	if (a)
		BN_free(a);
	if (b)
		BN_free(b);
	if (order)
		BN_free(order);
	if (x)
		BN_free(x);
	if (y)
		BN_free(y);

	return group;
}


#if defined(ECC_NEED_NID_X9_62_PRIME192V4)

static EC_KEY *
get_eckey(void)
{
	int ok = 0;
	EC_KEY *eckey = NULL;
	EC_GROUP *ecgroup = NULL;

	if (NULL == (eckey = EC_KEY_new()))
		goto DONE;
		
	if (NULL == (ecgroup = curve_get_ecgroup(get_curve_x9_62_prime_192v4())))
		goto DONE;

	if (!EC_KEY_set_group(eckey, ecgroup))
		goto DONE;

	ok = 1;

DONE:
	if (!ok) {
		EC_KEY_free(eckey);
		eckey = NULL;
	}
	if (NULL != ecgroup)
		EC_GROUP_free(ecgroup);

	return eckey;
}

#else /* defined(ECC_NEED_NID_X9_62_PRIME192V4) */

static EC_KEY *
get_eckey(void)
{
	return EC_KEY_new_by_curve_name(NID_X9_62_prime192v4);
}

#endif /* defined(ECC_NEED_NID_X9_62_PRIME192V4) */

/* decode BLOb to ECDSA_SIG structure */
static ECDSA_SIG *
bin2ecdsa_sig(const unsigned char *in, size_t in_len)
{
	ECDSA_SIG *sig, *out_sig=NULL;
	
	if (NULL == (sig = ECDSA_SIG_new()))
		goto DONE;

	if (   NULL == BN_bin2bn(in, in_len/2, sig->r)
		|| NULL == BN_bin2bn(in+in_len/2, in_len/2, sig->s))
	{
		goto DONE;
	}

	out_sig = sig;
	sig = NULL;

DONE:
	if (NULL == sig)
		ECDSA_SIG_free(sig);

    return out_sig;
}

/* encode ECDSA_SIG structure to BLOb */
static int
ecdsa_sig2bin(const ECDSA_SIG *sig, unsigned char *out, size_t *out_len)
{
	int err;
	int r_len, s_len;

	if (NULL == sig || NULL == out || 0 == out_len) {
		err = -1;
		goto DONE;
	}

	r_len = BN_num_bytes(sig->r);
	s_len = BN_num_bytes(sig->s);

	if (r_len > 24 || s_len > 24) {
		err = -2;
		goto DONE;
	}

	memset(out, 0, 48);

	/* pad left since output is big-endian */
	(void) BN_bn2bin(sig->r, out+24-r_len);
	(void) BN_bn2bin(sig->s, out+48-s_len);
	*out_len = 48;

	err = 0;

DONE:
	return err;
}

/*
 * public definitions
*/

int
ECC_Init(void)
{
	return 1;
}

int
ecc192_genkey(unsigned char *priv_key, unsigned char *pub_key)
{
	int err;
	EC_KEY *eckey=NULL;

	if (NULL == priv_key || NULL == pub_key) {
		err = -1;
		goto DONE;
	}

	if (NULL == (eckey = get_eckey())) {
		err = -2;
		goto DONE;
	}

	if (0 == EC_KEY_generate_key(eckey)) {
		err = -3;
		goto DONE;
	}

	if (0 != eckey_pubkey2bin(eckey, pub_key, 49)) {
		err = -4;
		goto DONE;
	}

	if (0 != eckey_privkey2bin(eckey, priv_key, 24)) {
		err = -5;
		goto DONE;
	}

	err = 0;

DONE:
	if (NULL != eckey)
		EC_KEY_free(eckey);
		
	return err;
}

int
ecc192_sign(const unsigned char *priv_key, const unsigned char *in,
			int in_len, unsigned char *out)
{
	int err;
	size_t out_len=0;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	size_t md_len;
	EC_KEY *eckey=NULL;
	ECDSA_SIG *sig=NULL;

	if (NULL == priv_key || NULL == in || 0 == in_len || NULL == out)
		goto DONE;

	if (NULL == (eckey = get_eckey()))
		goto DONE;

	if ((err = eckey_bin2privkey(eckey, priv_key, 24)))
		goto DONE;

	if ((err = eckey_calculate_digest(eckey, in, in_len, &md_value, &md_len)))
		goto DONE;

	if (NULL == (sig = ECDSA_do_sign(md_value, md_len, eckey)))
		goto DONE;

	if ((err = ecdsa_sig2bin(sig, out, &out_len)))
		goto DONE;

DONE:
	if (NULL != eckey)
		EC_KEY_free(eckey);
	if (NULL != sig)
		ECDSA_SIG_free(sig);

	return (int) out_len;
}

int
ecc192_verify(const unsigned char *pub_key, const unsigned char *in,
			  int in_len, const unsigned char *sign, int sign_len)
{
	int err, ok = 0;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	size_t md_len;
	EC_KEY *eckey=NULL;
	ECDSA_SIG *sig=NULL;

	if (   NULL == pub_key
		|| NULL == in
		|| 0 == in_len
		|| NULL == sign
		|| 0 == sign_len)
	{
		goto DONE;
	}
	
	if (NULL == (eckey = get_eckey()))
		goto DONE;

	if ((err = eckey_bin2pubkey(eckey, pub_key, 49)))
		goto DONE;

	if ((err = eckey_calculate_digest(eckey, in, in_len, &md_value, &md_len)))
		goto DONE;

	if (NULL == (sig = bin2ecdsa_sig(sign, sign_len)))
		goto DONE;
		
	if (1 != ECDSA_do_verify(md_value, md_len, sig, eckey))
		goto DONE;

	ok = 1;
	
DONE:
	if (NULL != eckey)
		EC_KEY_free(eckey);
	if (NULL != sig)
		ECDSA_SIG_free(sig);

	return ok;
}

#if defined(ECC_NO_ECC192_ECDH)
int
ecc192_ecdh(const unsigned char *priv_key, const unsigned char *pub_key,
			unsigned char *ecdhkey)
{
	int err;
	size_t len, out_len=0;
	EC_KEY *eckey;
	
	if (NULL == (eckey = get_eckey()))
		goto DONE;

	if ((err = eckey_bin2privkey(eckey, priv_key, 24)))
		goto DONE;

	if ((err = eckey_bin2pubkey(eckey, pub_key, 49)))
		goto DONE;

	if (0 >= (len = ECDH_compute_key(ecdhkey, 24,
		 EC_KEY_get0_public_key(eckey), eckey, NULL)))
	{
		goto DONE;
	}

	out_len = len;

DONE:
	if (NULL != eckey)
		EC_KEY_free(eckey);

	return out_len;
}
#endif /* defined(ECC_NO_ECC192_ECDH) */
