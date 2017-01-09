// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <fcntl.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "type.h"
#include "sim.h"
#include <cutils/properties.h>
/**************************************/
//add the new sim check plan to check the
//sim and the
#include "atci.h"
/*************************************/
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_sim {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#define MODEM_DEV_BASE_NUM  13
#define MODEM_DEV_PREFIX_T    "/dev/CHNPTYT"
#define MODEM_DEV_PREFIX_W    "/dev/CHNPTYW"
#define MODEM_DEV_PREFIX_TL    "/dev/CHNPTYTL"
#define MODEM_DEV_PREFIX_LF    "/dev/CHNPTYLF"
#define MODEM_DEV_PREFIX_L     "/dev/CHNPTYL"

#define SIM_CHECK_AT        "AT+EUICC?\r\n";
#define PROPERTY_VALUE_MAX                  92
const char * s_modem = NULL;
static int sim_num;
char SP_SIM_NUM[20]; // ro.modem.*.count

//------------------------------------------------------------------------------
int simOpen( void )
{
	return 0;
}

//------------------------------------------------------------------------------
int simCheck( int index )
{   
    int fd, ret = 0;
    char pty[32];
    char resp[64];
    const char * at = SIM_CHECK_AT;

    snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_W, MODEM_DEV_BASE_NUM + index);
    fd = open(pty, O_RDWR);
    if( fd < 0 ) {
        ERRMSG("open w %s fail: %s\n", pty, strerror(errno));
        ret = fd;

        //if "CHNPTYW" not find, try  "CHNPTYT"
        memset(pty,0,sizeof(pty));
        snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_T, MODEM_DEV_BASE_NUM + index);
        fd = open(pty, O_RDWR);

		if( fd < 0 ) {
			ERRMSG("open t %s fail: %s\n", pty, strerror(errno));
			ret = fd;

			//if "CHNPTYT" not find, try  "CHNPTYTL"
			memset(pty,0,sizeof(pty));
			snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_TL, MODEM_DEV_BASE_NUM + index);
			fd = open(pty, O_RDWR);
			if( fd < 0 ) {
				ERRMSG("open tl %s fail: %s\n", pty, strerror(errno));
				ret = fd;

				//if "CHNPTYTL" not find, try  "CHNPTYLF"
				memset(pty,0,sizeof(pty));
				snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_LF, MODEM_DEV_BASE_NUM + index);
				fd = open(pty, O_RDWR);
				if( fd < 0 ) {
					ERRMSG("open lf %s fail: %s\n", pty, strerror(errno));
					ret = fd;
					//if "CHNPTYTLF" not find, try  "CHNPTYL"
					memset(pty,0,sizeof(pty));
					snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_L, MODEM_DEV_BASE_NUM + index);
					fd = open(pty, O_RDWR);
					if(fd < 0){
						ERRMSG("open l %s fail: %s\n", pty, strerror(errno));
						ret = fd;
					}
				}
			}
		}
	}

    while( fd >= 0 ) {
        
        if( write(fd, at, strlen(at)) < 0 ) {
            ERRMSG("write fail: %s\n", strerror(errno));
			close(fd);
            ret = -1;
            break;
        }
        resp[0] = 0;
        resp[sizeof(resp) - 1] = 0;
        
        struct pollfd pfd;
        int timeout = 1000; // ms
        
        pfd.fd     = fd;
        pfd.events = POLLIN;
        errno = 0;
        ret   = poll(&pfd, 1, timeout);
        if (ret < 0) {
            ERRMSG("poll() error: %s\n", strerror(errno));
            break;
        } else if( 0 == ret ) {
            ret = -1;
            WRNMSG("poll() timeout: %d ms\n", timeout);
            break;
        }
        
        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            ERRMSG("poll() returned  success (%d), "
                 "but with an unexpected revents bitmask: %#x\n", ret, pfd.revents);
            ret = -2;
            break;
        }
        
        if( read(fd, resp, sizeof(resp) - 1) < 0 ) {
            ERRMSG("read fail: %s\n", strerror(errno));
            ret = -3;
        } else {
            // +EUICC: 2,1,1
            char * pval = strstr(resp, "EUICC");
            DBGMSG("read OK: %s\n", resp);
            if( NULL != pval ) {
                pval += 5;
                while( *pval && (*pval < '0' || *pval > '9') ) {
                    pval++;
                }
                DBGMSG("%s\n", pval);
                ret = ('0' == *pval || '1' == *pval) ? 0 : -4;
            } else {
                ret = -5;
            }
        }

        close(fd);
        break;
    } 
            
    DBGMSG("simCheck: ret = %d\n", ret);
    return ret;
}

//------------------------------------------------------------------------------
int simClose( void )
{
	return 0;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//add the new func to check sim card
//interface func sendCmd() from atic.h  if check sim success return ok or fail reutrn other
//new AT CMD AT+CIMI
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int sim_newCheck( int slot)
{
    int ret = -1;
    const char *at_cmd_send = "AT+CIMI";
    char at_cmd_recv[63] = {0};
    //memset(at_cmd_send, 0, sizeof(at_cmd_send));
    memset(at_cmd_recv, 0, sizeof(at_cmd_recv));
    //sprintf(at_cmd_send, "%s","AT+CIMI");
    if(slot>1){
	DBGMSG("the sim card num too big than the fact num\n");
	return -1;
    }
    ret = sendATCmd(slot, at_cmd_send, at_cmd_recv, sizeof(at_cmd_recv));
    if(0 != ret){
	DBGMSG("error:senCmd fail !!!!\n");
	return -1;
    }
    if (at_cmd_recv[0] != '\0') {
	if (strstr(at_cmd_recv,"OK") != NULL) {
	    ret = 0;
	}else{
	    DBGMSG("simCheck at_cmd_recv return  fail !!\n");
	    ret = -1;
	}
    }else{
	DBGMSG("simCheck at_cmd_recv copy fail !!\n");
	ret = -1;
    }
    DBGMSG("simCheck: ret = %d\n", ret);
    //just check the phone support the single sim card or dual sim card
    check_sim_num();
    return ret;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//add the new func to check sim card num
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int check_sim_num(void)
{
    int ret = -1;
    //int sim_num = -1;
    char phone_count[PROPERTY_VALUE_MAX] = {0};
    if(s_modem == NULL){
	s_modem = (char *)malloc(PROPERTY_VALUE_MAX);
	property_get("ro.radio.modemtype", (char*) s_modem, "");
	if(strcmp(s_modem, "") == 0){
	    DBGMSG("get modem type failed, exit!");
	    free((char *)s_modem);
	    exit(-1);
	}
    }
    snprintf(SP_SIM_NUM, sizeof(SP_SIM_NUM), "ro.modem.%s.count", s_modem);
    property_get(SP_SIM_NUM, phone_count, "");
    if(strcmp(phone_count, "1")){
	sim_num = 2;
    }else{
	sim_num = 1;
    }
    DBGMSG("the sim  number = %d\n", sim_num);
    return ret;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
