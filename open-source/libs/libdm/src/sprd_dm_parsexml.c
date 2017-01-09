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
** 04/2011       maryxiao       解析syncxml协议dm部分xml      *
****************************************************************************/
#define MMIPARSEXML_C


#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include "sprd_dm.h"
#include "sprd_dm_adapter.h"
#include "sprd_dm_parsexml.h"
#include "dm_jni.h"
#include "sprd_dm_hander.h"



/*****************************************************************************/
//  alert link pointer
/*****************************************************************************/
LOCAL  DMXML_TAG_ALERT_T* s_alerTag_head = PNULL;
LOCAL  DMXML_TAG_ALERT_T* s_alerTag_tail = PNULL;


/*****************************************************************************/
//  status link pointer
/*****************************************************************************/
LOCAL DMXML_TAG_STATUS_T* s_statusTag_head = PNULL;
LOCAL DMXML_TAG_STATUS_T* s_statusTag_tail = PNULL;
LOCAL DMXML_TAG_STATUS_T* s_statusTag_cur = PNULL;

/*****************************************************************************/
//  result link pointer
/*****************************************************************************/
LOCAL DMXML_TAG_RESULT_T* s_resultTag_head = PNULL;
LOCAL DMXML_TAG_RESULT_T* s_resultTag_tail = PNULL;

/*****************************************************************************/
//  replace link pointer
/*****************************************************************************/
LOCAL DMXML_TAG_REPLACE_T* s_replaceTag_head = PNULL;
LOCAL DMXML_TAG_REPLACE_T* s_replaceTag_tail = PNULL;


/*****************************************************************************/
//  Description :通过id获得tag的name str
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL char* MMIDM_GetTagStr(MMI_DM_TAGID_E id);
/*****************************************************************************/
//  Description :set a uri which to send
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_setResUri(char* ResUri_content);
/*****************************************************************************/
//  Description :get a uri which to send
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
 char* MMIDM_getResUri(void);
/*****************************************************************************/
//  Description : when a session ended clear the data
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_clearDmXmlData(void);
/*****************************************************************************/
//  Description :  根据tag产生tag的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_CreateTag(DMXML_TAG_T* tag_info, char* buf, uint16 buf_size, BOOL if_free);
/*****************************************************************************/
//  Description :  NONCE的加密
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_B64_Md5(char* outbuf, int buf_size, int len);
/*****************************************************************************/
//  Description :  产生鉴权字符串
//  Global resource dependence : b64(md5(b64(md5(name:pwd):nonce)))
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_GetCreddata(char* outbuf, uint16 buf_size);
/*****************************************************************************/
//  Description :  初始话和产生xml的header部分xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN InitDMXMLHeader(char* headerbuf, uint16 buf_size);
/*****************************************************************************/
//  Description : 根据alert节点内容产生alert部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildAlert(char* alert_buf, uint16 buf_size);
/*****************************************************************************/
//  Description : 根据source节点内容产生source部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildISource(DMXML_TAG_SOURCE_T source_info, char* source_buf);
/*****************************************************************************/
//  Description : 根据meta节点内容产生meta部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildIMeta(DMXML_TAG_META_T *meta_info, char* meta_buf, uint16 buf_size);
/*****************************************************************************/
//  Description : 根据item节点内容产生item部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildItem(DMXML_TAG_ITEM_T *item_info, char* item_buf, uint16 buf_size);
/*****************************************************************************/
//  Description : 根据replace节点内容产生replace部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildReplace(char* replace_buf, uint16 buf_size);
/*****************************************************************************/
//  Description : 根据CHAL节点内容产生CHAL部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildIChal(DMXML_TAG_CHAL_T *chal_info, char* chal_buf);
/*****************************************************************************/
//  Description : 根据status节点内容产生status部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildStatus(char* status_buf, uint16 buf_size);
/*****************************************************************************/
//  Description : 根据result节点内容产生result部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildResult(char* result_buf, uint16 buf_size);
/*****************************************************************************/
//  Description : generate the body of the xml msg
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_CodecXmlBody(char* bodybuf, uint16 buf_size);
/*****************************************************************************/
//  Description : GET THE CONTENT OF THE TAG
//  Global resource dependence :
//  Author:MARY.XIAO
//  xmlbuf: CONNTENT NEED TO PARSE
//  tagid: THE FIRST TAG WHICH CONTENT WANT TO GET
//  content: OUT: THE TAG CONTENT
//  return:  return the left content
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL char* MMIDM_getNextXmlTagBuf(char* xmlbuf, MMI_DM_TAGID_E tagid, char* content, uint16 buf_size);
/*****************************************************************************/
//  Description : GET THE CONTENT OF THE TAG
//  Global resource dependence :
//  Author:MARY.XIAO
//  xmlbuf: CONNTENT NEED TO PARSE
//  tagid: THE FIRST TAG WHICH CONTENT WANT TO GET
//  content: OUT: THE TAG CONTENT
//  return:  return the left content
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_generateXMLData(char* sendbuf);
/*****************************************************************************/
//  Description : deal with the Exec command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithExecData(char* execbuf);
/*****************************************************************************/
//  Description : parse the the Exec command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的Exec命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLExec(char* xmlbuf);
/*****************************************************************************/
//  Description : deal with the Status command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理Status命令，加入Status链表中
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithStatusData(char* statusbuf);
/*****************************************************************************/
//  Description : parse the the Status command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的Status命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLStatus(char* xmlbuf);
/*****************************************************************************/
//  Description : deal with the get command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理get命令，加入get链表中
/*****************************************************************************/
LOCAL   BOOLEAN MMIDM_DealWithGetData(char* getbuf);
/*****************************************************************************/
//  Description : parse the the get command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的get命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLGet(char* xmlbuf);
/*****************************************************************************/
//  Description : deal with the replace command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理replace命令，加入replace链表中
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithReplaceData(char* replacebuf);
/*****************************************************************************/
//  Description : parse the the replace command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的replace命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLReplace(char* xmlbuf);
/*****************************************************************************/
//  Description : deal with the alert command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理get命令，
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithAlertData(char* alertbuf);
/*****************************************************************************/
//  Description : parse the the alert command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的alert命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLAlert(char* xmlbuf);
/*****************************************************************************/
//  Description : release all the node malloc
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_releaseItemContent(DMXML_TAG_ITEM_T* item_tag);
/*****************************************************************************/
//  Description : release all the node malloc
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_ReleaseXMLData(void);
/*****************************************************************************/
//  Description : parse the receive xml content
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析接受到的xml数据
/*****************************************************************************/
LOCAL void MMIDM_ParseXMLData(char* xmlbuf);
LOCAL void MMIDM_GetDmParaInfo(MMIDM_DEBUG_TYPE_E type,char* string,uint32 len);


void MMIDM_IU32toa(uint32 value, char *string, uint32 radix);
int32 DM_Base64_decode(char *src, uint32 srcl, char *dest,uint32 destl);


static uint32 s_g_SessionID = 0;
static uint32 s_g_MsgID = 1;
static uint32 s_g_CmdID = 1;
static MMI_DM_STEP_E s_g_step = STEP_GETNONCE;
static BOOLEAN   s_g_needreplay = FALSE;
static BOOLEAN   s_g_callClearFunc = FALSE;
static BOOLEAN   s_g_callLockFunc = FALSE;
static BOOLEAN   s_g_callUnlockFunc = FALSE;


static char   s_g_resUri[MAX_RESURI_LEN] ={0};
static char   s_g_tresUri[MAX_RESURI_LEN] ={0};

static char   s_g_nonce[MAX_NONCE_LEN] ={0};


static char  *s_g_memptr = NULL;
static char  *s_tag_memptr = NULL;
static char  *s_xml_memptr = NULL;
static char  *s_oth_memptr = NULL;

static unsigned int     s_g_mem_size = 0;
static unsigned int     s_g_mem_used = 0;

#define MAX_TAG_COUNT 	50
#define MAX_XML_COUNT   20

static char max_tag_count[MAX_TAG_COUNT];
static int first_tag = 0;
static char max_xml_count[MAX_XML_COUNT];	
static char first_xml = 0;

LOCAL const DMXML_TAG_INFO_T  s_dmxml_tag_tab[] =
{
    {"VerDTD",                       TAG_VERDTD},
    {"VerProto",                     TAG_VERPROTO},
    {"SessionID",                    TAG_SESSIONID},
    {"MsgID",                        TAG_MSGID},
    {"Target",                       TAG_TARGETID},
    {"Source",                       TAG_SOURCEID},
    {"LocURI",                       TAG_LOCURIID},
    {"LocName",                      TAG_LOCNAMEID},
    {"Cred",                         TAG_CREDID},
    {"Meta",                         TAG_METAID},
    {"MaxMsgSize",                   TAG_MAXMSGID},
    {"MaxObjSize",                   TAG_MAXOBJID},
    {"Data",                         TAG_DATAID},
    {"SyncHdr",                      TAG_SYNCHDRID},
    {"SyncBody",                     TAG_SYNCBODYID},
    {"Alert",                        TAG_ALERTID},
    {"Replace",                      TAG_REPLACEID},
    {"Status",                       TAG_STATUSID},
    {"Results",                      TAG_RESULTID},
    {"Cmd",                          TAG_CMDID},
    {"CmdID",                        TAG_CMDIDID},
    {"MsgRef",                       TAG_MSGREFID},
    {"CmdRef",                       TAG_CMDREFID},
    {"TargetRef",                    TAG_TARGETREFID},
    {"SourceRef",                    TAG_SOURCEREFID},
    {"Chal",                         TAG_CHALID},
    {"Item",                         TAG_ITEMID},
    {"Format",                       TAG_FORMATID},
    {"Type",                         TAG_TYPEID},
    {"NextNonce",                    TAG_NEXTNONCEID},
    {"RespURI",                      TAG_RESURIID},
    {"Get",                          TAG_GETID},
    {"SyncML",                       TAG_SYNCMLID},
    {"Exec",                         TAG_EXECID},
};

LOCAL void *SCI_ALLOCA(int _SIZE) 
{
	int i;
	unsigned int found = 0xff;
	if (s_g_memptr!=NULL)
	{
		if (_SIZE == MAX_TAG_BUF_SIZE && first_tag != 0xff)
		{
			if (max_tag_count[first_tag] == 0 )
				{
					found = first_tag;
					max_tag_count[found] = 1;
					for (i=first_tag; i<MAX_TAG_COUNT;i++)
					{
						if (max_tag_count[i]== 0) break;
					}
					first_tag = (i<MAX_TAG_COUNT)?i:0xff;
					
					SCI_TRACE_LOW("DM SCI_ALLOCA s_tag_memptr %d next:%d",found, first_tag);
					memset(s_tag_memptr+MAX_TAG_BUF_SIZE*found,0,MAX_TAG_BUF_SIZE);
					
					return (s_tag_memptr+MAX_TAG_BUF_SIZE*found);
				}
			else
				SCI_TRACE_LOW("DM SCI_ALLOCA s_tag_memptr FAILED");
				return NULL;
		}else if (_SIZE == MAX_XML_BUF_SIZE && first_xml != 0xff)
		{
			if (max_xml_count[first_xml] == 0 )
				{
					found = first_xml;
					max_xml_count[found] = 1;
					for (i=first_xml; i<MAX_XML_COUNT;i++)
					{
						if (max_xml_count[i]== 0) break;
					}
					first_xml = (i<MAX_XML_COUNT)?i:0xff;
					SCI_TRACE_LOW("DM SCI_ALLOCA s_xml_memptr %d",found);
					memset(s_xml_memptr+MAX_XML_BUF_SIZE*found,0,MAX_XML_BUF_SIZE);
					
					return (s_xml_memptr+MAX_XML_BUF_SIZE*found);
				}
			else
				SCI_TRACE_LOW("DM SCI_ALLOCA s_xml_memptr FAILED");
				return NULL;		
		} 
		else if (s_oth_memptr+_SIZE+1+s_g_mem_used < s_g_memptr+s_g_mem_size)
		{
			memset(s_oth_memptr+s_g_mem_used, 0, _SIZE+1);
			s_g_mem_used +=_SIZE+1; 
			
			SCI_TRACE_LOW("DM SCI_ALLOCA s_other_memptr size:%d used:%d total:%d",
				_SIZE, s_g_mem_used,s_g_memptr+s_g_mem_size-s_oth_memptr);
			return s_oth_memptr+s_g_mem_used-_SIZE-1;
		} 
	}
	return malloc(_SIZE);
}

LOCAL void SCI_FREEAA(void *ptr)
{
	if ((char *)ptr>=s_tag_memptr && (char *)ptr < (s_xml_memptr))
	{
	//local pool
		int i ;
		i = ((char *)ptr -s_tag_memptr)/MAX_TAG_BUF_SIZE;
		if ((char *)ptr != s_tag_memptr+i*MAX_TAG_BUF_SIZE)
			{
			SCI_TRACE_LOW("SCI_FREEAA s_tag_memptr FREE Fail %d", ((char *)ptr -s_tag_memptr));
			}
		else
			{
				if (first_tag> i) first_tag = i;
				max_tag_count[i]=0; 
				SCI_TRACE_LOW("SCI_FREEAA s_tag_memptr free %d next:%d", i, first_tag);
			}
	}else if ((char *)ptr>=s_xml_memptr && (char *)ptr < (s_oth_memptr))
	{
		int i ;
		i = ((char *)ptr -s_xml_memptr)/MAX_XML_BUF_SIZE;
		if ((char *)ptr != s_xml_memptr+i*MAX_XML_BUF_SIZE)
			{
			SCI_TRACE_LOW("SCI_FREEAA s_xml_memptr FREE Fail %d", ((char *)ptr -s_xml_memptr));
			}
		else
			{
				if (first_xml> i) first_xml = i;
				max_xml_count[i]=0; 
			SCI_TRACE_LOW("SCI_FREEAA s_xml_memptr free %d next:%d", i, first_xml);
			}	//local pool
	}else if ((char *)ptr>=s_oth_memptr && (char *)ptr < (s_g_mem_size+s_tag_memptr))
	{
	//local pool  do nothing
	}
	else
	free(ptr);
}

/*****************************************************************************/
//  Description :设置session的id
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
 void MMIDM_setSessionId(uint32 id)
{
   
   SCI_TRACE_LOW("MMIDM_setSessionId id %d", id);
   s_g_SessionID = id;
#if LOCAL_MEMPOOL
     if (s_g_memptr != NULL)
     	{
	  	 free(s_g_memptr);
	  	 s_g_memptr = NULL;
     	}
	s_g_mem_size = 0;
	s_g_mem_used = 0;  
	s_g_memptr = malloc(MAX_LOCAL_POOL_LEN);
	memset(s_g_memptr,0,MAX_LOCAL_POOL_LEN);
	if (s_g_memptr !=NULL)
	{
		s_g_mem_size = MAX_LOCAL_POOL_LEN;
		s_tag_memptr = s_g_memptr;
		s_xml_memptr = s_g_memptr + MAX_TAG_COUNT*MAX_TAG_BUF_SIZE;	
		s_oth_memptr = s_xml_memptr + MAX_XML_COUNT*MAX_XML_BUF_SIZE;
		s_g_mem_used = 0;  
	}
	{
	int i;
	for (i=0; i<MAX_TAG_COUNT;i++)
		{
		max_tag_count[i]=0;
		}
	for (i=0; i<MAX_XML_COUNT;i++)
		{
		max_xml_count[i]=0;
		}	

	first_tag = 0;
	first_xml = 0;
	}
   SCI_TRACE_LOW("MMIDM_setSessionId s_g_memptr SIZE %d", s_g_mem_size);
#endif   
}
void MMIDM_setMemPool(char *ptr, unsigned int len)
{
	s_g_memptr = ptr;
	s_g_mem_size = len;
	s_g_mem_used = 0;  
}
/*****************************************************************************/
//  Description :设置session的step
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
 void MMIDM_setSessionStep(MMI_DM_STEP_E step)
{
   s_g_step = step;
}

/*****************************************************************************/
//  Description :通过id获得tag的name str
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
/*LOCAL void MMIDM_GetTagStr(MMI_DM_TAGID_E id, char* str,uint8 size)
{
    if(str == PNULL)
    {
        SCI_TRACE_LOW("MMIDM_GetTagStr str == PNULL");
        MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        return;
    }
    if(strlen(s_dmxml_tag_tab[id].tagtype)<size)
    {
        SCI_STRCPY(str, s_dmxml_tag_tab[id].tagtype);
    }
    else
    {
        SCI_TRACE_LOW("MMIDM_GetTagStr the buf is too small %d", size);
    }
}*/

