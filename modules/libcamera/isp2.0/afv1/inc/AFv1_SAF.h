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
 * SAF.h
 *
 * \brief
 * search algorithm for single AF
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

#ifndef __AFV1_SAF_H__
#define  __AFV1_SAF_H__
#include "AFv1_Common.h"

ERRCODE SAF_Init(AF_Data* pAF_Data);
ERRCODE SAF_DeInit(AF_Data* pAF_Data);
ERRCODE SAF_main(AF_Data* pAF_Data);
ERRCODE Set_FineSearch_Para(AF_Data* pAF_Data, uint8 DIR);
ERRCODE Set_RoughSearch_Para(AF_Data* pAF_Data, uint8 DIR);
ERRCODE SAF_Search(AF_Data* pAF_Data);
ERRCODE SAF_Search_main(AF_Data* pAF_Data);
ERRCODE SAF_Cal_RoughScan_Table(AF_Data* pAF_Data, uint8 CurIdx, uint8 DIR);












#endif
