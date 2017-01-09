// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <pthread.h>
#include <signal.h>

#include "type.h"
#include "diag.h"
#include "channel.h"

//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_diag {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--using namespace sci_chl;

//------------------------------------------------------------------------------
#define DIAG_PORT_PATH       "/dev/vser"

#define DIAG_FLAG            0x7E
#define DIAG_ESCAPE          0x7D
#define DIAG_COMPLEMENT      0x20

#define DIAG_AUTOTEST_CMD    0x38
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
struct cmd_info_t {
    struct diag_cmd_t * dcmd;
    struct cmd_info_t * next;
};

//------------------------------------------------------------------------------
static int                 sChlHandle = -1;
static volatile int        sReceiving = 0;
static pthread_t           sThreadID  = -1;
static pthread_mutex_t     sMutxEvent = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t      sCondEvent = PTHREAD_COND_INITIALIZER;
static struct cmd_info_t * sCmdLstHdr = NULL;
static struct cmd_info_t * sCmdLstTal = NULL;
//------------------------------------------------------------------------------

static void              * diagReceiverThread( void *param );
static struct cmd_info_t * diagMallocCmdInfo( uint sn, uchar cmd, uchar * data, int len );
static void                diagFreeCmdInfo( struct cmd_info_t * cmdinfo );
static void                diagPushCmd( struct cmd_info_t * info );
static int                 diagParseData( uchar * data, int len, cmd_info_t ** pinfo );
//------------------------------------------------------------------------------

static void diagSigRoutine( int sig ) 
{
    DBGMSG("diagSigRoutine: sig = %d\n", sig);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int diagStart( void )
{
    DBGMSG(" %s enter\n", __FUNCTION__);
    
    AT_ASSERT( -1 == sChlHandle );
    
    //signal(SIGALRM, diagSigRoutine);
    
    sChlHandle = chlOpen(DIAG_PORT_PATH);
    if( sChlHandle < 0 ) {
        ERRMSG("open '%s' error!\n", DIAG_PORT_PATH);
        return sChlHandle;
    }
    
    sCmdLstHdr = sCmdLstTal = NULL;
        
    sReceiving = 1;
    
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&sThreadID, &attr, diagReceiverThread, NULL);
    
    DBGMSG(" %s exit\n", __FUNCTION__);
    return 0;
}