LOCAL char* MMIDM_GetTagStr(MMI_DM_TAGID_E id)
{
   // SCI_TRACE_LOW("MMIDM_GetTagStr %s", s_dmxml_tag_tab[id].tagtype);
    return (char*) s_dmxml_tag_tab[id].tagtype;/*lint !e605*/
}
/*****************************************************************************/
//  Description :set a uri which to send
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_setResUri(char* ResUri_content)
{
    SCI_TRACE_LOW("MMIDM_setResUri %s", ResUri_content);
    if(MAX_RESURI_LEN > strlen(ResUri_content))
    {
        SCI_STRCPY(s_g_resUri, ResUri_content);
    }
    else
    {
        SCI_TRACE_LOW("MMIDM_setResUri the buf is too small %d", strlen(ResUri_content));
    }
}
/*****************************************************************************/
//  Description :get a uri which to send
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
PUBLIC char* MMIDM_getResUri(void)
{

    if(!strlen(s_g_resUri))
    {
       MMIDM_GetDmParaInfo(DM_SRV_ADDR, s_g_resUri,MAX_RESURI_LEN);
    }
/* remove by Hong@20120302
    if(s_g_step != STEP_CREDED)
    {
        MMIDM_GetDmParaInfo(DM_SRV_ADDR, s_g_tresUri,MAX_RESURI_LEN);
      SCI_TRACE_LOW("MMIDM_getResUri tmp: %s", s_g_tresUri);
	  return s_g_tresUri;
    }
*/
    SCI_TRACE_LOW("MMIDM_getResUri %s", s_g_resUri);
    return s_g_resUri;
}
/*****************************************************************************/
//  Description : when a session ended clear the data
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_clearDmXmlData(void)
{
    MMIDM_setResUri("");
    s_g_MsgID = 1;
    s_g_CmdID = 1;
    s_g_step = STEP_GETNONCE;
}
//extern  char*  hs_calc_md5_cred(char* creddata);
//extern  void hs_calc_b64_cred(char* creddata, unsigned long cbLength);
/*****************************************************************************/
//  Description :  根据tag产生tag的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_CreateTag(DMXML_TAG_T* tag_info, char* buf, uint16 buf_size, BOOL if_free)
{
   // char    str[50]={0};
    uint16  size = 0;
    BOOLEAN ret = TRUE;
    do {
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_CreateTag PNULL == buf");
        // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
           spdm_stopDm(SPRD_DM_PARSE_ERROR);
           ret = FALSE;
           break;
        }

        SCI_MEMSET(buf, 0, buf_size);

      //  MMIDM_GetTagStr(tag_info->tagId, str, 50);
       // SCI_TRACE_LOW("MMIDM_CreateTag str= %s, tagContent %s tagArr %s", str, tag_info->tagContent, tag_info->tagArr);

        if(tag_info->hasChildTag)
        {
            //2*strlen(MMIDM_GetTagStr(tag_info->tagId)): tag type len; tag contenct len;strlen(tag_info->tagArr) tag arr len ;other: "<".....
            if((2*strlen(MMIDM_GetTagStr(tag_info->tagId))+strlen(tag_info->tagContent)+strlen(tag_info->tagArr)+6) >= buf_size)
            {
                SCI_TRACE_LOW("MMIDM_CreateTag bufsize is too small %d", buf_size);
              // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
              spdm_stopDm(SPRD_DM_PARSE_ERROR);
               ret = FALSE;
               break;
            }
        }
        else
        {
            if((2*strlen(MMIDM_GetTagStr(tag_info->tagId))+strlen(tag_info->tagContent)+strlen(tag_info->tagArr)+18) >= buf_size)
            {
                SCI_TRACE_LOW("MMIDM_CreateTag bufsize is too small %d", buf_size);
              // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
              spdm_stopDm(SPRD_DM_PARSE_ERROR);
               ret = FALSE;
               break;
            }
        }

        SCI_STRCPY(buf,(char*)"<");

        strcat(buf, MMIDM_GetTagStr(tag_info->tagId));
        if(strlen(tag_info->tagArr))
        {
            strcat(buf, " ");
            strcat(buf, tag_info->tagArr);
        }
        strcat(buf, ">");

    //    if(tag_info->hasChildTag)//
    //    {
    //        size = strlen(buf);
    //        buf[size]=0x0a;
    //        buf[size+1]=0;
    //    }
        if(tag_info->hasChildTag)
        {
            strcat(buf, tag_info->tagContent);
        }
        else
        {
           // strcat(buf, "<![CDATA[");
            strcat(buf, tag_info->tagContent);
           // strcat(buf, "]]>");
        }
        strcat(buf, "<");
        strcat(buf, "/");
        strcat(buf, MMIDM_GetTagStr(tag_info->tagId));
        strcat(buf, ">");
        //size = strlen(buf);
        //buf[size]=0x0a;
        //buf[size+1]=0;
        size = strlen(buf);
        buf[size]='\0';//Add NUL terminator

        if(PNULL!=tag_info->tagContent && if_free)
        {
            SCI_FREEAA(tag_info->tagContent);
            tag_info->tagContent = PNULL;
        }
    }while(0);


    if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
   // SCI_TRACE_LOW("MMIDM_CreateTag buf %s", buf);
}

/*****************************************************************************/
//  Description :  NONCE的加密
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_B64_Md5(char* outbuf, int buf_size, int len)
{
    char  buf[128]={0};
//    char  temp[4]={0};
 //   int i=0;

    if(PNULL == outbuf)
    {
        SCI_TRACE_LOW("MMIDM_GetCreddata PNULL == outbuf");
        return;
    }
   // SCI_STRCPY(buf, outbuf);
   SCI_MEMCPY(buf, outbuf, len);

    SCI_MEMSET(outbuf, 0, buf_size);

   // hs_calc_md5_cred(buf);
    mmidm_calc_md5_cred(buf);
  /*  for(i=0;i<16;i++)
    {
        sprintf(temp, "%02x", buf[i]);
        strcat(outbuf, temp);
    }*/

    mmidm_calc_b64_cred(buf,16);
    SCI_STRCPY(outbuf,buf);
}


/*****************************************************************************/
//  Description :  产生鉴权字符串
//  Global resource dependence : b64(md5(b64(md5(name:pwd):nonce)))
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_GetCreddata(char* outbuf, uint16 buf_size)
{
    char  nonce_buf[MAX_NONCE_LEN]={0};
    char  buf[128]={0};
//    char  temp[4]={0};
    long  len=0;
    long  len1=MAX_NONCE_LEN;
    long  len2=MAX_NONCE_LEN;
//    int i=0;

    if(PNULL == outbuf)
    {
        SCI_TRACE_LOW("MMIDM_GetCreddata PNULL == outbuf");
        return;
    }
    SCI_MEMSET(outbuf, 0, buf_size);

    sprintf(buf, "%s%s%s", "mvpdm",":","mvpdm");
    MMIDM_B64_Md5(buf, 128, strlen(buf));
    SCI_TRACE_LOW("MMIDM_GetCreddata after MMIDM_B64_Md51 %s",buf);
   // SCI_STRCPY(s_g_nonce,"1P4QL3UJEUjgFO/mi3eQPQ==");
    len = strlen(s_g_nonce);
    if(!len)
    {
         SCI_STRCPY(s_g_nonce,(char*)"JE48dmgiOyogPzxXPkdUJQ==");
         len = strlen(s_g_nonce);
    }
    len1 = DM_Base64_decode(s_g_nonce, len,nonce_buf,len1); /*lint !e718 !e746 */
	if (len1 == -1)  //2013-2-20@hong
		{
		  SCI_TRACE_LOW("MMIDM_GetCreddata base64 decode error! ");
		return;
		}   //2013-2-20@hong
    sprintf(outbuf, "%s%s", buf,":");
    SCI_MEMCPY(outbuf+strlen(outbuf), nonce_buf,len1);
    SCI_TRACE_LOW("MMIDM_GetCreddata  %s",outbuf);
    len2=len+1+len1;
    MMIDM_B64_Md5(outbuf, buf_size, len2);
    SCI_TRACE_LOW("MMIDM_GetCreddata after MMIDM_B64_Md52 %s",outbuf);

}
/*****************************************************************************/
//  Description :  产生鉴权字符串
//  Global resource dependence : b64(md5(name:pwd:nonce))
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
/*void MMIDM_GetCreddata(unsigned char* outbuf, uint16 buf_size)
{
    unsigned char  nonce_buf[MAX_NONCE_LEN]={0};
    unsigned char  buf[128]={0};
    unsigned char  temp[4]={0};
    unsigned long  len=0;
    unsigned long  len1=MAX_NONCE_LEN;
    int i=0;

    if(PNULL == outbuf)
    {
        SCI_TRACE_LOW("MMIDM_GetCreddata PNULL == outbuf");
        MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        return;
    }
    SCI_MEMSET(outbuf, 0, buf_size);
    len = strlen(s_g_nonce);
    SCI_TRACE_LOW("MMIDM_GetCreddata s_g_nonce  %s",s_g_nonce);
    if(len)
    {
        Base64_decode(s_g_nonce, len,nonce_buf, &len1);
        SCI_TRACE_LOW("MMIDM_GetCreddata nonece %s",nonce_buf);
       //hs_base64Decode(nonce_buf, MAX_NONCE_LEN,s_g_nonce, &len);
       sprintf(buf, "%s%s%s%s%s", "OMADM",":","mvpdm",":",nonce_buf);
    }
    else
    {
        sprintf(buf, "%s%s%s", "OMADM",":","mvpdm");
    }
    SCI_TRACE_LOW("MMIDM_GetCreddata after  %s",buf);
    hs_calc_md5_cred(buf);
    for(i=0;i<16;i++)
    {
        sprintf(temp, "%02x", buf[i]);
        strcat(outbuf, temp);
    }
    SCI_TRACE_LOW("MMIDM_GetCreddata after md5 %s",outbuf);
    hs_calc_b64_cred(outbuf,strlen(outbuf));
     SCI_TRACE_LOW("MMIDM_GetCreddata after b64 %s",outbuf);

} */

 void MMIDM_GetDmParaInfo(MMIDM_DEBUG_TYPE_E type,char* string,uint32 len)
{

    switch(type)
    {
    case DM_APN_SET:
       // strcpy(string,s_dm_info.apn_info);
        break;
    case DM_MON_SET:
        // strcpy(string,s_server_type_tab[s_dm_info.server_type].mode);
         spdm_readCb(MODEL_IO_HANDLER, string, 0, len);
        break;
    case DM_DEV_VERSION_SET:
        //strcpy(string,s_server_type_tab[s_dm_info.server_type].version);
          spdm_readCb(DMVERSION_IO_HANDLER, string, 0, len);
        break;
    case DM_MAN_SET:
       // strcpy(string,s_server_type_tab[s_dm_info.server_type].man);
        spdm_readCb(MAN_IO_HANDLER, string, 0, len);
        break;
    case DM_IMEI_SET:
        spdm_readCb(DEVID_IO_HANDLER, string, 0, len);
        break;

    case DM_SRV_ADDR:
        //  strcpy(string,s_dm_info.srv_addr);
        spdm_readCb(SERVER_ADDR_IO_HANDLER, string, 0, len);
        break;
    case DM_SELF_REG_NUM:
       // MMINV_READ(MMINV_DM_SRNUM, s_dm_info.sr_number, return_value);
        //strcpy(string,s_dm_info.sr_number);
        break;
    case DM_SELF_REGPORT_NUM:
        // MMINV_READ(MMINV_DM_SRPORT, s_dm_info.sr_port, return_value);
        //strcpy(string,s_dm_info.sr_port);
        break;
    default:
        break;
    }
}

