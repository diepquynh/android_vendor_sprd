/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 * $History$
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved.
 *
 * \file
 * AF_Interface.h
 *
 * \brief
 * Interface for AF 
 *
 * \date
 * 2016/01/03
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */

#ifndef __AFV1_INTERFACE_H__
#define  __AFV1_INTERFACE_H__

/*!
 *******************************************************************************
 * UI
 *
 *******************************************************************************
*/


ERRCODE AF_Trigger(AF_Data *pAF_Data, eAF_MODE  AF_mode, eAF_Triger_Type AF_Trigger_Type);
ERRCODE AF_STOP(AF_Data *pAF_Data, eAF_MODE  AF_mode);

ERRCODE AF_init(AF_Data *pAF_Data);
ERRCODE AF_deinit(AF_Data *pAF_Data);

ERRCODE AF_Process_Frame(AF_Data *pAF_Data);
ERRCODE AF_Get_Statistic(AF_Data *pAF_Data);
ERRCODE AF_Get_StartPosStatistic(AF_Data *pAF_Data);
ERRCODE AF_Get_SAF_Result(AF_Data *pAF_Data, uint8* SAF_Result);
ERRCODE AF_Search_LENS_MOVE(uint8* pLensMoveCnt, 
                            uint8* pMaxSearchTableNum, 
                            uint16 pos,
                            AF_Ctrl_Ops* pAF_Ops,
                            uint8* pAF_Result );

#endif
