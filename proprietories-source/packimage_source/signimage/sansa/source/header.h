#ifndef _HEADER_H
#define _HEADER_H


typedef int             INT;
typedef unsigned int   UINT;
typedef unsigned int   DWORD;
typedef void *          PVOID;
typedef void *          LPVOID;
typedef DWORD*           LPDWORD;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef short           SHORT;
typedef long            LONG;
typedef unsigned long   ULONG;
typedef BYTE *          LPBYTE;

/* Stdio functions */

#ifdef  _POSIX_
#define _tfdopen    fdopen
#else
#define _tfdopen    _fdopen
#endif
#define _tfsopen    _fsopen
#define _tfopen     fopen
#define _tfreopen   freopen
#define _tperror    perror
#define _tpopen     _popen
#define _ttempnam   _tempnam
#define _ttmpnam    tmpnam

/* Formatted i/o */

#define _tprintf    printf
#define _ftprintf   fprintf
#define _stprintf   sprintf
#define _sntprintf  _snprintf
#define _vtprintf   vprintf
#define _vftprintf  vfprintf
#define _vstprintf  vsprintf
#define _vsntprintf _vsnprintf
#define _tscanf     scanf
#define _ftscanf    fscanf
#define _stscanf    sscanf


#define SAFE_DEL_BUF(p) do{ \
    if((p)!=NULL) \
    {\
        delete [] (p);\
        (p) = NULL;\
    }\
}while(0)

#define __T(x)      x
#define _T(x)       __T(x)


typedef unsigned int uint32;
typedef unsigned char     uint8;
typedef unsigned long long uint64;
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed int        int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
//typedef unsigned long long uint32_t; //CPU=ARM64 don't need this info  

#define MAGIC_SIZE 8
#define FILE_NAME_MAX_SIZE 2048

typedef struct fmpreambleheader {

    /* Magic number */
    uint8_t magic[MAGIC_SIZE];
    /* Version of this header format */
    uint32_t header_version_major;
    /* Version of this header format */
    uint32_t header_version_minor;
    /*option information,bitfield*/
    //uint64_t opt_prv_info;

    /*offset from itself start*/
    /*certificationA size,if 0,ignore*/
    uint64_t certa_size;
    uint64_t certa_offset;

    /*certificationB size,if 0,ignore*/
    uint64_t certb_size;
    uint64_t certb_offset;

    /*content certification size,if 0,ignore*/
    uint64_t certcnt_size;
    uint64_t certcnt_offset;

    /*(opt)private content size,if 0,ignore*/
    uint64_t priv_size;
    uint64_t priv_offset;

    /*(opt)debug/rma certification primary size,if 0,ignore*/
    //uint64_t cert_dbg_prim_size;
    //uint64_t cert_dbg_prim_offset;

    /*(opt)debug/rma certification second size,if 0,ignore*/
    uint64_t cert_dbg_second_size;
    uint64_t cert_dbg_second_offset;

} fmpreambleheader;

#endif

