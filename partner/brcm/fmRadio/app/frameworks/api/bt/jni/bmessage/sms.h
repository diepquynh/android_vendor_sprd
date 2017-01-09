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
#include <stdlib.h>
#include "sms_ext.h"


#define MAX_PDU_BUFFER_SIZE         (1024)


#define WCHAR char
#define ULONG unsigned long
#define BOOL bool
#define byte unsigned char
#define FALSE 0
#define TRUE 1


/*
 * make sure (a) is an array type.
 */
#define COUNTOF(a)                  (sizeof(a)/sizeof(a[0]))

inline int HI_NIBBLE(unsigned char c)   {return (c & 0xF0) >> 4;}
inline int LO_NIBBLE(unsigned char c)   {return (c & 0x0F);}
inline int LO_BYTE(unsigned short c)    {return (int) (c & 0x00FF);}
inline int HI_BYTE(unsigned short c)    {return (int) ((c >> 8) & 0x00FF);}

size_t Unicode2GSM(const WCHAR* pszUnicode, WCHAR** pszEndUnicode, unsigned char* pGSM, const unsigned char* pEndGSM);
size_t GSM2Unicode(const unsigned char* pGSM, const unsigned char* pEndGSM, WCHAR* pszUnicode);
size_t UCS2Unicode(const unsigned char* pUCS, const unsigned char* pEndUCS, WCHAR* pszUnicode);
size_t Unicode2UCS(const WCHAR* pszUnicode, WCHAR** pszEndUnicode, unsigned char* pUCS, const unsigned char* pEndUCS);
size_t Byte2Unicode(const unsigned char* pByte, const unsigned char* pEndByte, WCHAR* pszUnicode);
size_t Unicode2Byte(const WCHAR* pszUnicode, WCHAR** pszEndUnicode, byte* pByte, const byte* pEndByte);
size_t PackGSM7(size_t* pOct, unsigned char* pOut, unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn);
size_t UnpackGSM7(size_t nOct, unsigned char* pOut, unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn);
size_t EncodeHex(unsigned char* pOut, const unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn);
size_t DecodeHex(unsigned char* pOut, const unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn);

typedef enum _SMS_CHARSET
{
    DEFAULT_ALPHABET    = 0x00,
    EIGHT_BIT           = 0x04,
    UCS_2               = 0x08,
    SMS_RESERVED        = 0x0C,
    SMS_CHARSET_MASK    = 0x0C
} SMS_CHARSET;

class CConcatenatedSMS
{
public:
    CConcatenatedSMS() :
        m_cReferenceNumber(0),
        m_cMaxNumber(0),
        m_cSequenceNumber(0),
        m_eCharset(DEFAULT_ALPHABET)
    {}
    unsigned short ReferenceNumber() const      {return m_cReferenceNumber;}
    unsigned char  MaxNumber() const            {return m_cMaxNumber;}
    unsigned char  SequenceNumber() const       {return m_cSequenceNumber;}
    SMS_CHARSET    Charset() const              {return m_eCharset;}

    void NextReferenceNumber()                  {m_cReferenceNumber = ++_cReferenceNumber;}
    void ReferenceNumber(unsigned short i)      {m_cReferenceNumber = i;}
    void MaxNumber(unsigned char cVal)          {m_cMaxNumber = cVal;}
    void SequenceNumber(unsigned char cVal)     {m_cSequenceNumber = cVal;}
    void Charset(SMS_CHARSET eVal)              {m_eCharset = eVal;}
    void NextSequenceNumber()                   {++m_cSequenceNumber;}

    void Reset()
    {
        m_cReferenceNumber = 0;
        m_cMaxNumber = 0;
        m_cSequenceNumber = 0;
        m_eCharset = DEFAULT_ALPHABET;
    }
private:
static  unsigned short  _cReferenceNumber;
        unsigned short  m_cReferenceNumber;
        unsigned char   m_cMaxNumber;
        unsigned char   m_cSequenceNumber;
        SMS_CHARSET     m_eCharset;
};

class CSMSSpecialMessageIndication
{
public:
    typedef enum _MessageIndication {
        VOICE_MESSAGE_WAITING,
        FAX_MESSAGE_WAITING,
        EMAIL_MESSAGE_WAITING,
        OTHER_MESSAGE_WAITING
    } MESSAGE_INDICATION;

    CSMSSpecialMessageIndication() :
        m_eMsgIndication(VOICE_MESSAGE_WAITING),
        m_bStoreMessage(false),
        m_cMessageCount(0)
        {};

