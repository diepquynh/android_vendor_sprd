/* cplogctl_cmn.h - common definitions for cplogctl
 *
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * History:
 * 2016-11-15 YAN Zhihang
 * initial version
 */

#ifndef CPLOGCTL_CMN_H_
#define CPLOGCTL_CMN_H_

#include <cstddef>
#include <cstdint>

enum MediaType { MT_INTERNAL, MT_EXTERNAL };

/*  non_negative_number - check the number is
 *                        a non-negative number or not.
 *  @result - string pointer
 *  @number - number converted from result
 *  @stop - the stop pointer
 *
 *  Return true if result is a valid non-negative string number
 */
bool non_negative_number(const char* result, unsigned long& number,
                         const char*& stop);

/*  spaces_only - check whether there are only spaces in the specified string.
 *  @s - string pointer
 *
 *  Return true if there are only spaces in the string.
 */
bool spaces_only(const char* s);

bool spaces_only(const uint8_t* remain, size_t len);

#endif  //!CPLOGCTL_CMN_H_
