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

unsigned char aHexDigits[] = "0123456789ABCDEF";

//
// GSM-7 alphabets are covered under BMP Basic Multilingual Plane of Unicode (0x0000 - 0xFFFF)
// most of te alphabets are also covered under 8859-1 (0x0000 - 0x00FF)
//
static const WCHAR GSM7_2_UNICODE[] = {
//    _T('@'), _T('£'), _T('$'), _T('¥'), _T('è'), _T('é'), _T('ù'), _T('ì'),  _T('ò'), _T('Ç'),    0x0A, _T('Ø'), _T('ø'),    0x0D, _T('Å'), _T('å'),
//     0x0394, _T('_'),  0x03A6,  0x0393,  0x039B,  0x03A9,  0x03A0,  0x03A8,  0x003A3,  0x0398,  0x039E,    0x1B, _T('Æ'), _T('æ'), _T('ß'), _T('É'),
//    _T(' '), _T('!'), _T('"'), _T('#'), _T('¤'), _T('%'), _T('&'), _T('\''), _T('('), _T(')'), _T('*'), _T('+'), _T(','), _T('-'), _T('.'), _T('/'),
    '@', 0x00A3,  '$',  0xA5,    0xE8,    0xE9,    0xF9,    0xEC,    0xF2,    0xC7,    0x0A,    0xD8,    0xF8,    0x0D,    0xC5,    0xE5,
     0x0394, '_',  0x03A6,  0x0393,  0x039B,  0x03A9,  0x03A0,  0x03A8,  0x03A3,  0x0398,  0x039E,  0x1B,    0xC6,    0xE6,    0xDF,    0xC9,
    ' ', '!', '"', '#',  0xA4,   '%', '&', '\'','(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
     0xA1,   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',  0xC4,    0xD6,    0xD1,    0xDC,    0xA7,
     0xBF,   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',  0xE4,    0xF6,    0xF1,    0xFC,    0xE0
};

// GSM7 escape sequence mapping to Unicode
static const unsigned char GSM7_ESC_CHAR        = 0x1B;
static const unsigned char GSM7_INVALID_CHAR    = 0xFF;
static const char GSM7_ESC[] = {
    0x0A, 0x14, 0x28, 0x29, 0x2F, 0x3C, 0x3D, 0x3E, 0x40, 0x65
};
static const WCHAR GSM7_ESC_2_UNICODE[] = {
//    0x0C,_T('^'),_T('{'),_T('}'),_T('\\'),_T('['),_T('~'),_T(']'),_T('|'),_T('€')
      0x0C, 0x5E,   0x7B,   0x7D,   0x5C,    0x5B,   0x7E,   0x5D,   0x7C,   0x80
};

static WCHAR GSMEscape2Unicode(unsigned char cGSM)
{
    for (int i = 0; i < sizeof(GSM7_ESC) / sizeof(GSM7_ESC[0]); ++i)
        if (GSM7_ESC[i] == cGSM)
            return GSM7_ESC_2_UNICODE[i];
    return 0;
}

// special Unicode characters that are included in GSM-7 alphabet but not within 8859-1
static const WCHAR UNICODE_SPECIAL[] = {
    0x0394, 0x005F, 0x03A6, 0x0393, 0x039B, 0x03A9, 0x03A0, 0x03A8, 0x03A3, 0x0398, 0x039E
};

static const unsigned char GSM7_SPECIAL_BASE = 0x10;

static unsigned char Unicode2GSMSpecial(WCHAR cUnicode)
{
    for (int i = 0; i < sizeof(UNICODE_SPECIAL) / sizeof(UNICODE_SPECIAL[0]); ++i)
        if (UNICODE_SPECIAL[i] == cUnicode)
            return GSM7_SPECIAL_BASE + i;
    return GSM7_INVALID_CHAR;
}

static const WCHAR UNICODE_EURO = '€';
static const unsigned char GSM7_EURO =  0x65;

