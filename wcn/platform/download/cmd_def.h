#ifndef CMD_DEFINE_H
#define CMD_DEFINE_H

typedef enum _CMD_TYPE
{
    BSL_PKT_TYPE_MIN = 0,                       /* the bottom of the DL packet type range */
    BSL_CMD_TYPE_MIN = BSL_PKT_TYPE_MIN,        /* 0x0 */

    /* Link Control */
    BSL_CMD_CONNECT = BSL_CMD_TYPE_MIN,         /* 0x0 */
    /* Data Download */
    /* the start flag of the data downloading */
    BSL_CMD_START_DATA,                         /* 0x1 */
    /* the midst flag of the data downloading */
    BSL_CMD_MIDST_DATA,                         /* 0x2 */
    /* the end flag of the data downloading */
    BSL_CMD_END_DATA,                           /* 0x3 */
    /* Execute from a certain address */
    BSL_CMD_EXEC_DATA,                          /* 0x4 */
    BSL_CMD_NORMAL_RESET,                       /* 0x5 */
    BSL_CMD_READ_FLASH,                         /* 0x6 */
    BSL_CMD_READ_CHIP_TYPE,                     /* 0x7 */
    BSL_CMD_LOOKUP_NVITEM,                      /* 0x8 */
    BSL_SET_BAUDRATE,                           /* 0x9 */
    BSL_ERASE_FLASH,                            /* 0xA */
    BSL_REPARTITION,                            /* 0xB */
#ifdef GET_MARLIN_CHIPID
    BSL_CMD_READ_CHIPID,						/* 0xC */
#endif
    BSL_CMD_SWITCH_MODE = 0x0D,
    BSL_CMD_DUMP_MEM,                         	/* 0xE */
    BSL_CMD_TYPE_MAX,

    /* Start of the Command can be transmited by phone*/
    BSL_REP_TYPE_MIN = 0x80,

    /* The operation acknowledge */
    BSL_REP_ACK = BSL_REP_TYPE_MIN,         /* 0x80 */
    BSL_REP_VER,                            /* 0x81 */

    /* the operation not acknowledge */
    /* system  */
    BSL_REP_INVALID_CMD,                    /* 0x82 */
    BSL_REP_UNKNOW_CMD,                     /* 0x83 */
    BSL_REP_OPERATION_FAILED,               /* 0x84 */

    /* Link Control*/
    BSL_REP_NOT_SUPPORT_BAUDRATE,           /* 0x85 */

    /* Data Download */
    BSL_REP_DOWN_NOT_START,                 /* 0x86 */
    BSL_REP_DOWN_MULTI_START,               /* 0x87 */
    BSL_REP_DOWN_EARLY_END,                 /* 0x88 */
    BSL_REP_DOWN_DEST_ERROR,                /* 0x89 */
    BSL_REP_DOWN_SIZE_ERROR,                /* 0x8A */
    BSL_REP_VERIFY_ERROR,                   /* 0x8B */
    BSL_REP_NOT_VERIFY,                     /* 0x8C */

    /* Phone Internal Error */
    BSL_PHONE_NOT_ENOUGH_MEMORY,            /* 0x8D */
    BSL_PHONE_WAIT_INPUT_TIMEOUT,           /* 0x8E */

    /* Phone Internal return value */
    BSL_PHONE_SUCCEED,                      /* 0x8F */
    BSL_PHONE_VALID_BAUDRATE,               /* 0x90 */
    BSL_PHONE_REPEAT_CONTINUE,              /* 0x91 */
    BSL_PHONE_REPEAT_BREAK,                 /* 0x92 */

    BSL_REP_READ_FLASH,                     /* 0x93 */
    BSL_REP_READ_CHIP_TYPE,                 /* 0x94 */
    BSL_REP_LOOKUP_NVITEM,                  /* 0x95 */

    BSL_INCOMPATIBLE_PARTITION,             /* 0x96 */
    BSL_UNKNOWN_DEVICE,                     /* 0x97 */
    BSL_INVALID_DEVICE_SIZE,                /* 0x98 */

    BSL_ILLEGAL_SDRAM,                      /* 0x99 */
    BSL_WRONG_SDRAM_PARAMETER,              /* 0x9a */
    BSL_EEROR_CHECKSUM = 0xA0,
    BSL_CHECKSUM_DIFF,
    BSL_WRITE_ERROR,
    BSL_PKT_TYPE_MAX
} CMD_TYPE;

typedef CMD_TYPE DLSTATUS;

typedef CMD_TYPE cmd_pkt_type;
typedef cmd_pkt_type ret_status;


/**---------------------------------------------------------------------------*
 ** The Follow defines the packet processed result table                      *
 ** packet_protocol:                                                          *
 ** HDLC_FLAG   PKT_TYPE     DATALENGHT    [DATA]    CRC       HDLC_FLAG      *
 **   0x7E      MAX:255      MAX:65536      ...      ...          0x7E        *
 **    1           2             2           0        2             1         *
 ** response_packet:                                                          *
 **   0x7E      MAX:255          0           --      ...          0x7E        *
 **    1           2             2           --       2             1         *
 ** response packet length: 8 bytes                                           *
 **---------------------------------------------------------------------------*/

typedef struct _PKT_HEADER
{
    unsigned short type;
    unsigned short size;
} PKT_HEADER, *PPKT_HEADER;

typedef struct pkt_header_tag
{
    unsigned short type;
    unsigned short length;
} pkt_header, *pkt_header_ptr;

#define PKT_FLAG_SIZE              1
#define PKT_CRC_SIZE               2
#define PKT_TYPE_SIZE              1
#define PKT_LEN_SIZE               2

#define PKT_HEADER_SIZE            sizeof(PKT_HEADER)
#define DATA_ADDR                  PKT_HEADER_SIZE


#define SEND_ERROR_RSP(x)         \
    {                       \
        FDL_SendAckPacket(x);        \
        while(1);           \
    }
#endif /* CMD_DEFINE_H */
