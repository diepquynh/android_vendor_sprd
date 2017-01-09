#ifndef FDL_CRC_H
#define FDL_CRC_H
#ifdef __cplusplus
extern   "C"
{
#endif

#include "cmd_def.h"
//////////////////////////////////////////////////////////
//CRC
#define CRC_16_POLYNOMIAL       0x1021
#define CRC_16_L_POLYNOMIAL     0x8000
#define CRC_16_L_SEED           0x80
#define CRC_16_L_OK             0x00
#define HDLC_FLAG               0x7E
#define HDLC_ESCAPE             0x7D
#define HDLC_ESCAPE_MASK        0x20
#define CRC_CHECK_SIZE          0x02



///////////////////////////////////////////////////////////
unsigned int crc_16_l_calc (char *buf_ptr,unsigned int len);

unsigned short frm_chk (const unsigned short *src, int len);
unsigned short boot_checksum (const unsigned char *src, int len);
#ifdef __cplusplus
}
#endif
#endif /* FDL_CRC_H */
