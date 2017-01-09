
#ifndef __SPRDDM_ADAPTER_H__
#define __SPRDDM_ADAPTER_H__


#ifndef FALSE
#define FALSE (0)
#endif


#ifndef  TRUE
#define TRUE (1)
#endif

#ifndef NULL
#define NULL (0L)
#endif

#ifndef PNULL
#define PNULL  (void*)0
#endif


typedef unsigned char BOOLEAN;
typedef unsigned char BOOL;

  #ifndef uchar
#define uchar   unsigned char
#endif

 #ifndef uint8
#define uint8   unsigned char
#endif

 #ifndef uint16
#define uint16   unsigned short
#define int16    short
#endif


#ifndef uint32
#define uint32    unsigned int
#define int32     int
#endif

#ifndef ulong
#define ulong   unsigned long int
#endif

#ifndef SCI_STRLEN
#define SCI_STRLEN(PTR)   (PTR!=PNULL)?strlen(PTR):0
#endif

#ifndef SCI_STRCPY
#define SCI_STRCPY strcpy
#endif

#ifndef SCI_MEMCPY
#define SCI_MEMCPY(aac_des, aac_src, aac_m)     memcpy(aac_des, aac_src, aac_m)
#endif

#ifndef SCI_MEMSET
#define SCI_MEMSET(aac_data, aac_n, aac_m)      memset(aac_data, aac_n, aac_m)
#endif

#define SCI_TRACE_LOW spdm_print


/*
#ifndef SCI_ALLOC
#define SCI_ALLOC(_SIZE)     (_SIZE == 0)? malloc(_SIZE):PNULL     
#define SCI_ALLOCA(_SIZE)   (_SIZE == 0)? malloc(_SIZE):PNULL        
#define SCI_MPFREE(_MEM_PTR)  free(_MEM_PTR);   
#define SCI_FREE(_MEM_PTR)  free(_MEM_PTR);  
#endif
*/
#ifndef SCI_ALLOC
#define SCI_ALLOC(_SIZE)      /*(!_SIZE)?PNULL:*/malloc(_SIZE) 
#define SCI_MPFREE(_MEM_PTR)  free(_MEM_PTR);   
#define SCI_FREE(_MEM_PTR)  free(_MEM_PTR);  
#endif

 #ifndef LOCAL
 #define  LOCAL static
 #endif
 
  #ifndef PUBLIC
 #define  PUBLIC
  #endif
  
#ifndef   ARR_SIZE
#define ARR_SIZE(x)   (sizeof(x) / sizeof(x[0]) )
#endif

#endif






