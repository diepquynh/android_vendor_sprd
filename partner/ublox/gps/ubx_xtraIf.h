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
 *******************************************************************************
 *
 * Project: PE_AND
 *
 ******************************************************************************/
/*!
  \file
  \brief  Xtra interface implementation

*/
/*******************************************************************************
 * $Id: ubx_xtraIf.h 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/
#ifndef __UBX_XTRAIF_H__
#define __UBX_XTRAIF_H__

#include "std_inc.h"

class CXtraIf
{
public:
    CXtraIf();
	static const void *getIf() { return &s_interface; }
	// callbacks
	static void requestDownload(void);

private:
	// interface
 	static int init(GpsXtraCallbacks* callbacks);
	static int injectData(char* data, int length);
	
	// variables
	const static GpsXtraInterface s_interface;
	GpsXtraCallbacks m_callbacks;
    bool m_ready;
};

#endif /* __UBX_XTRAIF_H__ */
