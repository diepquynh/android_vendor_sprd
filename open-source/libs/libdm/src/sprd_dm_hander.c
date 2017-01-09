/****************************************************************************
** File Name:      MMIDM_PARSEXML.C                                               *
** Author:                                                                 *
** Date:           28/04/2011                                              *
** Copyright:      2011 Spreadtrum, Incorporated. All Rights Reserved.       *
** Description:    This file is used to describe the MMI DM XML PARSE             *
*****************************************************************************
**                         Important Edit History                          *
** ------------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                             *
** 04/2011       maryxiao       ����syncxmlЭ��dm����xml      *
****************************************************************************/
#define MMIPARSEXML_C


#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include "sprd_dm.h"
#include "sprd_dm_adapter.h"
#include "sprd_dm_parsexml.h"
#include "sprd_dm_hander.h"


//define dm init tree date table
LOCAL const DM_TREE_ITEM_T  s_dm_initdata_tab[] =
{
    {"./DevInfo/Mod",                       MODEL_IO_HANDLER },//m
    {"./DevInfo/Man",                       MAN_IO_HANDLER},//m
    {"./DevInfo/OEM",                       OEM_IO_HANDLER},//o
    {"./DevInfo/Lang",                      LANG_IO_HANDLER},//o
    {"./DevInfo/DmV",                       DMVERSION_IO_HANDLER},//o
    {"./DevInfo/DevId",                     DEVID_IO_HANDLER},//m
    {"./DevDetail/FwV",                     FWVERSION_IO_HANDLER},//o
    {"./DevDetail/SwV",                     SWVERSION_IO_HANDLER},//m
    {"./DevDetail/HwV",                     HWVERSION_IO_HANDLER},//o
    //DM
    {"./Settings/DM/ConnProfile",           DM_CONN_PROFILE_IO_HANDLER}, //m
    {"./Settings/DM/APN",                   DM_CONN_PROFILE_IO_HANDLER}, //m
    {"./Settings/DM/Proxy",                 DM_PROXY_IO_HANDLER}, //m
    {"./Settings/DM/Port",                  DM_PORT_IO_HANDLER}, //m
    //GPRS_CMNet������Ϣ
    {"./Settings/GPRS/CMNet/APN",           GPRS_CMNET_APN_IO_HANDLER}, //m
    {"./Settings/GPRS/CMNet/UserName",      GPRS_CMNET_USERNAME_IO_HANDLER}, //m
    {"./Settings/GPRS/CMNet/PassWord",      GPRS_CMNET_PASSWORD_IO_HANDLER}, //m
    {"./Settings/GPRS/CMNet/ProxyAddr",     GPRS_CMNET_PROXY_IO_HANDLER}, //m
    {"./Settings/GPRS/CMNet/ProxyPortNbr",  GPRS_CMNET_PORT_IO_HANDLER}, //m
    //GPRS_CMWap������Ϣ
    {"./Settings/GPRS/CMWap/APN",           GPRS_CMWAP_APN_IO_HANDLER}, //m
    {"./Settings/GPRS/CMWap/UserName",      GPRS_CMWAP_USERNAME_IO_HANDLER}, //m
    {"./Settings/GPRS/CMWap/PassWord",      GPRS_CMWAP_PASSWORD_IO_HANDLER}, //m
    {"./Settings/GPRS/CMWap/ProxyAddr",     GPRS_CMWAP_PROXY_IO_HANDLER}, //m
    {"./Settings/GPRS/CMWap/ProxyPortNbr",  GPRS_CMWAP_PORT_IO_HANDLER}, //m
    //MMS������Ϣ
    {"./Settings/MMS/MMSC",                 MMS_MMSC_IO_HANDLER}, //m
    {"./Settings/MMS/ConnProfile",          MMS_CONNPROFILE_IO_HANDLER}, //m
    //WAP2.0
    {"./Settings/WAP/ConnProfile",          WAP_CONNPROFILE_IO_HANDLER  }, //m
    {"./Settings/WAP/MonternetPage",            WAP_HOMEPAGE_IO_HANDLER }, //m
    {"./Settings/WAP/StartPage",            BROWSER_HOMEPAGE_IO_HANDLER }, //m
    //PIM������Ϣ
    {"./Settings/PIM/ConnProfile",	PIM_CONNPROFILE_URI_IO_HANDLER},
    {"./Settings/PIM/Addr",                 PIM_SERVER_ADDR_IO_HANDLER}, //m
    {"./Settings/PIM/AddressBookURI",       PIM_ADDRESS_BOOK_URI_IO_HANDLER}, //m
    {"./Settings/PIM/CalendarURI",          PIM_CALENDAR_URI_IO_HANDLER}, //m
    //streaming
    {"./Settings/Streaming/Name",           STREAMING_NAME_IO_HANDLER}, //m
    //{"./Settings/Streaming/Proxy",          DM_GetStreamingProxy,       DM_SetStreamingProxy,       PNULL}, //m
   // {"./Settings/Streaming/ProxyPort",      DM_GetStreamingProxyPort,   DM_SetStreamingProxyPort,   PNULL}, //m
    {"./Settings/Streaming/ConnProfile",    STREAMING_CONNPROFILE_IO_HANDLER}, //m
    {"./Settings/Streaming/NetInfo",        STREAMING_NET_INFO_IO_HANDLER}, //m
    {"./Settings/Streaming/MinUdpPort",     STREAMING_MIN_UDP_PORT_IO_HANDLER}, //m
    {"./Settings/Streaming/MaxUdpPort",     STREAMING_MAX_UDP_PORT_IO_HANDLER}, //m

    //CMWAPDM
    {"./Settings/CMWAPDM/ConnProfile",           DM_WAP_CONN_PROFILE_IO_HANDLER}, //m
    {"./Settings/CMWAPDM/APN",                   DM_WAP_APN_IO_HANDLER}, //m
    {"./Settings/CMWAPDM/Proxy",                 DM_WAP_PROXY_IO_HANDLER}, //m
    {"./Settings/CMWAPDM/Port",                  DM_WAP_PORT_IO_HANDLER}, //m

    {"./Settings/Cred/Nonce",                  DM_SAVE_NONCE}, //m
	//bug 292626 begin for AGPS
	{ "./Settings/AGPS/ConnProfile", AGPS_CONNPROFILE_IO_HANDLER }, //0
	{ "./Settings/AGPS/AGPSServer", AGPS_SERVER_IO_HANDLER }, //0
	{ "./Settings/AGPS/AGPSName", AGPS_SERVER_NAME_IO_HANDLER }, //0
	{ "./Settings/AGPS/IAPID", AGPS_IAPID_IO_HANDLER }, //0
	{ "./Settings/AGPS/AGPSServerPort", AGPS_PORT_IO_HANDLER }, //0
	{ "./Settings/AGPS/ProviderIP", AGPS_PROVIDER_ID_IO_HANDLER }, //0
	{ "./Settings/AGPS/PrefConRef", AGPS_PREFCONREF_ID_IO_HANDLER }, //O
	{ "./Settings/AGPS/ConRef", AGPS_CONREF_IO_HANDLER }, //O
	//bug 292626 end
    	
  
    //{"./DMAcc/TDAcc/AppAddr/SrvAddr/Addr",  DM_GetSrvAddrURL,           PNULL,                      PNULL}, //o

    //{"./LAWMO/State",                       DM_GetDeviceLockStatus,     PNULL,                      PNULL},//m

};


PUBLIC int32 DM_GET_TREE_READFUNC(char* path)
{
    uint32		 i = 0;
    for (i=0;i<ARR_SIZE(s_dm_initdata_tab);i++)
    {
        if(!strcmp((char*)s_dm_initdata_tab[i].inPath,path))
        return s_dm_initdata_tab[i].handletype;
    }
    return -1;
}

PUBLIC int32 DM_GET_TREE_WRITEFUNC(char* path)
{
    uint32		 i = 0;
    for (i=0;i<ARR_SIZE(s_dm_initdata_tab);i++)
    {
        if(!strcmp((char*)s_dm_initdata_tab[i].inPath,path))
        return s_dm_initdata_tab[i].handletype;
    }
    return -1;
}


