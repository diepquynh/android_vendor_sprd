/*---------------------------------------------------------------------------**
Copyright (C) 2007-2014 ACUTElogic Corp.
ACUTElogic Confidential. All rights reserved.

    <<<<< comply with MISRA standard. >>>>>
**---------------------------------------------------------------------------**
$HeadURL: http://localhost:50120/svn/SPREADTRUM/Projects/15_03_Z3_Android/trunk/Protected/Ais/AisCore/include/AlAisCommand.h $
$LastChangedDate:: 2015-07-24 14:33:20 #$
$LastChangedRevision: 259 $
$LastChangedBy: seino $
**------------------------------------------------------------------------*//**
Command definition header


    @file
    @author Masashi Seino
    @date   2007/02/28 start
    @date   2014/07/07 18:59:44 Update by kawashima( ALCS-N0086 )
    @note   エクセルより生成されるため直接編集不可！
*//*-------------------------------------------------------------------------*/
#ifndef ALAISCOMMAND_H
#define ALAISCOMMAND_H
/*===========================================================================**
    Include
**===========================================================================*/
#include "AlTypStd.h"
#include "AlEmbStd.h"

#include "AlUtilCmd.h"
/*===========================================================================**
    Command Version Infomation
**===========================================================================*/
#define	ALAIS_CMD_VER							((UI_16)0x0100)							/**< コマンド定義Version */


/*===========================================================================**
    UtilLib化に伴うSymbol変更の暫定対応マクロ
**===========================================================================*/
#define   AlAisCmd_GetTYP      AlUtilCmd_GetTYP
#define   AlAisCmd_GetCLS      AlUtilCmd_GetCLS
#define   AlAisCmd_GetCAT      AlUtilCmd_GetCAT
#define   AlAisCmd_GetCMD      AlUtilCmd_GetCMD
#define   AlAisCmd_SetTYP      AlUtilCmd_SetTYP
#define   AlAisCmd_SetCLS      AlUtilCmd_SetCLS
#define   AlAisCmd_SetCAT      AlUtilCmd_SetCAT
#define   AlAisCmd_SetCMD      AlUtilCmd_SetCMD
#define   AlAisCmd_SetRES      AlUtilCmd_SetRES
#define   AlAisCmd_GetRES      AlUtilCmd_GetRES
#define   AlAisCmd_SetERRSTAT  AlUtilCmd_SetERRSTAT
#define   AlAisCmd_GetERRSTAT  AlUtilCmd_GetERRSTAT
#define   AlAisCmdSearch       AlUtilCmdSearch

/** コマンド構造型*/
#define TT_AlAisCmd    TT_AlUtilCmd
#define TT_AlAisRes    TT_AlUtilRes

/** コマンドテーブル要素型*/
#define TT_AlAisCmdTbl TT_AlUtilCmdTbl
/** コマンド関数型*/
#define   TFN_AlAisCmdProc TFN_AlCmdProc

/** コマンド処理関数型*/
#define ALAIS_CMD_CHECK    ALUTIL_CMD_CHECK
#define ALAIS_CMD_EXEC     ALUTIL_CMD_EXEC

/*==================================================================================================**

    Paramter Definition

**==================================================================================================*/
/* ------------------------------------------------------------------------------------------------ */
/*モード定義 */
/* ------------------------------------------------------------------------------------------------ */
//====== FMT
    #define	ALAIS_PRM_MODE_FMT_Draft0				((UI_08)0)								/**< Draft0*/
    #define	ALAIS_PRM_MODE_FMT_Draft2				((UI_08)1)								/**< Draft2*/
    #define	ALAIS_PRM_MODE_FMT_Draft3				((UI_08)2)								/**< Draft3*/
    #define	ALAIS_PRM_MODE_FMT_Draft4				((UI_08)3)								/**< Draft4*/
    #define	ALAIS_PRM_MODE_FMT_FulMon				((UI_08)4)								/**< Full Mon*/
    #define	ALAIS_PRM_MODE_FMT_MAX					((UI_08)5)								/**< MAX Index*/
//====== FPS
    #define	ALAIS_PRM_MODE_FPS_30p					((UI_08)0)								/**< 30FPS*/
    #define	ALAIS_PRM_MODE_FPS_15p					((UI_08)1)								/**< 15FPS*/
    #define	ALAIS_PRM_MODE_FPS_7p5					((UI_08)2)								/**< 7.5FPS*/
    #define	ALAIS_PRM_MODE_FPS_MAX					((UI_08)3)								/**< MAX Index*/
/* ------------------------------------------------------------------------------------------------ */
/*測光枠上位置設定 */
/* ------------------------------------------------------------------------------------------------ */
//====== UPP
    #define	ALAIS_PRM_UPP_UPP_UPP					((UI_08)0)								/**< ブロックの上側*/
    #define	ALAIS_PRM_UPP_UPP_RIGHT					((UI_08)1)								/**< ブロックの右側*/
    #define	ALAIS_PRM_UPP_UPP_LOW					((UI_08)2)								/**< ブロックの下側*/
    #define	ALAIS_PRM_UPP_UPP_LEFT					((UI_08)3)								/**< ブロックの左側*/
    #define	ALAIS_PRM_UPP_UPP_MAX					((UI_08)4)								/**< MAX Index*/
/* ------------------------------------------------------------------------------------------------ */
/*動作シーケンスNO設定 */
/* ------------------------------------------------------------------------------------------------ */
//====== SEQ
    #define	ALAIS_PRM_SEQ_SEQ_AUTO					((UI_08)0)								/**< AUTO*/
    #define	ALAIS_PRM_SEQ_SEQ_FOCUS					((UI_08)1)								/**< FOCUS*/
    #define	ALAIS_PRM_SEQ_SEQ_TTL0					((UI_08)2)								/**< TTL0*/
    #define	ALAIS_PRM_SEQ_SEQ_TTL1					((UI_08)3)								/**< TTL1*/
    #define	ALAIS_PRM_SEQ_SEQ_TTL2					((UI_08)4)								/**< TTL2*/
    #define	ALAIS_PRM_SEQ_SEQ_TTL3					((UI_08)5)								/**< TTL3*/
    #define	ALAIS_PRM_SEQ_SEQ_MAX					((UI_08)6)								/**< MAX Index*/
/* ------------------------------------------------------------------------------------------------ */
/*モード定義カスタム */
/* ------------------------------------------------------------------------------------------------ */
//====== HS
    #define	ALAIS_PRM_MODE_HS_OFF					((UI_08)0)								/**< Standard*/
    #define	ALAIS_PRM_MODE_HS_ON					((UI_08)1)								/**< Hi-Sencitivity*/
    #define	ALAIS_PRM_MODE_HS_MAX					((UI_08)2)								/**< MAX Index*/

/*==================================================================================================**

    ErrorCode Definition

**==================================================================================================*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_OK							((UI_16)0x8000)							/**< OK*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_NON_SUPPORT					((UI_16)0x8001)							/**< 非サポートコマンド*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_BUSY							((UI_16)0x8002)							/**< BUSY*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_INVALID_PRM					((UI_16)0x8003)							/**< パラメータ不正*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_INVALID_TUNE					((UI_16)0x8004)							/**< チューニングデータサイズ不正*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_INVALID_LINE					((UI_16)0x8005)							/**< ラインデータサイズ不正*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_UNNKOWN						((UI_16)0x8FFF)							/**< 未定義のエラー*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_WORK_UNDERFLOW				((UI_16)0x9F00)							/**< ワークメモリ不足*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_EXT_IP_OVERLAAP				((UI_16)0x9F01)							/**< 外部IP重複*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_RET_EXT_IP_RANGEOVER				((UI_16)0x9F02)							/**< 外部IPセクション範囲エラー*/

