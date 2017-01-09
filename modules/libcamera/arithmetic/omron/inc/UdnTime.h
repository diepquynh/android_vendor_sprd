#ifndef UDNTIME_H__
#define UDNTIME_H__

#include    "UdnDef.h"

#if (defined( WIN32 ) || defined( WIN64 ))
typedef LARGE_INTEGER   OV_CLOCK_T;
#else   /* WIN32 || WIN64 */
typedef UINT32          OV_CLOCK_T;
#endif  /* WIN32 || WIN64 */

#ifdef  __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************/
/* To use these timeout functions, the application software, that calls the Library, requires following two functions. */
/* Even if you do not require these timout functions, dummy functions must be prepared. Otherwise linker error occurs. */
/***********************************************************************************************************************/
/* This function is called when starting the stopwatch inside the UDN Vision library */
OV_CLOCK_T  UdnExtraInitTime(void);

/* This function returns the millisecond time since initialization using the argument and the current time */
UINT32  UdnExtraGetTime(OV_CLOCK_T unInitTime);

#ifdef  __cplusplus
}
#endif

#endif  /* UDNTIME_H__ */
