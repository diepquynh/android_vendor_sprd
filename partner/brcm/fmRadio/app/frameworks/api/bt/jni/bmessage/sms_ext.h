/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ************************************************************************************/
#pragma once

#if 0
/*
 * ETSI TS 100 901 v7.5.0
 * Technical realization of the Short Message Service (SMS)
 * Point-to-Point (PP)
 * 3GPP TS 03.40 7.5.0 Release 1998
 * GSM 03.40
 * http://www.etsi.com
 */
#endif
/*
 * Section 9.1.2.5 Adress Fields
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 * | 1 |   TYPE    | PLAN          |
 * +---+---+---+---+---+---+---+---+
 *
 */

#define MAX_GSM_COUNT                       (160)
#define MAX_BYTE_COUNT                      (140)
#define MAX_UCS_COUNT                       (70)

#define	SMS_ADDRESS_FLAG                    (0x80)

#define SMS_ADDRESS_TYPE_UNKNOWN            (0x00)
#define SMS_ADDRESS_TYPE_INTERNATIONAL      (0x10)
#define SMS_ADDRESS_TYPE_NATIONAL           (0x20)
#define SMS_ADDRESS_TYPE_NETWORKSPECIFIC    (0x30)
#define SMS_ADDRESS_TYPE_SUBSCRIBER         (0x40)
#define SMS_ADDRESS_TYPE_ALPHANUMERIC       (0x50)
#define SMS_ADDRESS_TYPE_ABBREVIATED        (0x60)
#define SMS_ADDRESS_TYPE_RESERVED           (0x70)

#define SMS_ADDRESS_TYPE_MASK               (0x70)

#define SMS_ADDRESS_PLAN_UNKNOWN            (0x00)
#define SMS_ADDRESS_PLAN_ISDN               (0x01)
#define SMS_ADDRESS_PLAN_DATA               (0x03)
#define SMS_ADDRESS_PLAN_TELEX              (0x04)
#define SMS_ADDRESS_PLAN_NATIONAL           (0x08)
#define SMS_ADDRESS_PLAN_PRIVATE            (0x09)
#define SMS_ADDRESS_PLAN_ERMES              (0x0A)
#define SMS_ADDRESS_PLAN_RESERVED           (0x0F)

#define SMS_ADDRESS_PLAN_MASK               (0x0F)

#define SMS_ADDRESS_HEADER(type, plan)      (SMS_ADDRESS_FLAG | (type) | (plan))
#define SMS_ADDRESS_TYPE(a)                 ((a) & SMS_ADDRESS_TYPE_MASK)
#define SMS_ADDRESS_PLAN(a)                 ((a) & SMS_ADDRESS_PLAN_MASK)

#define MAX_ADDR_LEN        (10)

/*
 * 9.2.2 PDU MESSAGE-TYPE-INDICATOR
 * SMS_DELIVER
 * SMS_DELIVER_REPORT
 * SMS_SUBMIT
 * SMS_SUBMIT_REPORT
 * SMS_REPORT
 * SMS_COMMAND
 */
#define SMS_TP_MTI_MASK             (0x03)
#define SMS_TP_MTI_DELIVER          (0x00)
#define SMS_TP_MTI_DELIVER_REPORT   (0x00)
#define SMS_TP_MTI_STATUS_REPORT    (0x02)
#define SMS_TP_MTI_COMMAND          (0x02)
#define SMS_TP_MTI_SUBMIT           (0x01)
#define SMS_TP_MTI_SUBMIT_REPORT    (0x01)
#define SMS_TP_MTI_RESERVED         (0x03)

#define SMS_TP_MTI_GET(a)           ((a) & SMS_TP_MTI_MASK)
inline unsigned char SMS_TP_MTI_SET(unsigned char c, unsigned char mti) {c &= ~SMS_TP_MTI_MASK; c |= mti; return c;}
#define SMS_TP_MTI_QUERY(a, mti)    (((a) & SMS_TP_MTI_MASK) == mti)

/*
 * 9.2.3.2 TP_MMS
 */
#define SMS_TP_MMS                  (0x04)

