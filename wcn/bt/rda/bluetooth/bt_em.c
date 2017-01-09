
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <dlfcn.h>
#include <string.h>

#include "bt_cfg.h"
#include "bt_em.h"
#include "bt_rda.h"


typedef unsigned long DWORD;
typedef unsigned long* PDWORD;
typedef unsigned long* LPDWORD;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef unsigned long HANDLE;
typedef void VOID;
typedef void* LPCVOID;
typedef void* LPVOID;
typedef void* LPOVERLAPPED;
typedef unsigned char* PUCHAR;
typedef unsigned char* PBYTE;
typedef unsigned char* LPBYTE;


#ifndef BT_DRV_MOD_NAME
#define BT_DRV_MOD_NAME     "bluetooth"
#endif

//===============        Global Variables         =======================
static int   bt_fd = -1;
unsigned char EM_type;

/**************************************************************************
  *                         F U N C T I O N S                             *
***************************************************************************/

BOOL EM_BT_init(unsigned char type)
{
    EM_type = type;
    if(set_bluetooth_power(1) < 0) 
    {
        ERR_PRINT("BT power on fails\n");  
        
        goto err;
    }

    bt_fd = init_uart();

    if(bt_fd < 0)
    {
        ERR_PRINT("BT init_uart fails\n");  
        
        goto err;
    }

    tcflush(bt_fd, TCIOFLUSH);

    DBG_PRINT("BT is enabled success\n");
    
    return TRUE;
    
err:
    set_bluetooth_power(0);

    ERR_PRINT("EM_BT_init fails\n");

    return FALSE;
}

void EM_BT_deinit(void)
{
    close_uart();

    set_bluetooth_power(0);

    bt_fd =-1;

    DBG_PRINT("EM_BT_deinit.\n");
}

BOOL EM_BT_write(unsigned char *peer_buf, int  peer_len)
{
    if (peer_buf == NULL)
    {
        ERR_PRINT("NULL write buffer\n");
        
        return FALSE;
    }
    
    if (peer_len < 3)
    {
        ERR_PRINT("Invalid buffer length %d\n", peer_len);
        
        return FALSE;    
    }
    
    if ((peer_buf[0] != 0x01) && (peer_buf[0] != 0x02))
    {
        ERR_PRINT("Invalid packet type 0x%02x\n", peer_buf[0]);
        
        return FALSE;    
    }
    
    if (bt_fd < 0)
    {
        ERR_PRINT("bt driver fd is invalid!\n");
        
        return FALSE;
    }
    
    if (write(bt_fd, peer_buf, peer_len) < 0)
    {
        ERR_PRINT("write serial failed!\n");
    
        return FALSE;
    }
    
    return TRUE;
}

BOOL EM_BT_read(unsigned char *peer_buf, int  peer_len, unsigned int *piResultLen)
{
    BOOL  RetVal = TRUE;
    UCHAR ucHeader = 0;
    int iLen = 0, pkt_len = 0;
	
    if (peer_buf == NULL)
    {
        ERR_PRINT("NULL read buffer\n");
        return FALSE;
    }
    
    if (peer_len < 258)
    {
        ERR_PRINT("Invalid buffer length %d\n", peer_len);
        
        return FALSE;    
    }
     
    if (bt_fd < 0)
    {
        ERR_PRINT("bt driver fd is invalid!\n");
        
        return FALSE;
    }

LOOP:
    if(read(bt_fd, &ucHeader, sizeof(ucHeader)) < 0)
    {
        ERR_PRINT("Read packet header fails\n");
        
        goto LOOP;
    }
    
    peer_buf[0] = ucHeader;
    iLen ++;
    
    switch (ucHeader)
    {
        case 0x04:
            DBG_PRINT("Receive HCI event\n");
            
            if(read(bt_fd, &peer_buf[1], 2) < 0)
            {
                ERR_PRINT("Read event header fails\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += 2;
            
            if(read(bt_fd, &peer_buf[3], (int)peer_buf[2]) < 0)
            {
                ERR_PRINT("Read event parameter fails\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += (int)peer_buf[2];
            *piResultLen = iLen;
            break;
                
        case 0x02:
            DBG_PRINT("Receive ACL data\n");
            
            if(read(bt_fd, &peer_buf[1], 4) < 0)
            {
                ERR_PRINT("Read ACL header fails\n");
                *piResultLen = iLen;
                return FALSE;
            }
            iLen += 4;
            
            pkt_len = (((int)peer_buf[4]) << 8);
            pkt_len += peer_buf[3];//little endian
            
            if(read(bt_fd, &peer_buf[5], pkt_len) < 0)
            {
                ERR_PRINT("Read ACL data fails\n");
                *piResultLen = iLen;
                return FALSE;
            }
            
            iLen += pkt_len;
            *piResultLen = iLen;
            break;                
            
        default:
            ERR_PRINT("Unexpected BT packet header, %d\r\n", ucHeader);
            return FALSE;
            break; 
    }
    
    return TRUE;
}

void EM_BT_getChipInfo(unsigned char *chip_id)
{
    unsigned char id = rdabt_chip_version;
    
    *chip_id = id;    
   
    DBG_PRINT("ChipID:%02x\n", id);
}