    void Reset()
    {
        m_eMsgIndication = VOICE_MESSAGE_WAITING;
        m_bStoreMessage  = false;
        m_cMessageCount  = 0;
    }

    MESSAGE_INDICATION  MessageIndication() const   {return m_eMsgIndication;}
    bool                StoreMessage() const        {return m_bStoreMessage;}
    unsigned char       MessageCount() const        {return m_cMessageCount;}

    void MessageIndication(MESSAGE_INDICATION eValue)   {m_eMsgIndication   = eValue;}
    void StoreMessage(bool bValue)                      {m_bStoreMessage    = bValue;}
    void MessageCount(unsigned char cValue)             {m_cMessageCount    = cValue;}
private:
    MESSAGE_INDICATION  m_eMsgIndication;
    bool                m_bStoreMessage;
    unsigned char       m_cMessageCount;
};

class CApplicationPort
{
public:
    typedef enum _mode
    {
        EIGHT,
        SIXTEEN
    } MODE;

    CApplicationPort() :
        m_iOriginator(0),
        m_iDestination(0),
        m_eMode(EIGHT)
        {}

    unsigned short Originator() const   {return m_iOriginator;}
    unsigned short Destination() const  {return m_iDestination;}
    MODE Mode() const                   {return m_eMode;}

    void Originator(unsigned short i)   {m_iOriginator = i;}
    void Destination(unsigned short i)  {m_iDestination = i;}
    void Mode(MODE m)                   {m_eMode = m;}

    void Reset()
    {
        m_iOriginator   = 0;
        m_iDestination  = 0;
        m_eMode         = EIGHT;
    }
private:
    MODE            m_eMode;
    unsigned short  m_iOriginator;
    unsigned short  m_iDestination;
};

class CSMSAddress
{
public:
    typedef enum _Address_Encoding {
        SMS_ADDRESS,
        SERVICE_CENTER_ADDRESS
    } ADDRESS_ENCODING;

    CSMSAddress() :
        m_eAddressEncoding(SMS_ADDRESS),
        address_type(SMS_ADDRESS_TYPE_SUBSCRIBER),
        address_plan(SMS_ADDRESS_PLAN_ISDN)
        {
            pszAddress[0] = L'\0';
        };
    virtual ~CSMSAddress(){};

    CSMSAddress& operator=(CSMSAddress& rhs);

    size_t Parse(const void* pPDU, const void* pEnd);
    size_t Compose(void* pPDU, const void* pEnd);

    int     Type() const {return address_type;}
    int     Plan() const {return address_plan;}
    const WCHAR* Address() const {return (const WCHAR*)&pszAddress[0];}

    void    Type(int i) {address_type = i;}
    void    Plan(int i) {address_plan = i;}
    void    Address(const WCHAR* pAddress) {strncpy(pszAddress,pAddress, COUNTOF(pszAddress) - 1);}
    void    Reset();

    void    SetAddressEncoding(ADDRESS_ENCODING e)
    {
        m_eAddressEncoding = e;
    };
    bool    operator==(CSMSAddress& rhs) const;
private:
    ADDRESS_ENCODING AddressEncoding() const {return m_eAddressEncoding;}
    int     address_type;
    int     address_plan;
    WCHAR   pszAddress[32];
protected:
    ADDRESS_ENCODING    m_eAddressEncoding;
};

class CSMSCAddress : public CSMSAddress
{
public:
    CSMSCAddress()
    {
        m_eAddressEncoding = SERVICE_CENTER_ADDRESS;
    };
    CSMSAddress& operator=(CSMSAddress& rhs) {return CSMSAddress::operator=(rhs);}
    bool    operator==(CSMSAddress& rhs) const {return CSMSAddress::operator==(rhs);}
};

class CSMSTime
{
public:
    CSMSTime(){
        time[0] =
        time[1] =
        time[2] =
        time[3] =
        time[4] =
        time[5] =
        time[6] = 0;
    };
    virtual ~CSMSTime(){};

    CSMSTime& operator=(CSMSTime& rhs);

    size_t Parse(const void* pPDU, const void* pEnd);
    size_t Compose(void* pPDU, const void* pEnd) const;
    const char* ToUTC();

    int year() const     {return time[0];}
    int month() const    {return time[1];}
    int day() const      {return time[2];}
    int hour() const     {return time[3];}
    int minute() const   {return time[4];}
    int second() const   {return time[5];}
    int timezone() const {return time[6];}

