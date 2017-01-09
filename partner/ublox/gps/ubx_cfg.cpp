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

  Module for time functions
*/
/*******************************************************************************
 * $Id: ubx_cfg.cpp 61785 2012-09-10 14:35:47Z michael.ammann $
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "std_types.h"
#include "ubx_cfg.h"
#include "ubx_log.h"

CCfg::CCfg()
{
	m_num = 0;
	memset(m_itemName, 0, sizeof(m_itemName));
	memset(m_itemData, 0, sizeof(m_itemData));
}

CCfg::~CCfg()
{
	while (m_num --)
	{
		delete [] m_itemName[m_num];
		delete [] m_itemData[m_num];
	}
	m_num = 0;
}

void CCfg::load(const char* fileName)
{
	FILE* file = fopen(fileName, "r");
	LOGD("CCfg::%s : fileName=\"%s\"", __FUNCTION__, fileName);
	if (file == NULL)
    {
        // failed
        LOGV("CCfg::%s : Can not open '%s' file : %i", __FUNCTION__, fileName, errno);
        return;
    }

    int row = 0;
	while(!feof(file))
    {
		char line[1024];
        memset(line, 0, sizeof(line));
        // Load each line
        if (fgets(line, sizeof(line), file) != NULL)
        {
			row ++;
			const char* name;
			const char* data;
			const char* p;
			int nameLen = 0;
			int dataLen = 0;
			p = line;
			// skip spaces
			while (isspace(*p))	p++;
			name = p;
			// find end of name
			while (isgraph(*p) && (*p != '=') && (*p != '#')) 
				p++;
			nameLen = p-name;
			if (nameLen > 0)
			{
				// skip spaces
				while (isspace(*p) && (*p != '#') && (*p != '\r') && (*p != '\n'))	p ++;
				// skip equal sign
				if (*p == '=') p ++;
				// skip spaces 
				while (isspace(*p) && (*p != '#') && (*p != '\r') && (*p != '\n'))	p ++;
				data = p;
				// take all until we find end of line or comment start
				while ((*p != '\0') && (*p != '#') && (*p != '\r') && (*p != '\n'))
				{
					if (isgraph(*p))
						dataLen = p - data + 1;
					p ++;
				}
				if (m_num < MAX_ITEM)
				{
					m_itemName[m_num] = new char[nameLen+1];
					m_itemData[m_num] = new char[dataLen+1];
					strncpy(m_itemName[m_num], name, nameLen);
					strncpy(m_itemData[m_num], data, dataLen);
					m_itemName[m_num][nameLen] = '\0'; 
					m_itemData[m_num][dataLen] = '\0'; 
					//LOGD("CCfg::%s index=%d name=\"%s\" data=\"%s\"", __FUNCTION__, m_num, m_itemName[m_num], m_itemData[m_num]);
					m_num ++;
				}
				else
					LOGE("CCfg::%s items list full", __FUNCTION__);
			}
		}
	}
	fclose(file);
}

int CCfg::get(const char* item, int def)
{
	int cnt = 0;
	int val = def;
	for (int i = 0; i < m_num; i ++)
	{
		if (strcmp(m_itemName[i], item) == 0)
		{
			char* end;
			if ((item[0] == '0') && (item[1] == 'x' || item[1] == 'X'))
				val = strtol(m_itemData[i]+2, &end, 16);
			else if ((item[0] == '0') && (item[1] == 'b' || item[1] == 'B'))
				val = strtol(m_itemData[i]+2, &end, 2);
			else
				val = strtol(m_itemData[i], &end, 10);
			if (*end != 0) 
				val = def;
			break;
		}
	}
	LOGD("CCfg::%s item=\"%s\" def=%d => %d", __FUNCTION__, item, def, val);
	return val;
}

const char* CCfg::get(const char* item, const char* def)
{
	const char*	val = def;
	for (int i = 0; i < m_num; i ++)
	{
		if (strcmp(m_itemName[i], item) == 0)
		{
			val = m_itemData[i];
			break;
		}
	}
	LOGD("CCfg::%s item=\"%s\" def=\"%s\" => \"%s\"", __FUNCTION__, item, def ? def : "NULL", val ? val : "NULL");
	return val;
}

