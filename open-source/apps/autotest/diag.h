// 
// Spreadtrum Auto Tester
//
// anli   2012-11-21
//
#ifndef _DIAG_20121121_H__
#define _DIAG_20121121_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//#define DIAG_PKT_FORMT_IS_BIG_ENDIAN
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_diag {
//------------------------------------------------------------------------------
#define DIAG_CMD_RESERVED              0x00
#define DIAG_CMD_KPD                   0x01
#define DIAG_CMD_LCD                   0x02 // parallel
#define DIAG_CMD_LCD_SPI               0x03
#define DIAG_CMD_CAM_IIC               0x04
#define DIAG_CMD_CAM_PARAL             0x05
#define DIAG_CMD_CAM_MIPI               0x06
#define DIAG_CMD_GPIO                  0x07
#define DIAG_CMD_TCARD                 0x08
#define DIAG_CMD_SIM                   0x09
#define DIAG_CMD_AUDIO_IN              0x0A
#define DIAG_CMD_AUDIO_OUT             0x0B
#define DIAG_CMD_LKBV                  0x0C
    #define DIAG_CMD_LKBV_LCDBACKLIGHT     0x01
    #define DIAG_CMD_LKBV_KPDBACKLIGHT     0x02
    #define DIAG_CMD_LKBV_VIBRATE          0x03
#define DIAG_CMD_FM                    0x0D
#define DIAG_CMD_ATV                   0x0E
#define DIAG_CMD_BT                    0x0F
#define DIAG_CMD_WIFI                  0x10
#define DIAG_CMD_IIC                   0x11
#define DIAG_CMD_CHARGE                0x12

//#define DIAG_CMD_RESULT_READ           0x13
//#define DIAG_CMD_RESULT_WRITE          0x14
#define DIAG_CMD_SENSOR                0x15
#define DIAG_CMD_GPS                   0x16

#define DIAG_CMD_MAX                   0x20

#define DIAG_CMD_EXIT                  0xFF

#define DIAG_PACKET_MIN_SIZE           0x09

struct diag_cmd_t {
    unsigned int    sn;
    
    unsigned char   cmd;
    unsigned char   rsv0;  // for align, not used
    unsigned short  rsv1;  // for align, not used
    
    unsigned char * buf;
    
    int             len;
};

int  diagStart( void );

int  diagGetCmd( struct diag_cmd_t ** pcmd, int timeout );
void diagFreeCmd( struct diag_cmd_t * pcmd );

int  diagSendResult( const struct diag_cmd_t * cmd );

int  diagStop( void );

//------------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//------------------------------------------------------------------------------

#endif // _DIAG_20121121_H__
