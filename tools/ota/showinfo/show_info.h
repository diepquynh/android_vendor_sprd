#ifndef _SHOW_INFO_
#define _SHOW_INFO_

#ifdef __cplusplus
extern "C" {
#endif

int printf_with_time(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#define printf printf_with_time
#endif


