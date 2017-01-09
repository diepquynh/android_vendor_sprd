// SMS4的加解密函数
// 参数说明：Input为输入信息分组，Output为输出分组，rk为轮密钥
void SMS4Crypt(unsigned char *Input, unsigned char *Output, unsigned int *rk);


// SMS4的密钥扩展算法
// 参数说明：Key为加密密钥，rk为子密钥，CryptFlag为加解密标志
void SMS4KeyExt(unsigned char *Key, unsigned int *rk, unsigned int CryptFlag);