/*****************************************************************************/
//  Description :  初始话和产生xml的header部分xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN InitDMXMLHeader(char* headerbuf, uint16 buf_size)
{
    char* buf=PNULL;
    char* buf2=PNULL;
    char str[50] ={0};
    char creddata[128];
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN   ret = TRUE;

    SCI_TRACE_LOW("ENTER InitDMXMLHeader");

 do {

        if(PNULL == headerbuf)
        {
           SCI_TRACE_LOW("InitDMXMLHeader PNULL == headerbuf");
           //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
           spdm_stopDm(SPRD_DM_PARSE_ERROR);
           ret =FALSE;
           break;
        }
        SCI_MEMSET(headerbuf, 0, buf_size);

        buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("InitDMXMLHeader PNULL == buf");
           ret =FALSE;
           break;
        }
        SCI_MEMSET(buf, 0, MAX_TAG_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("InitDMXMLHeader PNULL == buf2");
           ret =FALSE;
           break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);

        ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
        if(PNULL== ptr)
        {
            SCI_TRACE_LOW("InitDMXMLHeader PNULL== ptr");
               ret =FALSE;
               break;
        }
        SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));

        ptr->tagContent = SCI_ALLOCA(MAX_XML_BUF_SIZE);
        if(PNULL== ptr->tagContent)
        {
            SCI_TRACE_LOW("InitDMXMLHeader PNULL== ptr->tagContent");
           ret =FALSE;
           break;
        }
        SCI_MEMSET(ptr->tagContent, 0, MAX_XML_BUF_SIZE);

        ptr->tagId = TAG_VERDTD;
        ptr->hasChildTag =FALSE;
        SCI_STRCPY(ptr->tagContent, (char*)"1.2");
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        SCI_STRCPY(headerbuf, buf);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_VERPROTO;
        ptr->hasChildTag =FALSE;
        SCI_STRCPY(ptr->tagContent, (char*)"DM/1.2");
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_SESSIONID;
        ptr->hasChildTag =FALSE;
        MMIDM_IU32toa(s_g_SessionID, str, 32);
        SCI_STRCPY(ptr->tagContent, str);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);


       SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_MSGID;//target
        ptr->hasChildTag =FALSE;
        MMIDM_IU32toa(s_g_MsgID, str, 32);
        SCI_STRCPY(ptr->tagContent, str);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_LOCURIID;
        ptr->hasChildTag =FALSE;
        SCI_STRCPY(ptr->tagContent, MMIDM_getResUri());/*lint !e666*/
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_TARGETID;
        ptr->hasChildTag =TRUE;
        SCI_STRCPY(ptr->tagContent, buf);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);

        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_LOCURIID;//source
        ptr->hasChildTag =FALSE;
        MMIDM_GetDmParaInfo(DM_IMEI_SET, str,50);
        SCI_STRCPY(ptr->tagContent, (char*)"IMEI:");
        strcat(ptr->tagContent, str);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);

        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_LOCNAMEID;
        ptr->hasChildTag =FALSE;
        SCI_STRCPY(ptr->tagContent, (char*)"mvpdm");
        MMIDM_CreateTag(ptr, buf2, MAX_TAG_BUF_SIZE, FALSE);
        strcat(buf, buf2);

        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_SOURCEID;
        ptr->hasChildTag =TRUE;
        SCI_STRCPY(ptr->tagContent, buf);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);

        if(s_g_step != STEP_CREDED)
        {
            SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
            ptr->tagId = TAG_FORMATID;//
            ptr->hasChildTag =FALSE;
            SCI_STRCPY(ptr->tagArr, (char*)"xmlns=\'syncml:metinf\'");
            SCI_STRCPY(ptr->tagContent, (char*)"b64");
            MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);

            SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
            ptr->tagId = TAG_TYPEID;//
            ptr->hasChildTag =FALSE;
            SCI_STRCPY(ptr->tagArr, (char*)"xmlns=\'syncml:metinf\'");
            SCI_STRCPY(ptr->tagContent, (char*)"syncml:auth-md5");
            MMIDM_CreateTag(ptr, buf2, MAX_TAG_BUF_SIZE, FALSE);
            strcat(buf, buf2);


            SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
            ptr->tagId = TAG_METAID;
            ptr->hasChildTag =TRUE;
            SCI_STRCPY(ptr->tagContent, buf);
            MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);


            SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
            ptr->tagId = TAG_DATAID;
            ptr->hasChildTag =FALSE;
            //sprintf(creddata, "%s%s", "OMADM","mvpdm");
          /*  if(strlen(s_g_nonce))
            {*/
                MMIDM_GetCreddata(creddata, 128);
                SCI_TRACE_LOW("MMIDM creddata %s", creddata);
          /*  }
            else
            {
                SCI_STRCPY(ptr->tagContent, "F7C4SIWTyiwtLZ/YfXMGbg==");
            }*/
            SCI_STRCPY(ptr->tagContent, creddata);
            MMIDM_CreateTag(ptr, buf2, MAX_TAG_BUF_SIZE, FALSE);
            strcat(buf, buf2);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_CREDID;
        ptr->hasChildTag =TRUE;
        SCI_STRCPY(ptr->tagContent, buf);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);
    }


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_MAXMSGID;
        ptr->hasChildTag =FALSE;
        SCI_STRCPY(ptr->tagArr, (char*)"xmlns=\'syncml:metinf\'");
        SCI_STRCPY(ptr->tagContent, (char*)"9000");
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_MAXOBJID;
        ptr->hasChildTag =FALSE;
        SCI_STRCPY(ptr->tagArr, (char*)"xmlns=\'syncml:metinf\'");
        SCI_STRCPY(ptr->tagContent,(char*)"524288");
        MMIDM_CreateTag(ptr, buf2, MAX_TAG_BUF_SIZE, FALSE);
        strcat(buf, buf2);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_METAID;
        ptr->hasChildTag =TRUE;
        SCI_STRCPY(ptr->tagContent, buf);
        MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, FALSE);
        strcat(headerbuf, buf);


        SCI_MEMSET(ptr->tagArr, 0, MAX_TAG_ARR_LEN);
        ptr->tagId = TAG_SYNCHDRID;
        ptr->hasChildTag =TRUE;
        SCI_STRCPY(ptr->tagContent, headerbuf);
        MMIDM_CreateTag(ptr, headerbuf, MAX_XML_BUF_SIZE, FALSE);
  } while(0);
    if(PNULL!=ptr)
    {
        if(PNULL!=ptr->tagContent)
        {
            SCI_FREEAA( ptr->tagContent);
             ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(PNULL!=buf)
    {
        SCI_FREEAA(buf);
        buf =PNULL;
    }
    if(PNULL!=buf2)
    {
        SCI_FREEAA(buf2);
        buf2 =PNULL;

    }

  SCI_TRACE_LOW("InitDMXMLHeader ret %d",ret);

    if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}


/*****************************************************************************/
//  Description : 根据alert节点内容产生alert部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildAlert(char* alert_buf, uint16 buf_size)
{

    char* buf=PNULL;
    char* buf2=PNULL;
    DMXML_TAG_ALERT_T* cur_tag = PNULL;
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_BuildAlert");
    do{
        if(PNULL == alert_buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildAlert PNULL == alert_buf");
            ret = FALSE;
            break;
        }

        buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildItem PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_TAG_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildItem PNULL == buf2");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);

        ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
        if(PNULL== ptr)
        {
            SCI_TRACE_LOW("MMIDM_BuildAlert PNULL== ptr");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));

        for(cur_tag=s_alerTag_head;cur_tag!=PNULL;cur_tag = cur_tag->next)
        {

            if(cur_tag->CmdId.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->CmdId), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if((strlen(buf)+strlen(buf2))<MAX_TAG_BUF_SIZE)
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildAlert buf size is too small buf2 %s", buf2);
                    ret = FALSE;
                    break;
                }
            }

            if(cur_tag->DATA.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->DATA), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if((strlen(buf)+strlen(buf2))<MAX_TAG_BUF_SIZE)
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildAlertbuf size is too small  buf2 %s", buf2);
                    ret = FALSE;
                    break;
                }
            }

            if(strlen(buf))
            {
                SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
                ptr->tagId = TAG_ALERTID;
                ptr->hasChildTag = TRUE;
                ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
                if(PNULL==  ptr->tagContent)
                {
                    SCI_TRACE_LOW("MMIDM_BuildAlert PNULL==  ptr->tagContent");
                    ret = FALSE;
                    break;
                }
                SCI_MEMSET(ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/

                SCI_STRCPY(ptr->tagContent, buf);
                MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, TRUE);
                if(buf_size > strlen(buf))
                {
                    strcat(alert_buf, buf);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildAlert alert_buf is too small buf_size %d", buf_size);
                    ret = FALSE;
                    break;
                }
            }

        }
    }while(0);

    while(PNULL!=s_alerTag_head)
    {
        cur_tag = s_alerTag_head->next;
        SCI_FREEAA(s_alerTag_head);
        s_alerTag_head= cur_tag;
    }
    if(PNULL!=ptr)
    {
        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(PNULL!=buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL!=buf2)
    {
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    SCI_TRACE_LOW("LEAVE MMIDM_BuildAlert");

    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}
/*****************************************************************************/
//  Description : 根据source节点内容产生source部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildISource(DMXML_TAG_SOURCE_T source_info, char* source_buf)
{
    char* buf=PNULL;
    char* buf2=PNULL;
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN    ret = TRUE;
    SCI_TRACE_LOW("ENTER MMIDM_BuildISource");
    do {

        buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildISource PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_TAG_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildISource PNULL == buf2");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);

        if(source_info.locuri.tagContent)
        {
            MMIDM_CreateTag(&(source_info.locuri), buf2, MAX_TAG_BUF_SIZE, TRUE);
            strcat(buf, buf2);
        }
        if(source_info.locname.tagContent)
        {
            MMIDM_CreateTag(&(source_info.locname), buf2, MAX_TAG_BUF_SIZE, TRUE);
            strcat(buf, buf2);
        }

        if(strlen(buf))
        {
            ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
            if(PNULL== ptr)
            {
                SCI_TRACE_LOW("MMIDM_BuildIsource PNULL== ptr");
                ret = FALSE;
                break;
            }

            SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
            ptr->tagId = TAG_SOURCEID;
            ptr->hasChildTag = TRUE;
            ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
            if(PNULL==  ptr->tagContent)
            {
                SCI_TRACE_LOW("MMIDM_BuildISource PNULL==  ptr->tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET( ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/

            SCI_STRCPY(ptr->tagContent, buf);
            MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, TRUE);

            SCI_STRCPY(source_buf, buf);

        }

    } while(0);
    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != buf2)
    {
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}
/*****************************************************************************/
//  Description : 根据meta节点内容产生meta部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildIMeta(DMXML_TAG_META_T *meta_info, char* meta_buf, uint16 buf_size)
{
    char* buf=PNULL;
    char* buf2=PNULL;
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN ret   = TRUE;
    do{
        SCI_TRACE_LOW("ENTER MMIDM_BuildIMeta");
        if(PNULL == meta_buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildIMeta PNULL == meta_buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(meta_buf, 0, buf_size);

        buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildIMeta PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_TAG_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildIMeta PNULL == buf2");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);

        if(meta_info->format.tagContent)
        {
            MMIDM_CreateTag(&(meta_info->format), buf2, MAX_TAG_BUF_SIZE, TRUE);
            strcat(buf, buf2);
        }
        if(meta_info->type.tagContent)
        {
            MMIDM_CreateTag(&(meta_info->type), buf2, MAX_TAG_BUF_SIZE, TRUE);
            strcat(buf, buf2);
        }
        if(meta_info->nextnonce.tagContent)
        {
            MMIDM_CreateTag(&(meta_info->nextnonce), buf2, MAX_TAG_BUF_SIZE, TRUE);
            strcat(buf, buf2);
        }
        if(strlen(buf))
        {
            ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
            if(PNULL== ptr)
            {
                SCI_TRACE_LOW("MMIDM_BuildIMeta PNULL== ptr");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
            ptr->tagId = TAG_METAID;
            ptr->hasChildTag = TRUE;
            ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
            if(PNULL==  ptr->tagContent)
            {
                SCI_TRACE_LOW("MMIDM_BuildIMeta PNULL==  ptr->tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/
            SCI_STRCPY(ptr->tagContent, buf);
            MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, TRUE);
            SCI_STRCPY(meta_buf, buf);
        }
    }while(0);

    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != buf2)
    {
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}
/*****************************************************************************/
//  Description : 根据item节点内容产生item部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildItem(DMXML_TAG_ITEM_T *item_info, char* item_buf, uint16 buf_size)
{
    char* buf=PNULL;
    char* buf2 =PNULL;
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN  ret = TRUE;

    do {

        SCI_TRACE_LOW("ENTER MMIDM_BuildItem");
        if(PNULL == item_buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildItem PNULL == item_buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(item_buf, 0, buf_size);

        buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildItem PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_TAG_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildItem PNULL == buf2");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);


        MMIDM_BuildISource(item_info->source, buf);
        MMIDM_BuildIMeta(&(item_info->meta), buf2, MAX_TAG_BUF_SIZE);
        if(MAX_TAG_BUF_SIZE> strlen(buf)+strlen(buf2))
        {
            strcat(buf, buf2);
        }
        else
        {
            SCI_TRACE_LOW("MMIDM_BuildItem buf size is too small");

        }

        if(item_info->data.tagContent)
        {
            MMIDM_CreateTag(&(item_info->data), buf2, MAX_TAG_BUF_SIZE, TRUE);
            if(MAX_TAG_BUF_SIZE> strlen(buf)+strlen(buf2))
            {
                strcat(buf, buf2);
            }
            else
            {
                SCI_TRACE_LOW("MMIDM_BuildItem buf size is too small");

            }
        }

        if(strlen(buf))
        {
            ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
            if(PNULL== ptr)
            {
                SCI_TRACE_LOW("MMIDM_BuildItem PNULL== ptr");
                ret = FALSE;
                break;
            }

            SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
            ptr->tagId = TAG_ITEMID;
            ptr->hasChildTag = TRUE;
            ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
            if(PNULL==  ptr->tagContent)
            {
                SCI_TRACE_LOW("MMIDM_BuildItem PNULL==  ptr->tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/
            SCI_STRCPY(ptr->tagContent, buf);
            MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, TRUE);
            SCI_STRCPY(item_buf, buf);

        }
    } while(0);
    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != buf2)
    {
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
	spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}
/*****************************************************************************/
//  Description : 根据replace节点内容产生replace部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildReplace(char* replace_buf, uint16 buf_size)
{

    char *buf = PNULL;
    char* buf2 = PNULL;
    //unsigned char str[200] ={0};
    DMXML_TAG_REPLACE_T* cur_tag = PNULL;
    DMXML_TAG_ITEM_T*       item_tag = PNULL;
    DMXML_TAG_T * ptr = PNULL;
    char * replace_ptr = PNULL;
    BOOLEAN ret = TRUE;

    do {

        SCI_TRACE_LOW("ENTER MMIDM_BuildReplace");
        if(PNULL == replace_buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildReplace PNULL == replace_buf");
            ret =FALSE;
            break;
        }
        replace_ptr = replace_buf;
        SCI_MEMSET(replace_ptr, 0, buf_size);

        buf = SCI_ALLOCA(MAX_XML_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildReplace PNULL == buf");
            ret =FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildReplace PNULL == buf2");
            ret =FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);
        SCI_TRACE_LOW("MMIDM_BuildReplace after malloc buf");

        for(cur_tag=s_replaceTag_head;cur_tag!=PNULL;cur_tag = cur_tag->next)
        {
             SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);
            if(cur_tag->CmdId.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->CmdId), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildReplace buf is too small");
                }
            }
            for(item_tag = cur_tag->item_ptr; item_tag!= PNULL; item_tag=item_tag->next)
            {
                MMIDM_BuildItem(item_tag, buf2, MAX_TAG_BUF_SIZE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildReplace buf is too small");
                }
            }
            if(strlen(buf))
            {
                ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
                if(PNULL== ptr)
                {
                    SCI_TRACE_LOW("MMIDM_BuildReplace PNULL== ptr");
                    ret =FALSE;
                    break;
                }

                SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
                ptr->tagId = TAG_REPLACEID;
                ptr->hasChildTag = TRUE;
                ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
                if(PNULL==  ptr->tagContent)
                {
                    SCI_TRACE_LOW("MMIDM_BuildReplace PNULL==  ptr->tagContent");
                    ret =FALSE;
                    break;
                }
                SCI_MEMSET(ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/
                SCI_STRCPY(ptr->tagContent, buf);
                MMIDM_CreateTag(ptr, buf, MAX_XML_BUF_SIZE, TRUE);
                strcat(replace_buf, buf);
		    if(PNULL != ptr)  //2013-2-20@hong
		    {
		        if(PNULL != ptr->tagContent)
		        {
		            SCI_FREEAA(ptr->tagContent);
		            ptr->tagContent = PNULL;
		        }

		        SCI_FREEAA(ptr);
		        ptr = PNULL;
		    }  //2013-2-20@hong
            }
        }


    } while(0);



    while(PNULL!=s_replaceTag_head)
    {
        while(PNULL!=s_replaceTag_head->item_ptr)
        {
            item_tag = s_replaceTag_head->item_ptr->next;
            SCI_FREEAA(s_replaceTag_head->item_ptr);
            s_replaceTag_head->item_ptr= item_tag;
        }
        cur_tag = s_replaceTag_head->next;
        SCI_FREEAA(s_replaceTag_head);
        s_replaceTag_head= cur_tag;
    }

    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != buf2)
    {
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}

/*****************************************************************************/
//  Description : 根据CHAL节点内容产生CHAL部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/

LOCAL BOOLEAN MMIDM_BuildIChal(DMXML_TAG_CHAL_T *chal_info, char* chal_buf)
{
    char* buf=PNULL;
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN ret = TRUE;
    SCI_TRACE_LOW("ENTER MMIDM_BuildISource");

    do {

        buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildISource PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_TAG_BUF_SIZE);


        MMIDM_BuildIMeta(&(chal_info->meta), buf, MAX_TAG_BUF_SIZE);

        if(strlen(buf))
        {
            ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
            if(PNULL== ptr)
            {
                SCI_TRACE_LOW("MMIDM_BuildIChal PNULL== ptr");
                ret = FALSE;
                break;
            }

            SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
            ptr->tagId = TAG_CHALID;
            ptr->hasChildTag = TRUE;
            ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
            if(PNULL==  ptr->tagContent)
            {
                SCI_TRACE_LOW("MMIDM_BuildIChal PNULL==  ptr->tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET( ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/

            SCI_STRCPY(ptr->tagContent, buf);
            MMIDM_CreateTag(ptr, buf, MAX_TAG_BUF_SIZE, TRUE);

            SCI_STRCPY(chal_buf, buf);

        }

    } while(0);
    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}
/*****************************************************************************/
//  Description : 根据status节点内容产生status部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildStatus(char* status_buf, uint16 buf_size)
{
    char* buf = PNULL;
    char* buf2= PNULL;
    //unsigned char str[200] ={0};
    DMXML_TAG_STATUS_T* cur_tag = PNULL;
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN  ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_BuildStatus");

    do {

        if(PNULL == status_buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildStatus PNULL == status_buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(status_buf, 0, buf_size);

        buf = SCI_ALLOCA(MAX_XML_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildStatus PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildReplace PNULL == buf2");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);

        for(cur_tag=s_statusTag_head;cur_tag!=PNULL;cur_tag = cur_tag->next)
        {
            SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);
            if(cur_tag->CmdId.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->CmdId), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
		  SCI_TRACE_LOW("MMIDM_BuildStatus buf2 ++++++ %s",buf2);
            }

            if(cur_tag->msgRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->msgRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }

		  SCI_TRACE_LOW("MMIDM_BuildStatus msgRef.tagContent   %s",buf2);
            }

            if(cur_tag->cmdRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->cmdRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }

		 SCI_TRACE_LOW("MMIDM_BuildStatus cmdRef.tagContent   %s",buf2);
            }
            if(cur_tag->cmd.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->cmd), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
		  SCI_TRACE_LOW("MMIDM_BuildStatus cmd.tagContent   %s",buf2);
            }
#if 0 //hong2012 remove these tag for test
            if(cur_tag->targetRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->targetRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
		SCI_TRACE_LOW("MMIDM_BuildStatus targetRef.tagContent  buf2 %s",buf2);
            }

		if(cur_tag->sourceRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_BuildStatus %s", cur_tag->sourceRef.tagContent);
                MMIDM_CreateTag(&(cur_tag->sourceRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
		  SCI_TRACE_LOW("MMIDM_BuildStatus sourceRef.tagContent  buf2 %s",buf2);
            }
#endif

	     SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);
            MMIDM_BuildIChal(&(cur_tag->chal), buf2);
	     SCI_TRACE_LOW("MMIDM_BuildStatus MMIDM_BuildIChal  %s",buf2);
            strcat(buf, buf2);
            if(cur_tag->data.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->data), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }

		  SCI_TRACE_LOW("MMIDM_BuildStatus data.tagContent   %s",buf2);
            }
            if(strlen(buf))
            {
                ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
                if(PNULL== ptr)
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus PNULL== ptr");
                    ret = FALSE;
                    break;
                }

                SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
                ptr->tagId = TAG_STATUSID;
                ptr->hasChildTag = TRUE;
                ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
                if(PNULL==  ptr->tagContent)
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus PNULL==  ptr->tagContent");
                    ret = FALSE;
                    break;
                }
                SCI_MEMSET(ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/

                SCI_STRCPY(ptr->tagContent, buf);
                MMIDM_CreateTag(ptr, buf, MAX_XML_BUF_SIZE, TRUE);
                strcat(status_buf, buf);
		    if(PNULL != ptr)  //2013-2-20@hong
		    {
		        if(PNULL != ptr->tagContent)
		        {
		            SCI_FREEAA(ptr->tagContent);
		            ptr->tagContent = PNULL;
		        }

		        SCI_FREEAA(ptr);
		        ptr = PNULL;
		    }		//2013-2-20@hong
		   SCI_TRACE_LOW("MMIDM_BuildStatus status_buf  %s",status_buf);
            }
        }

    } while(0);


    while(PNULL!=s_statusTag_head)
    {
        cur_tag = s_statusTag_head->next;
        SCI_FREEAA(s_statusTag_head);
        s_statusTag_head= cur_tag;
    }
    if(PNULL != buf)
    {
         SCI_TRACE_LOW("MMIDM_BuildStatus buf %s",buf);
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != buf2)
    {
           SCI_TRACE_LOW("MMIDM_BuildStatus buf2 %s",buf2);
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }

     SCI_TRACE_LOW("MMIDM_BuildStatus status_buf %s",status_buf);
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}
/*****************************************************************************/
//  Description : 根据result节点内容产生result部分的xml
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_BuildResult(char* result_buf, uint16 buf_size)
{
    char* buf = PNULL;
    char* buf2=PNULL;
   // unsigned char str[200] ={0};
    DMXML_TAG_RESULT_T* cur_tag = PNULL;
    DMXML_TAG_T * ptr = PNULL;
    DMXML_TAG_ITEM_T*       item_tag = PNULL;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_BuildResult");
    do {

        if(PNULL == result_buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildResult PNULL == result_buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(result_buf, 0, buf_size);

        buf = SCI_ALLOCA(MAX_XML_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildResult  PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);

        buf2 = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL == buf2)
        {
            SCI_TRACE_LOW("MMIDM_BuildResult PNULL == buf2");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf2, 0, MAX_TAG_BUF_SIZE);

        for(cur_tag=s_resultTag_head;cur_tag!=PNULL;cur_tag = cur_tag->next)
        {
             SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);
            if(cur_tag->CmdId.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->CmdId), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }
            if(cur_tag->msgRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->msgRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }
//@hong2012 move msgref before cmdref
            if(cur_tag->cmdRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->cmdRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }
            if(cur_tag->cmd.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->cmd), buf2,MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }

            for(item_tag = cur_tag->item_ptr; item_tag!= PNULL; item_tag=item_tag->next)
            {
                MMIDM_BuildItem(item_tag, buf2, MAX_TAG_BUF_SIZE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }

            if(cur_tag->sourceRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->sourceRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }
            if(cur_tag->targetRef.tagContent)
            {
                MMIDM_CreateTag(&(cur_tag->targetRef), buf2, MAX_TAG_BUF_SIZE, TRUE);
                if(MAX_XML_BUF_SIZE > (strlen(buf)+strlen(buf2)))
                {
                    strcat(buf, buf2);
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_BuildStatus buf is too small");
                }
            }
            if(strlen(buf))
            {
                ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
                if(PNULL== ptr)
                {
                    SCI_TRACE_LOW("MMIDM_BuildReplace PNULL== ptr");
                    ret = FALSE;
                    break;
                }

                SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
                ptr->tagId = TAG_RESULTID;
                ptr->hasChildTag = TRUE;
                ptr->tagContent = SCI_ALLOCA(strlen(buf)+1);
                if(PNULL==  ptr->tagContent)
                {
                    SCI_FREEAA(ptr);
		    ptr= PNULL;  //coverity-47450. free ptr and set to PNULL.
                    SCI_TRACE_LOW("MMIDM_BuildResult PNULL==  ptr->tagContent");
                    ret = FALSE;
                    break;
                }
                SCI_MEMSET(ptr->tagContent, 0, (strlen(buf)+1));/*lint !e666*/
                SCI_STRCPY(ptr->tagContent, buf);
                MMIDM_CreateTag(ptr, buf, MAX_XML_BUF_SIZE, TRUE);


                strcat(result_buf, buf);
            }
        }

    } while(0);

    while(PNULL!=s_resultTag_head)
    {
        while(PNULL!=s_resultTag_head->item_ptr)
        {
            item_tag = s_resultTag_head->item_ptr->next;
            SCI_FREEAA(s_resultTag_head->item_ptr);
            s_resultTag_head->item_ptr= item_tag;
        }
        cur_tag = s_resultTag_head->next;
        SCI_FREEAA(s_resultTag_head);
        s_resultTag_head= cur_tag;
    }
    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != buf2)
    {
        SCI_FREEAA(buf2);
        buf2 = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}
