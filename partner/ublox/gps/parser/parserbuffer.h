/*******************************************************************************
 *
 * Copyright (C) u-blox ag
 * u-blox ag, Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * This  source  file  is  the  sole  property  of  u-blox  AG. Reproduction or
 * utilization of this source in whole or part is forbidden without the written 
 * consent of u-blox AG.
 *
 ******************************************************************************/
/*!
  \file
  \brief 
*/
/*******************************************************************************
 * $Id: parserbuffer.h 49130 2011-07-04 14:28:49Z andrea.foni $
 ******************************************************************************/
#ifndef __PARSERBUFFER_H__
#define __PARSERBUFFER_H__

#include "protocol.h"

#ifndef PARSER_ASSERT
 #define PARSER_ASSERT(a)
#endif

class CParserBuffer
{
public:
	enum { WAIT = 0, NOT_FOUND = -1};
	
	CParserBuffer();
	virtual ~CParserBuffer();

	void Compact();
	bool Empty(CProtocol* &pProtocol, unsigned char*& pData, int& iSize);
	bool Parse(CProtocol* &pProtocol, unsigned char* &pData, int &iSize);
	bool Register(CProtocol* pProtocol);
	void RegisterUnknown(CProtocol* pProtocol);
	
	unsigned char* GetPointer();
	unsigned char* GetData();
	int GetSpace();
	int GetSize();
	void Append(int iSize);
	void Remove(int iSize);

protected:
	typedef struct RegisterInfo_s
	{
		CProtocol* pProtocol;
		struct RegisterInfo_s* pNext;
	} RegisterInfo;
	RegisterInfo* mpRoot;
	CProtocol* mpProtocolUnknown;
	unsigned char* mpBuffer;
	int miSize;
	int miUsed;
	int miDone;
};

inline unsigned char* CParserBuffer::GetPointer()
{
	return &mpBuffer[miUsed];
}

inline unsigned char* CParserBuffer::GetData()
{
	return &mpBuffer[miDone];
}
inline int CParserBuffer::GetSpace()
{
	return miSize - miUsed;
}

inline int CParserBuffer::GetSize()
{
	return miUsed - miDone;
}

inline void CParserBuffer::Append(int iSize)
{
	PARSER_ASSERT(iSize > 0);
	PARSER_ASSERT(miUsed + iSize <= miSize);
	miUsed += iSize;
}

inline void CParserBuffer::RegisterUnknown(CProtocol* pProtocol)
{
	mpProtocolUnknown = pProtocol;
}

#endif // __PARSERBUFFER_H__
