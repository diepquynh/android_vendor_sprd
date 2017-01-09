/******************************************************************************
*
* Copyright (C) 2016 SpreadTrum Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************************/
/**
********************************************************************************
@file
*  AudioCustom_MmiApi.h
*
* @brief
*  API definitions used in the code
*
* @author
*
*
* @remarks
*  None
*
*******************************************************************************
*/

/**
RC for Audio MMI test
**/

#define MMI_TEST_NO_ERR     0
#define MMI_TEST_ERR_BASE   0x12120000
#define MMI_TEST_ARG_ERR    (MMI_TEST_ERR_BASE +0x1)
#define MMI_TEST_IS_OFF     (MMI_TEST_ERR_BASE +0x2)

#define MMI_TEST_START   (1)
#define MMI_TEST_CLOSE   (0)


typedef void* AUDIO_MMI_HANDLE;

AUDIO_MMI_HANDLE AudioMMIInit(void);

int AudioMMIGetDevice(AUDIO_MMI_HANDLE handle,
                                   int *pOutDevice,
                                   int *pInDevice);

int AudioMMIParse(struct str_parms *str_parms, AUDIO_MMI_HANDLE handle);

int AudioMMIIsEnable(AUDIO_MMI_HANDLE handle);