/*****************************************************************************/
//  Description : generate the body of the xml msg
//  Global resource dependence :
//  Author:MARY.XIAO
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_CodecXmlBody(char* bodybuf, uint16 buf_size)
{
    char* buf =PNULL;
    char str[50] ={0};

    DMXML_TAG_ALERT_T* alert_tag = PNULL;
    DMXML_TAG_REPLACE_T*    replace_tag = PNULL;
    DMXML_TAG_ITEM_T*    item_tag = PNULL;
    DMXML_TAG_ITEM_T*    item_tag_tail = PNULL;
    DMXML_TAG_T* ptr = PNULL;
    BOOLEAN ret =TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_CodecXmlBody");
    do {


        if(PNULL == bodybuf)
        {
            SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL == bodybuf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(bodybuf, 0, buf_size);

        buf = SCI_ALLOCA(MAX_XML_BUF_SIZE);
        if(PNULL == buf)
        {
            SCI_TRACE_LOW("MMIDM_BuildResult  PNULL == buf");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);



        if(s_g_step == STEP_GETNONCE )
        {
            alert_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ALERT_T)) ;
            if(PNULL== alert_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== alert_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(alert_tag, 0X0, sizeof(DMXML_TAG_ALERT_T));

            MMIDM_BuildStatus(buf, MAX_XML_BUF_SIZE);
            strcat(bodybuf, buf);

            alert_tag->CmdId.hasChildTag = FALSE;
            alert_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            alert_tag->CmdId.tagContent = SCI_ALLOCA(10);
            if(PNULL==  alert_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==  alert_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(alert_tag->CmdId.tagContent, 0, 10);
            SCI_STRCPY(alert_tag->CmdId.tagContent, str) ;


            alert_tag->DATA.hasChildTag = FALSE;
            alert_tag->DATA.tagId = TAG_DATAID;
            alert_tag->DATA.tagContent = SCI_ALLOCA(10);
            if(PNULL==  alert_tag->DATA.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== alert_tag->DATA.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(alert_tag->DATA.tagContent, 0, 10);
            SCI_STRCPY(alert_tag->DATA.tagContent, (char*)"1200") ;

            alert_tag->next = (DMXML_TAG_ALERT_T*)PNULL;
            s_alerTag_head = alert_tag;
            MMIDM_BuildAlert(bodybuf, MAX_XML_BUF_SIZE);


            replace_tag = SCI_ALLOCA(sizeof(DMXML_TAG_REPLACE_T)) ;
            if(PNULL== replace_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== replace_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(replace_tag, 0, sizeof(DMXML_TAG_REPLACE_T));

            SCI_TRACE_LOW("MMIDM_CodecXmlBody after malloc replace_tag");

            replace_tag->CmdId.hasChildTag = FALSE;
            replace_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            replace_tag->CmdId.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL== replace_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==replace_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(replace_tag->CmdId.tagContent, 0, strlen(str)+1);/*lint !e666*/
            SCI_STRCPY(replace_tag->CmdId.tagContent, str);

            SCI_TRACE_LOW("MMIDM_CodecXmlBody after TAG_CMDIDID");

            item_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T)) ;
            if(PNULL== item_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== item_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag, 0, sizeof(DMXML_TAG_ITEM_T));
            item_tag->source.locuri.hasChildTag = FALSE;
            item_tag->source.locuri.tagId = TAG_LOCURIID;
            item_tag->source.locuri.tagContent = SCI_ALLOCA(20);
            if(PNULL== item_tag->source.locuri.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->source.locuri.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->source.locuri.tagContent, 0, 20);
            SCI_STRCPY(item_tag->source.locuri.tagContent, (char*)"./DevInfo/Mod");
            item_tag->data.hasChildTag = FALSE;
            item_tag->data.tagId = TAG_DATAID;
            MMIDM_GetDmParaInfo(DM_MON_SET, str,50);
            item_tag->data.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL== item_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->data.tagContent, 0, (strlen(str)+1));/*lint !e666*/
            SCI_STRCPY(item_tag->data.tagContent, str);
            replace_tag->item_ptr = item_tag;
            item_tag_tail = item_tag;


            item_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T)) ;
            if(PNULL== item_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== item_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag, 0, sizeof(DMXML_TAG_ITEM_T));
            item_tag->source.locuri.hasChildTag = FALSE;
            item_tag->source.locuri.tagId = TAG_LOCURIID;
            item_tag->source.locuri.tagContent = SCI_ALLOCA(20);
            if(PNULL== item_tag->source.locuri.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->source.locuri.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->source.locuri.tagContent, 0, 20);
            SCI_STRCPY(item_tag->source.locuri.tagContent, (char*)"./DevInfo/Man");
            item_tag->data.hasChildTag = FALSE;
            item_tag->data.tagId = TAG_DATAID;
            MMIDM_GetDmParaInfo(DM_MAN_SET, str,50);
            item_tag->data.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL== item_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->data.tagContent, 0, (strlen(str)+1));/*lint !e666*/
            SCI_STRCPY(item_tag->data.tagContent, str);

            item_tag_tail->next = item_tag;
            item_tag_tail = item_tag_tail->next;


            item_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T)) ;
            if(PNULL== item_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== item_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag, 0, sizeof(DMXML_TAG_ITEM_T));
            item_tag->source.locuri.hasChildTag = FALSE;
            item_tag->source.locuri.tagId = TAG_LOCURIID;
            item_tag->source.locuri.tagContent = SCI_ALLOCA(20);
            if(PNULL== item_tag->source.locuri.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->source.locuri.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->source.locuri.tagContent, 0, 20);
            SCI_STRCPY(item_tag->source.locuri.tagContent, (char*)"./DevInfo/Lang");
            item_tag->data.hasChildTag = FALSE;
            item_tag->data.tagId = TAG_DATAID;
            item_tag->data.tagContent =SCI_ALLOCA(10);
            if(PNULL== item_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->data.tagContent, 0, 10);
            SCI_STRCPY(item_tag->data.tagContent, (char*)"en-US");
            item_tag_tail->next = item_tag;
            item_tag_tail = item_tag_tail->next;


            item_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T)) ;
            if(PNULL== item_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== item_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag, 0, sizeof(DMXML_TAG_ITEM_T));
            item_tag->source.locuri.hasChildTag = FALSE;
            item_tag->source.locuri.tagId = TAG_LOCURIID;
            item_tag->source.locuri.tagContent = SCI_ALLOCA(20);
            if(PNULL== item_tag->source.locuri.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->source.locuri.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->source.locuri.tagContent, 0, 20);
            SCI_STRCPY(item_tag->source.locuri.tagContent, (char*)"./DevInfo/DmV");
            item_tag->data.hasChildTag = FALSE;
            item_tag->data.tagId = TAG_DATAID;
            item_tag->data.tagContent = SCI_ALLOCA(10);
            if(PNULL== item_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->data.tagContent, 0, 10);
            SCI_STRCPY(item_tag->data.tagContent, (char*)"1.2");
            item_tag_tail->next = item_tag;
            item_tag_tail = item_tag_tail->next;

            item_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T)) ;
            if(PNULL== item_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== item_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag, 0, sizeof(DMXML_TAG_ITEM_T));
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId 0");
            item_tag->source.locuri.hasChildTag = FALSE;
            item_tag->source.locuri.tagId = TAG_LOCURIID;
            item_tag->source.locuri.tagContent = SCI_ALLOCA(20);
            if(PNULL== item_tag->source.locuri.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->source.locuri.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->source.locuri.tagContent, 0, 20);
            SCI_STRCPY(item_tag->source.locuri.tagContent, (char*)"./DevInfo/DevId");
            item_tag->meta.format.hasChildTag = FALSE;
            SCI_STRCPY(item_tag->meta.format.tagArr, (char*)"xmlns=\'syncml:metinf\'");
            item_tag->meta.format.tagContent = SCI_ALLOCA(10);
            if(PNULL== item_tag->meta.format.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->meta.format.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_tag->meta.format.tagContent, 0, 10);
            SCI_STRCPY(item_tag->meta.format.tagContent, (char*)"chr");
            item_tag->meta.format.tagId=TAG_FORMATID;
            item_tag->data.hasChildTag = FALSE;
            item_tag->data.tagId = TAG_DATAID;
            MMIDM_GetDmParaInfo(DM_IMEI_SET, str,50);
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId 1.5");
            item_tag->data.tagContent = SCI_ALLOCA(30);
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId 1.6");
            if(PNULL== item_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==item_tag->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId 1.7");
            SCI_MEMSET(item_tag->data.tagContent, 0, 30);
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId 1.8");
            SCI_STRCPY(item_tag->data.tagContent, (char*)"IMEI:");
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId 2");
            strcat(item_tag->data.tagContent, str);
            item_tag_tail->next = item_tag;
            item_tag_tail = item_tag_tail->next;
            item_tag_tail->next =PNULL;
            //item_tag =PNULL;
            SCI_TRACE_LOW("MMIDM_CodecXmlBody after ./DevInfo/DevId");
            s_replaceTag_head = replace_tag;
            SCI_TRACE_LOW("MMIDM_CodecXmlBody begin to MMIDM_BuildReplace");
            MMIDM_BuildReplace(buf, MAX_XML_BUF_SIZE);

            strcat(bodybuf, buf);
            //strcat(bodybuf, "<Final />");
	     strcat(bodybuf, "<Final></Final>");



        }
        else if(s_g_step == STEP_CREDED)
        {
            SCI_MEMSET(bodybuf, 0, MAX_XML_BUF_SIZE);

            MMIDM_BuildAlert(buf, MAX_XML_BUF_SIZE);
            if(MAX_XML_BUF_SIZE>(strlen(bodybuf)+strlen(buf)))
            {
                strcat(bodybuf, buf);
            }
            else
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small1111 %d %d ", strlen(bodybuf),strlen(buf));
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small");
            }
            SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);
            MMIDM_BuildStatus(buf, MAX_XML_BUF_SIZE);
            if(MAX_XML_BUF_SIZE>(strlen(bodybuf)+strlen(buf)))
            {
                strcat(bodybuf, buf);
            }
            else
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small2222 %d %d ", strlen(bodybuf),strlen(buf));
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small");
            }
            SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);
            MMIDM_BuildReplace(buf, MAX_XML_BUF_SIZE);
            if(MAX_XML_BUF_SIZE>(strlen(bodybuf)+strlen(buf)))
            {

                strcat(bodybuf, buf);
            }
            else
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small3333 %d %d ", strlen(bodybuf),strlen(buf));
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small");
            }
            SCI_MEMSET(buf, 0, MAX_XML_BUF_SIZE);
            MMIDM_BuildResult(buf, MAX_XML_BUF_SIZE);
            if(MAX_XML_BUF_SIZE>(strlen(bodybuf)+strlen(buf)))
            {
                strcat(bodybuf, buf);
            }
            else
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small4444 %d %d ", strlen(bodybuf),strlen(buf));
                SCI_TRACE_LOW("MMIDM_CodecXmlBody buf is too small");
            }
           // strcat(bodybuf, "<Final />");
	     strcat(bodybuf, "<Final></Final>");
        }
        else if(s_g_step == STEP_ABORT)
        {
           alert_tag = SCI_ALLOCA(sizeof(DMXML_TAG_ALERT_T)) ;
            if(PNULL== alert_tag)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== alert_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(alert_tag, 0X0, sizeof(DMXML_TAG_ALERT_T));


            alert_tag->CmdId.hasChildTag = FALSE;
            alert_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            alert_tag->CmdId.tagContent = SCI_ALLOCA(10);
            if(PNULL==  alert_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==  alert_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(alert_tag->CmdId.tagContent, 0, 10);
            SCI_STRCPY(alert_tag->CmdId.tagContent, str) ;


            alert_tag->DATA.hasChildTag = FALSE;
            alert_tag->DATA.tagId = TAG_DATAID;
            alert_tag->DATA.tagContent = SCI_ALLOCA(10);
            if(PNULL==  alert_tag->DATA.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== alert_tag->DATA.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(alert_tag->DATA.tagContent, 0, 10);
            SCI_STRCPY(alert_tag->DATA.tagContent, (char*)"1223") ;

            alert_tag->next = (DMXML_TAG_ALERT_T*)PNULL;
            s_alerTag_head = alert_tag;
            MMIDM_BuildAlert(bodybuf, MAX_XML_BUF_SIZE);
            //strcat(bodybuf, "<Final />");
	      strcat(bodybuf, "<Final></Final>");
        }

        ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
        if(PNULL== ptr)
        {
            SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL== ptr");
            ret = FALSE;
            break;
        }

        SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
        ptr->tagId = TAG_SYNCBODYID;
        ptr->hasChildTag = TRUE;
        ptr->tagContent = SCI_ALLOCA(strlen(bodybuf)+1);
        if(PNULL==  ptr->tagContent)
        {
            SCI_TRACE_LOW("MMIDM_CodecXmlBody PNULL==  ptr->tagContent");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(ptr->tagContent, 0, (strlen(bodybuf)+1));/*lint !e666*/
        SCI_STRCPY(ptr->tagContent, bodybuf);
        MMIDM_CreateTag(ptr, bodybuf, MAX_XML_BUF_SIZE, TRUE);
    } while(0);

    if(PNULL != buf)
    {
        SCI_FREEAA(buf);
        buf = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }

        SCI_FREEAA(ptr);
        ptr = PNULL;
    }

    SCI_TRACE_LOW("MMIDM_CodecXmlBody ret %d",ret);

    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}
