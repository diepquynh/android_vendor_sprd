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

#include <parser.h>

/**********************************************
 * name: strStartsWith
 * function: judge if a string start with a short string
 * return value: 1 if line starts with prefix, 0 if it does not
 ***********************************************/
int strStartsWith(const char *line, const char *prefix)
{
	for (; *line != '\0' && *prefix != '\0'; line++, prefix++) {
		if (*line != *prefix) {
			return 0;
		}
	}

	return *prefix == '\0';
}

/**********************************************
 * name: FindString
 * function: return the index of the first src in dst
 * return value: dst = "abnmjmhddf" src="jm" return 4
 if fails, return -1
 ***********************************************/
int FindString(char *dst, char *src)
{
	int index = 0;

	if (!dst || !src)
		return -1;

	while (*dst) {
		if (!strncmp(dst,src,strlen(src))) {
			break;
		}
		dst++;
		index++;
	}

	if (*dst == '\0')
		return -1;
	else
		return index;
}

/**********************************************
 * name: at_tok_start
 * function: Skip ":" in an AT response string
 * return value: -1 fail, 0 for success
 ***********************************************/
int at_tok_start(char **ppCur)
{
	int ret;

	ret = FindString(*ppCur, ":");

	if (ret < 0) {
		return -1;
	}

	*ppCur += ret+1;//skip ":", update *ppCur
	return 0;
}

/**********************************************
 * name: get_current_tok
 * function: Skip space chars and get next string after ',' in an AT response string
 * return value: next string after ','
 ***********************************************/
static char * get_current_tok(char **ppCur)
{
	char *tmp;

	// skip the space chars
	while ((*ppCur != NULL) && (**ppCur != '\0') && isspace(**ppCur)) {
		(*ppCur)++;
	}

	if (*ppCur == NULL) {
		return NULL;
	}

	if (strStartsWith(*ppCur, "\"")) {
		// string is in "..."
		(*ppCur)++; // skip first "\""
		tmp = strsep(ppCur, "\"");
		strsep(ppCur, ",");
	} else if (strStartsWith(*ppCur, "\n")) {
		(*ppCur)++;
		tmp = strsep(ppCur, "\n");
		strsep(ppCur, ",");
	} else {
		tmp = strsep(ppCur, ",");
	}

	//if (tmp != NULL) printf(">>%s\n", tmp);

	return tmp;
}

/**********************************************
 * name: parse_nextint
 * function: Parse next int in the AT rsp line
 * return value: <0 for fail, 0 for success
 ***********************************************/
int parse_nextint(char **ppCur, int *ppOut)
{
	char *pcur, *pend;
	long tmp;

	if (*ppCur == NULL) {
		return -1;
	}
	pcur = get_current_tok(ppCur);

	if (pcur == NULL) {
		return -2;
	}

	tmp = strtol(pcur, &pend, 10);
	*ppOut = (int)tmp;

	return 0;
}

/**********************************************
 * name: parse_nextstr
 * function: Parse next str in the AT rsp line
 * return value: 0
 ***********************************************/
int parse_nextstr(char **ppCur, char **ppOut)
{
	char *tmp;

	tmp = get_current_tok(ppCur);

	*ppOut = tmp;

	return 0;
}

/**********************************************
 * name: SemiByteCharsToByte
 * function: Combine 2 characters representing semi-bytes into a byte
 * return value: combine result
 ***********************************************/
unsigned char SemiByteCharsToByte(const char chHigh, const char chLow)
{
	unsigned char bRet;

	if ('0' <= chHigh && '9' >= chHigh) {
		bRet = (chHigh - '0') << 4;
	} else {
		if (chHigh <= 'F' && chHigh >= 'A')
			bRet = (0x0a + chHigh - 'A') << 4;
		else
			// lower case
			bRet = (0x0a + chHigh - 'a') << 4;
	}

	if ('0' <= chLow && '9' >= chLow) {
		bRet |= (chLow - '0');
	} else {
		if (chLow<='F' && chLow>='A')
			bRet |= (0x0a + chLow - 'A');
		else
			// lower case
			bRet |= (0x0a + chLow - 'a');
	}

	return bRet;
}

/**********************************************
 * name: GSMHexToGSM
 * function: GSM data convert
 * return value: shift number
 ***********************************************/
int GSMHexToGSM(const char* sIn, const unsigned int cbIn, const char* sOut, const unsigned int cbOut)
{
	char* pchIn		= (char *) (sIn);
	char* pchInEnd	= (char *) (sIn + cbIn);
	char* pchOut	= (char *) (sOut);
	char* pchOutEnd	= (char *) (sOut + cbOut);
	int rcbUsed;

	while (pchIn < pchInEnd - 1 && pchOut < pchOutEnd) {
		*pchOut++ = SemiByteCharsToByte(*pchIn, *(pchIn + 1));
		pchIn += 2;
	}

	rcbUsed = pchOut - sOut;
	return rcbUsed;
}