#define SMS_TP_MMS_GET(a)           ((a) & SMS_TP_MMS)
#define SMS_TP_MMS_SET(a, v)        ((v) ? ((a) | SMS_TP_MMS) : ((a) & ~SMS_TP_MMS))
#define SMS_TP_MMS_QUERY(a, v)      (((a) & SMS_TP_MMS) == SMS_TP_MMS)

/*
 * 9.2.3.3 TP_VPF
 */
#define SMS_TP_VPF_MASK             (0x18)
#define SMS_TP_VPF_NONE             (0x00)
#define SMS_TP_VPF_ENHANCED         (0x08)
#define SMS_TP_VPF_RELATIVE         (0x10)
#define SMS_TP_VPF_ABSOLUTE         (0x18)

#define SMS_TP_VPF_GET(a)           ((a) & SMS_TP_VPF_MASK)
inline unsigned char SMS_TP_VPF_SET(unsigned char c, unsigned char vpf) {c &= ~SMS_TP_VPF_MASK; c |= vpf; return c;}
#define SMS_TP_VPF_QUERY(a, vpf)    (((a) & SMS_TP_VPF_MASK) == vpf)

/*
 * 9.2.3.4 TP_SRI
 */
#define SMS_TP_SRI                  (0x20)
#define SMS_TP_SRI_GET(a)           ((a) & SMS_TP_SRI)
#define SMS_TP_SRI_SET(a, sri)      ((sri) ? (a | SMS_TP_SRI) : (a & ~SMS_TP_SRI))
#define SMS_TP_SRI_QUERY(a, sri)    (((a) & sri) == SMS_TP_SRI)

/*
 * 9.2.3.5 TP_SRR
 */
#define SMS_TP_SRR                  (0x20)

#define SMS_TP_SRR_GET(a)           ((a) & SMS_TP_SRR)
#define SMS_TP_SRR_SET(a, v)        ((v) ? ((a) | SMS_TP_SRR) : ((a) & ~SMS_TP_SRR))
#define SMS_TP_SRR_QUERY(a, v)      (((a) & v) == SMS_TP_SRR)

/*
 * 9.2.3.9 TP_PID
 */
#define SMS_TP_PID_TYPE_MASK        (0xC0)
#define SMS_TP_PID_TELEMATIC_BIT    (0x20)
#define SMS_TP_PID_TELEMATIC_QUERY(c) ((c & SMS_TP_PID_TELEMATIC_BIT) == SMS_TP_PID_TELEMATIC_BIT)
#define SMS_TP_PID_TYPE2_MASK       (0x1F)
#define SMS_TP_PID_TYPE2_MASK2      (0x3F)

/*
 * 9.2.3.10 TP_DCS (GSM 03.38 Section 4)
 */

#define SMS_TP_DCS_TYPE_MASK        (0xC0)
#define SMS_TP_DCS_COMPRESSED       (0x20)
#define SMS_TP_DCS_COMPRESSED_QUERY(c) ((c & SMS_TP_DCS_COMPRESSED) == SMS_TP_DCS_COMPRESSED)
#define SMS_TP_DCS_COMPRESSED_SET(c, v) ((v) ? ((c) |= SMS_TP_DCS_COMPRESSED) : ((c) &= ~SMS_TP_DCS_COMPRESSED))
#define SMS_TP_DCS_MESSAGECLASS     (0x10)

/*
 * 9.2.3.17 TP-Reply-Path
 */
#define SMS_TP_RP                   (0x80)
#define SMS_TP_RP_GET(a)            ((a) & SMS_TP_RP)
#define SMS_TP_RP_SET(a, v)         ((v) ? ((a) | SMS_TP_RP) : ((a) & ~SMS_TP_RP))
#define SMS_TP_RP_QUERY(a, v)       (((a) & SMS_TP_RP) == SMS_TP_RP)

/*
 * 9.2.3.22 TP-Failure-Cause
 */
#define SMS_TP_FCS_FLAG             (0x80)
#define SMS_TP_FCS_FLAG_GET(a)      ((a) & SMS_TP_FCS_FLAG)
#define SMS_TP_FCS_FLAG_QUERY(a, v) (((a) & SMS_TP_FCS_FLAG) == SMS_TP_FCS_FLAG)