/*****************************************************************************/
//  Description : GET THE CONTENT OF THE TAG
//  Global resource dependence :
//  Author:MARY.XIAO
//  xmlbuf: CONNTENT NEED TO PARSE
//  tagid: THE FIRST TAG WHICH CONTENT WANT TO GET
//  content: OUT: THE TAG CONTENT
//  return:  return the left content
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL char* MMIDM_getNextXmlTagBuf(char* xmlbuf, MMI_DM_TAGID_E tagid, char* content, uint16 buf_size)
{
 //   char tagStr[50]    = {0};
//    char* content_ptr   = PNULL;
    char* scanner_ptr   = PNULL;
    int len_end                  =0;
    int len_head                 =0;
    int len                      =0;
    int tag_len                  =0;
	int xmlbuf_len                   =0;
    
    if(PNULL == content || PNULL == xmlbuf)
    {
      SCI_TRACE_LOW("MMIDM_getNextXmlTagBuf PNULL == content");
      return PNULL;
    }
    xmlbuf_len = strlen(xmlbuf);
    tag_len = strlen(MMIDM_GetTagStr(tagid));
		
	 SCI_TRACE_LOW("enter MMIDM_getNextXmlTagBuf xmlbuf_len=%d tag=%s", xmlbuf_len, MMIDM_GetTagStr(tagid)); 
		
    //Length of less than,don't parse
    if (xmlbuf_len < ((tag_len * 2) + 5)) //eg. <TAG></TAG>
  	{
  	   SCI_TRACE_LOW("ERR:MMIDM_getNextXmlTagBuf xmlbuf=%s tag=%s", xmlbuf, MMIDM_GetTagStr(tagid)); 
		return PNULL;
	}
		
    memset(content,0, buf_size);
    scanner_ptr = xmlbuf;

   // MMIDM_GetTagStr(tagid, tagStr, 50);
    while(strlen(scanner_ptr))
    {
        scanner_ptr = strstr(scanner_ptr, MMIDM_GetTagStr(tagid));
        if(PNULL == scanner_ptr)
        {
            return PNULL;
        }
        len = strlen(xmlbuf) - strlen(scanner_ptr);
        if(1 <= len && xmlbuf[len-1] == '<')
        {
            break;
        }
        scanner_ptr +=tag_len;
    }

	if (PNULL==scanner_ptr)
	{
		SCI_TRACE_LOW("ERR:MMIDM_getNextXmlTagBuf PNULL==scanner_ptr"); 
		return PNULL;
	}
	
	if (strlen(scanner_ptr) < tag_len)
	{
		SCI_TRACE_LOW("ERR:MMIDM_getNextXmlTagBuf tag_len=%d scanner_len=%d", tag_len, strlen(scanner_ptr)); 
		return PNULL;
	}
		
    scanner_ptr +=tag_len;
    while(PNULL!=scanner_ptr)
    {
        if(scanner_ptr[0] == '>')
        {
            scanner_ptr++;/*lint !e831*/
            break;
        }
        else
        {
            scanner_ptr++;/*lint !e831*/
        }
    }
    if(PNULL == scanner_ptr)/*lint !e774*/
    {
        return PNULL;
    }
    len_head = strlen(xmlbuf)-strlen(scanner_ptr);

	if (0 > len_head)
	{
		SCI_TRACE_LOW("ERR:MMIDM_getNextXmlTagBuf len_head=%d", len_head); 
       return PNULL;
	}
		
    scanner_ptr = strstr(scanner_ptr, MMIDM_GetTagStr(tagid));
    if(PNULL == scanner_ptr)
    {
        return PNULL;
    }
    len_end = strlen(xmlbuf)-strlen(scanner_ptr)-3;

   if (0 > len_end || xmlbuf_len < (len_end + 3 + tag_len))
	{
		SCI_TRACE_LOW("ERR:MMIDM_getNextXmlTagBuf len_end=%d", len_end); 
       return PNULL;
	}

    if((xmlbuf[len_end+2] == '/')&&(xmlbuf[len_end+1] == '<')&&xmlbuf[len_end+3+tag_len] == '>')
    {
		  int copy_len = (len_end-len_head+1);
          scanner_ptr = xmlbuf;
          //memset(content, 0, strlen(content));
		   if (copy_len > buf_size)
		   {
			 	SCI_TRACE_LOW("ERR:MMIDM_getNextXmlTagBuf needbuffer=%d buf_size=%d", copy_len, buf_size); 
				copy_len = buf_size;
			}
          strncpy(content, scanner_ptr+len_head, copy_len);
          scanner_ptr = scanner_ptr+ len_end +tag_len+3+1;
          return scanner_ptr;
    }
    else
    {
//        if(scanner_ptr!=PNULL)
//        {
            MMIDM_getNextXmlTagBuf(scanner_ptr, tagid, content, buf_size);
//        }
//        else
//        {
//            return PNULL;
//        }
    }
     SCI_TRACE_LOW("leave  MMIDM_getNextXmlTagBuf");
     return PNULL;
}

