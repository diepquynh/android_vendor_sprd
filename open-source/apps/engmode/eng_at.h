#ifndef __ENG_AT_H__
#define __ENG_AT_H__

#include "eng_pcclient.h"

typedef enum {
    ENG_AT_REQUEST_MODEM_VERSION                = 0,   //get version
    ENG_AT_REQUEST_IMEI                         = 1,    //get imei
    ENG_AT_SELECT_BAND                          = 2,   //band select
    ENG_AT_CURRENT_BAND                         = 3,   //current band
    ENG_AT_SETARMLOG                            = 4,   //start/stop armlog
    ENG_AT_GETARMLOG                            = 5,   //get armlog            //5
    ENG_AT_SETAUTOANSWER                        = 6,   //set call auto answer
    ENG_AT_GETAUTOANSWER                        = 7,   //get call auto answer status
    ENG_AT_SETSPPSRATE                          = 8,   //set download/upload rate
    ENG_AT_GETSPPSRATE                          = 9,  //get current rate
    ENG_AT_SETSPTEST                            = 10,    //set sp test           //10
    ENG_AT_GETSPTEST                            = 11, //get sp test
    ENG_AT_SPID                                 = 12,  //get UE Identity
    ENG_AT_SETSPFRQ                             = 13,  //lock frequnece
    ENG_AT_GETSPFRQ                             = 14, //get frequence
    ENG_AT_SPAUTE                               = 15, //audio loopback test       //15
    ENG_AT_SETSPDGCNUM                          = 16,//set dummy gsm cell
    ENG_AT_GETSPDGCNUM                          = 17,//get dummy gsm cell
    ENG_AT_SETSPDGCINFO                         = 18,//set dunmmy gsm info
    ENG_AT_GETSPDGCINFO                         = 19,//set dunmmy gsm info
    ENG_AT_GRRTDNCELL                           = 20,//set dnummy td ncell info //20
    ENG_AT_SPL1ITRRSWITCH                       = 21,//start/stop L1ITa
    ENG_AT_GETSPL1ITRRSWITCH                    = 22,//get ENG_AT_SPL1ITRRSWITCH status
    ENG_AT_PCCOTDCELL                           = 23,//tdd target cell
    ENG_AT_SDATACONF                            = 24,//data config
    ENG_AT_L1PARAM                              = 25,//set l1param value     //25
    ENG_AT_TRRPARAM                             = 26,//TRR BCFE param
    ENG_AT_SETTDMEASSWTH                        = 27,//set RR TD switch
    ENG_AT_GETTDMEASSWTH                        = 28,//get RR TD switch
    ENG_AT_RRDMPARAM                            = 29,//RRDM param
    ENG_AT_DMPARAMRESET                         = 30,//reset param           //30
    ENG_AT_SMTIMER                              = 31,//set timer
    ENG_AT_TRRDCFEPARAM                         = 32,//TRR DCFE
    ENG_AT_CIMI                                 = 33,//get imsi
    ENG_AT_MBCELLID                             = 34,//CELL ID
    ENG_AT_MBAU                                 = 35,//                 //35
    ENG_AT_EUICC                                = 36,//get usim/sim
    ENG_AT_CGREG                                = 37,//get lai
    ENG_AT_EXIT                                 = 38,//set eng exit
    ENG_AT_NOHANDLE_CMD                         = 39,//no need handle
    ENG_AT_SYSINFO                              = 40,//get system info
    ENG_AT_HVER                                 = 41,//get hardware version
    ENG_AT_GETSYSCONFIG                         = 42,
    ENG_AT_SETSYSCONFIG                         = 43,
    ENG_AT_SPVER                                = 44,
    ENG_AT_AUTOATTACH                           = 45,
    ENG_AT_SETAUTOATTACH                        = 46,
    ENG_AT_PDPACTIVE                            = 47,
    ENG_AT_GETPDPACT                            = 48,
    ENG_AT_SGPRSDATA                            = 49,
    ENG_AT_GETUPLMN                             = 50,
    ENG_AT_CGSMS                                = 51,
    ENG_AT_CAOC                                 = 52, //aoc active,and {Deactive see@ENG_AT_CAOCD} {Query see@ENG_AT_CAOCQ}
        ENG_AT_CAMM                                 = 53,
        ENG_AT_SETCOPS                              = 54,
        ENG_AT_SADC                                 = 55,
        ENG_AT_CFUN                                 = 56,
        ENG_AT_CGMR                                 = 57,
        ENG_AT_SETCAPLOG                            = 58,
        ENG_AT_SETUPLMN                             = 59,
        ENG_AT_GETUPLMNLEN                          = 60,
        ENG_AT_GETDSPLOG                            = 61,   //
        ENG_AT_SETDSPLOG                            = 62,  //
        ENG_AT_SGMR                                 = 70,
        ENG_AT_CAOCD                                = 80,//aoc deactive
        ENG_AT_CAOCQ                                = 81,//aoc query
        ENG_AT_CCED                                 =101,//net info of sim
        ENG_AT_L1MON                                =103,

        ENG_AT_GET_ASSERT_MODE          = 108,
        ENG_AT_SET_ASSERT_MODE          = 109,
        ENG_AT_SET_MANUAL_ASSERT        = 110,
        ENG_AT_SFPL                     = 111,
        ENG_AT_SEPL                     = 112,

        ENG_AT_SPENGMD_QUERY            = 117,
        ENG_AT_SPENGMD_OPEN             = 118,
        ENG_AT_SPENGMD_CLOSE            = 119,

        ENG_AT_SSMP                     = 200,
        ENG_AT_CMD_END,
}ENG_AT_CMD;

int eng_at_pcmodem(eng_dev_info_t* dev_info);

#endif /*__ENG_AT_H__*/
