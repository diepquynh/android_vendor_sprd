// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#ifndef _BT_20121110_H__
#define _BT_20121110_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_bt {
//-----------------------------------------------------------------------------

struct hci_ver_t {
	uint16_t manufacturer;
	uint8_t  hci_ver;
	uint8_t  lmp_ver;
};

#define MAX_SUPPORT_RMTDEV_NUM 12
#define MAX_REMOTE_DEVICE_NAME_LEN (18)
#define BD_ADDR_LEN_HEX (6)
#define BD_ADDR_LEN_STRING (18)

typedef struct bdremote_t{
    unsigned char addr_u8[BD_ADDR_LEN_HEX];
    char addr_str[BD_ADDR_LEN_STRING];
    char name[MAX_REMOTE_DEVICE_NAME_LEN];
    int rssi_val;
}bdremote_t;


int btOpen( void );
int btIsOpened( void );

int btGetLocalVersion( struct hci_ver_t *ver );

int btGetRssi( int *rssi );

int btInquire(bdremote_t * bdrmt, int maxnum );
typedef char bdstr_t[18];

#define BT_STATUS_INQUIRING   -1
#define BT_STATUS_INQUIRE_END -2
#define BT_STATUS_INQUIRE_ERR -3
#define BT_STATUS_INQUIRE_UNK -4

int btAsyncInquire( void );
int btGetInquireStatus( void );
void btSetInquireStatus(int status);
int btGetInquireResult( struct bdremote_t * bdrmt, int maxnum );

int btClose( void );

#ifdef SPRD_WCNBT_MARLIN
int fmOpenEx( void );

int fmPlayEx( uint freq );

int fmStopEx( void );

int fmCloseEx( void );
#endif
//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _BT_20121110_H__