/*
 * 9.2.3.23 TP-User-Data-Header-Indicator (GSM 03.38 Section 4)
 */
#define SMS_TP_UDHI                 (0x40)
#define SMS_TP_UDHI_GET(a)          ((a) & SMS_TP_UDHI)
#define SMS_TP_UDHI_SET(a, v)       ((v) ? ((a) | SMS_TP_UDHI) : ((a) & ~SMS_TP_UDHI))
#define SMS_TP_UDHI_QUERY(a, v)     (((a) & SMS_TP_UDHI) == SMS_TP_UDHI)

/*
 * 9.2.3.24 TP-User Data-Header
 */
#define SMS_TP_UDH_CONCATENATED_SMS_8BIT    (0)
#define SMS_TP_UDH_SPECIAL_SMS_INDICATION   (1)
#define SMS_TP_UDH_APPLICATION_PORT_8BIT    (4)
#define SMS_TP_UDH_APPLICATION_PORT_16BIT   (5)
#define SMS_TP_UDH_SMSC_CONTROL_PARAMETER   (6)
#define SMS_TP_UDH_SOURCE_INDICATOR         (7)
#define SMS_TP_UDH_CONCATENATED_SMS_16BIT   (8)
#define SMS_TP_UDH_WIRELESS_CONTROL_MESSAGE (9)

#define SMS_TP_UDH_STORE_MESSAGE            (0x80)

#define SMS_TP_UDH_SOURCE_SENDER            (01)
#define SMS_TP_UDH_SOURCE_RECEIVED          (02)
#define SMS_TP_UDH_SOURCE_SMSC              (03)

/*
 * 9.2.3.25 TP-Reject-Dupicates
 */
#define SMS_TP_RD                   (0x04)
#define SMS_TP_RD_GET(a)            ((a) & SMS_TP_RD)
#define SMS_TP_RD_SET(a, v)         ((v) ? ((a) | SMS_TP_RD) : ((a) & ~SMS_TP_RD))
#define SMS_TP_RD_QUERY(a, v)       (((a) & SMS_TP_RD) == SMS_TP_RD)

/*
 * 9.2.3.26 TP-Status-Report-Qualifier
 */
#define SMS_TP_SRQ                  (0x20)
#define SMS_TP_SRQ_GET(a)           ((a) & SMS_TP_SRQ)
#define SMS_TP_SRQ_SET(a, v)        ((v) ? ((a) | SMS_TP_SRQ) : ((a) & ~SMS_TP_SRQ))
#define SMS_TP_SRQ_QUERY(a, v)      (((a) & SMS_TP_SRQ) == SMS_TP_SRQ)

/*
 * 9.2.3.27 TP-Parameter-Indicator
 */
#define SMS_TP_PI_PID               (0x01)
#define SMS_TP_PI_PID_GET(a)        ((a) & SMS_TP_PI_PID)
#define SMS_TP_PI_PID_SET(a, v)     ((v) ? ((a) | SMS_TP_PI_PID) : ((a) & ~SMS_TP_PI_PID))
#define SMS_TP_PI_PID_QUERY(a, v)   (((a) & SMS_TP_PI_PID) == SMS_TP_PI_PID)

#define SMS_TP_PI_DCS               (0x02)
#define SMS_TP_PI_DCS_GET(a)        ((a) & SMS_TP_PI_DCS)
#define SMS_TP_PI_DCS_SET(a, v)     ((v) ? ((a) | SMS_TP_PI_DCS) : ((a) & ~SMS_TP_PI_DCS))
#define SMS_TP_PI_DCS_QUERY(a, v)   (((a) & SMS_TP_PI_DCS) == SMS_TP_PI_DCS)

#define SMS_TP_PI_UDL               (0x04)
#define SMS_TP_PI_UDL_GET(a)        ((a) & SMS_TP_PI_UDL)
#define SMS_TP_PI_UDL_SET(a, v)     ((v) ? ((a) | SMS_TP_PI_UDL) : ((a) & ~SMS_TP_PI_UDL))
#define SMS_TP_PI_UDL_QUERY(a, v)   (((a) & SMS_TP_PI_UDL) == SMS_TP_PI_UDL)

