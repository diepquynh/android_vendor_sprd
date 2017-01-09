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

/*9.1.2.5*/
typedef struct __SMS_ADDRESS_LAYOUT
{
    unsigned char   len;
    unsigned char   header;
    unsigned char   addr[MAX_ADDR_LEN];
} _SMS_ADDRESS_LAYOUT;

void CSMSAddress::Reset()
{
    address_type = SMS_ADDRESS_TYPE_SUBSCRIBER |SMS_ADDRESS_TYPE_NATIONAL;
    address_plan = SMS_ADDRESS_PLAN_ISDN;
    pszAddress[0] = 0;
}

CSMSAddress& CSMSAddress::operator=(CSMSAddress& rhs)
{
    address_type = rhs.address_type;
    address_plan = rhs.address_plan;
    strncpy(pszAddress, rhs.pszAddress, COUNTOF(pszAddress) - 1);
    return *this;
}

bool CSMSAddress::operator==(CSMSAddress& rhs) const
{
    return Type() == rhs.Type() &&
           Plan() == rhs.Plan() &&
           !strncmp(Address(), rhs.Address(), sizeof(pszAddress)/sizeof(pszAddress[0]));
}

static const WCHAR digits[] =
{
    '0','1','2','3',
    '4','5','6','7',
    '8','9','*','#',
    'A','B','C','\0'
};

static const unsigned char cFiller = 0xF0;

size_t CSMSAddress::Compose(void* pPDU, const void* pEnd)
{
    size_t cb =0;
    _SMS_ADDRESS_LAYOUT* pAddr = (_SMS_ADDRESS_LAYOUT*) pPDU;
    if (!pAddr || pAddr >= pEnd)
        return cb;
    size_t i = 0;
    const WCHAR* pszTemp = (const WCHAR*) pszAddress;
    if (*pszTemp == '+')
    {
        Type(SMS_ADDRESS_TYPE_INTERNATIONAL);
        ++pszTemp;
    }
    else if (*pszTemp == '1')
    {
        Type(SMS_ADDRESS_TYPE_NATIONAL);
    }
    else
    {
        Type(SMS_ADDRESS_TYPE_SUBSCRIBER);
    }
    pAddr->header = (unsigned char)SMS_ADDRESS_HEADER(Type(), Plan());
    unsigned char* pDest = &(pAddr->addr[i]);
    unsigned char cStored = 0;
    bool     bHiNibble = false;
    while (*pszTemp && i <= MAX_ADDR_LEN && pDest < pEnd)
    {
        unsigned char cValue = 0xFF;
        if (*pszTemp >= '0' && *pszTemp <= '9')
        {
            cValue = (unsigned char)(*pszTemp - '0');
        }
        else if (*pszTemp == '*')
        {
            cValue = 10;
        }
        else if (*pszTemp == '#')
        {
            cValue = 11;
        }
        else if (*pszTemp == 'a' || *pszTemp == 'A')
        {
            cValue = 12;
        }
        else if (*pszTemp == 'b' || *pszTemp == 'B')
        {
            cValue = 13;
        }
        else if (*pszTemp == 'c' || *pszTemp == 'C')
        {
            cValue = 14;
        }

        if (cValue != 0xFF)
        {
            if (bHiNibble)
            {
                cValue <<= 4;
                *pDest++ = cStored | cValue;
            }
            else
            {
                cStored = cValue;
            }
            ++i;
            bHiNibble = !bHiNibble;
        }
        ++pszTemp;
    }
    if (bHiNibble && pDest < pEnd)
    {
        *pDest++ = cStored | cFiller;
    }

    pAddr->len = (unsigned char)((AddressEncoding() == SERVICE_CENTER_ADDRESS) ? (i+1)/2+1 : i);
    cb =(AddressEncoding() == SERVICE_CENTER_ADDRESS) ? (pAddr->len + 1) : (pAddr->len + 1) / 2 + 2;
    return cb;
}

