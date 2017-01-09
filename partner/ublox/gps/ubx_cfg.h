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
  \brief  time utility library

  Module for time functions, header
*/
/*******************************************************************************
 * $Id: ubx_cfg.h 60939 2012-08-10 15:29:44Z philippe.kehl $
 ******************************************************************************/
#ifndef __UBX_CFG_H__
#define __UBX_CFG_H__

class CCfg
{
public:
    CCfg();
	~CCfg();
	void load(const char* name);
	int get(const char* item, int def);
	const char* get(const char* item, const char* def);
protected:
	enum { MAX_ITEM = 100 };
	int m_num;
	char* m_itemName[MAX_ITEM];
	char* m_itemData[MAX_ITEM];
};

#endif /* __UBX_CFG_H__ */
