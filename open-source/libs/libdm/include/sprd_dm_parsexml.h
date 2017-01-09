#ifndef DM_PARSEXML_H_
#define DM_PARSEXML_H_


//#include "sci_types.h"

#ifdef __cplusplus
    extern   "C"
    {
#endif
#define BOOLEAN int
#define uint32 unsigned int


#define RDM_MAX_SEND_SIZE      (8*1024)
#define MAX_RESURI_LEN  128
#define MAX_NONCE_LEN  64
/*****************************************************************************/
//  Description : THE STEP DM SESSION IS IN

/*****************************************************************************/
typedef enum
{
    STEP_GETNONCE = 0,
    STEP_INIT,
    STEP_CREDED,
    STEP_ABORT,
} MMI_DM_STEP_E;

/*****************************************************************************/
//  Description : THE TAG SYNCML WILL BE USE
/*****************************************************************************/
typedef enum {
    TAG_VERDTD = 0,
    TAG_VERPROTO,
    TAG_SESSIONID,
    TAG_MSGID,
    TAG_TARGETID,
    TAG_SOURCEID,
    TAG_LOCURIID,
    TAG_LOCNAMEID,
    TAG_CREDID,
    TAG_METAID,
    TAG_MAXMSGID,
    TAG_MAXOBJID,
    TAG_DATAID,
    TAG_SYNCHDRID,
    TAG_SYNCBODYID,
    TAG_ALERTID,
    TAG_REPLACEID,
    TAG_STATUSID,
    TAG_RESULTID,
    TAG_CMDID,
    TAG_CMDIDID,
    TAG_MSGREFID,
    TAG_CMDREFID,
    TAG_TARGETREFID,
    TAG_SOURCEREFID,
    TAG_CHALID,
    TAG_ITEMID,
    TAG_FORMATID,
    TAG_TYPEID,
    TAG_NEXTNONCEID,
    TAG_RESURIID,
    TAG_GETID,
    TAG_SYNCMLID,
    TAG_EXECID,
} MMI_DM_TAGID_E;

#define DM_TAGTYPE_LEN       16

/*****************************************************************************/
//  Description : THE BASIC TAG TYPE STRUCT
/*****************************************************************************/
typedef struct
{
    char                     tagtype[DM_TAGTYPE_LEN];
    MMI_DM_TAGID_E                tagid;
} DMXML_TAG_INFO_T;


typedef  enum{
	SPRD_DM_PARSE_RESULT_OK = 0,
	SPRD_DM_PARSE_RESULT_FAILURE,
	SPRD_DM_PARSE_RESULT_UNKOWN,
	SPRD_DM_PARSE_RESULT_START,
       SPRD_DM_PARSE_RESULT_EXIT,
}SPRD_DM_PARSE_RESULT;


#define MAX_XML_BUF_SIZE   (1024*12)
#define MAX_TAG_BUF_SIZE   1024
#define MAX_TAG_ARR_LEN    32

#define MAX_TARGET_BUF_SIZE 128
#define MAX_CMDID_BUF_SIZE 32

#define DM_SESSIONID_1        27
#define DM_SESSIONID_2        28
#define DM_ALERT_ID        23
/*****************************************************************************/
//  Description : THE BASIC TAG STRUCT 
/*****************************************************************************/
typedef struct
{   
    MMI_DM_TAGID_E                   tagId;
    char*               tagContent;
    char                tagArr[MAX_TAG_ARR_LEN];  
    BOOLEAN                      hasChildTag; 
} DMXML_TAG_T;
/*****************************************************************************/
//  Description : THE META TAG STRUCT 
/*****************************************************************************/
typedef struct
{
    DMXML_TAG_T                   format;
    DMXML_TAG_T                   type;
    DMXML_TAG_T                   nextnonce;
} DMXML_TAG_META_T;
/*****************************************************************************/
//  Description : THE CHAL TAG STRUCT 
/*****************************************************************************/
typedef struct
{
    DMXML_TAG_META_T             meta;  
} DMXML_TAG_CHAL_T;
/*****************************************************************************/
//  Description : THE SOURCE TAG STRUCT 
/*****************************************************************************/
typedef struct
{
    DMXML_TAG_T                   locuri;
    DMXML_TAG_T                   locname;
} DMXML_TAG_SOURCE_T;
/*****************************************************************************/
//  Description : THE ITEM TAG STRUCT 
/*****************************************************************************/
typedef struct DMXML_TAG_ITEM
{
    DMXML_TAG_SOURCE_T            source;
    DMXML_TAG_META_T              meta;
    DMXML_TAG_T                   data;
    struct DMXML_TAG_ITEM*        next;
} DMXML_TAG_ITEM_T;
/*****************************************************************************/
//  Description : THE ALERT TAG STRUCT 
/*****************************************************************************/
typedef struct DMXML_TAG_ALERT
{
    DMXML_TAG_T                   CmdId;
    DMXML_TAG_T                   DATA;
    struct DMXML_TAG_ALERT*        next;
} DMXML_TAG_ALERT_T;

/*****************************************************************************/
//  Description : THE STATUS TAG STRUCT 
/*****************************************************************************/
typedef struct DMXML_TAG_STATUS
{
    DMXML_TAG_T                   CmdId;
    DMXML_TAG_T                   msgRef;
    DMXML_TAG_T                   cmdRef;
    DMXML_TAG_T                   cmd;
    DMXML_TAG_T                   targetRef;
    DMXML_TAG_T                   sourceRef;
    DMXML_TAG_CHAL_T              chal;
    DMXML_TAG_T                   data;
    struct DMXML_TAG_STATUS*        next;
} DMXML_TAG_STATUS_T;

/*****************************************************************************/
//  Description : THE RESULT TAG STRUCT 
/*****************************************************************************/
typedef struct DMXML_TAG_RESULT
{
    DMXML_TAG_T                   CmdId;
    DMXML_TAG_T                   msgRef;
    DMXML_TAG_T                   cmdRef;
    DMXML_TAG_T                   cmd;
    DMXML_TAG_T                   targetRef;
    DMXML_TAG_T                   sourceRef;
    DMXML_TAG_ITEM_T*              item_ptr;
    struct DMXML_TAG_RESULT*        next;
} DMXML_TAG_RESULT_T;


/*****************************************************************************/
//  Description : THE REPLACE TAG STRUCT 
/*****************************************************************************/

typedef struct DMXML_TAG_REPLACE
{
    DMXML_TAG_T                   CmdId;
    DMXML_TAG_ITEM_T*              item_ptr;
    struct DMXML_TAG_REPLACE*        next;
} DMXML_TAG_REPLACE_T;

/*****************************************************************************/
//  Description : 打印trace的函数
//  Global resource dependence : 
//  Author:mary.xiao
//  Modify:
//  Note: 
/*****************************************************************************/
 void MMIDM_PrintTrace(char*buf, int size);
/*****************************************************************************/
//  Description :设置session的id
//  Global resource dependence : 
//  Author:MARY.XIAO
//  Modify:
//  Note: 
/*****************************************************************************/
 void MMIDM_setSessionId(uint32 id);
/*****************************************************************************/
//  Description :设置session的step
//  Global resource dependence : 
//  Author:MARY.XIAO
//  Modify:
//  Note: 
/*****************************************************************************/
 void MMIDM_setSessionStep(MMI_DM_STEP_E step);
/*****************************************************************************/
//  Description : 结束dm
//  Global resource dependence : 
//  Author:mary.xiao
//  Modify:
//  Note: 
/*****************************************************************************/
 void MMIDM_EndDm(void);
/*****************************************************************************/
//  Description : 产生xml包并发送
//  Global resource dependence : 
//  Author:mary.xiao
//  Modify:
//  Note: 
/*****************************************************************************/
 SPRD_DM_PARSE_RESULT MMIDM_GenerateDmData( char* sendbuf , uint32* bufflen);

/*****************************************************************************/
//  Description : for the alert confirm window to callback which result the user choose
//  Global resource dependence : 
//  iscontinue: true: yes
//              false: no
//  Author:mary.xiao
//  Modify:
//  Note: 
/*****************************************************************************/
 SPRD_DM_PARSE_RESULT MMIDM_NotifyAlertResult(BOOLEAN iscontinue);
/*****************************************************************************/
//  Description : 开启一个dm的http
//  Global resource dependence : 
//  Author:mary.xiao
//  Modify:
//  Note: 
/*****************************************************************************/
 void MMIDM_BeginDmTansport(void);
/*****************************************************************************/
//  Description : 解析收到的xml包，放入相应链表
//  Global resource dependence : 
//  Author:mary.xiao
//  Modify:
//  Note: 
/*****************************************************************************/
 SPRD_DM_PARSE_RESULT MMIDM_ParseReceiveData(char* data);

void MMIDM_setMemPool(char *ptr, unsigned int len);
/******************************************************************* 
** 函数名:mmidm_calc_b64_cred
** 功能描述：计算b64 authentication所用的cred值
** 输  出: 
** 作  者:mary.xiao

*******************************************************************/
 void mmidm_calc_b64_cred(char* creddata, unsigned long cbLength);
/******************************************************************* 
** 函数名:mmidm_calc_md5_cred
** 功能描述：计算md5 authentication所用的cred值
** 输  出: 
** 作  者:MARYXIAO

*******************************************************************/
 char*  mmidm_calc_md5_cred(char* creddata);

char* MMIDM_getResUri(void);
#ifdef   __cplusplus
    }
#endif


#endif