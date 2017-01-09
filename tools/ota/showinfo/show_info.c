/*
 *Create by Spreadst for show info
 */
#include<stdio.h>
#include<time.h>
#include<stdarg.h>
#include "show_info.h"
int printf_with_time(const char *fmt, ...)
{
    int ret;
    va_list ap;
    time_t time_t;
    struct tm * tm;
    time(&time_t);
    tm=localtime(&time_t);

    fprintf(stdout,"%d.%02d.%02d %02d:%02d:%02d  : ",
            (1900+tm->tm_year),(1+tm->tm_mon),tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
    va_start(ap, fmt);
    ret = vprintf(fmt, ap);
    va_end(ap);
    return (ret);

}
