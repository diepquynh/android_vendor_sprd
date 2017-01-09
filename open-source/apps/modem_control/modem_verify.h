/******************************************************************************
** Copyright (C) 2015 SPREADTRUM
** Create for secure boot
******************************************************************************/
#ifndef MODEM_VERIFY_H
#define MODEM_VERIFY_H
/*-----------------------------------------------------------------------------
 DEFINES
-----------------------------------------------------------------------------*/
#define BOOT_INFO_SIZE  (512)
#define KEY_INFO_SIZ    (512)
#define VLR_INFO_SIZ    (512)
#define MIN_UNIT        (512)
#define HEADER_NAME     "SPRD-SECUREFLAG"
#define VLR_NAME        (0x3A524C56)
#define PUK_NAME        (0x3A4B5550)
#define CODE_NAME       (0x45444F43)
#define CMDLINE_LENGTH  (2048)
#define CMD_PUKSTRING   "pubk="
#define PUBKEY_LEN      (260)
/*-----------------------------------------------------------------------------
 ENUMERATED TYPEDEF'S
-----------------------------------------------------------------------------*/
typedef struct{
    uint32_t  tag_name;
    uint32_t  tag_offset;
    uint32_t  tag_size;
    uint8_t   reserved[4];
}tag_info_t;

typedef struct{
    uint8_t     header_magic[16];
    uint32_t    header_ver;
    uint32_t    tags_number;
    uint8_t     header_ver_padding[8];
    tag_info_t  tag[3];
    uint8_t     reserved[432];
}header_info_t;
/*-----------------------------------------------------------------------------
 GLOBAL FUNCTION DEFINITIONS
-----------------------------------------------------------------------------*/
void secure_verify(uint8_t *data_ptr, uint8_t *puk_ptr);
#endif