size_t CSMSAddress::Parse(const void* pPDU, const void* pEnd)
{
    size_t cb = 0;
    Reset();

    if (pPDU && pEnd >= pPDU)
    {
        const _SMS_ADDRESS_LAYOUT* pLayout = (const _SMS_ADDRESS_LAYOUT*) pPDU;
        size_t j = 0;
        size_t iCount = 0;
        if ((AddressEncoding() == SERVICE_CENTER_ADDRESS))
        {
            cb      = pLayout->len + 1;
            iCount  = pLayout->len > 0 ? pLayout->len - 1 : 0;
        }
        else
        {
            cb      = (pLayout->len + 1) / 2 + 2;
            iCount  = (pLayout->len + 1) / 2;
        }
        if (iCount > 0)
        {
            Type(SMS_ADDRESS_TYPE(pLayout->header));
            Plan(SMS_ADDRESS_PLAN(pLayout->header));
            if (Type() == SMS_ADDRESS_TYPE_INTERNATIONAL)
                pszAddress[j++] = '+';
            size_t i = 0;
            const unsigned char* pData = &(pLayout->addr[i]);
            for (;i < iCount &&
                  pData <= pEnd &&
                  j+1 < sizeof(pszAddress)/sizeof(pszAddress[0]);
                  ++i, ++pData)
            {
                pszAddress[j++] = digits[LO_NIBBLE(*pData)];
                if (HI_NIBBLE(*pData) != 0x0F)
                    pszAddress[j++] = digits[HI_NIBBLE(*pData)];
            }
        }
        pszAddress[j] = '\0';
    }
    return cb;
}


CSMSTime& CSMSTime::operator=(CSMSTime& rhs)
{
    year(rhs.year());
    month(rhs.month());
    day(rhs.day());
    hour(rhs.hour());
    minute(rhs.minute());
    second(rhs.second());
    timezone(rhs.timezone());
    return *this;
}

bool CSMSTime::operator==(CSMSTime& rhs) const
{
    if (timezone() == rhs.timezone())
        return year()   == rhs.year() &&
               month()  == rhs.month() &&
               day()    == rhs.day() &&
               hour()   == rhs.hour() &&
               minute() == rhs.minute() &&
               second() == rhs.second();
    return false;
}

void CSMSTime::Reset()
{
    year(0);
    month(0);
    day(0);
    hour(0);
    minute(0);
    second(0);
    timezone(0);
    memset(m_szBuffer, 0, sizeof(m_szBuffer));
}

typedef struct __SMS_TIMESTAMP_LAYOUT
{
     char   year;
     char   month;
     char   day;
     char   hour;
     char   minute;
     char   second;
     char   tz;
} _SMS_TIMESTAMP_LAYOUT;

size_t CSMSTime::Compose(void* pPDU, const void* pEnd) const
{
    size_t cb = 7;
    char* pData = (char*) pPDU;
    for (int i = 0; i < sizeof(time)/sizeof(time[0]) && pData < pEnd; ++i)
    {
        int bNegative = time[i] < 0;
        int iValue = (bNegative ? -time[i] : time[i]) % 100;
        *pData = (iValue / 10) | ((iValue % 10) << 4);
        if (i == 6)
        {
            if (!bNegative)
            *pData |= 0x08;
        }
        ++pData;
    }
    return cb;
}

size_t CSMSTime::Parse(const void* pPDU, const void* pEnd)
{
    size_t cb = 7;
    const char* pData = (const  char*) pPDU;
    for (int i = 0; i < sizeof(time)/sizeof(time[0]); ++i)
    {
        int bNegative = *pData & 0x08;
        int iValue = LO_NIBBLE(*pData) * 10 + HI_NIBBLE(*pData);
        if (i == 6)
        {
            iValue = LO_NIBBLE(*pData & 0x07) * 10 + HI_NIBBLE(*pData);
            if (!bNegative)
                iValue = -iValue;
        }


        ++pData;
        time[i] = iValue;
    }
    return cb;
}

//
// UTC = YYYYMMDDTHHMMSS+zzzz
//
char CSMSTime::m_szBuffer[];
const char* CSMSTime::ToUTC()
{
    int iWidth = 4;
    char* p = &m_szBuffer[0];
    for (int i = 0; i < 6; ++i)
    {
        int iVal = time[i];
        if (i == 0 && iVal < 100)
            iVal += 2000;
        for  (int j = 0; j < iWidth; ++j)
        {
            p[iWidth - j - 1] = (iVal % 10) + '0';
            iVal /= 10;
        }
        p += iWidth;
        if (iWidth != 2)
            iWidth = 2;
        if (i == 2)
            *p++ = 'T';
    }
    int iVal = time[6];
    if (iVal < 0)
    {
        *p++ = '+';
        iVal = 0 - iVal;
    }
    else
    {
        *p++ = '-';
    }
    iVal *= 15;
    int tz[2];
    tz[0] = iVal / 60;
    tz[1] = iVal % 60;
    for (int i = 0; i < 2; ++i)
    {
        int iVal = tz[i];
        for  (int j = 0; j < iWidth; ++j)
        {
            p[iWidth - j - 1] = (iVal % 10) + '0';
            iVal /= 10;
        }
        p += iWidth;
    }
    return &m_szBuffer[0];
}

