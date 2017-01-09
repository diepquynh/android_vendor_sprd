/*---------------------------------------------------------------------------**
Copyright (C) 2007-2010 ACUTElogic Corp.
ACUTElogic Confidential. All rights reserved.

    <<<<< comply with MISRA standard. >>>>>
**---------------------------------------------------------------------------**
$HeadURL: http://localhost:50120/svn/SPREADTRUM/Projects/15_03_Z3_Android/trunk/Protected/Ais/AisCore/include/AlUtilCmd.h $
$LastChangedDate:: 2015-07-24 14:33:20 #$
$LastChangedRevision: 259 $
$LastChangedBy: seino $
**------------------------------------------------------------------------*//**
コマンド定義ヘッダ


    @file
    @author Masashi Seino
    @date   2007/2/28 start
    @date   
*//*-------------------------------------------------------------------------*/
#ifndef ALUTIL_CMD
#define ALUTIL_CMD
/*===========================================================================**
    Include
**===========================================================================*/
#include "AlTypStd.h" 
#include "AlEmbStd.h" 

/*===========================================================================**
    Macro for Command Header Structure
**===========================================================================*/
#define ALUTIL_CMD_HED_SIZE        (2)
#define ALUTIL_CMD_RSV_SIZE        (2)
#define ALUTIL_CMD_DATA_SIZE       (8)
#define ALUTIL_CMD_PKT_SIZE        (ALUTIL_CMD_FRM_HED_SIZE+ALUTIL_CMD_FRM_RSV_SIZE+ALUTIL_CMD_FRM_DATA_SIZE)
/*------------------------------------------**
    Command Header LSB bit position
**------------------------------------------*/
#define ALUTIL_CMD_CLS_BIT             (12)
#define ALUTIL_CMD_CAT_BIT             (8)
#define ALUTIL_CMD_CMDID_BIT           (0)
#define ALUTIL_CMD_TYP_BIT             (15)
/*------------------------------------------**
    Command Header MASK Value
**------------------------------------------*/
#define ALUTIL_CMD_CLS_MSK             (0x0007 << ALUTIL_CMD_CLS_BIT )
#define ALUTIL_CMD_CAT_MSK             (0x000F << ALUTIL_CMD_CAT_BIT )
#define ALUTIL_CMD_CMDID_MSK           (0x00FF << ALUTIL_CMD_CMDID_BIT)
#define ALUTIL_CMD_TYP_MSK             (0x0001 << ALUTIL_CMD_TYP_BIT )
/*------------------------------------------**
    Respornce Header MASK Value
**------------------------------------------*/
#define ALUTIL_CMD_RES_BIT             (15)
#define ALUTIL_CMD_ERRSTAT_BIT         (0)

/*for RESPONSE*/
#define ALUTIL_CMD_RES_MSK             (0x0001 << ALUTIL_CMD_RES_BIT )
#define ALUTIL_CMD_ERRSTAT_MSK         (0x7FFF << ALUTIL_CMD_ERRSTAT_BIT )
/*===========================================================================**
    typedef for Command Common
**===========================================================================*/
/** コマンド構造型*/
typedef struct tg_AlUtilCmd
{
    UI_16   muiCmdId;       /**< コマンドID */
    UI_16   muiRsved;       /**< 予約領域*/
    UI_08   muiData[8];    /**< コマンドデータ(双方向)*/
}TT_AlUtilCmd,TT_AlUtilRes;

/** コマンド処理関数型*/
#define ALUTIL_CMD_CHECK	(bAlTrue)
#define ALUTIL_CMD_EXEC		(bAlFalse)
/** コマンド処理関数型*/
typedef UI_16 (*TFN_AlCmdProc)(VP_32  ppvRsv,const TT_AlUtilCmd* pptCmd , TT_AlUtilCmd* pptRes);

/** コマンドテーブル要素型*/
typedef struct tg_AlUtilCmdTbl
{
    UI_16           muiId;
    TFN_AlCmdProc   muiProc;
}TT_AlUtilCmdTbl;
/*===========================================================================**
    Command Util Prototype
**===========================================================================*/
/** TYP値の取得 */
extern  UI_16       AlUtilCmd_GetTYP(
                    const UI_16 puiHeader  );
/** CLS値の取得 */
extern  UI_16       AlUtilCmd_GetCLS(
                    const UI_16 puiHeader  );
/** CAT値の取得 */
extern  UI_16       AlUtilCmd_GetCAT(
                    const UI_16 puiHeader  );
/** コマンド番号 */
extern  UI_16       AlUtilCmd_GetCMD(
                    const UI_16 puiHeader  );
/** TYP値の設定 */
extern  UI_16        AlUtilCmd_SetTYP(
                    const UI_16 puiHeader  ,
                    UI_16           auiTyp  );
/** CLS値の設定 */
extern  UI_16        AlUtilCmd_SetCLS(
                    const UI_16 puiHeader  ,
                    UI_16           auiCls  );
/** CAT値の設定 */
extern  UI_16        AlUtilCmd_SetCAT(
                    const UI_16 puiHeader  ,
                    UI_16           auiCat  );
/** CMD値の設定 */
extern  UI_16        AlUtilCmd_SetCMD(
                    const UI_16 puiHeader      ,
                    UI_16           auiCmdId    );

extern  UI_16        AlUtilCmd_SetRES(
                    const UI_16 puiHeader      );
extern  UI_16       AlUtilCmd_GetRES(
                    const UI_16 puiHeader  );
extern  UI_16        AlUtilCmd_SetERRSTAT(
                    const UI_16 puiHeader      ,
                    UI_16           auiCmdId    );
extern  UI_16       AlUtilCmd_GetERRSTAT(
                    const UI_16 puiHeader  );

EXTERN TFN_AlCmdProc    AlUtilCmdSearch(
                        const TT_AlUtilCmd*       pptCmd      ,
                        const TT_AlUtilCmdTbl*    pptCmdIdTbl ,
                        const UI_32                 puiTblSize  );
#endif
