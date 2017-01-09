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
#include "sms.h"
#include "utils/Log.h"
#include "utils/misc.h"

unsigned short CConcatenatedSMS::_cReferenceNumber = 0;

typedef enum _TP_TAG
{
    TP_NULL,

    TP_MTI,     /*Message Type Indicator*/
    TP_MMS,     /*More Message to Send*/
    TP_RP,      /*Reply Path*/
    TP_UDHI,    /*User Data Header Indicator*/
    TP_SRI,     /*Status Report Indicator*/
    TP_OA,      /*Originating Address*/
    TP_PID,     /*Protocol Identifier*/
    TP_DCS,     /*Date Coding Scheme*/
    TP_SCTS,    /*Service Center Time Stamp*/
    TP_UDL,     /*User Data Length*/
    TP_UD,      /*User Data*/

    TP_FCS,     /*Failure Cause*/
    TP_PI,      /*Parameter Indicator*/

    TP_RD,      /*Reject Duplicates*/
    TP_VPF,     /*Validity Period Format*/
    TP_SRR,     /*Status Report Request*/
    TP_MR,      /*Message Reference*/
    TP_DA,      /*Destination Address*/
    TP_VP,      /*Validity Period*/

    TP_SRQ,     /*Status Report Qualifier*/
    TP_RA,      /*Recipient Address*/
    TP_DT,      /*Discharge Time*/
    TP_ST,      /*Status*/

    TP_CT,      /*Command Type*/
    TP_MN,      /*Message Number*/
    TP_CDL,     /*Command Data Length*/
    TP_CD,      /*Command Data*/
    TP_SMSC,    /* SMSC Address*/
    TP_MAXIMUM,
    TP_OPTIONAL = 0x80  // optional tag may have zero or variable length bytes
} eTP_TAG;

/*
 * Tag Length determines how to advance TPDU pointer in the encoding/decoding process.
 *   0 means variable length field, the decoder/encoder decides number of bytes to advance in the return size_t.
 * > 0 means a fixed length field, TPDU pointer always advances number of bytes specified in this table
 * < 0 means a bit field (less than 1 byte), TPDU pointer does not advance until next tag is non-negative
 *
 * NOTE: so far the aggregate of bit fields has never exceeded one byte (8 bits.)  If this happends in the future
 *       need to accumulate number of bits and advance the TPDU pointers accordingly.
 *
 */
const int TP_TAG_len[] =
{
     0,               /* TP_BEGIN */

    -2, //TP_MTI,     /*Message Type Indicator*/
    -1, //TP_MMS,     /*More Message to Send*/
    -1, //TP_RP,      /*Reply Path*/
    -1, //TP_UDHI,    /*User Data Header Indicator*/
    -1, //TP_SRI,     /*Status Report Indicator*/
     0, //TP_OA,      /*Originating Address*/
     1, //TP_PID,     /*Protocol Identifier*/
     1, //TP_DCS,     /*Date Coding Scheme*/
     7, //TP_SCTS,    /*Service Center Time Stamp*/
     1, //TP_UDL,     /*User Data Length*/
     0, //TP_UD,      /*User Data*/

     0, //TP,FCS,     /*Failure Cause*/
     1, //TP_PI,      /*Parameter Indicator*/

    -1, //TP_RD,      /*Reject Duplicates*/
    -2, //TP_VPF,     /*Validity Period Format*/
    -1, //TP_SRR,     /*Status Report Request*/
     1, //TP_MR,      /*Message Reference*/
     0, //TP_DA,      /*Destination Address*/
     7, //TP_VP,      /*Validity Period*/

    -1, //TP_SRQ,     /*Status Report Qualifier*/
     0, //TP_RA,      /*Recipient Address*/
     7, //TP_DT,      /*Discharge Time*/
     1, //TP_ST,      /*Status*/

     1, //TP_CT,      /*Command Type*/
     1, //TP_MN,      /*Message Number*/
     1, //TP_CDL,     /*Command Data Length*/
     1, //TP_CD       /*Command Data*/
     0, //TP_SMSC     /*SMSC Address */
     0                /*not used */
};