static const unsigned char UNICODE_2_GSM7[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0xFF, 0x1B, 0x0D, 0xFF, 0xFF, /* 03 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 00 */
    0x20, 0x21, 0x22, 0x23, 0x02, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, /* 16 */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, /* 16 */
    0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, /* 16 */
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x1B, 0x1B, 0x1B, 0x1B, 0x11, /* 16 */
    0xFF, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, /* 15 */
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x1B, 0x1B, 0x1B, 0x1B, 0xFF, /* 15 */ /* 97 */

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 00 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 00 */
    0xFF, 0x40, 0xFF, 0x01, 0x24, 0x03, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 05 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60, /* 01 */
    0xFF, 0xFF, 0xFF, 0xFF, 0x5B, 0x0E, 0x1C, 0x09, 0xFF, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 05 */
    0xFF, 0x5D, 0xFF, 0xFF, 0xFF, 0xFF, 0x5C, 0xFF, 0x0B, 0xFF, 0xFF, 0xFF, 0x5E, 0xFF, 0xFF, 0x1E, /* 05 */
    0x7F, 0xFF, 0xFF, 0xFF, 0x7B, 0x0F, 0x1D, 0xFF, 0x04, 0x05, 0xFF, 0xFF, 0x07, 0xFF, 0xFF, 0xFF, /* 07 */
    0xFF, 0x7D, 0x08, 0xFF, 0xFF, 0xFF, 0x7C, 0xFF, 0x0C, 0x06, 0xFF, 0xFF, 0x7E, 0xFF, 0xFF, 0xFF, /* 06 */ /* 29 */
};