    void year(int i)     {time[0] = i;}
    void month(int i)    {time[1] = i;}
    void day(int i)      {time[2] = i;}
    void hour(int i)     {time[3] = i;}
    void minute(int i)   {time[4] = i;}
    void second(int i)   {time[5] = i;}
    void timezone(int i) {time[6] = i;}

    void Reset();

    bool operator==(CSMSTime& rhs) const;

private:
    int time[7];
    static char m_szBuffer[64];
};


class CSmsTpdu
{
public:
    typedef enum _MessageType
    {
        SMS_DELIVER,
        SMS_DELIVER_REPORT_ERROR,
        SMS_STATUS_REPORT,
        SMS_COMMAND,
        SMS_SUBMIT,
        SMS_SUBMIT_REPORT_ERROR,
        MAX_SMS_MESSAGE_TYPE
    } MESSAGE_TYPE;

    CSmsTpdu(MESSAGE_TYPE _type = SMS_SUBMIT) :
        m_eMessageType(_type),
        m_eVPF(SMS_VPF_NONE),
        m_eProtocolIndicator(GSM_DEFAULT),
        m_eDCS(CLASS_0),
        m_eCharset(DEFAULT_ALPHABET),
        m_cchUserData(0),
        m_cchCommandData(0),
        m_cbPDU(0),
        m_cbPDURaw(0),
        m_iStatus(0),
        m_bMoreMessageToSend(false),
        m_bStatusReportIndicator(false),
        m_bStatusReportRequest(false),
        m_bReplyPath(false),
        m_bCompressed(false),
        m_bUDHIndicator(false),
        m_bRejectDuplicates(false),
        m_bStatusReportQualifier(false),
        m_bHasUDL(false),
        m_bHasDCS(false),
        m_bHasPID(false),
        m_cMessageReference(0),
        m_cMessageNumber(0),
        m_cCommandType(0),
        m_cFailureCause(0),
        m_eUDHSource(UNKNOWN),
        m_cSMSCControlParameters(0),
        m_ulPDULength(0)
    {
        m_aUserData[0]      = 0;
        m_aCommandData[0]   = 0;
        memset(m_aPDU,      0, sizeof(m_aPDU));
        memset(m_aPDU_HEX,  0, sizeof(m_aPDU_HEX));
        m_saServiceCenter.SetAddressEncoding(CSMSAddress::SERVICE_CENTER_ADDRESS);
    }

    MESSAGE_TYPE    MessageType() const {return m_eMessageType;}
    bool            MoreMessagetoSend() const{return m_bMoreMessageToSend;}
    typedef enum _ValidityPeriodFormat
    {
        SMS_VPF_NONE = SMS_TP_VPF_NONE,
        SMS_VPF_ENHANCED = SMS_TP_VPF_ENHANCED,
        SMS_VPF_RELATIVE = SMS_TP_VPF_RELATIVE,
        SMS_VPF_ABSOLUTE = SMS_TP_VPF_ABSOLUTE
    } SMS_VPF;
    SMS_VPF             ValidityPeriodFormat() const {return m_eVPF;}
    bool                StatusReportIndicator() const {return m_bStatusReportIndicator;}
    unsigned char       MessageReference() const {return m_cMessageReference;}
    const CSMSAddress&  OriginatingAddress() const {return m_saOriginating;}
    const CSMSAddress&  DestinationAddress() const {return m_saDestination;}

    typedef enum _ProtocolIdentifier
    {
        IMPLICIT = 0,
        TELEX,
        GROUP3_FAX,
        GROUP4_FAX,
        VOCIE_PHONE,
        ERMES,
        NATIONAL_PAGING,
        VIDEO_TEX,
        TELETEX_UNSPECIFIED,
        TELETEX_PSPDN,
        TELETEX_CSPDN,
        TELETEX_PSTN,
        TELETEX_ISDN,
        UCI,
        RESERVED_1 = 0x0E,
        RESERVED_2 = 0x0F,
        MESSAGING_HANDLING_FACILITY,        /*0x10*/
        MESSAGING_HANDLING_FACILITY_X400,
        INTERNET_EMAIL,
        MUTUAL,
        GSM_DEFAULT,
        SMS_TYPE_0 = 0x40,
        SMS_TYPE_1,
        SMS_TYPE_2,
        SMS_TYPE_3,
        SMS_TYPE_4,
        SMS_TYPE_5,
        SMS_TYPE_6,
        SMS_TYPE_7,
        RETURN_CALL_MESSAGE     = 0x5F,
        ME_DATA_DOWNLOAD        = 0x7D,
        ME_DEPERSONALIZATION_SMS,
        SIM_DATA_DOWNLOAD       /* 0x7F*/
    } SMS_PROTOCOL;
    SMS_PROTOCOL    ProtocolIdentifier() const {return m_eProtocolIndicator;}

