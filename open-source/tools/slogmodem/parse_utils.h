/*
 *  parse_utils.h - The parsing utility functions declarations.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-2-16 Zhang Ziyi
 *  Initial version.
 */
#ifndef _PARSE_UTILS_H_
#define _PARSE_UTILS_H_

#include <cstdint>
#include <cstddef>

const uint8_t* get_token(const uint8_t* buf, size_t& tlen);
const uint8_t* get_token(const uint8_t* data, size_t len, size_t& tlen);

const uint8_t* find_str(const uint8_t* data, size_t len, const uint8_t* pat,
                        size_t pat_len);

int parse_number(const uint8_t* data, size_t len, unsigned& num);
int parse_number(const uint8_t* data, size_t len, unsigned& num,
                 size_t& parsed);

#endif  // !_PARSE_UTILS_H_