/*****************************************************************************/
//  Description : GET THE CONTENT OF THE TAG
//  Global resource dependence :
//  Author:MARY.XIAO
//  xmlbuf: CONNTENT NEED TO PARSE
//  tagid: THE FIRST TAG WHICH CONTENT WANT TO GET
//  content: OUT: THE TAG CONTENT
//  return:  return the left content
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_generateXMLData(char* sendbuf)
{
    char *bodybuf=PNULL;
    char* sendContent=PNULL;
    char* head_ptr ="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    DMXML_TAG_T * ptr = PNULL;
    BOOLEAN  ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_generateXMLData");

    do {

        bodybuf = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
        if(PNULL== bodybuf)
        {
            SCI_TRACE_LOW("MMIDM_generateXMLData PNULL== bodybuf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(bodybuf, 0X0, MAX_XML_BUF_SIZE);

        sendContent = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
        if(PNULL== sendContent)
        {
            SCI_TRACE_LOW("MMIDM_generateXMLData PNULL== sendContent");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(sendContent, 0X0, MAX_XML_BUF_SIZE);

        InitDMXMLHeader(sendContent, MAX_XML_BUF_SIZE);
        SCI_TRACE_LOW("MMIDM_generateXMLData HEADER");
        MMIDM_CodecXmlBody(bodybuf, MAX_XML_BUF_SIZE);
        SCI_TRACE_LOW("MMIDM_generateXMLData BODY");
        strcat(sendContent, bodybuf);

        ptr = SCI_ALLOCA(sizeof(DMXML_TAG_T));
        if(PNULL== ptr)
        {
            SCI_TRACE_LOW("MMIDM_generateXMLData PNULL== ptr");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(ptr, 0, sizeof(DMXML_TAG_T));
        ptr->tagId = TAG_SYNCMLID;
        ptr->hasChildTag = TRUE;
        SCI_STRCPY(ptr->tagArr, (char*)"xmlns=\'SYNCML:SYNCML1.2\'");
        ptr->tagContent = SCI_ALLOCA(strlen(sendContent)+1);
        if(PNULL==  ptr->tagContent)
        {
            SCI_TRACE_LOW("MMIDM_generateXMLData PNULL==  ptr->tagContent");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(ptr->tagContent, 0, (strlen(sendContent)+1));/*lint !e666*/
        SCI_STRCPY(ptr->tagContent, sendContent);
        MMIDM_CreateTag(ptr, sendContent, MAX_XML_BUF_SIZE, TRUE);
        SCI_STRCPY(sendbuf, head_ptr);
        strcat(sendbuf, sendContent);


    } while(0);

    if(PNULL != sendContent)
    {
        SCI_FREEAA(sendContent);
        sendContent = PNULL;
    }
    if(PNULL != bodybuf)
    {
        SCI_FREEAA(bodybuf);
        bodybuf = PNULL;
    }
    if(PNULL != ptr)
    {
        if(PNULL != ptr->tagContent)
        {
            SCI_FREEAA(ptr->tagContent);
            ptr->tagContent = PNULL;
        }
        SCI_FREEAA(ptr);
        ptr = PNULL;
    }
   SCI_TRACE_LOW(" MMIDM_generateXMLData ret %d",ret);
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;



}

/*****************************************************************************/
//  Description : deal with the Exec command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithExecData(char* execbuf)
{
    char locuri[MAX_TARGET_BUF_SIZE + 1]={0};
    DMXML_TAG_STATUS_T* status_tag = PNULL;
    char cmdid[MAX_CMDID_BUF_SIZE + 1]={0};
    char target[MAX_TARGET_BUF_SIZE + 1]={0};
    char target_uri[MAX_TARGET_BUF_SIZE + 1]={0};
    char str[20]={0};
    BOOLEAN  ret = TRUE;

    do {

        MMIDM_getNextXmlTagBuf(execbuf, TAG_CMDIDID, cmdid, MAX_CMDID_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(execbuf, TAG_TARGETID, target,MAX_TARGET_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(target, TAG_LOCURIID, target_uri, MAX_TARGET_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(execbuf, TAG_LOCURIID, locuri, MAX_TARGET_BUF_SIZE);

        if(!strcmp(locuri, "./LAWMO/Operations/FactoryReset")
           || !strcmp(locuri, "./LAWMO/Operations/PartiallyLock")
           || !strcmp(locuri, "./LAWMO/Operations/UnLock"))
        {


            status_tag = SCI_ALLOCA(sizeof(DMXML_TAG_STATUS_T));
            if(PNULL== status_tag)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL== status_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag, 0X0, sizeof(DMXML_TAG_STATUS_T));


            status_tag->CmdId.hasChildTag =FALSE;
            status_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            status_tag->CmdId.tagContent = SCI_ALLOCA(strlen(str)+1);/*lint !e666*/
            if(PNULL==  status_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL== status_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->CmdId.tagContent, 0, (strlen(str)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->CmdId.tagContent, str);

            status_tag->msgRef.hasChildTag = FALSE;
            status_tag->msgRef.tagId=TAG_MSGREFID;
            MMIDM_IU32toa(s_g_MsgID, str, 32);
            status_tag->msgRef.tagContent = SCI_ALLOCA(strlen(str)+1);/*lint !e666*/
            if(PNULL==  status_tag->msgRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL==status_tag->msgRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->msgRef.tagContent, 0, (strlen(str)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->msgRef.tagContent, str);

            status_tag->cmdRef.hasChildTag = FALSE;
            status_tag->cmdRef.tagId = TAG_CMDREFID;
            status_tag->cmdRef.tagContent = SCI_ALLOCA(strlen(cmdid)+1);
            if(PNULL== status_tag->cmdRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL==  status_tag->cmdRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->cmdRef.tagContent, 0, (strlen(cmdid)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->cmdRef.tagContent, cmdid);

            status_tag->targetRef.hasChildTag = FALSE;
            status_tag->targetRef.tagId = TAG_TARGETREFID;
            status_tag->targetRef.tagContent = SCI_ALLOCA(strlen(target_uri)+1);
            if(PNULL==status_tag->targetRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL== status_tag->targetRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->targetRef.tagContent, 0, (strlen(target_uri)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->targetRef.tagContent, target_uri);

            status_tag->cmd.hasChildTag = FALSE;
            status_tag->cmd.tagId = TAG_CMDID;
            status_tag->cmd.tagContent = SCI_ALLOCA(10);
            if(PNULL==status_tag->cmd.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL== status_tag->cmd.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->cmd.tagContent, 0, 10);
            SCI_STRCPY(status_tag->cmd.tagContent, (char*)"Exec");


            status_tag->data.hasChildTag = FALSE;
            status_tag->data.tagId = TAG_DATAID;
            status_tag->data.tagContent = SCI_ALLOCA(10);
            if(PNULL==status_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithExecData PNULL== status_tag->data.tagContent");

                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->data.tagContent, 0, 10);
            SCI_STRCPY(status_tag->data.tagContent, (char*)"200");


            if(s_statusTag_head == PNULL)
            {
                s_statusTag_head = s_statusTag_tail = status_tag;
            }
            else
            {
                s_statusTag_tail->next = status_tag;
                s_statusTag_tail = s_statusTag_tail->next;

            }
            if(!strcmp(locuri, "./LAWMO/Operations/FactoryReset"))
            {
                s_g_callClearFunc = TRUE;
            }
            else if(!strcmp(locuri, "./LAWMO/Operations/PartiallyLock"))
            {
                s_g_callLockFunc = TRUE;
            }
            else if(!strcmp(locuri, "./LAWMO/Operations/UnLock"))
            {
                s_g_callUnlockFunc = TRUE;
            }
           // MMIDM_ResetFactorySetting();
        }

    } while(0);

    if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}

/*****************************************************************************/
//  Description : parse the the Exec command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的Exec命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLExec(char* xmlbuf)
{
    char* body_buf=PNULL;
//    DMXML_TAG_STATUS_T* status_tag = PNULL;
    char* exec_buf=PNULL;
//    char buf_cmdref[10] ={0};
    char* scanner_ptr = PNULL;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_ParseXMLStatus");
    do {

        body_buf = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
        if(PNULL== body_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLStatus PNULL== bodybuf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(body_buf, 0X0, MAX_XML_BUF_SIZE);

        exec_buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
        if(PNULL== exec_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLExec PNULL== exec_buf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(exec_buf, 0X0, MAX_TAG_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(xmlbuf, TAG_SYNCBODYID, body_buf, MAX_XML_BUF_SIZE);
        scanner_ptr = body_buf;
        SCI_TRACE_LOW("MMIDM_ParseXMLExec body_buf");

        while((scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_EXECID, exec_buf, MAX_TAG_BUF_SIZE))!=PNULL)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLStatus exec_buf");
            MMIDM_DealWithExecData(exec_buf);
        }

    } while(0);

    if(PNULL != body_buf)
    {
        SCI_FREEAA(body_buf);
        body_buf = PNULL;
    }
    if(PNULL != exec_buf)
    {
        SCI_FREEAA(exec_buf);
        exec_buf = PNULL;
    }
    if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    SCI_TRACE_LOW("leave MMIDM_ParseXMLExec");
    return ret;


}
/*****************************************************************************/
//  Description : deal with the Status command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理Status命令，加入Status链表中
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithStatusData(char* statusbuf)
{
    char cmd[MAX_CMDID_BUF_SIZE + 1]={0};
    char cmdref[MAX_CMDID_BUF_SIZE]={0};
    char targetref[MAX_TARGET_BUF_SIZE + 1]={0};
    char sourceref[MAX_TARGET_BUF_SIZE + 1]={0};
    char data[20]={0};
    char msgref[MAX_CMDID_BUF_SIZE + 1];
    char str[20]={0};
//    char* scanner_ptr = PNULL;
    DMXML_TAG_STATUS_T* status_tag = PNULL;
    BOOLEAN  ret = TRUE;


    do
    {

        SCI_TRACE_LOW("ENTER MMIDM_DealWithStatusData");
        if(PNULL == statusbuf)
        {
            SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL == status_buf");
            ret = FALSE;
            break;
        }


        MMIDM_getNextXmlTagBuf(statusbuf, TAG_CMDID, cmd, MAX_CMDID_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(statusbuf, TAG_CMDREFID, cmdref, MAX_CMDID_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(statusbuf, TAG_DATAID, data, 20);
        MMIDM_getNextXmlTagBuf(statusbuf, TAG_MSGREFID, msgref, MAX_CMDID_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(statusbuf, TAG_TARGETREFID, targetref, MAX_TARGET_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(statusbuf, TAG_SOURCEREFID, sourceref, MAX_TARGET_BUF_SIZE);
        if(strlen(cmd))
        {
            if(!strcmp(cmd, "SyncHdr"))
            {
                MMIDM_getNextXmlTagBuf(statusbuf, TAG_NEXTNONCEID, s_g_nonce, MAX_NONCE_LEN);
                SCI_TRACE_LOW("MMIDM_DealWithStatusData the cred status s_g_nonce %s", s_g_nonce);

               if(!strcmp(data, "212"))
               {
                   s_g_step = STEP_CREDED;
                  // return;
               }
               else if(!strcmp(data, "401") || !strcmp(data, "407"))
               {
                    if(s_g_step == STEP_GETNONCE)
                    {
                        s_g_step = STEP_GETNONCE;
                    }
                    else
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData cred fail");
                        ret = FALSE;
                        break;
                    }
               }
               if(s_g_step == STEP_CREDED)
                {

                    status_tag = SCI_ALLOCA(sizeof(DMXML_TAG_STATUS_T));
                    if(PNULL== status_tag)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag, 0X0, sizeof(DMXML_TAG_STATUS_T));

                    status_tag->CmdId.hasChildTag =FALSE;
                    status_tag->CmdId.tagId = TAG_CMDIDID;
                    MMIDM_IU32toa(s_g_CmdID, str, 32);
                    s_g_CmdID++;
                    status_tag->CmdId.tagContent = SCI_ALLOCA(strlen(str)+1);
                    if(PNULL==  status_tag->CmdId.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->CmdId.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->CmdId.tagContent, 0, (strlen(str)+1));/*lint !e666*/
                    SCI_STRCPY(status_tag->CmdId.tagContent, str);

                    status_tag->msgRef.hasChildTag = FALSE;
                    status_tag->msgRef.tagId=TAG_MSGREFID;
                    MMIDM_IU32toa(s_g_MsgID, str, 32);
                    status_tag->msgRef.tagContent = SCI_ALLOCA(strlen(str)+1);
                    if(PNULL==  status_tag->msgRef.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL==status_tag->msgRef.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->msgRef.tagContent, 0, (strlen(str)+1));/*lint !e666*/
                    SCI_STRCPY(status_tag->msgRef.tagContent, str);

                    status_tag->cmdRef.hasChildTag = FALSE;
                    status_tag->cmdRef.tagId = TAG_CMDREFID;
                    status_tag->cmdRef.tagContent = SCI_ALLOCA(8);
                    if(PNULL== status_tag->cmdRef.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL==  status_tag->cmdRef.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->cmdRef.tagContent, 0, 8);
                    SCI_STRCPY(status_tag->cmdRef.tagContent, (char*)"0");

                    status_tag->targetRef.hasChildTag = FALSE;
                    status_tag->targetRef.tagId = TAG_TARGETREFID;
                    status_tag->targetRef.tagContent = SCI_ALLOCA(strlen(sourceref)+1);
                    if(PNULL==status_tag->targetRef.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->targetRef.tagContent");
                         ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->targetRef.tagContent, 0, (strlen(sourceref)+1));/*lint !e666*/
                    SCI_STRCPY(status_tag->targetRef.tagContent, sourceref);

                    SCI_STRCPY(targetref,MMIDM_getResUri());/*lint !e666*/
                    status_tag->sourceRef.hasChildTag = FALSE;
                    status_tag->sourceRef.tagId = TAG_SOURCEREFID;
                    status_tag->sourceRef.tagContent = SCI_ALLOCA(strlen(targetref)+1);
                    if(PNULL==status_tag->sourceRef.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->targetRef.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->sourceRef.tagContent, 0, (strlen(targetref)+1));/*lint !e666*/
                    SCI_STRCPY(status_tag->sourceRef.tagContent, targetref);//mary112

                    status_tag->cmd.hasChildTag = FALSE;
                    status_tag->cmd.tagId = TAG_CMDID;
                    status_tag->cmd.tagContent = SCI_ALLOCA(10);
                    if(PNULL==status_tag->cmd.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->cmd.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->cmd.tagContent, 0, 10);
                    SCI_STRCPY(status_tag->cmd.tagContent, (char*)"SyncHdr");


                    status_tag->chal.meta.format.hasChildTag=FALSE;
                    status_tag->chal.meta.format.tagId=TAG_FORMATID;
                    SCI_STRCPY(status_tag->chal.meta.format.tagArr,(char*)"xmlns=\'syncml:metinf\'");
                    status_tag->chal.meta.format.tagContent = SCI_ALLOCA(10);
                    if(PNULL==status_tag->chal.meta.format.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->chal.meta.format.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->chal.meta.format.tagContent, 0, 10);
                    SCI_STRCPY(status_tag->chal.meta.format.tagContent, (char*)"b64");

                    status_tag->chal.meta.type.hasChildTag=FALSE;
                    status_tag->chal.meta.type.tagId=TAG_TYPEID;
                    SCI_STRCPY(status_tag->chal.meta.type.tagArr,(char*)"xmlns=\'syncml:metinf\'");
                    status_tag->chal.meta.type.tagContent = SCI_ALLOCA(20);
                    if(PNULL==status_tag->chal.meta.type.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->chal.meta.type.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->chal.meta.type.tagContent, 0, 20);
                    SCI_STRCPY(status_tag->chal.meta.type.tagContent, (char*)"syncml:auth-MAC");

                    status_tag->chal.meta.nextnonce.hasChildTag=FALSE;
                    status_tag->chal.meta.nextnonce.tagId=TAG_NEXTNONCEID;
                    SCI_STRCPY(status_tag->chal.meta.nextnonce.tagArr,(char*)"xmlns=\'syncml:metinf\'");
                    status_tag->chal.meta.nextnonce.tagContent = SCI_ALLOCA(64);
                    if(PNULL==status_tag->chal.meta.nextnonce.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->chal.meta.nextnonce.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->chal.meta.nextnonce.tagContent, 0, 64);
                    SCI_STRCPY(status_tag->chal.meta.nextnonce.tagContent, (char*)"BKIYZbVlTKaMiFZbXU3euw==");

                    status_tag->data.hasChildTag = FALSE;
                    status_tag->data.tagId = TAG_DATAID;
                    status_tag->data.tagContent = SCI_ALLOCA(10);
                    if(PNULL==status_tag->data.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithStatusData PNULL== status_tag->data.tagContent");
                        ret = FALSE;
                        break;
                    }
                    SCI_MEMSET(status_tag->data.tagContent, 0, 10);
                    SCI_STRCPY(status_tag->data.tagContent, (char*)"200");




                    if(s_statusTag_head == PNULL)
                    {
                        s_statusTag_head = s_statusTag_tail = status_tag;
                    }
                    else
                    {
                        s_statusTag_tail->next = status_tag;
                        s_statusTag_tail = s_statusTag_tail->next;

                    }
                }


            }
        }
        else
        {
           SCI_TRACE_LOW("MMIDM_DealWithStatusData");
        }
    }while(0);

    if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}
/*****************************************************************************/
//  Description : parse the the Status command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的Status命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLStatus(char* xmlbuf)
{
    char* body_buf=PNULL;
//    DMXML_TAG_STATUS_T* status_tag = PNULL;
    char* status_buf=PNULL;
//    char buf_cmdref[10] ={0};
    char* scanner_ptr = PNULL;
    BOOLEAN ret =TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_ParseXMLStatus");

    do {


        body_buf = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
        if(PNULL== body_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLStatus PNULL== bodybuf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(body_buf, 0X0, MAX_XML_BUF_SIZE);

        status_buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
        if(PNULL== status_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLStatus PNULL== status_buf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(status_buf, 0X0, MAX_TAG_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(xmlbuf, TAG_SYNCBODYID, body_buf, MAX_XML_BUF_SIZE);
        scanner_ptr = body_buf;
        SCI_TRACE_LOW("MMIDM_ParseXMLStatus body_buf");

        while((scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_STATUSID, status_buf, MAX_TAG_BUF_SIZE))!=PNULL)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLStatus status_buf");
            MMIDM_DealWithStatusData(status_buf);
        }
    } while(0);

    if(PNULL != body_buf)
    {
        SCI_FREEAA(body_buf);
        body_buf = PNULL;
    }
    if(PNULL != status_buf)
    {
        SCI_FREEAA(status_buf);
        status_buf = PNULL;
    }
     if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}



extern  int32 DM_GET_TREE_READFUNC(char* path);
extern  int32 DM_GET_TREE_WRITEFUNC(char* path);
/*****************************************************************************/
//  Description : deal with the get command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理get命令，加入get链表中
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithGetData(char* getbuf)
{
    DMXML_TAG_STATUS_T* status_tag = PNULL;
    DMXML_TAG_RESULT_T* result_tag = PNULL;
    DMXML_TAG_ITEM_T* item_ptr = PNULL;
    char cmdid[MAX_CMDID_BUF_SIZE + 1]={0};
//    char msg[20]={0};
    char target[MAX_TARGET_BUF_SIZE + 1]={0};
    char target_uri[MAX_TARGET_BUF_SIZE + 1]={0};
    char* item=PNULL;
    char*  scanner_ptr = PNULL;
    char* item_buff=PNULL;
    char* content=PNULL;
    //unsigned char test[9000]={0};
    char str[20]={0};
    uint16        len = 0;
    //VDM_Tree_ReadExternalCB read_func = PNULL;
    int32 handletype = -1;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_DealWithGetData");

    do {

        content = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
        if(PNULL== content)
        {
            SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== content");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(content, 0X0, MAX_TAG_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(getbuf, TAG_CMDIDID, cmdid, MAX_CMDID_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(getbuf, TAG_TARGETID, target,MAX_TARGET_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(target, TAG_LOCURIID, target_uri, MAX_TARGET_BUF_SIZE);


        if(strlen(target_uri))
        {
            status_tag = SCI_ALLOCA(sizeof(DMXML_TAG_STATUS_T));
            if(PNULL== status_tag)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== status_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag, 0X0, sizeof(DMXML_TAG_STATUS_T));

            result_tag = SCI_ALLOCA(sizeof(DMXML_TAG_RESULT_T));
            if(PNULL== result_tag)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== result_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(result_tag, 0X0, sizeof(DMXML_TAG_RESULT_T));

/* remove by Hong2012
            item_ptr = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T));
            if(PNULL== item_ptr)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== item_ptr");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_ptr, 0X0, sizeof(DMXML_TAG_ITEM_T));
            //read_func = DM_GET_TREE_READFUNC(target_uri);
            handletype =  DM_GET_TREE_READFUNC(target_uri);
*/
        handletype =  DM_GET_TREE_READFUNC(target_uri);
           SCI_TRACE_LOW("MMIDM_DealWithGetData handletype %s:%d",target_uri, handletype);

    //////////////////////////////////////////////////BEGIN TO CONSTRUCT STAUTS  CONTENT//////////////////////////////////////////////////////////////////////////

            status_tag->CmdId.hasChildTag =FALSE;
            status_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            status_tag->CmdId.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL==  status_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== status_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->CmdId.tagContent, 0, (strlen(str)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->CmdId.tagContent, str);

            status_tag->msgRef.hasChildTag = FALSE;
            status_tag->msgRef.tagId=TAG_MSGREFID;
#if 0 //@hong20120120
	    if(MMIDM_getNextXmlTagBuf(getbuf, TAG_MSGREFID,str, 10))
	     {
	     //Donothing
	     }
		else
		{
		strcpy(str,"0");
		}
#else
            MMIDM_IU32toa(s_g_MsgID, str, 32);
#endif

            status_tag->msgRef.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL==  status_tag->msgRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==status_tag->msgRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->msgRef.tagContent, 0, (strlen(str)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->msgRef.tagContent, str);

            status_tag->cmdRef.hasChildTag = FALSE;
            status_tag->cmdRef.tagId = TAG_CMDREFID;
            status_tag->cmdRef.tagContent = SCI_ALLOCA(strlen(cmdid)+1);
            if(PNULL== status_tag->cmdRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==  status_tag->cmdRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->cmdRef.tagContent, 0, (strlen(cmdid)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->cmdRef.tagContent, cmdid);



            status_tag->targetRef.hasChildTag = FALSE;
            status_tag->targetRef.tagId = TAG_TARGETREFID;
            status_tag->targetRef.tagContent = SCI_ALLOCA(strlen(target_uri)+1);
            if(PNULL==status_tag->targetRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== status_tag->targetRef.tagContent");
                ret = FALSE;
                break;
            }

            SCI_MEMSET(status_tag->targetRef.tagContent, 0, (strlen(target_uri)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->targetRef.tagContent, target_uri);

            status_tag->cmd.hasChildTag = FALSE;
            status_tag->cmd.tagId = TAG_CMDID;
            status_tag->cmd.tagContent = SCI_ALLOCA(10);
            if(PNULL==status_tag->cmd.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== status_tag->cmd.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->cmd.tagContent, 0, 10);
            SCI_STRCPY(status_tag->cmd.tagContent, (char*)"Get");

           // if(PNULL!=read_func)
            if(handletype!=-1)
            {
                status_tag->data.hasChildTag = FALSE;
                status_tag->data.tagId = TAG_DATAID;
                status_tag->data.tagContent = SCI_ALLOCA(10);
                if(PNULL==status_tag->data.tagContent)
                {
                    SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== status_tag->data.tagContent");
                ret = FALSE;
                break;
                }
                SCI_MEMSET(status_tag->data.tagContent, 0, 10);
                SCI_STRCPY(status_tag->data.tagContent, (char*)"200");

               // read_func(PNULL, 0, content, 500, &len);/*lint !e64*/
                spdm_readCb(handletype, content, 0, 500);

            }
            else
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData, PNULL==read_func");
                status_tag->data.hasChildTag = FALSE;
                status_tag->data.tagId = TAG_DATAID;
                status_tag->data.tagContent = SCI_ALLOCA(10);
                if(PNULL==status_tag->data.tagContent)
                {
                    SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== status_tag->data.tagContent");
                    ret = FALSE;
                    break;
                }
                SCI_MEMSET(status_tag->data.tagContent, 0, 10);
                SCI_STRCPY(status_tag->data.tagContent, (char*)"204");
            }

            if(s_statusTag_head == PNULL)
            {
                s_statusTag_head = s_statusTag_tail = status_tag;
            }
            else
            {
                s_statusTag_tail->next = status_tag;
                s_statusTag_tail = s_statusTag_tail->next;

            }
           // MMIDM_BuildStatus(test);

    //////////////////////////////////////////////////BEGIN TO CONSTRUCT RESULT  CONTENT//////////////////////////////////////////////////////////////////////////
            result_tag->CmdId.hasChildTag =FALSE;
            result_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            result_tag->CmdId.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL==result_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==result_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(result_tag->CmdId.tagContent, 0, strlen(str)+1);/*lint !e666*/
            SCI_STRCPY(result_tag->CmdId.tagContent, str);

            result_tag->msgRef.hasChildTag = FALSE;
            result_tag->msgRef.tagId=TAG_MSGREFID;
            MMIDM_IU32toa(s_g_MsgID, str, 32);
            result_tag->msgRef.tagContent = SCI_ALLOCA(strlen(str)+1);
            if(PNULL==result_tag->msgRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==result_tag->msgRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(result_tag->msgRef.tagContent, 0, strlen(str)+1);/*lint !e666*/
            SCI_STRCPY(result_tag->msgRef.tagContent, str);

            result_tag->cmdRef.hasChildTag = FALSE;
            result_tag->cmdRef.tagId = TAG_CMDREFID;
            result_tag->cmdRef.tagContent = SCI_ALLOCA(strlen(cmdid)+1);
            if(PNULL==result_tag->cmdRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==result_tag->cmdRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(result_tag->cmdRef.tagContent, 0, strlen(cmdid)+1);/*lint !e666*/
            SCI_STRCPY(result_tag->cmdRef.tagContent, cmdid);

            /////ITEMPTR need alloc  //steven2012
	 scanner_ptr = getbuf;
	  item_buff = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;

          SCI_MEMSET(status_tag->data.tagContent, 0, 10); //add 2012-3-15
          SCI_STRCPY(status_tag->data.tagContent, (char*)"200"); // add 2012-3-15

	  while( (scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_ITEMID, item_buff, MAX_TAG_BUF_SIZE)) != PNULL)
        {

        MMIDM_getNextXmlTagBuf(item_buff, TAG_TARGETID, target,MAX_TARGET_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(target, TAG_LOCURIID, target_uri, MAX_TARGET_BUF_SIZE);
            item_ptr = SCI_ALLOCA(sizeof(DMXML_TAG_ITEM_T));
            if(PNULL== item_ptr)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL== item_ptr");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_ptr, 0X0, sizeof(DMXML_TAG_ITEM_T));
	    SCI_TRACE_LOW("MMIDM_DealWithGetData: target_uri:%s \n", target_uri);
            handletype =  DM_GET_TREE_READFUNC(target_uri);
          if(handletype!=-1){
	     spdm_readCb(handletype, content, 0, 500);
          } else
          	{
                SCI_MEMSET(status_tag->data.tagContent, 0, 10);
                SCI_STRCPY(status_tag->data.tagContent, (char*)"204");
          	}

            item_ptr->source.locuri.hasChildTag = FALSE;
            item_ptr->source.locuri.tagId = TAG_LOCURIID;
            item_ptr->source.locuri.tagContent = SCI_ALLOCA(strlen(target_uri)+1);
            if(PNULL==item_ptr->source.locuri.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==item_ptr->source.locuri.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_ptr->source.locuri.tagContent, 0, strlen(target_uri)+1);/*lint !e666*/
            SCI_STRCPY(item_ptr->source.locuri.tagContent, target_uri);

            item_ptr->meta.format.hasChildTag = FALSE;
            item_ptr->meta.format.tagId = TAG_FORMATID;
            SCI_STRCPY(item_ptr->meta.format.tagArr, (char*)"xmlns=\'syncml:metinf\'");

            item_ptr->data.hasChildTag = FALSE;
            item_ptr->data.tagId = TAG_DATAID;
            item_ptr->data.tagContent = SCI_ALLOCA(strlen(content)+1);
            if(PNULL==item_ptr->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithGetData PNULL==item_ptr->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(item_ptr->data.tagContent, 0, strlen(content)+1);/*lint !e666*/
            SCI_STRCPY(item_ptr->data.tagContent, content);
/*      DM2012  */
		if (result_tag->item_ptr == PNULL)
            result_tag->item_ptr = item_ptr;
		else
			{
			DMXML_TAG_ITEM_T *titemptr = PNULL;
			titemptr = result_tag->item_ptr;
			while(titemptr)
				{
					if (titemptr->next == NULL)
						{
						titemptr->next = item_ptr;
						item_ptr->next = PNULL;
						break;
						}
					titemptr = titemptr->next;
				}
			}
/*      DM2012  */
	  	}
            SCI_FREEAA(item_buff);

            if(s_resultTag_head == PNULL)
            {
                s_resultTag_head = s_resultTag_tail = result_tag;
            }
            else
            {
                s_resultTag_tail->next = result_tag;
                s_resultTag_tail = s_resultTag_tail->next;

            }
           // MMIDM_BuildResult(test);


        }
        else
        {
                SCI_TRACE_LOW("MMIDM_DealWithGetData, PNULL==target_uri");

        }
    } while(0);

    if(PNULL != content)
    {
        SCI_FREEAA(content);
        content = PNULL;
    }
     if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;
}

/*****************************************************************************/
//  Description : parse the the get command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的get命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLGet(char* xmlbuf)
{
    char* body_buf= PNULL;
    char* get_buf= PNULL;
    char* scanner_ptr = PNULL;
    BOOLEAN  ret =TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_ParseXMLGet");

    do {

        body_buf = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
        if(PNULL== body_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLGet PNULL== bodybuf");
            ret =FALSE;
            break;
        }
         SCI_MEMSET(body_buf, 0X0, MAX_XML_BUF_SIZE);

        get_buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
        if(PNULL== get_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLGet PNULL== get_buf");
            ret =FALSE;
            break;
        }
         SCI_MEMSET(get_buf, 0X0, MAX_TAG_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(xmlbuf, TAG_SYNCBODYID, body_buf, MAX_XML_BUF_SIZE);
        scanner_ptr = body_buf;
        while( (scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_GETID, get_buf, MAX_TAG_BUF_SIZE)) != PNULL)
        {
            MMIDM_DealWithGetData(get_buf);
	    SCI_TRACE_LOW("MMIDM_DealWithGetData get_buf %s" ,get_buf);
        }
    } while(0);

     if(PNULL != body_buf)
    {
        SCI_FREEAA(body_buf);
        body_buf = PNULL;
    }
     if(PNULL != get_buf)
    {
        SCI_FREEAA(get_buf);
        get_buf = PNULL;
    }
     if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}


/*****************************************************************************/
//  Description : deal with the replace command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理replace命令，加入replace链表中
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithReplaceData(char* replacebuf)
{
    DMXML_TAG_STATUS_T* status_tag = PNULL;
//    DMXML_TAG_RESULT_T* result_tag = PNULL;
//    DMXML_TAG_ITEM_T* item_ptr = PNULL;
    char* content=PNULL;
    char cmdid[MAX_CMDID_BUF_SIZE + 1]={0};
//    char msg[20]={0};
    char*  scanner_ptr = PNULL;
    char target[MAX_TARGET_BUF_SIZE + 1]={0};
    char target_uri[MAX_TARGET_BUF_SIZE + 1]={0};
   // unsigned char test[9000]={0};
    char* data=PNULL;
    char* item_buff=PNULL;
    char str[20]={0};
    uint16        len = 0;
    //VDM_Tree_WriteExternalCB write_func = PNULL;
    int32 handertype =-1;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_DealWithReplaceData");
    do {
        content = SCI_ALLOCA(MAX_TAG_BUF_SIZE);
        if(PNULL== content)
        {
            SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL== content");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(content, 0X0, MAX_TAG_BUF_SIZE);

        data = SCI_ALLOCA(200);
        if(PNULL== data)
        {
            SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL== data");
            ret = FALSE;
            break;
        }
        SCI_MEMSET(data, 0X0, 200);

        MMIDM_getNextXmlTagBuf(replacebuf, TAG_CMDIDID, cmdid, MAX_CMDID_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(replacebuf, TAG_TARGETID, target, MAX_TARGET_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(target, TAG_LOCURIID, target_uri, MAX_TARGET_BUF_SIZE);
        MMIDM_getNextXmlTagBuf(replacebuf, TAG_DATAID, data, 200);

        if(strlen(target_uri))
        {
            status_tag = SCI_ALLOCA(sizeof(DMXML_TAG_STATUS_T));
            if(PNULL== status_tag)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL== status_tag");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag, 0X0, sizeof(DMXML_TAG_STATUS_T));

          //  write_func = DM_GET_TREE_WRITEFUNC(target_uri);
//            handertype = DM_GET_TREE_WRITEFUNC(target_uri);
            status_tag->CmdId.hasChildTag =FALSE;
            status_tag->CmdId.tagId = TAG_CMDIDID;
            MMIDM_IU32toa(s_g_CmdID, str, 32);
            s_g_CmdID++;
            status_tag->CmdId.tagContent = SCI_ALLOCA(SCI_STRLEN(str)+1);
            if(PNULL==status_tag->CmdId.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL==status_tag->CmdId.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->CmdId.tagContent, 0, (SCI_STRLEN(str)+1)); /*lint !e666*/
            SCI_STRCPY(status_tag->CmdId.tagContent, str);

            status_tag->msgRef.hasChildTag = FALSE;
            status_tag->msgRef.tagId=TAG_MSGREFID;
            MMIDM_IU32toa(s_g_MsgID, str, 32);
            status_tag->msgRef.tagContent = SCI_ALLOCA(SCI_STRLEN(str)+1);
            if(PNULL==status_tag->msgRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL==status_tag->msgRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->msgRef.tagContent, 0, (SCI_STRLEN(str)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->msgRef.tagContent, str);

            status_tag->cmdRef.hasChildTag = FALSE;
            status_tag->cmdRef.tagId = TAG_CMDREFID;
            status_tag->cmdRef.tagContent = SCI_ALLOCA(SCI_STRLEN(cmdid)+1);
            if(PNULL==status_tag->cmdRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL==status_tag->cmdRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->cmdRef.tagContent, 0, (SCI_STRLEN(cmdid)+1)); /*lint !e666*/
            SCI_STRCPY(status_tag->cmdRef.tagContent, cmdid);

            status_tag->targetRef.hasChildTag = FALSE;
            status_tag->targetRef.tagId = TAG_TARGETREFID;
            status_tag->targetRef.tagContent = SCI_ALLOCA(SCI_STRLEN(target_uri)+1);
            if(PNULL==status_tag->targetRef.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL==status_tag->targetRef.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->targetRef.tagContent, 0, (SCI_STRLEN(target_uri)+1));/*lint !e666*/
            SCI_STRCPY(status_tag->targetRef.tagContent, target_uri);

            status_tag->cmd.hasChildTag = FALSE;
            status_tag->cmd.tagId = TAG_CMDID;
            status_tag->cmd.tagContent = SCI_ALLOCA(10);
            if(PNULL==status_tag->cmd.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL==status_tag->cmd.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->cmd.tagContent, 0, 10);
            SCI_STRCPY(status_tag->cmd.tagContent, (char*)"Replace");

/***  add by Hong begin                                                         ***/
            scanner_ptr = replacebuf;
            item_buff = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;

            status_tag->data.tagContent = SCI_ALLOCA(10);
            if(PNULL==status_tag->data.tagContent)
            {
                SCI_TRACE_LOW("MMIDM_DealWithReplaceData PNULL==status_tag->data.tagContent");
                ret = FALSE;
                break;
            }
            SCI_MEMSET(status_tag->data.tagContent, 0, 10);

            if(1)
            {
                SCI_STRCPY(status_tag->data.tagContent, (char*)"200");
            }
            else
            {
                SCI_STRCPY(status_tag->data.tagContent, (char*)"204");
            }

            while( (scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_ITEMID, item_buff, MAX_TAG_BUF_SIZE)) != PNULL)
            {
                SCI_MEMSET(target_uri, 0, 100);
                SCI_MEMSET(data, 0, 200);
                MMIDM_getNextXmlTagBuf(item_buff, TAG_TARGETID, target, 100);
                MMIDM_getNextXmlTagBuf(target, TAG_LOCURIID, target_uri, 100);
                MMIDM_getNextXmlTagBuf(item_buff, TAG_DATAID, data, 200);

                SCI_TRACE_LOW("DM Repace target_uri:%s data:%s \n", target_uri, data);
                handertype = DM_GET_TREE_WRITEFUNC(target_uri);
/***  add by Hong end                                                         ***/

            //if(PNULL!=write_func)
                if(-1!=handertype)
                {
                    status_tag->data.hasChildTag = FALSE;
                    status_tag->data.tagId = TAG_DATAID;
                    spdm_writeCb(handertype, data, 0, strlen(data));
                }
                else
                {
                    SCI_TRACE_LOW("MMIDM_DealWithGetData, PNULL==read_func");
                    status_tag->data.hasChildTag = FALSE;
                    status_tag->data.tagId = TAG_DATAID;
                    SCI_MEMSET(status_tag->data.tagContent, 0, 10);
                    SCI_STRCPY(status_tag->data.tagContent, (char*)"204");
                }
	   }
           if(s_statusTag_head == PNULL)
           {
               s_statusTag_head = s_statusTag_tail = status_tag;
           }
           else
           {
               s_statusTag_tail->next = status_tag;
               s_statusTag_tail = s_statusTag_tail->next;

           }
          //  MMIDM_BuildStatus(test);
        }

    } while(0);

     if(PNULL != content)
    {
        SCI_FREEAA(content);
        content = PNULL;
    }
     if(PNULL != data)
    {
        SCI_FREEAA(data);
        data = PNULL;
    }
	 if (item_buff != PNULL)
	 SCI_FREEAA(item_buff);
     if(!ret)
     {
        // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
     }
     return ret;
}


/*****************************************************************************/
//  Description : parse the the replace command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的replace命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLReplace(char* xmlbuf)
{
    char* body_buf=PNULL;
    char* replace_buf=PNULL;
    char* scanner_ptr = PNULL;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_ParseXMLReplace");

    do {

        body_buf = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
        if(PNULL== body_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLReplace PNULL== bodybuf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(body_buf, 0X0, MAX_XML_BUF_SIZE);

        replace_buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
        if(PNULL== replace_buf)
        {
            SCI_TRACE_LOW("MMIDM_ParseXMLReplace PNULL== replace_buf");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(replace_buf, 0X0, MAX_TAG_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(xmlbuf, TAG_SYNCBODYID, body_buf, MAX_XML_BUF_SIZE);
        scanner_ptr = body_buf;

        while((scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_REPLACEID, replace_buf, MAX_TAG_BUF_SIZE))!=PNULL)
        {
            MMIDM_DealWithReplaceData(replace_buf);
        }

    } while(0);


    if(PNULL != body_buf)
    {
        SCI_FREEAA(body_buf);
        body_buf = PNULL;
    }
     if(PNULL != replace_buf)
    {
        SCI_FREEAA(replace_buf);
        replace_buf = PNULL;
    }
     if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}

/*****************************************************************************/
//  Description : for the alert confirm window to callback which result the user choose
//  Global resource dependence :
//  iscontinue: true: yes
//              false: no
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
 SPRD_DM_PARSE_RESULT MMIDM_NotifyAlertResult(BOOLEAN iscontinue)
{
        SCI_TRACE_LOW("ENTER MMIDM_NotifyAlertResult");
        //unsigned char test[9000]={0};
        if(PNULL == s_statusTag_cur)
        {
            SCI_TRACE_LOW("MMIDM_NotifyAlertResult PNULL == s_statusTag_cur");
        }
        s_statusTag_cur->data.hasChildTag = FALSE;
        s_statusTag_cur->data.tagId = TAG_DATAID;
        s_statusTag_cur->data.tagContent = SCI_ALLOCA(10);
        if(PNULL==  s_statusTag_cur->data.tagContent)
        {
            SCI_TRACE_LOW("MMIDM_NotifyAlertResult PNULL==s_statusTag_cur->data.tagContent");
            //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
            spdm_stopDm(SPRD_DM_PARSE_ERROR);
            return SPRD_DM_PARSE_RESULT_EXIT;
        }
        SCI_MEMSET(s_statusTag_cur->data.tagContent, 0, 10);
        if(iscontinue)
        {
            SCI_STRCPY(s_statusTag_cur->data.tagContent,(char*)"200");
        }
        else
        {
            SCI_STRCPY(s_statusTag_cur->data.tagContent, (char*)"304");
        }

     //   MMIDM_BuildStatus(test);
        s_g_needreplay = FALSE;
       // MMIDM_generateXMLData();
       //MMIDM_SendDmData();
         return SPRD_DM_PARSE_RESULT_START;
      //  MMIDM_SendSigToDmTask(DM_TASK_START_MESSAGE,MMIDM_GetDmTaskID(),PNULL);

}
/*****************************************************************************/
//  Description : deal with the alert command data
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 处理get命令，
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_DealWithAlertData(char* alertbuf)
{

    char data[10]={0};
    char cmdid[MAX_CMDID_BUF_SIZE + 1]={0};
    char* subdata=PNULL;
    char* item=PNULL;
    char minitime[80]={0};
    char maxtime[80]={0};
    char str[50]={0};
    char* scanner_ptr = PNULL;
    char* mintimer_ptr = PNULL;
    char* maxtimer_ptr = PNULL;
    DMXML_TAG_STATUS_T* status_tag = PNULL;
    int i = 0;
    BOOLEAN     retvalue = FALSE;
    BOOLEAN     ret  = TRUE;


    SCI_TRACE_LOW("ENTER MMIDM_DealWithAlertData");

    do {

        item = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
        if(PNULL== item)
        {
            SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL== item");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(item, 0X0, MAX_TAG_BUF_SIZE);

        subdata = SCI_ALLOCA(200) ;
        if(PNULL== subdata)
        {
            SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL== subdata");
            ret = FALSE;
            break;
        }
         SCI_MEMSET(item, 0X0, 200);

        MMIDM_getNextXmlTagBuf(alertbuf, TAG_CMDIDID, cmdid, MAX_CMDID_BUF_SIZE);
        scanner_ptr = alertbuf;

        while( ( scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_DATAID, data, 10))!=PNULL)
        {
            if(!strcmp(data, "1101"))//现在只支持1101
            {
                SCI_TRACE_LOW("MMIDM_DealWithAlertData data");
                break;
            }
            else if(!strcmp(data, "1223"))
            {
                ret = FALSE;
                break;
            }

        }

        if(PNULL != scanner_ptr)
        {
            scanner_ptr = alertbuf;

            while((scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_ITEMID, item,MAX_TAG_BUF_SIZE))!=PNULL)
            {
                MMIDM_getNextXmlTagBuf(item, TAG_DATAID, subdata, 200);
                mintimer_ptr = strstr(subdata, "MINDT=");
                if(mintimer_ptr!=NULL)/*lint !e720*/
                {
                    i=0;
                    SCI_STRCPY(minitime, mintimer_ptr+strlen("MINDT="));/*lint !e666*/
                    while(minitime[i])
                    {
                        if((minitime[i]-'0')>9 || (minitime[i]-'0')<0)
                        {
                            minitime[i] = 0;
                            break;
                        }
                        i++;
                    }
                    SCI_TRACE_LOW("MMIDM_DealWithAlertData minimum display time %s", minitime);

                }
                maxtimer_ptr = strstr(subdata, "MAXDT=");
                if(maxtimer_ptr!=PNULL)/*lint !e720*/
                {
                    i= 0;
                    SCI_STRCPY(maxtime, maxtimer_ptr+strlen("MAXDT="));/*lint !e666*/
                    while(maxtime[i] !=0)
                    {
                        if((maxtime[i]-'0')>9 || (maxtime[i]-'0')<0)
                        {
                            maxtime[i] = 0;
                            break;
                        }
                        i++;
                    }
                    SCI_TRACE_LOW("MMIDM_DealWithAlertData maximum display time %s", maxtime);
                }
                if((PNULL == maxtimer_ptr) && (PNULL == mintimer_ptr) )
                {
                    SCI_TRACE_LOW("MMIDM_DealWithAlertData display content =%s", subdata);

                    //VDM_PL_atoIU32(maxtime, 32, &ret);
                    //MMIDM_OpenDmSessionAlertWin(MMIDM_ALERT_PL_CONFIRM,subdata, strlen(subdata), MMIDM_atoIU32(maxtime, 10, &retvalue));/*lint !e64*/
			spdm_task_need_confirm_msg(subdata, strlen(subdata),MMIDM_atoIU32(maxtime, 10, &retvalue));
                    status_tag = SCI_ALLOCA(sizeof(DMXML_TAG_STATUS_T));
                    if(PNULL== status_tag)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL== status_tag");
                         ret = FALSE;
                         break;
                    }
                    SCI_MEMSET(status_tag, 0X0, sizeof(DMXML_TAG_STATUS_T));


                    status_tag->CmdId.hasChildTag =FALSE;
                    status_tag->CmdId.tagId = TAG_CMDIDID;
                    MMIDM_IU32toa(s_g_CmdID, str, 32);
                    s_g_CmdID++;
                    status_tag->CmdId.tagContent = SCI_ALLOCA(strlen(str)+1);
                    if(PNULL==status_tag->CmdId.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL==status_tag->CmdId.tagContent");
                         ret = FALSE;
                         break;

                    }
                    SCI_MEMSET(status_tag->CmdId.tagContent, 0, (strlen(str)+1));  /*lint !e666*/
                    SCI_STRCPY(status_tag->CmdId.tagContent, str);

                    status_tag->msgRef.hasChildTag = FALSE;
                    status_tag->msgRef.tagId=TAG_MSGREFID;
                    MMIDM_IU32toa(s_g_MsgID, str, 32);
                    status_tag->msgRef.tagContent = SCI_ALLOCA(strlen(str)+1);
                    if(PNULL==status_tag->msgRef.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL==status_tag->msgRef.tagContent");
                         ret = FALSE;
                         break;

                    }
                    SCI_MEMSET(status_tag->msgRef.tagContent, 0, (strlen(str)+1));/*lint !e666*/
                    SCI_STRCPY(status_tag->msgRef.tagContent, str);

                    status_tag->cmdRef.hasChildTag = FALSE;
                    status_tag->cmdRef.tagId = TAG_CMDREFID;
                    status_tag->cmdRef.tagContent = SCI_ALLOCA(strlen(cmdid)+1);/*lint !e666*/
                    if(PNULL==status_tag->cmdRef.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL==status_tag->cmdRef.tagContent");
                         ret = FALSE;
                         break;

                    }
                    SCI_MEMSET(status_tag->cmdRef.tagContent, 0, (strlen(cmdid)+1));/*lint !e666*/
                    SCI_STRCPY(status_tag->cmdRef.tagContent, cmdid);

                    status_tag->cmd.hasChildTag = FALSE;
                    status_tag->cmd.tagId = TAG_CMDID;
                    status_tag->cmd.tagContent = SCI_ALLOCA(10);
                    if(PNULL==status_tag->cmd.tagContent)
                    {
                        SCI_TRACE_LOW("MMIDM_DealWithAlertData PNULL==status_tag->cmd.tagContent");
                         ret = FALSE;
                         break;

                    }
                    SCI_MEMSET(status_tag->cmd.tagContent, 0, 10);
                    SCI_STRCPY(status_tag->cmd.tagContent, (char*)"Alert");

                    s_g_needreplay = TRUE;
                    if(s_statusTag_head == PNULL)
                    {
                        s_statusTag_head = s_statusTag_tail = status_tag;
                    }
                    else
                    {
                        s_statusTag_tail->next = status_tag;
                        s_statusTag_tail = s_statusTag_tail->next;
                        s_statusTag_cur = s_statusTag_tail;

                    }

                }
            }
        }
    } while(0);

    if(PNULL != item)
    {
        SCI_FREEAA(item);
        item = PNULL;
    }
     if(PNULL != subdata)
    {
        SCI_FREEAA(subdata);
        subdata = PNULL;
    }
     if(!ret)
    {
        //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
        spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}

/*****************************************************************************/
//  Description : parse the the alert command
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析xml数据中的alert命令
/*****************************************************************************/
LOCAL BOOLEAN MMIDM_ParseXMLAlert(char* xmlbuf)
{
    char* body_buf=PNULL;
    char* alert_buf=PNULL;
//    char buf_cmdref[10] ={0};
    char* scanner_ptr = PNULL;
    BOOLEAN ret = TRUE;

    SCI_TRACE_LOW("ENTER MMIDM_ParseXMLAlert");

    do {

        body_buf = SCI_ALLOCA(MAX_XML_BUF_SIZE) ;
            if(PNULL== body_buf)
            {
                SCI_TRACE_LOW("MMIDM_ParseXMLAlert PNULL== bodybuf");
                ret =FALSE;
                break;
            }
         SCI_MEMSET(body_buf, 0X0, MAX_XML_BUF_SIZE);

        alert_buf = SCI_ALLOCA(MAX_TAG_BUF_SIZE) ;
            if(PNULL== alert_buf)
            {
                SCI_TRACE_LOW("MMIDM_ParseXMLAlert PNULL== alert_buf");
                ret =FALSE;
                break;
            }
         SCI_MEMSET(alert_buf, 0X0, MAX_TAG_BUF_SIZE);

        MMIDM_getNextXmlTagBuf(xmlbuf, TAG_SYNCBODYID, body_buf, MAX_XML_BUF_SIZE);
        scanner_ptr = body_buf;

        while( (scanner_ptr = MMIDM_getNextXmlTagBuf(scanner_ptr, TAG_ALERTID, alert_buf, MAX_TAG_BUF_SIZE))!=PNULL)
        {
            MMIDM_DealWithAlertData(alert_buf);
        }

    } while(0);


    if(PNULL != alert_buf)
    {
        SCI_FREEAA(alert_buf);
        alert_buf = PNULL;
    }
     if(PNULL != body_buf)
    {
        SCI_FREEAA(body_buf);
        body_buf = PNULL;
    }
     if(!ret)
    {
       // MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
    }
    return ret;

}

/*****************************************************************************/
//  Description : release all the node malloc
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_releaseItemContent(DMXML_TAG_ITEM_T* item_tag)
{
    if(PNULL != item_tag->data.tagContent)
    {
        SCI_FREEAA(item_tag->data.tagContent);
        item_tag->data.tagContent = PNULL;
    }
     if(PNULL != s_resultTag_head->item_ptr->meta.format.tagContent)
    {
        SCI_FREEAA(item_tag->meta.format.tagContent);
        item_tag->meta.format.tagContent = PNULL;
    }
     if(PNULL != item_tag->meta.nextnonce.tagContent)
    {
        SCI_FREEAA(item_tag->meta.nextnonce.tagContent);
        item_tag->meta.nextnonce.tagContent = PNULL;
    }
     if(PNULL != item_tag->meta.type.tagContent)
    {
        SCI_FREEAA(item_tag->meta.type.tagContent);
        item_tag->meta.type.tagContent = PNULL;
    }
     if(PNULL != item_tag->source.locname.tagContent)
    {
        SCI_FREEAA(item_tag->source.locname.tagContent);
        item_tag->source.locname.tagContent = PNULL;
    }
     if(PNULL != item_tag->source.locuri.tagContent)
    {
        SCI_FREEAA(item_tag->source.locuri.tagContent);
        item_tag->source.locuri.tagContent = PNULL;
    }
}


LOCAL void MMIDM_releaseResultContent(DMXML_TAG_RESULT_T* result_tag)
{
    if(PNULL != result_tag->cmd.tagContent)
    {
        SCI_FREEAA(result_tag->cmd.tagContent);
        result_tag->cmd.tagContent = PNULL;
    }
    if(PNULL != result_tag->CmdId.tagContent)
    {
        SCI_FREEAA(result_tag->CmdId.tagContent);
        result_tag->CmdId.tagContent = PNULL;
    }
    if(PNULL != result_tag->cmdRef.tagContent)
    {
        SCI_FREEAA(result_tag->cmdRef.tagContent);
        result_tag->cmdRef.tagContent = PNULL;
    }
    if(PNULL != result_tag->msgRef.tagContent)
    {
        SCI_FREEAA(result_tag->msgRef.tagContent);
        result_tag->msgRef.tagContent = PNULL;
    }
    if(PNULL != result_tag->sourceRef.tagContent)
    {
        SCI_FREEAA(result_tag->sourceRef.tagContent);
        result_tag->sourceRef.tagContent = PNULL;
    }
    if(PNULL != result_tag->targetRef.tagContent)
    {
        SCI_FREEAA(result_tag->targetRef.tagContent);
        result_tag->targetRef.tagContent = PNULL;
    }
}

LOCAL void MMIDM_releaseStatusContent(DMXML_TAG_STATUS_T* status_tag)
{
        if(PNULL != status_tag->cmd.tagContent)
        {
            SCI_FREEAA(status_tag->cmd.tagContent);
            status_tag->cmd.tagContent = PNULL;
        }
        if(PNULL != status_tag->CmdId.tagContent)
        {
            SCI_FREEAA(status_tag->CmdId.tagContent);
            status_tag->CmdId.tagContent = PNULL;
        }
        if(PNULL != status_tag->cmdRef.tagContent)
        {
            SCI_FREEAA(status_tag->cmdRef.tagContent);
            status_tag->cmdRef.tagContent = PNULL;
        }
        if(PNULL != status_tag->data.tagContent)
        {
            SCI_FREEAA(status_tag->data.tagContent);
            status_tag->data.tagContent = PNULL;
        }
        if(PNULL != status_tag->msgRef.tagContent)
        {
            SCI_FREEAA(status_tag->msgRef.tagContent);
            status_tag->msgRef.tagContent = PNULL;
        }
        if(PNULL != status_tag->sourceRef.tagContent)
        {
            SCI_FREEAA(status_tag->sourceRef.tagContent);
            status_tag->sourceRef.tagContent = PNULL;
        }
        if(PNULL != status_tag->targetRef.tagContent)
        {
            SCI_FREEAA(status_tag->targetRef.tagContent);
            status_tag->targetRef.tagContent = PNULL;
        }
        if(PNULL != status_tag->chal.meta.format.tagContent)
        {
            SCI_FREEAA(status_tag->chal.meta.format.tagContent);
            status_tag->chal.meta.format.tagContent = PNULL;
        }
        if(PNULL != status_tag->chal.meta.nextnonce.tagContent)
        {
            SCI_FREEAA(status_tag->chal.meta.nextnonce.tagContent);
            status_tag->chal.meta.nextnonce.tagContent = PNULL;
        }
        if(PNULL != status_tag->chal.meta.type.tagContent)
        {
            SCI_FREEAA(status_tag->chal.meta.type.tagContent);
            status_tag->chal.meta.type.tagContent = PNULL;
        }
}

LOCAL void MMIDM_releaseReplaceContent(DMXML_TAG_REPLACE_T* replace_tag)
{
        if(PNULL != replace_tag->CmdId.tagContent)
        {
            SCI_FREEAA(replace_tag->CmdId.tagContent);
            replace_tag->CmdId.tagContent = PNULL;
        }
}
/*****************************************************************************/
//  Description : release all the node malloc
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
LOCAL void MMIDM_ReleaseXMLData(void)
{
    DMXML_TAG_RESULT_T* result_cur_tag = PNULL;
    DMXML_TAG_STATUS_T* status_cur_tag = PNULL;
    DMXML_TAG_REPLACE_T* replace_cur_tag = PNULL;
    DMXML_TAG_ITEM_T*       item_tag = PNULL;

    SCI_TRACE_LOW("ENTER MMIDM_ReleaseXMLData");
    //release result tag buf
    while(PNULL!=s_resultTag_head)
    {
        while(PNULL!=s_resultTag_head->item_ptr)
        {

            item_tag = s_resultTag_head->item_ptr->next;
            MMIDM_releaseItemContent(s_resultTag_head->item_ptr);
            SCI_FREEAA(s_resultTag_head->item_ptr);
            s_resultTag_head->item_ptr= item_tag;
        }

        result_cur_tag = s_resultTag_head->next;
        MMIDM_releaseResultContent(s_resultTag_head);
        SCI_FREEAA(s_resultTag_head);
        s_resultTag_head= result_cur_tag;
    }

    //release status tag buf
    while(PNULL!=s_statusTag_head)
    {

        status_cur_tag = s_statusTag_head->next;
        MMIDM_releaseStatusContent(s_statusTag_head);
        SCI_FREEAA(s_statusTag_head);
        s_statusTag_head= status_cur_tag;
    }

    //release replace tag buf
    while(PNULL!=s_replaceTag_head)
    {
        while(PNULL!=s_replaceTag_head->item_ptr)
        {
            item_tag = s_replaceTag_head->item_ptr->next;
            MMIDM_releaseItemContent(s_replaceTag_head->item_ptr);
            SCI_FREEAA(s_replaceTag_head->item_ptr);
            s_replaceTag_head->item_ptr= item_tag;
        }
        replace_cur_tag = s_replaceTag_head->next;
        MMIDM_releaseReplaceContent(s_replaceTag_head);
        SCI_FREEAA(s_replaceTag_head);
        s_replaceTag_head= replace_cur_tag;
    }
    SCI_TRACE_LOW("LEAVE MMIDM_ReleaseXMLData");
}


/*****************************************************************************/
//  Description : parse the receive xml content
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note: 解析接受到的xml数据
/*****************************************************************************/
LOCAL void MMIDM_ParseXMLData(char* xmlbuf)
{
   char resuri_buf[128]={0};
//   char msg[10]={0};
   char sessionid[10]={0};
   char str[10]={0};

   SCI_TRACE_LOW("enter MMIDM_ParseXMLData");

   MMIDM_getNextXmlTagBuf(xmlbuf, TAG_SESSIONID, sessionid, 10);
   MMIDM_IU32toa(s_g_SessionID, str, 32);
   if(strcmp(str, sessionid))
   {
       SCI_TRACE_LOW("MMIDM_ParseXMLData, the wrong sessionid %s", sessionid);
       //MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       spdm_stopDm(SPRD_DM_PARSE_ERROR);
       return;
   }

/*   MMIDM_getNextXmlTagBuf(xmlbuf, TAG_MSGID, msg, 10);
   MMIDM_IU32toa(s_g_MsgID, str, 32);
   if(strcmp(str, msg))
   {
       SCI_TRACE_LOW("MMIDM_ParseXMLData, not the msg rely to our msg %s", msg);
       MMIDM_SendSigToDmTask(DM_TASK_DM_CLOSE,MMIDM_GetDmTaskID(),PNULL);
       return;
   }*/



 //  if(s_g_step == STEP_GETNONCE)
   {
     if(MMIDM_getNextXmlTagBuf(xmlbuf, TAG_RESURIID,resuri_buf, 128))
     {
         MMIDM_setResUri(resuri_buf);
     }
/********* @hong2012 add **********/
     if(MMIDM_getNextXmlTagBuf(xmlbuf, TAG_MSGID,str, 10))
     {
     BOOLEAN result;
	s_g_MsgID = MMIDM_atoIU32(str,32, &result);
      SCI_TRACE_LOW("MMIDM_ParseXMLData msgid:%s result:%d",str,result);
//     MMIDM_IU32toa(s_g_MsgID, str, 32);
     }
/********* @hong2012 add **********/

   }
     MMIDM_ReleaseXMLData();
     SCI_TRACE_LOW("enter MMIDM_ParseXMLData 1");
     MMIDM_ParseXMLStatus(xmlbuf);
     SCI_TRACE_LOW("enter MMIDM_ParseXMLData 2");
     MMIDM_ParseXMLGet(xmlbuf);
     SCI_TRACE_LOW("enter MMIDM_ParseXMLData 3");
     MMIDM_ParseXMLAlert(xmlbuf);
     SCI_TRACE_LOW("enter MMIDM_ParseXMLData 4");
     MMIDM_ParseXMLReplace(xmlbuf);
     SCI_TRACE_LOW("enter MMIDM_ParseXMLData 5");
     MMIDM_ParseXMLExec(xmlbuf);


}


/*****************************************************************************/
//  Description : 产生xml包并发送
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
 SPRD_DM_PARSE_RESULT MMIDM_GenerateDmData( char* sendbuf , uint32* bufflen)
{
    if(PNULL== sendbuf || (* bufflen ) < MAX_XML_BUF_SIZE)
    {
        SCI_TRACE_LOW("MMIDM_SendDmData PNULL== sendbuf");
        return SPRD_DM_PARSE_RESULT_FAILURE;
    }

    MMIDM_generateXMLData(sendbuf);
    //MMIDM_PrintTrace(sendbuf, strlen(sendbuf));
    //SCI_TRACE_LOW("MMIDM_GenerateDmData %s",sendbuf);
    (*bufflen) = strlen(sendbuf);

    if(s_g_step == STEP_ABORT)
    {
       //  MMIDM_SendSigToDmTask(DM_TASK_DM_EXIT,MMIDM_GetDmTaskID(),PNULL);//maryxiao111
	return 	SPRD_DM_PARSE_RESULT_EXIT;
    }
 SCI_TRACE_LOW("MMIDM_GenerateDmData SPRD_DM_PARSE_RESULT_OK");
    return 	SPRD_DM_PARSE_RESULT_OK;
}
/*****************************************************************************/
//  Description : 结束dm
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
 void MMIDM_EndDm(void)
{
   MMIDM_ReleaseXMLData();//free the memory alloc the tag content
   MMIDM_clearDmXmlData();
#if LOCAL_MEMPOOL   
   if (s_g_memptr != NULL)
     	{
	  	 free(s_g_memptr);
	  	 s_g_memptr = NULL;
		s_xml_memptr = NULL;	
		s_oth_memptr = NULL;
     	}
	s_g_mem_size = 0;
	s_g_mem_used = 0;     
#endif	
}

/*****************************************************************************/
//  Description : 开启一个dm的http
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
 void MMIDM_BeginDmTansport(void)
{
#if 0
    int inConnId = -1;
    SCI_TRACE_LOW("MMIDM_BeginDmTansport");
    if(DM_CANCEL == MMIDM_GetDmState())
    {
        SCI_TRACE_LOW("MMIDM_BeginDmTansport DM_CANCEL == MMIDM_GetDmState()");
        return;
    }

    VDM_Comm_PL_HTTP_open(PNULL, &inConnId, (unsigned char*)MMIDM_getResUri(), 0, 0, PNULL);/*lint !e718 !e18 */
    MMIDM_CreateDmSocketCheckTimer();
#endif
}


/*****************************************************************************/
//  Description : 解析收到的xml包，放入相应链表
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
 SPRD_DM_PARSE_RESULT MMIDM_ParseReceiveData(char* data)
{

    MMIDM_ParseXMLData(data);
    //s_g_MsgID++;  //@hong2012

    if((s_g_step == STEP_CREDED) && (PNULL == s_alerTag_head) && (PNULL == s_replaceTag_head) && (PNULL == s_statusTag_head->next  ) && (PNULL == s_resultTag_head))
    {
        SCI_TRACE_LOW("MMIDM_ParseReceiveData NO PACKET TO SEND");
        if(s_g_callClearFunc)
        {
            s_g_callClearFunc = FALSE;
            //DM_ConfirmFactoryReset();
        }
        if(s_g_callLockFunc)
        {
            s_g_callLockFunc = FALSE;
            //DM_ConfirmPartiallyLock();
        }
        if(s_g_callUnlockFunc)
        {
            s_g_callUnlockFunc = FALSE;
            //DM_ConfirmUnLock();
        }

        return 	SPRD_DM_PARSE_RESULT_EXIT;
       //MMIDM_SendSigToDmTask(DM_TASK_DM_EXIT,MMIDM_GetDmTaskID(),PNULL);//maryxiao111
    }


    if(!s_g_needreplay)
    {
        return 	SPRD_DM_PARSE_RESULT_START;
       // MMIDM_SendSigToDmTask(DM_TASK_START_MESSAGE,MMIDM_GetDmTaskID(),PNULL);
    }

    return 	SPRD_DM_PARSE_RESULT_OK;

}

/*****************************************************************************/
//  Description : 打印trace的函数
//  Global resource dependence :
//  Author:mary.xiao
//  Modify:
//  Note:
/*****************************************************************************/
 void MMIDM_PrintTrace(char*buf, int size)
{
    int i =0;

    for(i=0; i<size;i+=6)
    {
       SCI_TRACE_LOW("mmidm %c%c%c%c%c%c",buf[i],buf[i+1],buf[i+2],buf[i+3],buf[i+4],buf[i+5]);

    }
}