/*==================================================================================================**

    Command ID Definition

**==================================================================================================*/
#define ALCMD_COM_RESERVED_ID               ((UI_16)0x0000)                                /**< ダミーコマンドID(予約)*/
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_MODE_IP_PROC_CTL				((UI_16)0x1004)							/**< IP実行制御*/
//====== IPTYP
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_EXPOS		((UI_08)0)								/**< IP_AE*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_COLOR		((UI_08)1)								/**< IP_AWB*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_SHADE		((UI_08)2)								/**< IP_ASC*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_AF			((UI_08)3)								/**< IP_AF*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_GACHO		((UI_08)4)								/**< IP_GACHO*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_GP1		((UI_08)5)								/**< IP_GP1*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_GP2		((UI_08)6)								/**< IP_GP2*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_GP3		((UI_08)7)								/**< IP_GP3*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_GP4		((UI_08)8)								/**< IP_GP4*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_GP5		((UI_08)9)								/**< IP_GP5*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPTYP_MAX		((UI_08)10)								/**< MAX Index*/
//====== IPCTL
    #define	ALAIS_MODE_IP_PROC_CTL_IPCTL_OFF		((UI_08)0)								/**< 無効*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPCTL_ON			((UI_08)1)								/**< 有効*/
    #define	ALAIS_MODE_IP_PROC_CTL_IPCTL_MAX		((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetModeIp_proc_ctl
    {/** AlAisCmdSetModeIp_proc_ctl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiIptyp;                     /**< IPの種類 */
        UI_08 muiIpctl;                     /**< IPTYPで指定するIP種類の中のインデックス */
    }TT_AlAisCmdSetModeIp_proc_ctl;
    typedef struct tg_AlAisResSetModeIp_proc_ctl
    {/** AlAisResSetModeIp_proc_ctl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetModeIp_proc_ctl;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_MODE_IP_PROC_CTL				((UI_16)0x9004)							/**< IP実行制御状態取得*/
//====== IPTYP
//====== IPTYP
//====== IPCTL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetModeIp_proc_ctl
    {/** AlAisCmdGetModeIp_proc_ctl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiIptyp;                     /**< IPの種類 */
    }TT_AlAisCmdGetModeIp_proc_ctl;
    typedef struct tg_AlAisResGetModeIp_proc_ctl
    {/** AlAisResGetModeIp_proc_ctl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiIptyp;                     /**< IPの種類 */
        UI_08 muiIpctl;                     /**< IPTYPで指定するIP種類の中のインデックス */
    }TT_AlAisResGetModeIp_proc_ctl;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_MODE_SKIP						((UI_16)0x1005)							/**< セクション実行間隔設定*/
//====== IPTYP
    #define	ALAIS_MODE_SKIP_IPTYP_EXPOS				((UI_08)0)								/**< IP_AE*/
    #define	ALAIS_MODE_SKIP_IPTYP_COLOR				((UI_08)1)								/**< IP_AWB*/
    #define	ALAIS_MODE_SKIP_IPTYP_SHADE				((UI_08)2)								/**< IP_ASC*/
    #define	ALAIS_MODE_SKIP_IPTYP_AF				((UI_08)3)								/**< IP_AF*/
    #define	ALAIS_MODE_SKIP_IPTYP_GACHO				((UI_08)4)								/**< IP_GACHO*/
    #define	ALAIS_MODE_SKIP_IPTYP_GP1				((UI_08)5)								/**< IP_GP1*/
    #define	ALAIS_MODE_SKIP_IPTYP_GP2				((UI_08)6)								/**< IP_GP2*/
    #define	ALAIS_MODE_SKIP_IPTYP_GP3				((UI_08)7)								/**< IP_GP3*/
    #define	ALAIS_MODE_SKIP_IPTYP_GP4				((UI_08)8)								/**< IP_GP4*/
    #define	ALAIS_MODE_SKIP_IPTYP_GP5				((UI_08)9)								/**< IP_GP5*/
    #define	ALAIS_MODE_SKIP_IPTYP_MAX				((UI_08)10)								/**< MAX Index*/
//====== SKIP
    #define	ALAIS_MODE_SKIP_SKIP_RSV				((UI_08)0)								/**< 予約(設定不可)*/
    #define	ALAIS_MODE_SKIP_SKIP_CONT				((UI_08)255)							/**< 連続*/
    #define	ALAIS_MODE_SKIP_SKIP_MAX				((UI_08)256)							/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetModeSkip
    {/** AlAisCmdSetModeSkip */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiIptyp;                     /**< IPの種類 */
        UI_08 muiSkip;                      /**< スキップする処理数 */
    }TT_AlAisCmdSetModeSkip;
    typedef struct tg_AlAisResSetModeSkip
    {/** AlAisResSetModeSkip */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetModeSkip;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_MODE_SKIP						((UI_16)0x9005)							/**< セクション実行間隔状態取得*/
//====== IPTYP
//====== IPTYP
//====== SKIP
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetModeSkip
    {/** AlAisCmdGetModeSkip */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiIptyp;                     /**<  */
    }TT_AlAisCmdGetModeSkip;
    typedef struct tg_AlAisResGetModeSkip
    {/** AlAisResGetModeSkip */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiIptyp;                     /**< IPの種類 */
        UI_08 muiSkip;                      /**< スキップする処理数 */
    }TT_AlAisResGetModeSkip;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_MODE_LOG						((UI_16)0x90FF)							/**< ログ取得*/
//====== ADDR
//====== SIZE
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetModeLog
    {/** AlAisCmdGetModeLog */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetModeLog;
    typedef struct tg_AlAisResGetModeLog
    {/** AlAisResGetModeLog */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiAddr;                      /**< ログバッファアドレス */
        UI_32 muiSize;                      /**< ログサイズ */
    }TT_AlAisResGetModeLog;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_EXP						((UI_16)0x1401)							/**< AEモード*/
//====== Mode
    #define	ALAIS_EXPOS_EXP_MODE_AUTO				((UI_08)0)								/**< オート*/
    #define	ALAIS_EXPOS_EXP_MODE_MAN				((UI_08)1)								/**< マニュアル*/
    #define	ALAIS_EXPOS_EXP_MODE_AVP				((UI_08)2)								/**< AV優先*/
    #define	ALAIS_EXPOS_EXP_MODE_TVP				((UI_08)3)								/**< TV優先*/
    #define	ALAIS_EXPOS_EXP_MODE_OFF				((UI_08)4)								/**< AE	OFF*/
    #define	ALAIS_EXPOS_EXP_MODE_MAX				((UI_08)5)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposExp
    {/** AlAisCmdSetExposExp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< AE動作モード指定 */
    }TT_AlAisCmdSetExposExp;
    typedef struct tg_AlAisResSetExposExp
    {/** AlAisResSetExposExp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposExp;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_EXP						((UI_16)0x9401)							/**< AEモード取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposExp
    {/** AlAisCmdGetExposExp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposExp;
    typedef struct tg_AlAisResGetExposExp
    {/** AlAisResGetExposExp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< AE動作状態(コマンドと同じ) */
    }TT_AlAisResGetExposExp;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_MET						((UI_16)0x1402)							/**< 測光モード設定*/
//====== Mode
    #define	ALAIS_EXPOS_MET_MODE_EVA				((UI_08)0)								/**< 評価測光*/
    #define	ALAIS_EXPOS_MET_MODE_CENT				((UI_08)1)								/**< 中央重点*/
    #define	ALAIS_EXPOS_MET_MODE_SPOT				((UI_08)2)								/**< スポット*/
    #define	ALAIS_EXPOS_MET_MODE_EVA_CENT			((UI_08)3)								/**< 測光エリア中央（縦でも横でもない)*/
    #define	ALAIS_EXPOS_MET_MODE_EVA_H				((UI_08)4)								/**< 測光エリア横用*/
    #define	ALAIS_EXPOS_MET_MODE_EVA_V				((UI_08)5)								/**< 測光エリア縦用*/
    #define	ALAIS_EXPOS_MET_MODE_MAX				((UI_08)6)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposMet
    {/** AlAisCmdSetExposMet */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 測光モード指定 */
    }TT_AlAisCmdSetExposMet;
    typedef struct tg_AlAisResSetExposMet
    {/** AlAisResSetExposMet */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposMet;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_MET						((UI_16)0x9402)							/**< 測光モード取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposMet
    {/** AlAisCmdGetExposMet */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposMet;
    typedef struct tg_AlAisResGetExposMet
    {/** AlAisResGetExposMet */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 測光モード状態(コマンドと同じ) */
    }TT_AlAisResGetExposMet;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_BLC						((UI_16)0x1403)							/**< 逆光補正設定*/
//====== Mode
    #define	ALAIS_EXPOS_BLC_MODE_OFF				((UI_08)0)								/**< 逆光補正OFF*/
    #define	ALAIS_EXPOS_BLC_MODE_ON					((UI_08)1)								/**< 逆光補正ON*/
    #define	ALAIS_EXPOS_BLC_MODE_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposBlc
    {/** AlAisCmdSetExposBlc */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 逆光補正On./OFF */
    }TT_AlAisCmdSetExposBlc;
    typedef struct tg_AlAisResSetExposBlc
    {/** AlAisResSetExposBlc */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposBlc;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_BLC						((UI_16)0x9403)							/**< 逆光補正取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposBlc
    {/** AlAisCmdGetExposBlc */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposBlc;
    typedef struct tg_AlAisResGetExposBlc
    {/** AlAisResGetExposBlc */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 逆光補正On./OFF状態(コマンドと同じ) */
    }TT_AlAisResGetExposBlc;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_LOCK					((UI_16)0x1404)							/**< AEロック*/
//====== Mode
    #define	ALAIS_EXPOS_LOCK_MODE_OFF				((UI_08)0)								/**< LOCK_OFF*/
    #define	ALAIS_EXPOS_LOCK_MODE_ON				((UI_08)1)								/**< LOCK_ON*/
    #define	ALAIS_EXPOS_LOCK_MODE_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposLock
    {/** AlAisCmdSetExposLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< ロック/ロック解除指定 */
    }TT_AlAisCmdSetExposLock;
    typedef struct tg_AlAisResSetExposLock
    {/** AlAisResSetExposLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposLock;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_LOCK					((UI_16)0x9404)							/**< AEロック状態取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposLock
    {/** AlAisCmdGetExposLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposLock;
    typedef struct tg_AlAisResGetExposLock
    {/** AlAisResGetExposLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< ロック状態(コマンドと同じ) */
    }TT_AlAisResGetExposLock;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_ASS						((UI_16)0x1405)							/**< オートスローシャッター設定*/
//====== Mode
    #define	ALAIS_EXPOS_ASS_MODE_OFF				((UI_08)0)								/**< オートスローシャッターOFF*/
    #define	ALAIS_EXPOS_ASS_MODE_ON					((UI_08)1)								/**< オートスローシャッターON*/
    #define	ALAIS_EXPOS_ASS_MODE_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposAss
    {/** AlAisCmdSetExposAss */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< オートスローシャッター設定 */
    }TT_AlAisCmdSetExposAss;
    typedef struct tg_AlAisResSetExposAss
    {/** AlAisResSetExposAss */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposAss;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_ASS						((UI_16)0x9405)							/**< オートスローシャッター設定取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposAss
    {/** AlAisCmdGetExposAss */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposAss;
    typedef struct tg_AlAisResGetExposAss
    {/** AlAisResGetExposAss */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< オートスローシャッター設定 */
    }TT_AlAisResGetExposAss;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_BRK						((UI_16)0x1406)							/**< AEBモード設定*/
//====== Mode
    #define	ALAIS_EXPOS_BRK_MODE_EVB				((UI_08)0)								/**< EV	Bracket*/
    #define	ALAIS_EXPOS_BRK_MODE_AVB				((UI_08)1)								/**< AV	Bracket*/
    #define	ALAIS_EXPOS_BRK_MODE_TVB				((UI_08)2)								/**< TV	Bracket*/
    #define	ALAIS_EXPOS_BRK_MODE_MAX				((UI_08)3)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposBrk
    {/** AlAisCmdSetExposBrk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< オートスローシャッター設定 */
    }TT_AlAisCmdSetExposBrk;
    typedef struct tg_AlAisResSetExposBrk
    {/** AlAisResSetExposBrk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposBrk;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_BRK						((UI_16)0x9406)							/**< AEBモード取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposBrk
    {/** AlAisCmdGetExposBrk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposBrk;
    typedef struct tg_AlAisResGetExposBrk
    {/** AlAisResGetExposBrk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< オートスローシャッター設定 */
    }TT_AlAisResGetExposBrk;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_SPD						((UI_16)0x1407)							/**< 収束速度設定*/
//====== Mode
    #define	ALAIS_EXPOS_SPD_MODE_SLOW				((UI_08)0)								/**< SLOW*/
    #define	ALAIS_EXPOS_SPD_MODE_MIDDLE				((UI_08)1)								/**< MIDDLE*/
    #define	ALAIS_EXPOS_SPD_MODE_FAST				((UI_08)2)								/**< FAST*/
    #define	ALAIS_EXPOS_SPD_MODE_MAX				((UI_08)3)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposSpd
    {/** AlAisCmdSetExposSpd */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 収束速度 */
    }TT_AlAisCmdSetExposSpd;
    typedef struct tg_AlAisResSetExposSpd
    {/** AlAisResSetExposSpd */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposSpd;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_SPD						((UI_16)0x9407)							/**< 収束速度設定取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposSpd
    {/** AlAisCmdGetExposSpd */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposSpd;
    typedef struct tg_AlAisResGetExposSpd
    {/** AlAisResGetExposSpd */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 収束速度状態(コマンドと同じ) */
    }TT_AlAisResGetExposSpd;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_FLK						((UI_16)0x1408)							/**< フリッカーキャンセル*/
//====== Mode
    #define	ALAIS_EXPOS_FLK_MODE_OFF				((UI_08)0)								/**< OFF*/
    #define	ALAIS_EXPOS_FLK_MODE_AUTO				((UI_08)1)								/**< オート*/
    #define	ALAIS_EXPOS_FLK_MODE_50HZ				((UI_08)2)								/**< 50Hz*/
    #define	ALAIS_EXPOS_FLK_MODE_60HZ				((UI_08)3)								/**< 60Hz*/
    #define	ALAIS_EXPOS_FLK_MODE_AUTO50HZ			((UI_08)4)								/**< オート(50Hzからスタート)*/
    #define	ALAIS_EXPOS_FLK_MODE_AUTO60HZ			((UI_08)5)								/**< オート(60Hzからスタート)*/
    #define	ALAIS_EXPOS_FLK_MODE_MAX				((UI_08)6)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposFlk
    {/** AlAisCmdSetExposFlk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< フリッカーキャンセルOn/Off */
    }TT_AlAisCmdSetExposFlk;
    typedef struct tg_AlAisResSetExposFlk
    {/** AlAisResSetExposFlk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposFlk;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_FLK						((UI_16)0x9408)							/**< フリッカーキャンセル状態取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposFlk
    {/** AlAisCmdGetExposFlk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposFlk;
    typedef struct tg_AlAisResGetExposFlk
    {/** AlAisResGetExposFlk */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< フリッカーキャンセル設定状態(コマンドと同じ) */
    }TT_AlAisResGetExposFlk;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_ISO						((UI_16)0x1409)							/**< ISO感度設定*/
//====== Mode
    #define	ALAIS_EXPOS_ISO_MODE_AUTO				((UI_08)0)								/**< ISOオート*/
    #define	ALAIS_EXPOS_ISO_MODE_SV04p00			((UI_08)1)								/**< ISO50*/
    #define	ALAIS_EXPOS_ISO_MODE_SV05p00			((UI_08)2)								/**< ISO100*/
    #define	ALAIS_EXPOS_ISO_MODE_SV06p00			((UI_08)3)								/**< ISO200*/
    #define	ALAIS_EXPOS_ISO_MODE_SV07p00			((UI_08)4)								/**< ISO400*/
    #define	ALAIS_EXPOS_ISO_MODE_SV08p00			((UI_08)5)								/**< ISO800*/
    #define	ALAIS_EXPOS_ISO_MODE_SV09p00			((UI_08)6)								/**< ISO1600*/
    #define	ALAIS_EXPOS_ISO_MODE_SV10p00			((UI_08)7)								/**< ISO3200*/
    #define	ALAIS_EXPOS_ISO_MODE_SV11p00			((UI_08)8)								/**< ISO6400*/
    #define	ALAIS_EXPOS_ISO_MODE_MAX				((UI_08)9)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposIso
    {/** AlAisCmdSetExposIso */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< ISO感度指定 */
    }TT_AlAisCmdSetExposIso;
    typedef struct tg_AlAisResSetExposIso
    {/** AlAisResSetExposIso */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposIso;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_ISO						((UI_16)0x9409)							/**< ISO感度取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposIso
    {/** AlAisCmdGetExposIso */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposIso;
    typedef struct tg_AlAisResGetExposIso
    {/** AlAisResGetExposIso */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< ISO感度設定状態(コマンドと同じ) */
    }TT_AlAisResGetExposIso;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_AV						((UI_16)0x140A)							/**< 絞り設定*/
//====== Mode
    #define	ALAIS_EXPOS_AV_MODE_AV02p50				((UI_08)2)								/**< F2.4*/
    #define	ALAIS_EXPOS_AV_MODE_MAX					((UI_08)3)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposAv
    {/** AlAisCmdSetExposAv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 絞り指定 */
    }TT_AlAisCmdSetExposAv;
    typedef struct tg_AlAisResSetExposAv
    {/** AlAisResSetExposAv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposAv;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_AV						((UI_16)0x940A)							/**< 絞り設定取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposAv
    {/** AlAisCmdGetExposAv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposAv;
    typedef struct tg_AlAisResGetExposAv
    {/** AlAisResGetExposAv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 絞り設定状態(コマンドと同じ) */
    }TT_AlAisResGetExposAv;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_TV						((UI_16)0x140B)							/**< シャッター速度設定*/
//====== Mode
    #define	ALAIS_EXPOS_TV_MODE_TV00p00				((UI_08)0)								/**< 1/1sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV01p00				((UI_08)1)								/**< 1/2sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV02p00				((UI_08)2)								/**< 1/4sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV03p00				((UI_08)3)								/**< 1/8sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV03p33				((UI_08)4)								/**< 1/10sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV04p00				((UI_08)5)								/**< 1/16sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV04p66				((UI_08)6)								/**< 1/20sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV05p00				((UI_08)7)								/**< 1/32sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV05p66				((UI_08)8)								/**< 1/50sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV06p00				((UI_08)9)								/**< 1/64sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV06p66				((UI_08)10)								/**< 1/100sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV07p00				((UI_08)11)								/**< 1/128sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV08p00				((UI_08)12)								/**< 1/256sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV09p00				((UI_08)13)								/**< 1/512sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV10p00				((UI_08)14)								/**< 1/1024sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV11p00				((UI_08)15)								/**< 1/2048sec*/
    #define	ALAIS_EXPOS_TV_MODE_TV12p00				((UI_08)16)								/**< 1/4096sec*/
    #define	ALAIS_EXPOS_TV_MODE_MAX					((UI_08)17)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposTv
    {/** AlAisCmdSetExposTv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< シャッター速度指定 */
    }TT_AlAisCmdSetExposTv;
    typedef struct tg_AlAisResSetExposTv
    {/** AlAisResSetExposTv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposTv;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_TV						((UI_16)0x940B)							/**< シャッター速度設定取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposTv
    {/** AlAisCmdGetExposTv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposTv;
    typedef struct tg_AlAisResGetExposTv
    {/** AlAisResGetExposTv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< シャッター速度設定状態(コマンドと同じ) */
    }TT_AlAisResGetExposTv;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_EVB						((UI_16)0x140C)							/**< EV補正設定*/
//====== Mode
    #define	ALAIS_EXPOS_EVB_MODE_M200				((UI_08)0)								/**< -2.00Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_M166				((UI_08)1)								/**< -1.66Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_M133				((UI_08)2)								/**< -1.33Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_M100				((UI_08)3)								/**< -1.00Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_M066				((UI_08)4)								/**< -0.66Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_M033				((UI_08)5)								/**< -0.33Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_0000				((UI_08)6)								/**< +-0.00Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_P033				((UI_08)7)								/**< +0.33Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_P066				((UI_08)8)								/**< +0.66Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_P100				((UI_08)9)								/**< +1.00Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_P133				((UI_08)10)								/**< +1.33Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_P166				((UI_08)11)								/**< +1.66Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_P200				((UI_08)12)								/**< +2.00Ev*/
    #define	ALAIS_EXPOS_EVB_MODE_MAX				((UI_08)13)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposEvb
    {/** AlAisCmdSetExposEvb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< EV補正値指定 */
    }TT_AlAisCmdSetExposEvb;
    typedef struct tg_AlAisResSetExposEvb
    {/** AlAisResSetExposEvb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposEvb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_EVB						((UI_16)0x940C)							/**< EV補正取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposEvb
    {/** AlAisCmdGetExposEvb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposEvb;
    typedef struct tg_AlAisResGetExposEvb
    {/** AlAisResGetExposEvb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< EV補正状態(コマンドと同じ) */
    }TT_AlAisResGetExposEvb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_AEB						((UI_16)0x140D)							/**< AEB Bias設定*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposAeb
    {/** AlAisCmdSetExposAeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< AEB Bias値 */
    }TT_AlAisCmdSetExposAeb;
    typedef struct tg_AlAisResSetExposAeb
    {/** AlAisResSetExposAeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposAeb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_AEB						((UI_16)0x940D)							/**< AEB Bias取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposAeb
    {/** AlAisCmdGetExposAeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposAeb;
    typedef struct tg_AlAisResGetExposAeb
    {/** AlAisResGetExposAeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< AEB Bias値状態(コマンドと同じ) */
    }TT_AlAisResGetExposAeb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_FEB						((UI_16)0x140E)							/**< FEB Bias設定*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposFeb
    {/** AlAisCmdSetExposFeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< FEB Bias値 */
    }TT_AlAisCmdSetExposFeb;
    typedef struct tg_AlAisResSetExposFeb
    {/** AlAisResSetExposFeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposFeb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_FEB						((UI_16)0x940E)							/**< FEB Bias取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposFeb
    {/** AlAisCmdGetExposFeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposFeb;
    typedef struct tg_AlAisResGetExposFeb
    {/** AlAisResGetExposFeb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< FEB Bias状態(コマンドと同じ) */
    }TT_AlAisResGetExposFeb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_EVP						((UI_16)0x140F)							/**< EV	Program設定*/
//====== Mode
    #define	ALAIS_EXPOS_EVP_MODE_V_AUTO				((UI_08)0)								/**< DVC Auto*/
    #define	ALAIS_EXPOS_EVP_MODE_V_SCN1				((UI_08)1)								/**< DVC ScnA*/
    #define	ALAIS_EXPOS_EVP_MODE_V_SCN2				((UI_08)2)								/**< DVC ScnB*/
    #define	ALAIS_EXPOS_EVP_MODE_S_AUTO				((UI_08)3)								/**< DSC Auto*/
    #define	ALAIS_EXPOS_EVP_MODE_S_SCN1				((UI_08)4)								/**< DSC ScnA*/
    #define	ALAIS_EXPOS_EVP_MODE_S_SCN2				((UI_08)5)								/**< DSC ScnB*/
    #define	ALAIS_EXPOS_EVP_MODE_MAX				((UI_08)6)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposEvp
    {/** AlAisCmdSetExposEvp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< EVP　インデックス */
    }TT_AlAisCmdSetExposEvp;
    typedef struct tg_AlAisResSetExposEvp
    {/** AlAisResSetExposEvp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposEvp;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_EVP						((UI_16)0x940F)							/**< EV	Program取得*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposEvp
    {/** AlAisCmdGetExposEvp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetExposEvp;
    typedef struct tg_AlAisResGetExposEvp
    {/** AlAisResGetExposEvp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< EVPインデックス(コマンドと同じ) */
    }TT_AlAisResGetExposEvp;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_MODE					((UI_16)0x1501)							/**< WBモード*/
//====== AWBMD
    #define	ALAIS_COLOR_MODE_AWBMD_AUTO				((UI_08)0)								/**< オート*/
    #define	ALAIS_COLOR_MODE_AWBMD_MANUAL			((UI_08)1)								/**< マニュアル*/
    #define	ALAIS_COLOR_MODE_AWBMD_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorMode
    {/** AlAisCmdSetColorMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAwbmd;                     /**< AWB動作指定 */
    }TT_AlAisCmdSetColorMode;
    typedef struct tg_AlAisResSetColorMode
    {/** AlAisResSetColorMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorMode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_COLOR_MODE					((UI_16)0x9501)							/**< WBモード取得*/
//====== AWBMD
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetColorMode
    {/** AlAisCmdGetColorMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetColorMode;
    typedef struct tg_AlAisResGetColorMode
    {/** AlAisResGetColorMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAwbmd;                     /**< AWB動作状態(コマンドと同じ) */
    }TT_AlAisResGetColorMode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_MANUAL_WB				((UI_16)0x1502)							/**< マニュアルWB設定*/
//====== AWBX
//====== AWBY
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorManual_wb
    {/** AlAisCmdSetColorManual_wb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_16 muiAwbx;                      /**< AWBX */
        UI_16 muiAwby;                      /**< AWBY */
    }TT_AlAisCmdSetColorManual_wb;
    typedef struct tg_AlAisResSetColorManual_wb
    {/** AlAisResSetColorManual_wb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorManual_wb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_COLOR_MANUAL_WB				((UI_16)0x9502)							/**< マニュアルWB設定値取得*/
//====== AWBX
//====== AWBY
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetColorManual_wb
    {/** AlAisCmdGetColorManual_wb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetColorManual_wb;
    typedef struct tg_AlAisResGetColorManual_wb
    {/** AlAisResGetColorManual_wb */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_16 muiAwbx;                      /**< AWBX */
        UI_16 muiAwby;                      /**< AWBY */
    }TT_AlAisResGetColorManual_wb;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_LOCK					((UI_16)0x1503)							/**< AWBロック*/
//====== AWBLC
    #define	ALAIS_COLOR_LOCK_AWBLC_OFF				((UI_08)0)								/**< LOCK_OFF*/
    #define	ALAIS_COLOR_LOCK_AWBLC_ON				((UI_08)1)								/**< LOCK_ON*/
    #define	ALAIS_COLOR_LOCK_AWBLC_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorLock
    {/** AlAisCmdSetColorLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAwblc;                     /**< ロック/ロック解除指定 */
    }TT_AlAisCmdSetColorLock;
    typedef struct tg_AlAisResSetColorLock
    {/** AlAisResSetColorLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorLock;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_COLOR_LOCK					((UI_16)0x9503)							/**< AWBロック状態取得*/
//====== AWBLC
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetColorLock
    {/** AlAisCmdGetColorLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetColorLock;
    typedef struct tg_AlAisResGetColorLock
    {/** AlAisResGetColorLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAwblc;                     /**< ロック状態(コマンドと同じ) */
    }TT_AlAisResGetColorLock;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_SPEED					((UI_16)0x1504)							/**< 収束速度設定*/
//====== WBSPD
    #define	ALAIS_COLOR_SPEED_WBSPD_SLOW			((UI_08)0)								/**< SLOW*/
    #define	ALAIS_COLOR_SPEED_WBSPD_MIDDLE			((UI_08)1)								/**< MIDDLE*/
    #define	ALAIS_COLOR_SPEED_WBSPD_FAST			((UI_08)2)								/**< FAST*/
    #define	ALAIS_COLOR_SPEED_WBSPD_MAX				((UI_08)3)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorSpeed
    {/** AlAisCmdSetColorSpeed */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiWbspd;                     /**< 収束速度 */
    }TT_AlAisCmdSetColorSpeed;
    typedef struct tg_AlAisResSetColorSpeed
    {/** AlAisResSetColorSpeed */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorSpeed;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_COLOR_SPEED					((UI_16)0x9503)							/**< 収束速度設定*/
//====== WBSPD
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetColorSpeed
    {/** AlAisCmdGetColorSpeed */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetColorSpeed;
    typedef struct tg_AlAisResGetColorSpeed
    {/** AlAisResGetColorSpeed */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiWbspd;                     /**< 収束速度状態(コマンドと同じ) */
    }TT_AlAisResGetColorSpeed;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_SHADE_VERSION					((UI_16)0x9600)							/**< ASCVersion取得*/
//====== MAJOR
//====== MINOR
//====== LOCAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetShadeVersion
    {/** AlAisCmdGetShadeVersion */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetShadeVersion;
    typedef struct tg_AlAisResGetShadeVersion
    {/** AlAisResGetShadeVersion */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_16 muiMajor;                     /**< メジャーVersion */
        UI_16 muiMinor;                     /**< マイナーVersion */
        UI_16 muiLocal;                     /**< ローカルVersion */
    }TT_AlAisResGetShadeVersion;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_SHADE_MODE					((UI_16)0x1601)							/**< ASCモード*/
//====== ASCMD
    #define	ALAIS_SHADE_MODE_ASCMD_AUTO				((UI_08)0)								/**< オート*/
    #define	ALAIS_SHADE_MODE_ASCMD_OFF				((UI_08)1)								/**< OFF*/
    #define	ALAIS_SHADE_MODE_ASCMD_LOCK				((UI_08)2)								/**< Lock*/
    #define	ALAIS_SHADE_MODE_ASCMD_TABLE0			((UI_08)16)								/**< Table0固定*/
    #define	ALAIS_SHADE_MODE_ASCMD_TABLE1			((UI_08)17)								/**< Table1固定*/
    #define	ALAIS_SHADE_MODE_ASCMD_TABLE2			((UI_08)18)								/**< Table2固定*/
    #define	ALAIS_SHADE_MODE_ASCMD_DEBUG			((UI_08)221)							/**< DEBUG*/
    #define	ALAIS_SHADE_MODE_ASCMD_MAX				((UI_08)222)							/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetShadeMode
    {/** AlAisCmdSetShadeMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAscmd;                     /**< ASCモード */
    }TT_AlAisCmdSetShadeMode;
    typedef struct tg_AlAisResSetShadeMode
    {/** AlAisResSetShadeMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetShadeMode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_SHADE_MODE					((UI_16)0x9601)							/**< ASCモード取得*/
//====== ASCMD
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetShadeMode
    {/** AlAisCmdGetShadeMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetShadeMode;
    typedef struct tg_AlAisResGetShadeMode
    {/** AlAisResGetShadeMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAscmd;                     /**< ASCモード */
    }TT_AlAisResGetShadeMode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_BV						((UI_16)0x3406)							/**< BV初期値設定*/
//====== INIT
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposBv
    {/** AlAisCmdSetExposBv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiInit;                      /**< BV初期値設定 */
    }TT_AlAisCmdSetExposBv;
    typedef struct tg_AlAisResSetExposBv
    {/** AlAisResSetExposBv */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposBv;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_TARGET					((UI_16)0x3407)							/**< 目標反射率設定*/
//====== Rate
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposTarget
    {/** AlAisCmdSetExposTarget */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiRate;                      /**< 目標反射率設定 */
    }TT_AlAisCmdSetExposTarget;
    typedef struct tg_AlAisResSetExposTarget
    {/** AlAisResSetExposTarget */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposTarget;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_EVPM					((UI_16)0x3408)							/**< EV	Program	Movieモード*/
//====== Mode
    #define	ALAIS_EXPOS_EVPM_MODE_NORMAL			((UI_08)0)								/**< Normal*/
    #define	ALAIS_EXPOS_EVPM_MODE_ANTI				((UI_08)1)								/**< anti-hand shake Normal*/
    #define	ALAIS_EXPOS_EVPM_MODE_NORMAL_LED		((UI_08)2)								/**< Normal	LED*/
    #define	ALAIS_EXPOS_EVPM_MODE_ANTI_LED			((UI_08)3)								/**< anti-hand shake LED*/
    #define	ALAIS_EXPOS_EVPM_MODE_NIGHT_VIEW		((UI_08)4)								/**< Night view*/
    #define	ALAIS_EXPOS_EVPM_MODE_TWILIGHT			((UI_08)5)								/**< Twilight*/
    #define	ALAIS_EXPOS_EVPM_MODE_SPORTS			((UI_08)6)								/**< Sports*/
    #define	ALAIS_EXPOS_EVPM_MODE_PARTY				((UI_08)7)								/**< Party*/
    #define	ALAIS_EXPOS_EVPM_MODE_NIGHT				((UI_08)8)								/**< Night vision*/
    #define	ALAIS_EXPOS_EVPM_MODE_NIGHT_LED			((UI_08)9)								/**< Night vision LED*/
    #define	ALAIS_EXPOS_EVPM_MODE_MOVIE				((UI_08)10)								/**< movie*/
    #define	ALAIS_EXPOS_EVPM_MODE_QR_Code			((UI_08)11)								/**< QR	Code Reader*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern13			((UI_08)12)								/**< pattern13*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern14			((UI_08)13)								/**< pattern14*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern15			((UI_08)14)								/**< pattern15*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern16			((UI_08)15)								/**< pattern16*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern17			((UI_08)16)								/**< pattern17*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern18			((UI_08)17)								/**< pattern18*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern19			((UI_08)18)								/**< pattern19*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern20			((UI_08)19)								/**< pattern20*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern21			((UI_08)20)								/**< pattern21*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern22			((UI_08)21)								/**< pattern22*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern23			((UI_08)22)								/**< pattern23*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern24			((UI_08)23)								/**< pattern24*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern25			((UI_08)24)								/**< pattern25*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern26			((UI_08)25)								/**< pattern26*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern27			((UI_08)26)								/**< pattern27*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern28			((UI_08)27)								/**< pattern28*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern29			((UI_08)28)								/**< pattern29*/
    #define	ALAIS_EXPOS_EVPM_MODE_pattern30			((UI_08)29)								/**< pattern30*/
    #define	ALAIS_EXPOS_EVPM_MODE_MAX				((UI_08)30)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposEvpm
    {/** AlAisCmdSetExposEvpm */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< Movie(Mch)用EVPインデックス */
    }TT_AlAisCmdSetExposEvpm;
    typedef struct tg_AlAisResSetExposEvpm
    {/** AlAisResSetExposEvpm */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposEvpm;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_EVPS					((UI_16)0x3409)							/**< EV	Program	Stillモード*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposEvps
    {/** AlAisCmdSetExposEvps */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< Still(Sch)用EVPインデックス */
    }TT_AlAisCmdSetExposEvps;
    typedef struct tg_AlAisResSetExposEvps
    {/** AlAisResSetExposEvps */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposEvps;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_TVMAXM					((UI_16)0x340A)							/**< 最大シャッター速度Movie用*/
//====== VAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposTvmaxm
    {/** AlAisCmdSetExposTvmaxm */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiVal;                       /**< 最大シャッター速度Movie用 */
    }TT_AlAisCmdSetExposTvmaxm;
    typedef struct tg_AlAisResSetExposTvmaxm
    {/** AlAisResSetExposTvmaxm */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposTvmaxm;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_TVMAXS					((UI_16)0x340B)							/**< 最大シャッター速度Still用*/
//====== VAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposTvmaxs
    {/** AlAisCmdSetExposTvmaxs */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiVal;                       /**< 最大シャッター速度Still用 */
    }TT_AlAisCmdSetExposTvmaxs;
    typedef struct tg_AlAisResSetExposTvmaxs
    {/** AlAisResSetExposTvmaxs */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposTvmaxs;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_MAXISO					((UI_16)0x340C)							/**< 最大ISO感度モード*/
//====== Mode
    #define	ALAIS_EXPOS_MAXISO_MODE_AUTO			((UI_08)0)								/**< ISOオート*/
    #define	ALAIS_EXPOS_MAXISO_MODE_SV06p00			((UI_08)1)								/**< 最大ISO200*/
    #define	ALAIS_EXPOS_MAXISO_MODE_SV07p00			((UI_08)2)								/**< 最大ISO400*/
    #define	ALAIS_EXPOS_MAXISO_MODE_SV08p00			((UI_08)3)								/**< 最大ISO800*/
    #define	ALAIS_EXPOS_MAXISO_MODE_SV09p00			((UI_08)4)								/**< 最大ISO1600*/
    #define	ALAIS_EXPOS_MAXISO_MODE_SV10p00			((UI_08)5)								/**< 最大ISO3200*/
    #define	ALAIS_EXPOS_MAXISO_MODE_SV11p00			((UI_08)6)								/**< 最大ISO6400*/
    #define	ALAIS_EXPOS_MAXISO_MODE_MAX				((UI_08)7)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposMaxiso
    {/** AlAisCmdSetExposMaxiso */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 最大ISO感度設定 */
    }TT_AlAisCmdSetExposMaxiso;
    typedef struct tg_AlAisResSetExposMaxiso
    {/** AlAisResSetExposMaxiso */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposMaxiso;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_SVMAXM					((UI_16)0x340D)							/**< 最大ISO感度値Movie用*/
//====== VAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposSvmaxm
    {/** AlAisCmdSetExposSvmaxm */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiVal;                       /**< 最大ISO感度値Movie用 */
    }TT_AlAisCmdSetExposSvmaxm;
    typedef struct tg_AlAisResSetExposSvmaxm
    {/** AlAisResSetExposSvmaxm */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposSvmaxm;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_SVMAXS					((UI_16)0x340E)							/**< 最大ISO感度値Still用*/
//====== VAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposSvmaxs
    {/** AlAisCmdSetExposSvmaxs */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiVal;                       /**< 最大ISO感度値Still用 */
    }TT_AlAisCmdSetExposSvmaxs;
    typedef struct tg_AlAisResSetExposSvmaxs
    {/** AlAisResSetExposSvmaxs */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposSvmaxs;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_FACE					((UI_16)0x340F)							/**< 顔検出被写体距離*/
//====== Dist
    #define	ALAIS_EXPOS_FACE_DIST_ALL				((UI_08)0)								/**< 全距離*/
    #define	ALAIS_EXPOS_FACE_DIST_50CM				((UI_08)1)								/**< 0.5m*/
    #define	ALAIS_EXPOS_FACE_DIST_1M				((UI_08)2)								/**< 1m*/
    #define	ALAIS_EXPOS_FACE_DIST_2M				((UI_08)3)								/**< 2m以上*/
    #define	ALAIS_EXPOS_FACE_DIST_MAX				((UI_08)4)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposFace
    {/** AlAisCmdSetExposFace */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiDist;                      /**< 顔検出による被写体距離設定 */
    }TT_AlAisCmdSetExposFace;
    typedef struct tg_AlAisResSetExposFace
    {/** AlAisResSetExposFace */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposFace;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_STROBEEN				((UI_16)0x3410)							/**< ストロボ発光点灯設定*/
//====== Mode
    #define	ALAIS_EXPOS_STROBEEN_MODE_OFF			((UI_08)0)								/**< オフ(発光禁止)*/
    #define	ALAIS_EXPOS_STROBEEN_MODE_AUTO			((UI_08)1)								/**< オート*/
    #define	ALAIS_EXPOS_STROBEEN_MODE_ON			((UI_08)2)								/**< 強制発光*/
    #define	ALAIS_EXPOS_STROBEEN_MODE_MAX			((UI_08)3)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposStrobeen
    {/** AlAisCmdSetExposStrobeen */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< ストロボ発光点灯設定 */
    }TT_AlAisCmdSetExposStrobeen;
    typedef struct tg_AlAisResSetExposStrobeen
    {/** AlAisResSetExposStrobeen */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposStrobeen;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_EVB2					((UI_16)0x3411)							/**< EV	Bias 2*/
//====== Bias
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposEvb2
    {/** AlAisCmdSetExposEvb2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiBias;                      /**< EVB Bias 設定(Bias指定) */
    }TT_AlAisCmdSetExposEvb2;
    typedef struct tg_AlAisResSetExposEvb2
    {/** AlAisResSetExposEvb2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposEvb2;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_AEB2					((UI_16)0x3412)							/**< AE	Bracket	2*/
//====== Bias
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposAeb2
    {/** AlAisCmdSetExposAeb2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiBias;                      /**< AEB Bias 設定(Bias指定) */
    }TT_AlAisCmdSetExposAeb2;
    typedef struct tg_AlAisResSetExposAeb2
    {/** AlAisResSetExposAeb2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposAeb2;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_FEB2					((UI_16)0x3413)							/**< Flash Exp Bracket 2*/
//====== Bias
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposFeb2
    {/** AlAisCmdSetExposFeb2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiBias;                      /**< FEB Bias 設定(Bias指定) */
    }TT_AlAisCmdSetExposFeb2;
    typedef struct tg_AlAisResSetExposFeb2
    {/** AlAisResSetExposFeb2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposFeb2;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_FPS_CTRL				((UI_16)0x3414)							/**< TVクリップ用FPS速度設定*/
//====== Minfps
//====== Maxfps
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposFps_ctrl
    {/** AlAisCmdSetExposFps_ctrl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiMinfps;                    /**< TVクリップ用FPS */
        UI_32 muiMaxfps;                    /**< TV優先用FPS */
    }TT_AlAisCmdSetExposFps_ctrl;
    typedef struct tg_AlAisResSetExposFps_ctrl
    {/** AlAisResSetExposFps_ctrl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposFps_ctrl;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_ONE_SHOT				((UI_16)0x3415)							/**< One-Shot AE設定*/
//====== Mode
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposOne_shot
    {/** AlAisCmdSetExposOne_shot */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< Dummy */
    }TT_AlAisCmdSetExposOne_shot;
    typedef struct tg_AlAisResSetExposOne_shot
    {/** AlAisResSetExposOne_shot */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposOne_shot;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_FLK_FSMP				((UI_16)0x3416)							/**< フリッカ用サンプリング情報*/
//====== FSmpl
//====== Vline
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposFlk_fsmp
    {/** AlAisCmdSetExposFlk_fsmp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiFsmpl;                     /**<  */
        UI_16 muiVline;                     /**<  */
    }TT_AlAisCmdSetExposFlk_fsmp;
    typedef struct tg_AlAisResSetExposFlk_fsmp
    {/** AlAisResSetExposFlk_fsmp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposFlk_fsmp;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_MET2					((UI_16)0x3417)							/**< 測光モード指定2*/
//====== Mode
    #define	ALAIS_EXPOS_MET2_MODE_EVA				((UI_08)0)								/**< 評価測光*/
    #define	ALAIS_EXPOS_MET2_MODE_WEI				((UI_08)1)								/**< 中央重点*/
    #define	ALAIS_EXPOS_MET2_MODE_AVE				((UI_08)2)								/**< 平均測光*/
    #define	ALAIS_EXPOS_MET2_MODE_FACE				((UI_08)3)								/**< 評価測光(顔)*/
    #define	ALAIS_EXPOS_MET2_MODE_MAX				((UI_08)4)								/**< MAX Index*/
//====== Weight
    #define	ALAIS_EXPOS_MET2_WEIGHT_EVA				((UI_08)0)								/**< 評価測光用ウエイト指定IDX*/
    #define	ALAIS_EXPOS_MET2_WEIGHT_CENT			((UI_08)1)								/**< 中央重点用ウエイト指定IDX*/
    #define	ALAIS_EXPOS_MET2_WEIGHT_SPOT			((UI_08)2)								/**< スポット用ウエイト指定IDX*/
    #define	ALAIS_EXPOS_MET2_WEIGHT_EVA_CENT		((UI_08)3)								/**< 測光エリア中央（縦でも横でもない)用ウエイト指定IDX*/
    #define	ALAIS_EXPOS_MET2_WEIGHT_EVA_H			((UI_08)4)								/**< 測光エリア横用ウエイト指定IDX*/
    #define	ALAIS_EXPOS_MET2_WEIGHT_EVA_V			((UI_08)5)								/**< 測光エリア縦用ウエイト指定IDX*/
    #define	ALAIS_EXPOS_MET2_WEIGHT_MAX				((UI_08)6)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposMet2
    {/** AlAisCmdSetExposMet2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< 測光モード指定 */
        UI_08 muiWeight;                    /**< 測光ウエイト指定 */
    }TT_AlAisCmdSetExposMet2;
    typedef struct tg_AlAisResSetExposMet2
    {/** AlAisResSetExposMet2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposMet2;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_ZSL						((UI_16)0x3418)							/**< ZSLモード設定*/
//====== Mode
    #define	ALAIS_EXPOS_ZSL_MODE_OFF				((UI_08)0)								/**< OFF*/
    #define	ALAIS_EXPOS_ZSL_MODE_ON					((UI_08)1)								/**< ON*/
    #define	ALAIS_EXPOS_ZSL_MODE_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposZsl
    {/** AlAisCmdSetExposZsl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< OFF/ON */
    }TT_AlAisCmdSetExposZsl;
    typedef struct tg_AlAisResSetExposZsl
    {/** AlAisResSetExposZsl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposZsl;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_EXPOS_CAMMODE					((UI_16)0x3419)							/**< カメラモード種別*/
//====== Mode
    #define	ALAIS_EXPOS_CAMMODE_MODE_Moto_PREV		((UI_08)0)								/**< Preview*/
    #define	ALAIS_EXPOS_CAMMODE_MODE_Moto_CAP		((UI_08)1)								/**< Capture*/
    #define	ALAIS_EXPOS_CAMMODE_MODE_Moto_DVC30FPS	((UI_08)2)								/**< Movie30fps*/
    #define	ALAIS_EXPOS_CAMMODE_MODE_Moto_DVC60FPS	((UI_08)3)								/**< Movie60fps*/
    #define	ALAIS_EXPOS_CAMMODE_MODE_MAX			((UI_08)4)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetExposCammode
    {/** AlAisCmdSetExposCammode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< カメラモード指定（主に線図選択用） */
    }TT_AlAisCmdSetExposCammode;
    typedef struct tg_AlAisResSetExposCammode
    {/** AlAisResSetExposCammode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetExposCammode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_LPF_MODE				((UI_16)0x3500)							/**< LPFモード*/
//====== LPF
    #define	ALAIS_COLOR_LPF_MODE_LPF_AUTO			((UI_08)0)								/**< Auto*/
    #define	ALAIS_COLOR_LPF_MODE_LPF_DSC			((UI_08)1)								/**< DSC向けLPF*/
    #define	ALAIS_COLOR_LPF_MODE_LPF_DVC			((UI_08)2)								/**< DVC向けLPF*/
    #define	ALAIS_COLOR_LPF_MODE_LPF_MAX			((UI_08)3)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorLpf_mode
    {/** AlAisCmdSetColorLpf_mode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiLpf;                       /**<  */
    }TT_AlAisCmdSetColorLpf_mode;
    typedef struct tg_AlAisResSetColorLpf_mode
    {/** AlAisResSetColorLpf_mode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorLpf_mode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_COLOR_CTEMP					((UI_16)0xB501)							/**< 色温度取得*/
//====== CIF_X
//====== CIF_Y
//====== CTEMP
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetColorCtemp
    {/** AlAisCmdGetColorCtemp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiCif_x;                     /**< CIF x */
        UI_32 muiCif_y;                     /**< CIF y */
    }TT_AlAisCmdGetColorCtemp;
    typedef struct tg_AlAisResGetColorCtemp
    {/** AlAisResGetColorCtemp */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiCtemp;                     /**< 色温度 */
    }TT_AlAisResGetColorCtemp;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_MODE2					((UI_16)0x3502)							/**< WBモード2*/
//====== MD
    #define	ALAIS_COLOR_MODE2_MD_AUTO				((UI_08)0)								/**< オート*/
    #define	ALAIS_COLOR_MODE2_MD_MANU				((UI_08)1)								/**< マニュアルx,y指定*/
    #define	ALAIS_COLOR_MODE2_MD_PRESET				((UI_08)2)								/**< プリセット*/
    #define	ALAIS_COLOR_MODE2_MD_MOVIE				((UI_08)3)								/**< 動画*/
    #define	ALAIS_COLOR_MODE2_MD_MAX				((UI_08)4)								/**< MAX Index*/
//====== ACT
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorMode2
    {/** AlAisCmdSetColorMode2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMd;                        /**< AWB動作指定 */
        UI_08 muiAct;                       /**<  */
    }TT_AlAisCmdSetColorMode2;
    typedef struct tg_AlAisResSetColorMode2
    {/** AlAisResSetColorMode2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorMode2;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_COLOR_MODE2					((UI_16)0xB502)							/**< WBモード2取得*/
//====== MD
//====== ACT
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetColorMode2
    {/** AlAisCmdGetColorMode2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetColorMode2;
    typedef struct tg_AlAisResGetColorMode2
    {/** AlAisResGetColorMode2 */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMd;                        /**< AWB動作指定 */
        UI_08 muiAct;                       /**<  */
    }TT_AlAisResGetColorMode2;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_COLOR_PREF_MODE				((UI_16)0x3503)							/**< Preferred WB Mode*/
//====== Mode
    #define	ALAIS_COLOR_PREF_MODE_MODE_DISABLE		((UI_08)0)								/**< 無効*/
    #define	ALAIS_COLOR_PREF_MODE_MODE_ENABLE		((UI_08)1)								/**< 有効*/
    #define	ALAIS_COLOR_PREF_MODE_MODE_MAX			((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetColorPref_mode
    {/** AlAisCmdSetColorPref_mode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMode;                      /**< Preferred設定 */
    }TT_AlAisCmdSetColorPref_mode;
    typedef struct tg_AlAisResSetColorPref_mode
    {/** AlAisResSetColorPref_mode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetColorPref_mode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_SHADE_LOCK					((UI_16)0x3600)							/**< Shadingロック*/
//====== lock
    #define	ALAIS_SHADE_LOCK_LOCK_OFF				((UI_08)0)								/**< ロック解除*/
    #define	ALAIS_SHADE_LOCK_LOCK_ON				((UI_08)1)								/**< ロック*/
    #define	ALAIS_SHADE_LOCK_LOCK_MAX				((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetShadeLock
    {/** AlAisCmdSetShadeLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiLock;                      /**<  */
    }TT_AlAisCmdSetShadeLock;
    typedef struct tg_AlAisResSetShadeLock
    {/** AlAisResSetShadeLock */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetShadeLock;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_AF_VERSION					((UI_16)0x9800)							/**< AF	Version取得*/
//====== MAJOR
//====== MINOR
//====== LOCAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetAfVersion
    {/** AlAisCmdGetAfVersion */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetAfVersion;
    typedef struct tg_AlAisResGetAfVersion
    {/** AlAisResGetAfVersion */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_16 muiMajor;                     /**< メジャーVersion */
        UI_16 muiMinor;                     /**< マイナーVersion */
        UI_16 muiLocal;                     /**< ローカルVersion */
    }TT_AlAisResGetAfVersion;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_AF_MODE						((UI_16)0x1801)							/**< フォーカスモード*/
//====== FCSMD
    #define	ALAIS_AF_MODE_FCSMD_AUTO				((UI_08)0)								/**< オート*/
    #define	ALAIS_AF_MODE_FCSMD_MANUAL				((UI_08)1)								/**< マニュアル*/
    #define	ALAIS_AF_MODE_FCSMD_MAX					((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetAfMode
    {/** AlAisCmdSetAfMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiFcsmd;                     /**< フォーカスモード */
    }TT_AlAisCmdSetAfMode;
    typedef struct tg_AlAisResSetAfMode
    {/** AlAisResSetAfMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetAfMode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_AF_MODE						((UI_16)0x9801)							/**< フォーカスモード取得*/
//====== FCSMD
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetAfMode
    {/** AlAisCmdGetAfMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetAfMode;
    typedef struct tg_AlAisResGetAfMode
    {/** AlAisResGetAfMode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiFcsmd;                     /**< フォーカスモード(コマンドと同じ) */
    }TT_AlAisResGetAfMode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_AF_TRIGGER					((UI_16)0x1802)							/**< ワンショットAFトリガ*/
//====== AFTRG
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetAfTrigger
    {/** AlAisCmdSetAfTrigger */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiAftrg;                     /**< AFTRG */
    }TT_AlAisCmdSetAfTrigger;
    typedef struct tg_AlAisResSetAfTrigger
    {/** AlAisResSetAfTrigger */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetAfTrigger;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_AF_MOTOR_EN_CTL				((UI_16)0x3800)							/**< モータIC電源制御モード*/
//====== MECTL
    #define	ALAIS_AF_MOTOR_EN_CTL_MECTL_DISABLE		((UI_08)0)								/**< 電源制御有効*/
    #define	ALAIS_AF_MOTOR_EN_CTL_MECTL_ENABLE		((UI_08)1)								/**< 電源制御無効*/
    #define	ALAIS_AF_MOTOR_EN_CTL_MECTL_MAX			((UI_08)2)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetAfMotor_en_ctl
    {/** AlAisCmdSetAfMotor_en_ctl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiMectl;                     /**<  */
    }TT_AlAisCmdSetAfMotor_en_ctl;
    typedef struct tg_AlAisResSetAfMotor_en_ctl
    {/** AlAisResSetAfMotor_en_ctl */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetAfMotor_en_ctl;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_IMG_VERSION					((UI_16)0x9900)							/**< Version取得*/
//====== MAJOR
//====== MINOR
//====== LOCAL
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetImgVersion
    {/** AlAisCmdGetImgVersion */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisCmdGetImgVersion;
    typedef struct tg_AlAisResGetImgVersion
    {/** AlAisResGetImgVersion */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_16 muiMajor;                     /**< メジャーVersion */
        UI_16 muiMinor;                     /**< マイナーVersion */
        UI_16 muiLocal;                     /**< ローカルVersion */
    }TT_AlAisResGetImgVersion;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_SET_IMG_LVC_MODE					((UI_16)0x1901)							/**< LVCモード設定*/
//====== LVCMD
    #define	ALAIS_IMG_LVC_MODE_LVCMD_LVC_ON			((UI_08)0)								/**< */
    #define	ALAIS_IMG_LVC_MODE_LVCMD_LVC_OFF		((UI_08)1)								/**< */
    #define	ALAIS_IMG_LVC_MODE_LVCMD_LVC_LOCK		((UI_08)2)								/**< */
    #define	ALAIS_IMG_LVC_MODE_LVCMD_LVC_DEBUG		((UI_08)3)								/**< */
    #define	ALAIS_IMG_LVC_MODE_LVCMD_LVC_SP1		((UI_08)4)								/**< */
    #define	ALAIS_IMG_LVC_MODE_LVCMD_LVC_SP2		((UI_08)5)								/**< */
    #define	ALAIS_IMG_LVC_MODE_LVCMD_MAX			((UI_08)6)								/**< MAX Index*/
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdSetImgLvc_mode
    {/** AlAisCmdSetImgLvc_mode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiLvcmd;                     /**< LVC動作モード */
    }TT_AlAisCmdSetImgLvc_mode;
    typedef struct tg_AlAisResSetImgLvc_mode
    {/** AlAisResSetImgLvc_mode */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
            //==== Nothing ====/
    }TT_AlAisResSetImgLvc_mode;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
/* ------------------------------------------------------------------------------------------------ */
#define	ALAIS_GET_EXPOS_AE_INSTANCE				((UI_16)0xC400)							/**< AEインスタンス取得*/
//====== type
    #define	ALAIS_EXPOS_AE_INSTANCE_TYPE_AEI		((UI_08)0)								/**< */
    #define	ALAIS_EXPOS_AE_INSTANCE_TYPE_AE_CFI		((UI_08)1)								/**< */
    #define	ALAIS_EXPOS_AE_INSTANCE_TYPE_FLK_CFI	((UI_08)2)								/**< */
    #define	ALAIS_EXPOS_AE_INSTANCE_TYPE_HAL		((UI_08)3)								/**< */
    #define	ALAIS_EXPOS_AE_INSTANCE_TYPE_MAX		((UI_08)4)								/**< MAX Index*/
//====== instance
    //========== Cmd/Res Struct Typedef ==========/
    #ifndef AL_DOXYGEN_EXCULDE
    typedef struct tg_AlAisCmdGetExposAe_instance
    {/** AlAisCmdGetExposAe_instance */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_08 muiType;                      /**<  */
    }TT_AlAisCmdGetExposAe_instance;
    typedef struct tg_AlAisResGetExposAe_instance
    {/** AlAisResGetExposAe_instance */
        UI_16 muiCmdId;                     /**< コマンドID */
        UI_16 muiRsved;                     /**< 予約領域 */
        //==== Command Specific Member ====/
        UI_32 muiInstance;                  /**<  */
    }TT_AlAisResGetExposAe_instance;
    #endif //#ifndef AL_DOXYGEN_EXCULDE
#endif //#ifndef ALAISCOMMAND_H
