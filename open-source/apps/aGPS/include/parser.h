/*
 *  AT Parser
 *
 *  Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __PARSER_H
#define __PARSER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int FindString(char *dst, char *src);
int at_tok_start(char **ppCur);
static char * get_current_tok(char **ppCur);
int parse_nextint(char **ppCur, int *ppOut);
int parse_nextstr(char **ppCur, char **ppOut);
int strStartsWith(const char *line, const char *prefix);
unsigned char SemiByteCharsToByte(const char chHigh, const char chLow);
int GSMHexToGSM(const char* sIn, const unsigned int cbIn, const char* sOut, const unsigned int cbOut);

#endif