int diagGetCmd( struct diag_cmd_t ** pcmd, int timeout )
{
    //DBGMSG(" %s enter\n", __FUNCTION__);
    
    int ret = -1;
    pthread_mutex_lock(&sMutxEvent);
    while ( sReceiving ) {
        if( NULL != sCmdLstHdr ) {
            *pcmd = sCmdLstHdr->dcmd;
            if( sCmdLstHdr == sCmdLstTal ) {
                sCmdLstHdr = NULL;
                sCmdLstTal = NULL;
            } else {
                sCmdLstHdr = sCmdLstHdr->next;
            }
            ret = 0;
            DBGMSG("get cmd done.\n");
            break;
        } else {
            struct timespec to;
            to.tv_nsec = (timeout % 1000) * 1000;
            to.tv_sec  = time(NULL) + timeout / 1000;
        
            if( 0 == pthread_cond_timedwait(&sCondEvent, &sMutxEvent, &to) ) {
                //DBGMSG("cmd arrived.\n");
                continue;
            } else {
                //DBGMSG("get cmd time out.\n");
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&sMutxEvent);
    //DBGMSG(" %s exit, ret = %d\n", __FUNCTION__, ret);
    return ret;
}

void diagPushCmd( struct cmd_info_t * info )
{
    DBGMSG(" %s enter\n", __FUNCTION__);
    pthread_mutex_lock(&sMutxEvent);

    info->next = NULL;

    if( NULL == sCmdLstTal ) {
        sCmdLstHdr = info;
    } else {
        sCmdLstTal->next = info;
    }
    sCmdLstTal = info;

    pthread_cond_signal(&sCondEvent);
    pthread_mutex_unlock(&sMutxEvent);
    DBGMSG(" %s exit\n", __FUNCTION__);
}

void diagFreeCmd( struct diag_cmd_t * pcmd )
{
    struct cmd_info_t * cmdinfo = (struct cmd_info_t *)((char *)pcmd - sizeof(cmd_info_t));
     
    diagFreeCmdInfo( cmdinfo );
}

int diagSendResult( const struct diag_cmd_t * cmd )
{
    AT_ASSERT( cmd != NULL );
    
    DBGMSG("%s ++\n", __FUNCTION__);
    
    uchar  pckHdr[8];
    
    #define MAX_BUF_SIZE 1024
    static uchar packet[MAX_BUF_SIZE * 2];
    uchar * ppckt = packet;
    if( cmd->len > MAX_BUF_SIZE ) {
        ppckt = new uchar[cmd->len * 2];
        if( NULL == ppckt ) {
            ERRMSG("new %d bytes fail!\n", cmd->len);
            return -1;
        }
    }
    
    //--------------------------------------------------------------------------
    // packet header: sn 4 bytes, len 2 bytes, cmd 1 byte, sub cmd 1 byte
    //--------------------------------------------------------------------------
    
    // not include header and tail flag
    unsigned short pcktLen = 4 + 2 + 1 + 1 + cmd->len;
    
    // sn
    uint sn = cmd->sn;
    //DBGMSG("sn = 0x%08x\n", sn);
#ifdef DIAG_PKT_FORMT_IS_BIG_ENDIAN    
    pckHdr[0] = (uchar)(sn >> 24);
    pckHdr[1] = (uchar)(sn >> 16);
    pckHdr[2] = (uchar)(sn >> 8);
    pckHdr[3] = (uchar)(sn >> 0);
#else    
    pckHdr[0] = (uchar)(sn >> 0);
    pckHdr[1] = (uchar)(sn >> 8);
    pckHdr[2] = (uchar)(sn >> 16);
    pckHdr[3] = (uchar)(sn >> 24);
#endif // DIAG_PKT_FORMT_IS_BIG_ENDIAN
    // packet length
#ifdef DIAG_PKT_FORMT_IS_BIG_ENDIAN    
    pckHdr[4] = (uchar)(pcktLen >> 8);
    pckHdr[5] = (uchar)(pcktLen >> 0);
#else    
    pckHdr[4] = (uchar)(pcktLen >> 0);
    pckHdr[5] = (uchar)(pcktLen >> 8);
#endif // DIAG_PKT_FORMT_IS_BIG_ENDIAN
    // cmd
    pckHdr[6] = DIAG_AUTOTEST_CMD;
    // sub cmd
    pckHdr[7] = cmd->cmd;
    
    //--------------------------------------------------------------------------
    // packet data
    //--------------------------------------------------------------------------
    int cnt = 0;
    ppckt[cnt++] = DIAG_FLAG;
    
    // header
    for( int i = 0; i < 8; ++i ) {
        if( DIAG_FLAG == pckHdr[i] || DIAG_ESCAPE == pckHdr[i] ) {
            ppckt[cnt++] = DIAG_ESCAPE;
            ppckt[cnt++] = pckHdr[i] ^ DIAG_COMPLEMENT;
        } else {
            ppckt[cnt++] = pckHdr[i];
        }
    }
    
    // data
    uchar * data = cmd->buf;
    for( int i = 0; i < cmd->len; ++i ) {
        if( DIAG_FLAG == data[i] || DIAG_ESCAPE == data[i] ) {
            ppckt[cnt++] = DIAG_ESCAPE;
            ppckt[cnt++] = data[i] ^ DIAG_COMPLEMENT;
        } else {
            ppckt[cnt++] = data[i];
        }
    }
    ppckt[cnt++] = DIAG_FLAG;
    
    AT_ASSERT( cnt <= (2 + DIAG_PACKET_MIN_SIZE) + cmd->len * 2 );
    
    int write = chlWrite(sChlHandle, ppckt, cnt);
    if( ppckt != packet ) {
        delete []ppckt;
    }
    
    if( write != cnt ) {
        ERRMSG("write to channel error: %d\n", write);
        return -1;
    } 
    
    DBGMSG("%s --; write cnt = %d\n", __FUNCTION__, write);
    return 0;
}

int diagStop( void )
{
    DBGMSG(" %s enter\n", __FUNCTION__);
    
    sReceiving = 0;
        
    pthread_mutex_lock(&sMutxEvent);
    pthread_cond_signal(&sCondEvent);
    pthread_mutex_unlock(&sMutxEvent);
    
    pthread_kill(sThreadID, SIGALRM);
    usleep(100 * 1000);
    while( 0 == pthread_kill(sThreadID, 0) ) {
        
        pthread_kill(sThreadID, SIGALRM);
        usleep(600 * 1000);
    }

    DBGMSG("hdr = %p, tail = %p\n", sCmdLstHdr, sCmdLstTal);
    
    struct cmd_info_t * cur = sCmdLstHdr;
    struct cmd_info_t * nxt;
    while( NULL != cur ) {
        nxt = cur->next;
        
        diagFreeCmdInfo(cur);
        cur = nxt;
    }
    sCmdLstHdr = NULL;
    sCmdLstTal = NULL;
    
    chlClose(sChlHandle);
    sChlHandle = -1;
    
    DBGMSG(" %s exit\n", __FUNCTION__);
    return 0;
}

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void * diagReceiverThread( void *param )
{
    DBGMSG("---- diagReceiverThread enter ----\n");
    #define RCVSIZE (4 * 1024)
    
    uchar * rcvBuf = new uchar[RCVSIZE];
    if( NULL == rcvBuf ) {
        return NULL;
    }
    
    signal(SIGALRM, diagSigRoutine);
    
    // !! maybe have some problem, need improvement !!    
    struct cmd_info_t * pinfo;
    while( sReceiving ) {
        int rcvLen = chlRead(sChlHandle, rcvBuf, RCVSIZE, 1000);
        if( rcvLen <= 0 ) {
            WRNMSG("chrRead error: %s(%d)\n", strerror(errno), errno);
            if( sReceiving ) {
                chlClose(sChlHandle);
                sChlHandle = chlOpen(DIAG_PORT_PATH);
            }
            continue;
        }
        
        if( DIAG_FLAG != rcvBuf[0] || DIAG_FLAG != rcvBuf[rcvLen - 1] ) {
            DBGMSG("invalid packet: hdr = 0x%x, tail = 0x%x, len = %d\n",
                rcvBuf[0], rcvBuf[rcvLen - 1], rcvLen);
            continue;
        }
        
        pinfo = NULL;
        if( diagParseData(rcvBuf, rcvLen, &pinfo) < 0 ) {
            DBGMSG("parse data error!\n");
            continue;
        }
        
        diagPushCmd(pinfo);
    }
    delete []rcvBuf;
    
    DBGMSG("---- diagReceiverThread exit ----\n");
    return NULL;
}

struct cmd_info_t * diagMallocCmdInfo( uint sn, uchar cmd, uchar * data, int len )
{
    //DBGMSG("cmd = %d, len = %d\n", cmd, len);
    int totalsize = sizeof(cmd_info_t) + sizeof(diag_cmd_t) + len;
    struct cmd_info_t * info = (struct cmd_info_t *)new char[ totalsize ];
    if( NULL == info ) {
        ERRMSG("new %d bytes fail!\n", totalsize);
        return NULL;
    }
    
    struct diag_cmd_t * dcmd = (struct diag_cmd_t *)(info + 1);
    dcmd->sn  = sn;
    dcmd->cmd = cmd;
    dcmd->buf = (uchar *)(dcmd + 1);
    dcmd->len = len;
    if( data != NULL && len > 0 ) {
        memcpy(dcmd->buf, data, len);
    }
    
    info->dcmd = dcmd;
    info->next = NULL;
    
    //--DBGMSG("new: buf = %p, size = %d\n", info, totalsize);
    return info;
}

void diagFreeCmdInfo( struct cmd_info_t * cmdinfo )
{
    //DBGMSG(" %s enter: info = %p\n", __FUNCTION__, cmdinfo);
    //--DBGMSG("delete: buf = %p\n", cmdinfo);
    delete [] (char *)cmdinfo;
    //DBGMSG(" %s exit\n", __FUNCTION__);
}

int diagParseData( uchar * data, int data_len, cmd_info_t ** pinfo )
{
    AT_ASSERT( DIAG_FLAG == data[0] && DIAG_FLAG == data[data_len - 1] );
    
    static uchar dbg_out_data = 0;
    
    #define MAX_BUF_SIZE 1024
    static uchar packet[MAX_BUF_SIZE];
    
    uchar * ppckt = packet;
    if( data_len > MAX_BUF_SIZE ) {
        ppckt = new uchar[data_len];
        if( NULL == ppckt ) {
            ERRMSG("new %d bytes fail!", data_len);
            return -1;
        }
    }
    
    int flagLen = 2;
    int dataPos = 0;
    for( int i = 1; i < data_len - 1; ++i ) {
        if( DIAG_ESCAPE == data[i] ) {
            flagLen++;
            i++;
            ppckt[dataPos++] = data[i] ^ DIAG_COMPLEMENT;
        } else {
            ppckt[dataPos++] = data[i];
        }
    }
#ifdef DIAG_PKT_FORMT_IS_BIG_ENDIAN
    uint   sn      = (ppckt[0] << 24) | (ppckt[1] << 16) | (ppckt[2] << 8) | ppckt[3];
    ushort pcktLen = (ushort)((ppckt[4] << 8) | ppckt[5]);
#else    
    uint   sn      = (ppckt[0] << 0) | (ppckt[1] << 8) | (ppckt[2] << 16) | (ppckt[3] << 24);
    ushort pcktLen = (ushort)((ppckt[4] << 0) | (ppckt[5] << 8));
#endif // DIAG_PKT_FORMT_IS_BIG_ENDIAN
    
    //DBGMSG("packet sn: %x,%x,%x,%x sn = 0x%08x\n", ppckt[0], ppckt[1], ppckt[2], ppckt[3], sn);
    if( pcktLen != data_len - flagLen ) {
        ERRMSG("pkt_len = %d, data_len = %d, flag_len = %d\n", pcktLen, data_len, flagLen);
        if( ppckt != packet ) {
            delete []ppckt;
        }
        cmd_info_t * info = diagMallocCmdInfo(0x0FFFFFFF, 0x01, NULL, 0);
        if( NULL != info ) {
            diagSendResult(info->dcmd);
            diagFreeCmdInfo(info);
        }
        return -2;
    }
    
    DBGMSG("SN = %08X, PktLen = %02X, Cmd = %02X, SubCmd = %02X\n", sn, pcktLen, ppckt[6], ppckt[7]);
    
    if( 0xFE == ppckt[6] && 0xEF == ppckt[7] ) {
        dbg_out_data = 1;
	if( ppckt != packet ) {
		delete []ppckt;
	}
        return -1;
    }
    
    if( dbg_out_data && pcktLen > 8 ) {
        char dbuf[260];
        strcpy(dbuf, "Data = ");
        char * pb = dbuf + strlen(dbuf);
        int len = pcktLen;
        if( len > 8 + 64 ) {
            len = 8 + 64;
        }
        for( int d = 8; d < len; ++d ) {
            snprintf(pb, 4, " %02X", ppckt[d]);
            pb += 3;
        }
        pb += 3;
        *pb++ = '\n';
        *pb++ = 0;
        DBGMSG("%s", dbuf);
    }
    
    *pinfo = diagMallocCmdInfo(sn, ppckt[7], ppckt + 8, pcktLen - 8);
    
    if( ppckt != packet ) {
        delete []ppckt;
    }
    return ((NULL == *pinfo) ? -3 : 0);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