    typedef enum _SMS_DCS_MESSAGE_CLASS
    {
        CLASS_0             = 0,
        ME_SPECIFIC         = 0x01,
        SIM_SPECIFIC        = 0x02,
        TE_SPECIFIC         = 0x03,
        SMS_DCS_MASK        = 0x03,
        SMS_DCS_NONE        = 4
    } SMS_DCS_MESSAGE_CLASS;

    // Section 4. SMS Data Coding Scheme, bit 4 set to 1 means bit 1 and 0 has Message Class meaning.
#define HAS_MESSAGE_CLASS       (0x10)

    typedef enum _SMS_UDH_SOURCE
    {
        UNKNOWN     = 00,
        SENDER      = 01,
        RECEIVER    = 02,
        SMSC        = 03
    } SMS_UDH_SOURCE;

    bool Compressed() const                             {return m_bCompressed;}
    SMS_DCS_MESSAGE_CLASS DataCodingScheme() const      {return m_eDCS;}
    SMS_CHARSET Charset() const                         {return m_eCharset;}

    const CSMSTime&     ServiceCenterTimeStamp() const  {return m_stServiceCenter;}
    const CSMSTime&     ValidityPeriodRelative() const  {return m_stValidityPeriod;}
    const CSMSTime&     DischargeTime() const           {return m_stDischarge;}
    const CSMSAddress&  ServiceCenterAddress() const    {return m_saServiceCenter;}
    const CSMSAddress&  RecipientAddress() const        {return m_saRecipient;}
    const CConcatenatedSMS& ConcatenatedSMS() const     {return m_csConcatenatedSMS;}

    int             Status() const              {return m_iStatus;}
    size_t          UserDataLength() const      {return m_cchUserData;}
    unsigned char   MessageNumber() const       {return m_cMessageNumber;}
    unsigned char   CommandType() const         {return m_cCommandType;}
    size_t          CommandDataLength() const   {return m_cchCommandData;}

    const WCHAR*    CommandData() const         {return m_aCommandData;}
    bool            UDHIndicator() const        {return m_bUDHIndicator;}
    const WCHAR*    UserData() const            {return m_aUserData;}

    unsigned char   FailureCause() const        {return m_cFailureCause;}
    bool            RejectDuplicates() const    {return m_bRejectDuplicates;}
    bool            StatusReportQualifier() const {return m_bStatusReportQualifier;}
    bool            ReplyPath() const           {return m_bReplyPath;}

    bool                    HasUDL() const                  {return m_bHasUDL;}
    bool                    HasDCS() const                  {return m_bHasDCS;}
    bool                    HasPID() const                  {return m_bHasPID;}

    bool                    StatusReportRequest() const     {return m_bStatusReportRequest;}
    unsigned char           SMSCControlParameters() const   {return m_cSMSCControlParameters;}
    SMS_UDH_SOURCE          UDHSource() const               {return m_eUDHSource;}

    ULONG                   PDULength() const               {return m_ulPDULength;}
    const unsigned char*    GetEncodedString() const        {return m_aPDU_HEX;}

    void MessageType(MESSAGE_TYPE eMessageType) {m_eMessageType = eMessageType;}
    void MoreMessagetoSend(bool bMMS)           {m_bMoreMessageToSend = bMMS;}
    void ValidityPeriodFormat(SMS_VPF eVPF)     {m_eVPF = eVPF;}
    void StatusReportIndicator(bool bSRI)       {m_bStatusReportIndicator = bSRI;}
    void MessageReference(unsigned char cMR)    {m_cMessageReference = cMR;}
    void ServiceCenterAddress(CSMSAddress& saAddr){m_saServiceCenter = saAddr;}
    void OriginatingAddress(CSMSAddress& saAddr){m_saOriginating = saAddr;}
    void DestinationAddress(CSMSAddress& saAddr){m_saDestination = saAddr;}
    void RecipientAddress(CSMSAddress& saAddr)  {m_saRecipient = saAddr;}
    void ReplyPath(bool bValue)                 {m_bReplyPath = bValue;}

    void ProtocolIdentifier(SMS_PROTOCOL ePI)   {m_eProtocolIndicator = ePI;}

    void Compressed(bool bCompressed)           {m_bCompressed = bCompressed;}
    void DataCodingScheme(SMS_DCS_MESSAGE_CLASS eDCS) {m_eDCS = eDCS;}
    void Charset(SMS_CHARSET eCS)               {m_eCharset = eCS;}

