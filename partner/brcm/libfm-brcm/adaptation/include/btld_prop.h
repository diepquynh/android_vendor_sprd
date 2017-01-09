/* Copyright 2009 Broadcom Corporation
**
** This program is the proprietary software of Broadcom Corporation and/or its
** licensors, and may only be used, duplicated, modified or distributed
** pursuant to the terms and conditions of a separate, written license
** agreement executed between you and Broadcom (an "Authorized License").
** Except as set forth in an Authorized License, Broadcom grants no license
** (express or implied), right to use, or waiver of any kind with respect to
** the Software, and Broadcom expressly reserves all rights in and to the
** Software and all intellectual property rights therein.
** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
** ALL USE OF THE SOFTWARE.
**
** Except as expressly set forth in the Authorized License,
**
** 1.     This program, including its structure, sequence and organization,
**        constitutes the valuable trade secrets of Broadcom, and you shall
**        use all reasonable efforts to protect the confidentiality thereof,
**        and to use this information only in connection with your use of
**        Broadcom integrated circuit products.
**
** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
**        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
**        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
**        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
**        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
**        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
**        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
**        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
**        OF USE OR PERFORMANCE OF THE SOFTWARE.
**
** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
**        ITS LICENSORS BE LIABLE FOR
**        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
**              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
**              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
**              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
**        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
**              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
**              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
**              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*/

/*****************************************************************************
**                                                                           *
**  Name:          btld_prop.h                                               *
**                                                                           *
**  Description:   Property keys shared between btld and Android code        *
**                                                                           *
******************************************************************************/

#ifndef BTLD_PROP_H
#define BTLD_PROP_H

// NOTE : ALL PROPERTY NAMES MUST BE LESS THAN 32 CHARS LONG

#ifndef BRCM_PROPERTY_BT_ACTIVATION
#define BRCM_PROPERTY_BT_ACTIVATION "service.brcm.bt.activation"     /* '1' if Android BT is enabled (set by bluetooth.c) */
#endif

#ifndef BRCM_PROPERTY_BTLD_ACTIVE
#define BRCM_PROPERTY_BTLD_ACTIVE   "service.brcm.bt.btld"              /* '1' if btld is loaded and active (set by btld) */
#endif

#ifndef BRCM_PROPERTY_BTLD_PID
#define BRCM_PROPERTY_BTLD_PID      "service.brcm.bt.btld_pid"            /* pid of btld process if loaded, 0 otherwise */
#endif

#ifndef BRCM_PROPERTY_SOFT_ONOFF_ENABLE
// If this property is enabled, then Bluetooth power on/off shall be almost instantaneous. On the flip side, there shall be a marginal increase of 0.15mA
// in current consumption whenever Bluetooth is disabled.
#define BRCM_PROPERTY_SOFT_ONOFF_ENABLE      "service.brcm.bt.soft_onoff"     /* 1 if soft_onoff is enabled, 0 otherwise */
#endif

#ifndef BRCM_PROPERTY_CALL_ACTIVE
#define BRCM_PROPERTY_CALL_ACTIVE   "service.brcm.bt.call_active"       /* '1' if phone is in active call */
#endif

#ifndef BRCM_PROPERTY_AVRCP_PASS_THROUGH_ACTIVE
#define BRCM_PROPERTY_AVRCP_PASS_THROUGH_ACTIVE "service.brcm.bt.avrcp_pass_thru"    /* '1' if AVRCP pass through is active  */
#endif

#ifndef BRCM_PROPERTY_DUN_REDIRECTION
#define BRCM_PROPERTY_DUN_REDIRECTION "service.brcm.bt.btport_redir_on" /*'1' if redirection is active */
#endif

#ifndef DTUN_PROPERTY_SERVER_ACTIVE
#define DTUN_PROPERTY_SERVER_ACTIVE  "service.brcm.bt.srv_active"
#endif

#ifndef DTUN_PROPERTY_HCID_ACTIVE
#define DTUN_PROPERTY_HCID_ACTIVE    "service.brcm.bt.hcid_active"
#endif

#ifndef DTUN_PROPERTY_OBEXD_ACTIVE
#define DTUN_PROPERTY_OBEXD_ACTIVE   "service.brcm.bt.obexd_active"
#endif

#ifndef DTUN_PROPERTY_FM_BTLIF_SERVER_ACTIVE
#define DTUN_PROPERTY_FM_BTLIF_SERVER_ACTIVE   "service.brcm.bt.fm.active"
#endif

#endif /* BTLD_PROP_H */

