
#ifndef __EM_BT_H__
#define __EM_BT_H__

#include <stdbool.h>


#ifndef BOOL
#define BOOL  bool
#endif

#ifdef __cplusplus
extern "C" {
#endif

BOOL EM_BT_init(unsigned char type);
void EM_BT_deinit(void);
BOOL EM_BT_write(unsigned char *peer_buf, int peer_len);
BOOL EM_BT_read(unsigned char *peer_buf, int peer_len, unsigned int *piResultLen);

void EM_BT_getChipInfo(unsigned char *chip_id);
void EM_BT_getPatchInfo(char *patch, unsigned long *patch_len);

#ifdef __cplusplus
}
#endif

#endif