static const unsigned char UNICODE_2_GSM7_2[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3C, 0x2F, 0x3E, 0x14, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x28, 0x40, 0x29, 0x3D, 0xFF,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

size_t Unicode2GSM(const WCHAR* pszUnicode, WCHAR** pszEndUnicode, unsigned char* pGSM, const unsigned char* pEndGSM)
{
    size_t cb = 0;
    bool bNonGSM = false;

    if (!pszUnicode || !*pszUnicode || (pGSM && !(pGSM < pEndGSM)))
        return cb;

    while (*pszUnicode && !bNonGSM && ((pGSM && pGSM < pEndGSM) || !pGSM))
    {
        unsigned char c = 0xFF;
        if (*pszUnicode < 0x0100)
        {
            c = UNICODE_2_GSM7[*pszUnicode];
            if (GSM7_ESC_CHAR == c)
            {
                unsigned char c2 = UNICODE_2_GSM7_2[*pszUnicode];
                if (GSM7_INVALID_CHAR != c2)
                {
                    if (pGSM && pGSM+1 < pEndGSM)
                    {
                        {
                            *pGSM++ = c;
                            *pGSM++ = c2;
                        }
                    }
                    cb += 2;
                }
                else
                    bNonGSM = true;
            }
            else if (GSM7_INVALID_CHAR != c)
            {
                if (pGSM)
                    *pGSM++ = c;
                ++cb;
            }
            else
                bNonGSM = true;
        }
        else if (UNICODE_EURO == *pszUnicode)
        {
            if (pGSM && pGSM+1 < pEndGSM)
            {
                *pGSM++ = GSM7_ESC_CHAR;
                *pGSM++ = GSM7_EURO;
            }
            cb += 2;
        }
        else if ((c = Unicode2GSMSpecial(*pszUnicode)) != GSM7_INVALID_CHAR)
        {
            if (pGSM)
                *pGSM++ = c;
            ++cb;
        }
        else
            bNonGSM = true;
        ++pszUnicode;
    }

    if (pszEndUnicode)
        *pszEndUnicode = (WCHAR*)pszUnicode;

    return bNonGSM ? 0 : cb;
}

size_t GSM2Unicode(const unsigned char* pGSM, const unsigned char* pEndGSM, WCHAR* pszUnicode)
{
    size_t cch = 0;

    if (!pGSM || !pEndGSM || !(pGSM < pEndGSM))
        return cch;

    while (pGSM < pEndGSM)
    {
        if (*pGSM == GSM7_ESC_CHAR)
        {
            if (pGSM+1 < pEndGSM)
            {
                WCHAR c = GSMEscape2Unicode(*++pGSM);
                if (c)
                {
                    if (pszUnicode)
                        *pszUnicode++ = c;
                    ++cch;
                }
            }
        }
        else if (*pGSM < 0x80)
        {
            if (pszUnicode)
                *pszUnicode++ = GSM7_2_UNICODE[*pGSM];
            ++cch;
        }
        ++pGSM;
    }
    if (pszUnicode)
        *pszUnicode = L'\0';
    return cch;
}

// ISO/IEC10646
size_t UCS2Unicode(const unsigned char* pUCS, const unsigned char* pEndUCS, WCHAR* pszUnicode)
{
    size_t cch = 0;

    if (!pUCS || !pEndUCS || !(pUCS+1 < pEndUCS))
        return cch;

    unsigned char * pszOut = (unsigned char*) pszUnicode;

    while (pUCS + 1 < pEndUCS)
    {

        if (pszOut)
        {
            *pszOut++ = *(pUCS+1);
            *pszOut++ = *pUCS;
        }
        pUCS += 2;
        ++cch;
    }

    if (pszUnicode)
    {
        pszUnicode[cch] = L'\0';
    }

    return cch;
}

size_t Unicode2UCS(const WCHAR* pszUnicode, WCHAR** pszEndUnicode, unsigned char* pUCS, const unsigned char* pEndUCS)
{
    size_t cb = 0;

    if (!pszUnicode || !*pszUnicode || !pszEndUnicode || !pUCS || !pEndUCS || pUCS >= pEndUCS)
        return cb;
    while (*pszUnicode && (pszUnicode < *pszEndUnicode) && (pUCS+1 < pEndUCS))
    {
        *pUCS++ = HI_BYTE(*pszUnicode);
        *pUCS++ = LO_BYTE(*pszUnicode);
        ++pszUnicode;
        cb += 2;
    }

    *pszEndUnicode = (WCHAR*)pszUnicode;

    return cb;
}

//#if 0
// user defined.  either 8859-1 or UTF-8
// This implementation, takes the LO_BYTE() of Unicode default code page, 0x0000 - 0x00FF
// which is 8859-1 compatible.
//
size_t Byte2Unicode(const unsigned char* pByte, const unsigned char* pEndByte, WCHAR* pszUnicode)
{
    size_t cb = 0;

    if (!pByte || !pEndByte || !(pByte < pEndByte))
        return cb;

    while (pByte < pEndByte)
    {
        if (pszUnicode)
        {
            *pszUnicode++ = (WCHAR)*pByte++;
        }
        ++cb;
    }

    if (pszUnicode)
        *pszUnicode = L'\0';

    return cb;
}

size_t Unicode2Byte(const WCHAR* pszUnicode, WCHAR** pszEndUnicode, unsigned char* pByte, const unsigned char* pEndByte)
{
    size_t cb = 0;

    if (!pszUnicode || !*pszUnicode || !pByte || !pEndByte || pByte >= pEndByte)
        return cb;

    while (*pszUnicode && (pByte < pEndByte))
    {
        *pByte++ = LO_BYTE(*pszUnicode++);
        ++cb;
    }

    if (pszEndUnicode)
        *pszEndUnicode = (WCHAR*)pszUnicode;
    return cb;
}
//#endif

// Packing GSM-7 from pIn to pOut
//
//  7  6  5  4  3  2  1  0
// 2g 1a 1b 1c 1d 1e 1f 1g
// 3f 3g 2a 2b 2c 2d 2e 2f
// 4e 4f 4g 3a 3b 3c 3d 3e
// 5d 5e 5f 5g 4a 4b 4c 4d
// 6c 6d 6e 6f 6g 5a 5b 5c
// 7b 7c 7d 7e 7f 7g 6a 6b
// 8a 8b 8c 8d 8e 8f 8g 7a
//
size_t PackGSM7(size_t* pOct, unsigned char* pOut, unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn)
{
    size_t cb = 0;
    int nShift = 0;
    unsigned short c = 0;
    unsigned short t = 0;
    const unsigned char* pInWork = pIn;
    while (pInWork < pEndIn && pOut < pEndOut)
    {
        t = *pInWork++;
        if (nShift)
        {
            c |= (t << (8 - nShift));
            *pOut++ = LO_BYTE(c);
            ++cb;
            c = HI_BYTE(c);
        }
        else
            c = t;

        if (++nShift > 7)
            nShift = 0;
    }

    if (nShift != 0)
    {
        *pOut++ = LO_BYTE(c);
        ++cb;
    }
    if (pOct)
    {
        *pOct = pInWork - pIn;
    }
    return cb;
}

// Unpacking GSM-7 from pIn to pOut
//
//  7  6  5  4  3  2  1  0
// 2g 1a 1b 1c 1d 1e 1f 1g
// 3f 3g 2a 2b 2c 2d 2e 2f
// 4e 4f 4g 3a 3b 3c 3d 3e
// 5d 5e 5f 5g 4a 4b 4c 4d
// 6c 6d 6e 6f 6g 5a 5b 5c
// 7b 7c 7d 7e 7f 7g 6a 6b
// 8a 8b 8c 8d 8e 8f 8g 7a
//
size_t UnpackGSM7(size_t nOct, unsigned char* pOut, unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn)
{
    size_t cb = 0;
    const unsigned char* pInOrg = pIn;
    int nShift = 0;
    unsigned short c = 0;
    unsigned short t = 0;
    unsigned char uMask = 0xFF;
    while (pIn < pEndIn && pOut < pEndOut && cb < nOct)
    {
        uMask >>= 1;
        t = *pIn & uMask;
        if (!nShift)
        {
            *pOut++ = LO_BYTE(t);
            ++cb;
        }
        else
        {
            *pOut++ = (t << nShift) | c;
            ++cb;
        }
        c = *pIn & ~uMask;
        c >>= (7 - nShift);

        if (++nShift > 6 && cb < nOct)
        {
            nShift = 0;
            *pOut++ = LO_BYTE(c);
            ++cb;
            c = 0;
            uMask = 0xFF;
        }

        ++pIn;
    }

    return pIn - pInOrg;
}
size_t EncodeHex(unsigned char* pOut, const unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn)
{
    unsigned char* pOutWork = pOut;
    while (pIn < pEndIn && pOutWork+1 < pEndOut)
    {
        *pOutWork++ = aHexDigits[HI_NIBBLE(*pIn)];
        *pOutWork++ = aHexDigits[LO_NIBBLE(*pIn)];
        ++pIn;
    }

    if (pOutWork < pEndOut)
        *pOutWork = '\0';

    return pOutWork - pOut;
}

size_t DecodeHex(unsigned char* pOut, const unsigned char* pEndOut, const unsigned char* pIn, const unsigned char* pEndIn)
{
    unsigned char* pOutWork = pOut;
    unsigned char  c = 0;
    unsigned char  t = 0;
    bool bHi = true;

    while (pIn < pEndIn && pOutWork+1 < pEndOut)
    {
        if (*pIn >= '0' && *pIn <= '9')
            c = *pIn - '0';
        else if (*pIn >= 'A' && *pIn <= 'F')
            c = *pIn - 'A' + 10;
        else if (*pIn >= 'a' && *pIn <= 'f')
            c = *pIn - 'a' + 10;

        if (bHi)
            t = (c << 4);
        else
        {
            t |= c;
            *pOutWork++ = t;
        }

        bHi = !bHi;
        ++pIn;
    }

    if (pOutWork < pEndOut)
        *pOutWork = '\0';

    return pOutWork - pOut;
}

