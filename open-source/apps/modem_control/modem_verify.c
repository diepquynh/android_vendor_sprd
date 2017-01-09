/******************************************************************************
** Copyright (C) 2015 SPREADTRUM
** Create for secure boot
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "sec_boot.h"
#include "modem_verify.h"
#include "modem_control.h"
#include "packet.h"
/*-----------------------------------------------------------------------------
 LOCAL FUNCTION DEFINITIONS
-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
 DEFINES
-----------------------------------------------------------------------------*/
#define SHA1_SUM_LEN    20
/*-----------------------------------------------------------------------------
 LOCAL Variable declaration
-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
 FUNCTION CODE
-----------------------------------------------------------------------------*/
#if 0
static void BinToHex(char *bin_ptr, int length)
{
    int i;
    unsigned char tmp;
    char *hex_ptr = NULL;
    char *p = NULL;

    if (bin_ptr == NULL) {
        return;
    }

    hex_ptr = malloc(length * 2 + 1);
    if (hex_ptr == 0) {
        MODEM_LOGE("[secure]%s: Malloc failed!!", __FUNCTION__);
        return;
    }
    memset(hex_ptr, 0, (length * 2 + 1));
    p = hex_ptr;

    for (i=0; i<length; i++) {
        tmp = (unsigned char)((bin_ptr[i] & 0xf0)>>4);
        if (tmp <= 9) {
            *hex_ptr = (unsigned char)(tmp + '0');
        } else {
            *hex_ptr = (unsigned char)(tmp + 'A' - 10);
        }
        hex_ptr++;
        tmp = (unsigned char)(bin_ptr[i] & 0x0f);
        if (tmp <= 9) {
            *hex_ptr = (unsigned char)(tmp + '0');
        } else {
            *hex_ptr = (unsigned char)(tmp + 'A' - 10);
        }
        hex_ptr++;
    }
    MODEM_LOGD("[secure]%s: hex is: %s", __FUNCTION__, p);
}
#endif

int secure_header_parser(const char *header_addr)
{
    if (strcmp(header_addr, HEADER_NAME) == 0)
        return 1;
    else
        return 0;
}

uint8_t *get_code_addr(uint8_t *header_addr)
{
    uint32_t         i = 0;
    uint8_t         *addr = NULL;
    header_info_t   *header_p = (header_info_t *)header_addr;

    for (i = 0;i < header_p->tags_number;i++) {
        if (header_p->tag[i].tag_name == CODE_NAME) {
            addr = header_addr + (header_p->tag[i].tag_offset)*MIN_UNIT;
            break;
        }
    }
    return addr;
}

uint8_t *get_vlr_addr(uint8_t *header_addr)
{
    uint32_t         i = 0;
    uint8_t         *addr = NULL;
    header_info_t   *header_p = (header_info_t *)header_addr;

    for (i = 0;i < header_p->tags_number;i++) {
        if (header_p->tag[i].tag_name == VLR_NAME) {
            addr = header_addr + (header_p->tag[i].tag_offset)*MIN_UNIT;
            break;
        }
    }
    return addr;
}

void secure_verify(uint8_t *data_ptr, uint8_t *puk_ptr)
{
    uint8_t     *vlr_addr = NULL;
    uint8_t     *code_addr = NULL;
    int          i, len;

    MODEM_LOGD("[secure]%s: enter", __FUNCTION__);
    if (!secure_header_parser((const char *)data_ptr)) {
        MODEM_LOGD("[secure]%s: secure header mismatch", __FUNCTION__);
        return;
    }
    vlr_addr = get_vlr_addr(data_ptr);
    code_addr = get_code_addr(data_ptr);
    MODEM_LOGD("[secure]puk_ptr=%p,vlr_addr=%p,code_addr=%p\n",puk_ptr,vlr_addr,code_addr);

    MODEM_LOGD("[secure]secure_check begin");
    secure_check(code_addr, ((vlr_info_t*)vlr_addr)->length, vlr_addr, puk_ptr);
    MODEM_LOGD("[secure]secure_check end");
}