/*9.2.2.1*/
static const unsigned char aTP_DELIVER[] =
{
    (unsigned char)(TP_SMSC),
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_MMS),
    (unsigned char)(TP_RP),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_SRI  | TP_OPTIONAL),
    (unsigned char)(TP_OA),
    (unsigned char)(TP_PID),
    (unsigned char)(TP_DCS),
    (unsigned char)(TP_SCTS),
    (unsigned char)(TP_UDL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.1a (i)*/
static const unsigned char aTP_DELIVER_REPORT_RP_ERROR[] =
{
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_FCS),
    (unsigned char)(TP_PI),
    (unsigned char)(TP_PID  | TP_OPTIONAL),
    (unsigned char)(TP_DCS  | TP_OPTIONAL),
    (unsigned char)(TP_UDL  | TP_OPTIONAL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.1a (ii)*/
static const unsigned char aTP_DELIVER_REPORT_RP_ACK[] =
{
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_PI),
    (unsigned char)(TP_PID  | TP_OPTIONAL),
    (unsigned char)(TP_DCS  | TP_OPTIONAL),
    (unsigned char)(TP_UDL  | TP_OPTIONAL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.2*/
static const unsigned char aTP_SUBMIT[] =
{
    (unsigned char)(TP_SMSC),
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_RD),
    (unsigned char)(TP_VPF),
    (unsigned char)(TP_RP),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_SRR  | TP_OPTIONAL),
    (unsigned char)(TP_MR),
    (unsigned char)(TP_DA),
    (unsigned char)(TP_PID),
    (unsigned char)(TP_DCS),
    (unsigned char)(TP_VP   | TP_OPTIONAL),
    (unsigned char)(TP_UDL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.2a (i)*/
static const unsigned char aTP_SUBMIT_REPORT_RP_ERROR[] =
{
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_FCS),
    (unsigned char)(TP_PI),
    (unsigned char)(TP_SCTS),
    (unsigned char)(TP_PID  | TP_OPTIONAL),
    (unsigned char)(TP_DCS  | TP_OPTIONAL),
    (unsigned char)(TP_UDL  | TP_OPTIONAL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.2a (ii)*/
static const unsigned char aTP_SUBMIT_REPORT_RP_ACK[] =
{
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_PI),
    (unsigned char)(TP_SCTS),
    (unsigned char)(TP_PID  | TP_OPTIONAL),
    (unsigned char)(TP_DCS  | TP_OPTIONAL),
    (unsigned char)(TP_UDL  | TP_OPTIONAL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.3*/
static const unsigned char aTP_STATUS_REPORT[] =
{
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_MMS),
    (unsigned char)(TP_SRQ),
    (unsigned char)(TP_MR),
    (unsigned char)(TP_RA),
    (unsigned char)(TP_SCTS),
    (unsigned char)(TP_DT),
    (unsigned char)(TP_ST),
    (unsigned char)(TP_PI   | TP_OPTIONAL),
    (unsigned char)(TP_PID  | TP_OPTIONAL),
    (unsigned char)(TP_DCS  | TP_OPTIONAL),
    (unsigned char)(TP_UDL  | TP_OPTIONAL),
    (unsigned char)(TP_UD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

/*9.2.2.4*/
static const unsigned char aTP_COMMAND[] =
{
    (unsigned char)(TP_MTI),
    (unsigned char)(TP_UDHI | TP_OPTIONAL),
    (unsigned char)(TP_SRR),
    (unsigned char)(TP_MR),
    (unsigned char)(TP_PID),
    (unsigned char)(TP_CT),
    (unsigned char)(TP_MN),
    (unsigned char)(TP_DA),
    (unsigned char)(TP_CDL),
    (unsigned char)(TP_CD   | TP_OPTIONAL),
    (unsigned char)(TP_NULL)
};

const unsigned char* CSmsTpdu::m_aTPDU_Tags[] = {
    (const unsigned char*)aTP_DELIVER,
    (const unsigned char*)aTP_DELIVER_REPORT_RP_ACK,
    (const unsigned char*)aTP_STATUS_REPORT,
    (const unsigned char*)aTP_COMMAND,
    (const unsigned char*)aTP_SUBMIT,
    (const unsigned char*)aTP_SUBMIT_REPORT_RP_ACK
};

const unsigned char CSmsTpdu::m_aTPDU_MessageType[] = {
    SMS_TP_MTI_DELIVER,         //(0x00)
    SMS_TP_MTI_DELIVER_REPORT,  //(0x00)
    SMS_TP_MTI_STATUS_REPORT,   //(0x02)
    SMS_TP_MTI_COMMAND,         //(0x02)
    SMS_TP_MTI_SUBMIT,          //(0x01)
    SMS_TP_MTI_SUBMIT_REPORT    //(0x01)
};

void CSmsTpdu::Reset(BOOL bConcate/* = FALSE*/)
{
    m_saServiceCenter.Reset();
    m_saRecipient.Reset();
    m_stValidityPeriod.Reset();
    m_stDischarge.Reset();
    if (bConcate)
    {
        m_csConcatenatedSMS.Reset();        // do not reset Multi-Part SMS info...
    }
    else
    {
        m_saOriginating.Reset();
        m_saDestination.Reset();
        m_stServiceCenter.Reset();
    }
    m_csmiMessageIndication.Reset();
    m_capPort.Reset();
    m_eVPF                      = SMS_VPF_NONE;
    m_eProtocolIndicator        = GSM_DEFAULT;
    m_eDCS                      = CLASS_0;
    m_eCharset                  = DEFAULT_ALPHABET;
    m_eUDHSource                = UNKNOWN;
    m_cchUserData               = 0;
    m_cchCommandData            = 0;
    m_cbPDU                     = 0;
    m_cbPDURaw                  = 0;
    m_iStatus                   = 0;
    m_bMoreMessageToSend        = false;
    m_bStatusReportIndicator    = false;
    m_bStatusReportRequest      = false;
    m_bCompressed               = false;
    m_bUDHIndicator             = false;
    m_bRejectDuplicates         = false;
    m_bStatusReportQualifier    = false;
    m_bHasUDL                   = false;
    m_bHasDCS                   = false;
    m_bHasPID                   = false;
    m_bReplyPath                = false;
    m_cMessageReference         = 0;
    m_cMessageNumber            = 0;
    m_cCommandType              = 0;
    m_cFailureCause             = 0;
    m_aUserData[0]              = 0;
    m_aCommandData[0]           = 0;
    m_cSMSCControlParameters    = 0;
    m_ulPDULength               = 0;
    memset(m_aPDU, 0, sizeof(m_aPDU));
    memset(m_aPDU_HEX, 0, sizeof(m_aPDU_HEX));
}

const unsigned char* CSmsTpdu::Decode(const unsigned char* pPDU, const unsigned char* pEndPDU)
{
    ALOGD(" %s %s", __FUNCTION__, pPDU);
    bool bResult = false;
    if (!pPDU || !(pPDU < pEndPDU))
        return pPDU;

    Reset();


    size_t cbPDU = DecodeHex(m_aPDU, m_aPDU+sizeof(m_aPDU), pPDU, pEndPDU);
    if (!cbPDU)
        return pPDU;

    const unsigned char* pTags      = m_aTPDU_Tags[MessageType()];
    const unsigned char* pWorkPDU   = (const unsigned char*)m_aPDU;
    const unsigned char* pBeginPDU  = (const unsigned char*)m_aPDU;
    const unsigned char* pWorkEnd   = (const unsigned char*)m_aPDU + cbPDU;

    while (*pTags != TP_NULL && pWorkPDU < pWorkEnd)
    {
        const unsigned char tagCurrent = *pTags++;
        const unsigned char tagNext    = *pTags;
        size_t cb = 0;
        switch (tagCurrent & ~TP_OPTIONAL)
        {
        case TP_SMSC:

            cb = m_saServiceCenter.Parse(pWorkPDU, pWorkEnd);

            break;
        case TP_MTI: /*9.2.3.1*/

            if (!SMS_TP_MTI_QUERY(*pWorkPDU, m_aTPDU_MessageType[MessageType()]))
                return pPDU;

            bResult = true;
            break;
        case TP_MMS: /*9.2.3.2*/

            MoreMessagetoSend(SMS_TP_MMS_QUERY(*pWorkPDU, SMS_TP_MMS));
            break;
        case TP_VPF: /*9.2.3.3*/

            if (SMS_TP_VPF_QUERY(*pWorkPDU, SMS_TP_VPF_NONE))
                ValidityPeriodFormat(SMS_VPF_NONE);
            else if (SMS_TP_VPF_QUERY(*pWorkPDU, SMS_TP_VPF_RELATIVE))
                ValidityPeriodFormat(SMS_VPF_RELATIVE);
            else if (SMS_TP_VPF_QUERY(*pWorkPDU, SMS_TP_VPF_ENHANCED))
                ValidityPeriodFormat(SMS_VPF_ENHANCED);
            else if (SMS_TP_VPF_QUERY(*pWorkPDU, SMS_TP_VPF_ABSOLUTE))
                ValidityPeriodFormat(SMS_VPF_ABSOLUTE);
            break;
        case TP_SRI: /*9.2.3.4*/

            StatusReportIndicator(SMS_TP_SRI_QUERY(*pWorkPDU, SMS_TP_SRI));
            break;
        case TP_SRR: /*9.2.3.5*/

            StatusReportRequest(SMS_TP_SRR_QUERY(*pWorkPDU, SMS_TP_SRR));
            break;
        case TP_MR: /*9.2.3.6*/

            MessageReference(*pWorkPDU);
            break;
        case TP_OA: /*9.2.3.7*/

            cb = m_saOriginating.Parse(pWorkPDU, pWorkEnd);
            break;
        case TP_DA: /*9.2.3.8*/

            cb = m_saDestination.Parse(pWorkPDU, pWorkEnd);
            break;
        case TP_PID: /*9.2.3.9*/

            if (!(tagCurrent & TP_OPTIONAL) || HasPID())
            {
                ParsePID(*pWorkPDU);
                cb = 1;
            }
            break;
        case TP_DCS: /*9.2.3.10*/

            if (!(tagCurrent & TP_OPTIONAL) || HasDCS())
            {
                ParseDCS(*pWorkPDU);
                cb = 1;
            }
            break;
        case TP_SCTS: /*9.2.3.11*/

            cb = m_stServiceCenter.Parse(pWorkPDU, pWorkEnd);
            break;
        case TP_VP: /*9.2.3.12*/

            switch(ValidityPeriodFormat())
            {
            case SMS_VPF_RELATIVE:
                if (*pWorkPDU < 144)
                {
                    long min = (*pWorkPDU *5);
                    int hr = min / 60;
                    m_stValidityPeriod.hour(hr);
                    m_stValidityPeriod.minute(min % 60);
                }
                else if (*pWorkPDU < 168)
                {
                    long min = (*pWorkPDU - 143)*30;
                    int hr = 12 +  min / 60;
                    m_stValidityPeriod.hour(hr);
                    m_stValidityPeriod.minute(min % 60);
                }
                else if (*pWorkPDU < 197)
                {
                    m_stValidityPeriod.day(*pWorkPDU - 166);
                }
                else
                {
                    int day = (*pWorkPDU - 192) * 7;
                    m_stValidityPeriod.day(day);
                }
                cb = 1;
                break;
            case SMS_VPF_ENHANCED:
                // todo: parse out enhanced format...
                cb = 7;
            case SMS_VPF_NONE:
                break;
            case SMS_VPF_ABSOLUTE:
                cb = m_stValidityPeriod.Parse(pWorkPDU, pWorkEnd);
                break;
            }
            break;
        case TP_DT: /*9.2.3.13*/

            cb = m_stDischarge.Parse(pWorkPDU, pWorkEnd);
            break;
        case TP_RA: /*9.2.3.14*/

            cb = m_saRecipient.Parse(pWorkPDU, pWorkEnd);
            break;
        case TP_ST: /*9.2.3.15*/

            Status(*pWorkPDU);
            break;
        case TP_UDL: /*9.2.3.16*/

            if (!(tagCurrent & TP_OPTIONAL) || HasUDL())
            {
                UserDataLength(*pWorkPDU);
                cb = 1;
            }
            break;
        case TP_RP: /*9.2.3.17*/

            ReplyPath(SMS_TP_RP_QUERY(*pWorkPDU, SMS_TP_RP));
            break;
        case TP_MN: /*9.2.3.18*/

            MessageNumber(*pWorkPDU);
            break;
        case TP_CT: /*9.2.3.19*/

            CommandType(*pWorkPDU);
            break;
        case TP_CDL: /*9.2.3.20*/

            CommandDataLength(*pWorkPDU);
            break;
        case TP_CD: /*9.2.3.21*/

            if (cb = CommandDataLength())
            {
                memcpy(m_aCommandData, pWorkPDU, cb);
            }
            break;
        case TP_FCS: /*9.2.3.22*/

            if (SMS_TP_FCS_FLAG_QUERY(*pWorkPDU, SMS_TP_FCS_FLAG))
            {
                FailureCause(*pWorkPDU);
                cb = 1;
            }
            break;
        case TP_UDHI: /*9.2.3.23*/

            UDHIndicator(SMS_TP_UDHI_QUERY(*pWorkPDU, SMS_TP_UDHI));
            break;
        case TP_UD: /*9.2.3.24*/

            if ((cb = UserDataLength()) > 0)
            {
                if (UDHIndicator())
                {
                    size_t cbUDH = *pWorkPDU;
                    if (cb > cbUDH + 1)
                    {
                        // parse UDH
                        ParseUDH(++pWorkPDU, cbUDH);
                        // update cb and pWorkPDU to skip over
                        pWorkPDU += cbUDH;
                        cb -= (cbUDH + 1);
                    }
                }
                switch (Charset())
                {
                case DEFAULT_ALPHABET:
                    {
                    unsigned char aTemp[512];
                    size_t cb1 = UnpackGSM7(cb, aTemp, aTemp+sizeof(aTemp), pWorkPDU, pWorkEnd);
                    if (cb1)
                        GSM2Unicode(aTemp, aTemp+cb, m_aUserData);
                    cb = cb1;
                    break;
                    }
                case EIGHT_BIT:
                    cb = Byte2Unicode(pWorkPDU, pWorkPDU+cb, m_aUserData);
                    break;
                case UCS_2:
                    cb = UCS2Unicode(pWorkPDU, pWorkPDU+cb, m_aUserData);
                    break;
                default:
                    break;
                }
            }
            break;

        case TP_RD: /*9.2.3.25*/

            RejectDuplicates(SMS_TP_RD_QUERY(*pWorkPDU, SMS_TP_RD));
            break;
        case TP_SRQ: /*9.2.3.26*/

            StatusReportQualifier(SMS_TP_SRQ_QUERY(*pWorkPDU, SMS_TP_SRQ));
            break;
        case TP_PI: /*9.2.3.26*/

            if (SMS_TP_PI_PID_QUERY(*pWorkPDU, SMS_TP_PI_PID))
                HasPID(true);
            if (SMS_TP_PI_DCS_QUERY(*pWorkPDU, SMS_TP_PI_DCS))
                HasDCS(true);
            if (SMS_TP_PI_UDL_QUERY(*pWorkPDU, SMS_TP_PI_UDL))
                HasUDL(true);
            break;
        default:

            break;
        }
/*
 * advance TPDU pointer to decode next tag
 */

        if (TP_TAG_len[tagCurrent & ~TP_OPTIONAL] < 0)
        {
            if (TP_TAG_len[tagNext & ~TP_OPTIONAL] >= 0)
            {
                pWorkPDU++;
            }
            // else do not advance to next byte yet
        }
        else if ((TP_TAG_len[tagCurrent & ~TP_OPTIONAL] == 0) || (tagCurrent & TP_OPTIONAL))
        {
            pWorkPDU += cb;
        }
        else
        {
            pWorkPDU += TP_TAG_len[tagCurrent & ~TP_OPTIONAL];
        }
    }

    return pPDU+(pWorkPDU-pBeginPDU)*2;
}

void CSmsTpdu::ParsePID(unsigned char cPID)
{
    if ((cPID & SMS_TP_PID_TYPE_MASK) == 0x00)
    {
        unsigned char cType = cPID & SMS_TP_PID_TYPE2_MASK;
        bool bTelematic = SMS_TP_PID_TELEMATIC_QUERY(cPID);
        if (bTelematic)
        {
            if (cType < RESERVED_1)
                ProtocolIdentifier((SMS_PROTOCOL)cType);
            else if (cType <= 0x10)
                ProtocolIdentifier(RESERVED_1);
            else if (cType <= 0x12)
                ProtocolIdentifier((SMS_PROTOCOL)(MESSAGING_HANDLING_FACILITY + (cType & 0x0F)));
            else if (cType == 0x1F)
                ProtocolIdentifier(GSM_DEFAULT);
            else
                ProtocolIdentifier(RESERVED_1);
        }
    }
    else if ((cPID & SMS_TP_PID_TYPE_MASK) == 0x40)
    {
        unsigned char cType2 = cPID & SMS_TP_PID_TYPE2_MASK2;
        if (cType2 < 0x08)
            ProtocolIdentifier((SMS_PROTOCOL)(SMS_TYPE_0 + cType2));
        else if (cType2 < 0x1F)
            ProtocolIdentifier(RESERVED_1);
        else if (cType2 == 0x1F)
            ProtocolIdentifier(RETURN_CALL_MESSAGE);
        else  if (cType2 <0x3D)
            ProtocolIdentifier(RESERVED_1);
        else
            ProtocolIdentifier((SMS_PROTOCOL)(ME_DATA_DOWNLOAD + (cType2 & 0x03 - 1 )));
    }
}


void CSmsTpdu::ParseDCS(unsigned char cDCS)
{
    unsigned char cType = cDCS & SMS_TP_DCS_TYPE_MASK;
    if (cType == 0x00)
    {
        Compressed(SMS_TP_DCS_COMPRESSED_QUERY(cDCS));
        if (cDCS & SMS_TP_DCS_MESSAGECLASS)
            DataCodingScheme((SMS_DCS_MESSAGE_CLASS)(cDCS & SMS_DCS_MASK));
        else
            DataCodingScheme(SMS_DCS_NONE);
        Charset((SMS_CHARSET)(cDCS & SMS_CHARSET_MASK));
    }
    //
    // Other DCS ignored...
    // Todo:
    //
}

void CSmsTpdu::ParseUDH(const unsigned char* pWork, size_t cb)
{
    if (!pWork || !cb)
        return;

    while (cb > 0)
    {
        switch (*pWork++)
        {
        case SMS_TP_UDH_CONCATENATED_SMS_8BIT: /*9.2.3.24.1*/
            if (*pWork == 3) // safety check
            {
                m_csConcatenatedSMS.ReferenceNumber(*(pWork+1));
                m_csConcatenatedSMS.MaxNumber(*(pWork+2));
                m_csConcatenatedSMS.SequenceNumber(*(pWork+3));
            }
            break;
        case SMS_TP_UDH_SPECIAL_SMS_INDICATION: /*9.2.3.24.2*/
            if (*pWork == 2) // safety check
            {
                m_csmiMessageIndication.StoreMessage((*(pWork+1) & SMS_TP_UDH_STORE_MESSAGE) != SMS_TP_UDH_STORE_MESSAGE);
                m_csmiMessageIndication.MessageIndication((CSMSSpecialMessageIndication::MESSAGE_INDICATION)(*(pWork+1) & ~SMS_TP_UDH_STORE_MESSAGE));
                m_csmiMessageIndication.MessageCount(*(pWork+2));
            }
            break;
        case SMS_TP_UDH_APPLICATION_PORT_8BIT: /*9.2.3.24.3*/
            if (*pWork == 2) // safety check
            {
                m_capPort.Mode(CApplicationPort::EIGHT);
                m_capPort.Destination(*(pWork+1));
                m_capPort.Originator(*(pWork+2));
            }
            break;
        case SMS_TP_UDH_APPLICATION_PORT_16BIT: /*9.2.3.24.4*/
            if (*pWork == 4) // safety check
            {
                m_capPort.Mode(CApplicationPort::SIXTEEN);
                m_capPort.Destination((*(pWork+1) << 8) + *(pWork+2));
                m_capPort.Originator((*(pWork+3) << 8) + *(pWork+4));
            }
            break;
        case SMS_TP_UDH_SMSC_CONTROL_PARAMETER: /*9.2.3.24.5*/
            SMSCControlParameters(*pWork);
            break;
        case SMS_TP_UDH_SOURCE_INDICATOR: /*9.2.3.24.6*/
            UDHSource((SMS_UDH_SOURCE)(*pWork+1));
            break;
        case SMS_TP_UDH_CONCATENATED_SMS_16BIT: /*9.2.3.24.8*/
            if (*pWork == 4) // safety check
            {
                m_csConcatenatedSMS.ReferenceNumber((*(pWork+1) << 8) + *(pWork+2));
                m_csConcatenatedSMS.MaxNumber(*(pWork+3));
                m_csConcatenatedSMS.SequenceNumber(*(pWork+4));
            }
            break;
        case SMS_TP_UDH_WIRELESS_CONTROL_MESSAGE: /*9.2.3.24.9*/
            // Todo:
            break;
        default:
            break;
        }

        if (cb > (size_t)(*pWork + 1))
            cb -= (size_t)(*pWork + 1);
        else
            break;
    }
}

size_t CSmsTpdu::ComposeUDH(const unsigned char* pWorkPDU)
{
// only encode Multi-Part using 16bit reference number
typedef struct _UDH_Struct
{
    unsigned char cLen;
    unsigned char cTag;
    unsigned char cSubLen;
    unsigned char cReferenceNumber[2];
    unsigned char cMaxNumber;
    unsigned char cSequenceNumber;
} UDH_Header;

#define SIZE_OF_UDH_HEADER          (7)
#define SIZE_OF_UDH_HEADER_GSM      (8)

    UDH_Header* pHeader             = (UDH_Header*)pWorkPDU;
    pHeader->cLen                   = SIZE_OF_UDH_HEADER - 1;
    pHeader->cTag                   = SMS_TP_UDH_CONCATENATED_SMS_16BIT;
    pHeader->cSubLen                = 4;
    pHeader->cReferenceNumber[0]    = HI_BYTE(m_csConcatenatedSMS.ReferenceNumber());
    pHeader->cReferenceNumber[1]    = LO_BYTE(m_csConcatenatedSMS.ReferenceNumber());
    pHeader->cMaxNumber             = m_csConcatenatedSMS.MaxNumber();
    pHeader->cSequenceNumber        = m_csConcatenatedSMS.SequenceNumber();

    return SIZE_OF_UDH_HEADER;
}

bool CSmsTpdu::Encode(const WCHAR* pszUserData, WCHAR** pszEndUserData)
{
    bool bResult = false;
    ALOGD("%s UserData = %s",__FUNCTION__, pszUserData);
    const unsigned char* pTags  = m_aTPDU_Tags[MessageType()];
    unsigned char *pWorkPDU     = m_aPDU;
    unsigned char* pEndPDU      = m_aPDU + sizeof(m_aPDU);

    m_ulPDULength               = 0;
    memset(m_aPDU, 0, sizeof(m_aPDU));
    memset(m_aPDU_HEX, 0, sizeof(m_aPDU_HEX));

    size_t cch = pszUserData ? strlen (pszUserData) : 0;
    if (cch)
    {
        ALOGD("%s cch is ok",__FUNCTION__);
        if (m_csConcatenatedSMS.ReferenceNumber())
        {
            ALOGD("%s m_csConcatenatedSMS.ReferenceNumber()",__FUNCTION__);
            m_csConcatenatedSMS.NextSequenceNumber();
            SMS_CHARSET eCharset = m_csConcatenatedSMS.Charset();
            Charset(eCharset);
            if (eCharset == EIGHT_BIT)
            {
                size_t szByte = Unicode2Byte(pszUserData, NULL, NULL, NULL);
                UserDataLength(szByte > (MAX_BYTE_COUNT -  SIZE_OF_UDH_HEADER) ? MAX_BYTE_COUNT : (szByte + SIZE_OF_UDH_HEADER));
            }
            else // DEFAULT_ALPHABET, never use UCS2 in multipart...
            {
                size_t szGSM = Unicode2GSM(pszUserData, NULL, NULL, NULL);
                UserDataLength(szGSM > (MAX_GSM_COUNT - SIZE_OF_UDH_HEADER_GSM)? MAX_GSM_COUNT: (cch + SIZE_OF_UDH_HEADER));
            }
            UDHIndicator(true);
        }
        else // if (cch)
        {
            int  nMessageCount = 0;
            size_t szGSM = Unicode2GSM(pszUserData, NULL, NULL, NULL);
            if (!szGSM) // there are non-GSM in the UserData
            {
                ALOGD("%s !szGsm - Unicode2GSM failed",__FUNCTION__);
                if (cch <= MAX_UCS_COUNT)
                {
                    Charset(UCS_2);
                    UserDataLength(cch);
                }
                else
                {
                    Charset(EIGHT_BIT);
                    size_t szByte = Unicode2Byte(pszUserData, NULL, NULL, NULL);
                    if (szByte <= MAX_BYTE_COUNT)
                        UserDataLength(szByte);
                    else
                    {
                        UDHIndicator(true);
                        nMessageCount = (int)(szByte + MAX_BYTE_COUNT - SIZE_OF_UDH_HEADER - 1) / (MAX_BYTE_COUNT - SIZE_OF_UDH_HEADER);
                        UserDataLength(MAX_BYTE_COUNT);
                    }
                }
            }
            else if (szGSM < MAX_GSM_COUNT)
            {
                ALOGD("%s szGSM < MAX_GSM_COUNT",__FUNCTION__);
                Charset(DEFAULT_ALPHABET);
                UserDataLength(szGSM);
            }
            else // multi-part
            {
                ALOGD("%s multipart",__FUNCTION__);
                UDHIndicator(true);
                nMessageCount = (int)(szGSM + MAX_GSM_COUNT - SIZE_OF_UDH_HEADER_GSM - 1) / (MAX_GSM_COUNT - SIZE_OF_UDH_HEADER_GSM);
                UserDataLength(MAX_GSM_COUNT);
            }

            if (UDHIndicator())
            {
                m_csConcatenatedSMS.NextReferenceNumber();
                m_csConcatenatedSMS.MaxNumber(nMessageCount);
                m_csConcatenatedSMS.SequenceNumber(0);
                m_csConcatenatedSMS.Charset(Charset());
            }
        }
    }

    while (*pTags != TP_NULL && pWorkPDU < pEndPDU)
    {
        const unsigned char tagCurrent = *pTags++;
        const unsigned char tagNext    = *pTags;
        size_t cb = 0;
        switch (tagCurrent & ~TP_OPTIONAL)
        {
        case TP_SMSC:
            ALOGD("%s TP_SMSC",__FUNCTION__);
            cb = m_saServiceCenter.Compose(pWorkPDU, pEndPDU);
            break;
        case TP_MTI: /*9.2.3.1*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            {
                MESSAGE_TYPE eMsgType = MessageType();
                unsigned char cMsgType = m_aTPDU_MessageType[eMsgType];
                *pWorkPDU = SMS_TP_MTI_SET(*pWorkPDU, cMsgType);
                // *pWorkPDU = SMS_TP_MTI_SET(*pWorkPDU, m_aTPDU_MessageType[MessageType()]);
                bResult = true;
            }
            break;
        case TP_MMS: /*9.2.3.2*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_MMS_SET(*pWorkPDU, MoreMessagetoSend());
            break;
        case TP_VPF: /*9.2.3.3*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_VPF_SET(*pWorkPDU, ValidityPeriodFormat());
            break;
        case TP_SRI: /*9.2.3.4*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_SRI_SET(*pWorkPDU, StatusReportIndicator());
            break;
        case TP_SRR: /*9.2.3.5*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_SRR_SET(*pWorkPDU, StatusReportRequest());
            break;
        case TP_MR: /*9.2.3.6*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = MessageReference();
            break;
        case TP_OA: /*9.2.3.7*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            cb = m_saOriginating.Compose(pWorkPDU, pEndPDU);
            break;
        case TP_DA: /*9.2.3.8*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            cb = m_saDestination.Compose(pWorkPDU, pEndPDU);
            break;
        case TP_PID: /*9.2.3.9*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if (!(tagCurrent & TP_OPTIONAL) || HasPID())
            {
                // page 53:
                // For the straightforward case of simple MS-to-SC short message transfer
                // the Protocol Identifier is set to value 0.
                *pWorkPDU = 0;
                cb = 1;
            }
            break;
        case TP_DCS: /*9.2.3.10*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if (!(tagCurrent & TP_OPTIONAL)         ||
                HasDCS()                            ||
                DataCodingScheme() != SMS_DCS_NONE  ||
                Charset() != DEFAULT_ALPHABET       )
            {
                // never encode compressed SMS...
                *pWorkPDU = Charset() + DataCodingScheme() + HAS_MESSAGE_CLASS;
                cb = 1;
            }
            break;
        case TP_SCTS: /*9.2.3.11*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            cb = m_stServiceCenter.Compose(pWorkPDU, pEndPDU);
            break;
        case TP_VP: /*9.2.3.12*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            switch(ValidityPeriodFormat())
            {
            case SMS_VPF_RELATIVE:
            case SMS_VPF_ENHANCED:
            case SMS_VPF_NONE:
                break;
            case SMS_VPF_ABSOLUTE:
                cb = m_stValidityPeriod.Compose(pWorkPDU, pEndPDU);
                break;
            }
            break;
        case TP_DT: /*9.2.3.13*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            cb = m_stDischarge.Compose(pWorkPDU, pEndPDU);
            break;
        case TP_RA: /*9.2.3.14*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            cb = m_saRecipient.Compose(pWorkPDU, pEndPDU);
            break;
        case TP_ST: /*9.2.3.15*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = Status();
            break;
        case TP_UDL: /*9.2.3.16*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if (!(tagCurrent & TP_OPTIONAL) || HasUDL())
            {
                *pWorkPDU = (unsigned char)UserDataLength();
                cb = 1;
            }
            break;
        case TP_RP: /*9.2.3.17*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_RP_SET(*pWorkPDU, ReplyPath());
            break;
        case TP_MN: /*9.2.3.18*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = MessageNumber();
            break;
        case TP_CT: /*9.2.3.19*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = CommandType();
            break;
        case TP_CDL: /*9.2.3.20*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = (unsigned char)CommandDataLength();
            break;
        case TP_CD: /*9.2.3.21*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if (cb = CommandDataLength())
            {
                memcpy(pWorkPDU, m_aCommandData, cb);
            }
            break;
        case TP_FCS: /*9.2.3.22*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if (FailureCause())
            {
                *pWorkPDU = FailureCause();
                cb = 1;
            }
            break;
        case TP_UDHI: /*9.2.3.23*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_UDHI_SET(*pWorkPDU, UDHIndicator());
            break;
        case TP_UD: /*9.2.3.24*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if ((cb = UserDataLength()) > 0)
            {
                size_t cbUserData = UserDataLength();
                if (UDHIndicator())
                {
                    cb = ComposeUDH(pWorkPDU);
                    pWorkPDU += cb;
                    cbUserData -= cb;
                }
                switch (Charset())
                {
                case DEFAULT_ALPHABET:
                    {
                        if (UDHIndicator() && (m_csConcatenatedSMS.SequenceNumber() + 1 < m_csConcatenatedSMS.MaxNumber()))
                        {
                            cbUserData = ((MAX_BYTE_COUNT - SIZE_OF_UDH_HEADER) * 8) / 7;  //== 152
                        }
                        unsigned char aTemp[160];
                        size_t cb1 = Unicode2GSM(pszUserData, pszEndUserData, aTemp, aTemp + cbUserData);
                        cb = PackGSM7(&cb1, pWorkPDU, pEndPDU, aTemp, aTemp+cb1);
                    }
                    break;
                case EIGHT_BIT:
                    cb = Unicode2Byte(pszUserData, pszEndUserData, pWorkPDU, pWorkPDU+cbUserData);
                    break;
                case UCS_2:
                    cb = Unicode2UCS(pszUserData, pszEndUserData, pWorkPDU, pWorkPDU+cbUserData);
                    break;
                default:
                    break;
                }
            }
            break;

        case TP_RD: /*9.2.3.25*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_RD_SET(*pWorkPDU, RejectDuplicates());
            break;
        case TP_SRQ: /*9.2.3.26*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            *pWorkPDU = SMS_TP_SRQ_SET(*pWorkPDU, StatusReportQualifier());
            break;
        case TP_PI: /*9.2.3.26*/
            ALOGD("%s TP_SMSC",__FUNCTION__);
            if (HasPID())
                *pWorkPDU = SMS_TP_PI_PID_SET(*pWorkPDU, SMS_TP_PI_PID);
            if (HasDCS())
                *pWorkPDU = SMS_TP_PI_DCS_SET(*pWorkPDU, SMS_TP_PI_DCS);
            if (HasUDL())
                *pWorkPDU = SMS_TP_PI_UDL_SET(*pWorkPDU, SMS_TP_PI_UDL);
            break;
        default:
            ALOGD("%s TP_SMSC",__FUNCTION__);
            break;
        }
/*
 * advance TPDU pointer to encode next tag
 */
        if (TP_TAG_len[tagCurrent & ~TP_OPTIONAL] < 0)
        {
            if (TP_TAG_len[tagNext & ~TP_OPTIONAL] >= 0)
            {
                ++pWorkPDU;
                ++m_ulPDULength;
            }
            // else do not advance to next byte yet
        }
        else if ((TP_TAG_len[tagCurrent & ~TP_OPTIONAL] == 0) || (tagCurrent & TP_OPTIONAL))
        {
            pWorkPDU += cb;
            if (tagCurrent != TP_SMSC)
                m_ulPDULength += cb;
        }
        else
        {
            pWorkPDU        += TP_TAG_len[tagCurrent & ~TP_OPTIONAL];
            m_ulPDULength   += TP_TAG_len[tagCurrent & ~TP_OPTIONAL];
        }
    }

    EncodeHex(m_aPDU_HEX, m_aPDU_HEX + sizeof(m_aPDU_HEX), m_aPDU, pWorkPDU);

    return bResult;
}

