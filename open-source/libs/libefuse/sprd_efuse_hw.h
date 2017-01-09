/*
*  sprd_efuse_hw.h
*
*  Created on: 2014-06-05
*  Author: ronghua.yu
*/

#ifndef _SPRD_EFUSE_HW_H_
#define _SPRD_EFUSE_HW_H_

//#include <misc/fuse_driver.h>

int efuse_hash_read(unsigned char *hash, int count);
int efuse_hash_write(unsigned char *hash, int count);
int efuse_uid_read(unsigned char *uid, int count);
int efuse_secure_enable(void);
int efuse_secure_is_enabled(void);
int efuse_is_hash_write(void);

#endif /* _SPRD_EFUSE_HW_H_  */

