#include "sms4const.h"
//#define LE     //定义小端字节序
#define ENCRYPT  0     //定义加密标志
#define DECRYPT  1     //定义解密标志


// SMS4的加解密函数
// 参数说明：Input为输入信息分组，Output为输出分组，rk为轮密钥
void SMS4Crypt(unsigned char *Input, unsigned char *Output, unsigned int *rk)
{
	 unsigned int r, mid, x0, x1, x2, x3, *p;
	 p = (unsigned int *)Input;
	 x0 = p[0];
	 x1 = p[1];
	 x2 = p[2];
	 x3 = p[3];
#ifdef LE
	 x0 = Rotl(x0, 16); x0 = ((x0 & 0x00FF00FF) << 8) ^ ((x0 & 0xFF00FF00) >> 8);
	 x1 = Rotl(x1, 16); x1 = ((x1 & 0x00FF00FF) << 8) ^ ((x1 & 0xFF00FF00) >> 8);
	 x2 = Rotl(x2, 16); x2 = ((x2 & 0x00FF00FF) << 8) ^ ((x2 & 0xFF00FF00) >> 8);
	 x3 = Rotl(x3, 16); x3 = ((x3 & 0x00FF00FF) << 8) ^ ((x3 & 0xFF00FF00) >> 8);
#endif
#if 0
#ifdef LE
	printf("-------LITTLE_ENDIAN-----------(in %s)\n", __func__);
	printf("x0 = %08x\n", x0);printf("x1 = %08x\n", x1);
	printf("x2 = %08x\n", x2);printf("x3 = %08x\n", x3);
#else	
	printf("-------LITTLE_ENDIAN-----------(in %s)\n", __func__);
	printf("x0 = %08x\n", x0);printf("x1 = %08x\n", x1);
	printf("x2 = %08x\n", x2);printf("x3 = %08x\n", x3);
#endif
#endif
	 for (r = 0; r < 32; r += 4)
	 {
		  mid = x1 ^ x2 ^ x3 ^ rk[r + 0];
		  mid = ByteSub(mid);
		  x0 ^= L1(mid);
		  mid = x2 ^ x3 ^ x0 ^ rk[r + 1];
		  mid = ByteSub(mid);
		  x1 ^= L1(mid);
		  mid = x3 ^ x0 ^ x1 ^ rk[r + 2];
		  mid = ByteSub(mid);
		  x2 ^= L1(mid);
		  mid = x0 ^ x1 ^ x2 ^ rk[r + 3];
		  mid = ByteSub(mid);
		  x3 ^= L1(mid);
	 }
#ifdef LE
	 x0 = Rotl(x0, 16); x0 = ((x0 & 0x00FF00FF) << 8) ^ ((x0 & 0xFF00FF00) >> 8);
	 x1 = Rotl(x1, 16); x1 = ((x1 & 0x00FF00FF) << 8) ^ ((x1 & 0xFF00FF00) >> 8);
	 x2 = Rotl(x2, 16); x2 = ((x2 & 0x00FF00FF) << 8) ^ ((x2 & 0xFF00FF00) >> 8);
	 x3 = Rotl(x3, 16); x3 = ((x3 & 0x00FF00FF) << 8) ^ ((x3 & 0xFF00FF00) >> 8);
#endif
	 p = (unsigned int *)Output;
	 p[0] = x3;
	 p[1] = x2;
	 p[2] = x1;
	 p[3] = x0;
}

// SMS4的密钥扩展算法
// 参数说明：Key为加密密钥，rk为子密钥，CryptFlag为加解密标志
void SMS4KeyExt(unsigned char *Key, unsigned int *rk, unsigned int CryptFlag)
{
	 unsigned int r, mid, x0, x1, x2, x3, *p;

	CryptFlag = CryptFlag;
	 
	 p = (unsigned int *)Key;
	 x0 = p[0];
	 x1 = p[1];
	 x2 = p[2];
	 x3 = p[3];
#ifdef LE
	 x0 = Rotl(x0, 16); x0 = ((x0 & 0xFF00FF) << 8) ^ ((x0 & 0xFF00FF00) >> 8);
	 x1 = Rotl(x1, 16); x1 = ((x1 & 0xFF00FF) << 8) ^ ((x1 & 0xFF00FF00) >> 8);
	 x2 = Rotl(x2, 16); x2 = ((x2 & 0xFF00FF) << 8) ^ ((x2 & 0xFF00FF00) >> 8);
	 x3 = Rotl(x3, 16); x3 = ((x3 & 0xFF00FF) << 8) ^ ((x3 & 0xFF00FF00) >> 8);
#endif
#if 0
#ifdef LE
	printf("-------LITTLE_ENDIAN-----------(in %s)\n", __func__);
	printf("x0 = %08x\n", x0);printf("x1 = %08x\n", x1);
	printf("x2 = %08x\n", x2);printf("x3 = %08x\n", x3);
#else	
	printf("-------LITTLE_ENDIAN-----------(in %s)\n", __func__);
	printf("x0 = %08x\n", x0);printf("x1 = %08x\n", x1);
	printf("x2 = %08x\n", x2);printf("x3 = %08x\n", x3);
#endif
#endif
	 x0 ^= 0xa3b1bac6;
	 x1 ^= 0x56aa3350;
	 x2 ^= 0x677d9197;
	 x3 ^= 0xb27022dc;
	 for (r = 0; r < 32; r += 4)
	 {
		  mid = x1 ^ x2 ^ x3 ^ CK[r + 0];
		#if 0  
		  printf("mid1[%d]=%04x\n", r,mid);
		  printf("ByteSub(%d) (Sbox[(%d) >> 24 & 0xFF] << 24 ^ Sbox[(%d) >> 16 & 0xFF] << 16 ^ Sbox[(%d) >>  8 & 0xFF] <<  8 ^ Sbox[(%d) & 0xFF]\n",ByteSub(mid), mid, mid, mid, mid);
		#endif
		  mid = ByteSub(mid);
		  rk[r + 0] = x0 ^= L2(mid);
		  mid = x2 ^ x3 ^ x0 ^ CK[r + 1];
		  mid = ByteSub(mid);
		  rk[r + 1] = x1 ^= L2(mid);
		  mid = x3 ^ x0 ^ x1 ^ CK[r + 2];
		  mid = ByteSub(mid);
		  rk[r + 2] = x2 ^= L2(mid);
		  mid = x0 ^ x1 ^ x2 ^ CK[r + 3];
		  mid = ByteSub(mid);
		  rk[r + 3] = x3 ^= L2(mid);
	 }
#if 0	 
	 if (CryptFlag == DECRYPT)
	 {
	 	  for (r = 0; r < 16; r++)
	 	  	 mid = rk[r], rk[r] = rk[31 - r], rk[31 - r] = mid;
	 }
#endif	 
}