    void ServiceCenterTimeStamp(CSMSTime& st)   {m_stServiceCenter = st;}
    void ValidityPeriodRelative(CSMSTime& st)   {m_stValidityPeriod = st;}
    void DischargeTime(CSMSTime& st)            {m_stDischarge = st;}

    void Status(int iStatus)                    {m_iStatus = iStatus;}
    void UserDataLength(size_t sz)              {m_cchUserData = sz;}
    void MessageNumber(unsigned char cMN)       {m_cMessageNumber = cMN;}
    void CommandType(unsigned char cCT)         {m_cCommandType = cCT;}
    void CommandDataLength(size_t sz)           {m_cchCommandData = sz;}
    void UDHIndicator(bool bUDHI)               {m_bUDHIndicator = bUDHI;}

    void FailureCause(unsigned char cFC)        {m_cFailureCause = cFC;}
    void RejectDuplicates(bool bRD)             {m_bRejectDuplicates = bRD;}
    void StatusReportQualifier(bool bValue)     {m_bStatusReportQualifier = bValue;}

    void HasUDL(bool bValue)                    {m_bHasUDL = bValue;}
    void HasDCS(bool bValue)                    {m_bHasDCS = bValue;}
    void HasPID(bool bValue)                    {m_bHasPID = bValue;}

    void SMSCControlParameters(unsigned char cValue) {m_cSMSCControlParameters = cValue;}
    void StatusReportRequest(bool bValue)       {m_bStatusReportRequest = bValue;}
    void UDHSource(SMS_UDH_SOURCE e)            {m_eUDHSource = e;}

    void Reset(BOOL bConcate = FALSE);
    bool Encode(const WCHAR* pszUserData, WCHAR** pEndUserData);
    const unsigned char* Decode(const unsigned char* pPDU, const unsigned char* pEndPDU);

private:
    CSMSCAddress            m_saServiceCenter;
    CSMSAddress             m_saOriginating;
    CSMSAddress             m_saDestination;
    CSMSAddress             m_saRecipient;
    CSMSTime                m_stServiceCenter;
    CSMSTime                m_stValidityPeriod;
    CSMSTime                m_stDischarge;
    MESSAGE_TYPE            m_eMessageType;
    SMS_VPF                 m_eVPF;
    SMS_PROTOCOL            m_eProtocolIndicator;
    SMS_DCS_MESSAGE_CLASS   m_eDCS;
    SMS_CHARSET             m_eCharset;
    size_t                  m_cchUserData;
    size_t                  m_cchCommandData;
    size_t                  m_cbPDU;
    size_t                  m_cbPDURaw;
    bool                    m_bReplyPath;
    int                     m_iStatus;
    bool                    m_bMoreMessageToSend;
    bool                    m_bStatusReportIndicator;
    bool                    m_bStatusReportRequest;
    bool                    m_bCompressed;
    bool                    m_bUDHIndicator;
    bool                    m_bRejectDuplicates;
    bool                    m_bStatusReportQualifier;
    bool                    m_bHasUDL;
    bool                    m_bHasDCS;
    bool                    m_bHasPID;
    unsigned char           m_cMessageReference;
    unsigned char           m_cMessageNumber;
    unsigned char           m_cCommandType;
    unsigned char           m_cFailureCause;
    CConcatenatedSMS        m_csConcatenatedSMS;
    unsigned char           m_cSMSCControlParameters;
    CSMSSpecialMessageIndication m_csmiMessageIndication;
    CApplicationPort        m_capPort;
    SMS_UDH_SOURCE          m_eUDHSource;
    ULONG                   m_ulPDULength;
    WCHAR                   m_aUserData[MAX_PDU_BUFFER_SIZE];
    WCHAR                   m_aCommandData[MAX_PDU_BUFFER_SIZE];
    unsigned char           m_aPDU[MAX_PDU_BUFFER_SIZE];
    unsigned char           m_aPDU_HEX[2 * MAX_PDU_BUFFER_SIZE];
private:
    static const unsigned char*  m_aTPDU_Tags[MAX_SMS_MESSAGE_TYPE];
    static const unsigned char   m_aTPDU_MessageType[MAX_SMS_MESSAGE_TYPE];
    void ParsePID(unsigned char c);
    void ParseDCS(unsigned char c);
    void ParseUDH(const unsigned char* pWork, size_t cb);
    size_t ComposeUDH(const unsigned char* pWorkPDU);
};
