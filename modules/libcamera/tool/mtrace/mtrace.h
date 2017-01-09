
#ifndef MTRACE_H
#define MTRACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include  "config.h"

#include <stdint.h>
#include <errno.h>


#ifndef MAX_ALLOC_ENTRYS
	#define MAX_ALLOC_ENTRYS	128
#endif
#ifndef TRACE_MEMSTAT
 #define TRACE_MEMSTAT &default_memstat
#endif

//fit to yourown requirement
//#define  u32   unsigned  int
//#define  s32   int
//#define  size_t   int

typedef struct
{
	int calloc;
	int malloc;
	int realloc;
	int total;

	int max_once;
	int max_total;

	int calloc_cnt;
	int malloc_cnt;
	int realloc_cnt;
	int total_cnt;
	int max_total_cnt;

	struct
	{
		int  flag;
		int  limit;
#if(TRACE_MEM_MODE&2)
		int  max_index;        //max valid index,fast find  matched log
		int  free_index;       //first free log index,fast for next log apply
		int  alloc_count;      //current total used log entrys
		int  max_alloc_count;
		char *logstr[MAX_ALLOC_ENTRYS];
//		char **logstr;
#endif
		int  alloc_p_buffersize;
		int  alloc_p_count;
		long **alloc_p;

	}log;

}memstat_t;

#if(TRACE_MEM_MODE&2)
#define   DEFINE_MEMSTAT(hinst)   memstat_t hinst={0,0,0,0, 0,0, 0,0,0,0,0, {0,MAX_ALLOC_ENTRYS,-1,0,0,0,{NULL},0,0,NULL}}
#else
#define   DEFINE_MEMSTAT(hinst)   memstat_t hinst={0,0,0,0,0,0,0,0,0,0,0,{0,MAX_ALLOC_ENTRYS}}
#endif
#define   DECLARE_MEMSTAT(hinst)  extern  memstat_t hinst

DECLARE_MEMSTAT(default_memstat);
void mtrace_print_alllog(memstat_t *stat);

#if(TRACE_MEM_MODE==0)   //mode 0

#define mtrace_calloc(n, size)   raw_calloc(n, size)
#define mtrace_malloc( size)     raw_malloc(size)
#define mtrace_realloc(p, size)  raw_realloc(p, size )
#define mtrace_free(p)           raw_free(p)

#ifndef MTRACE_REPLACE_API
#define  MTRACE_REPLACE_API 0
#endif

#elif(TRACE_MEM_MODE>0)&&(TRACE_MEM_MODE<3)   //mode 1,2


extern void *mtrace_calloc_trace(size_t size, int nunit, const char *file, const char* fun, int line, memstat_t *stat);
extern void *mtrace_malloc_trace(size_t size, const char *file, const char* fun, int line, memstat_t *stat);
extern void *mtrace_realloc_trace(void *old, size_t size, const char *file, const char* fun, int line, memstat_t *stat);
extern void mtrace_free_trace (void *pmen, const char *file, const char* fun, int line, memstat_t *stat);
extern void* mtrace_memset_trace(void * ptr, int value, size_t num, const char *file, const char* fun, int line, memstat_t *stat);
extern void* mtrace_memcpy_trace(void * destination, const void * source, size_t num, const char *file, const char* fun, int line, memstat_t *stat);

#define mtrace_calloc(size, nunit)	mtrace_calloc_trace(size, nunit, __FILE__, __FUNCTION__, __LINE__, TRACE_MEMSTAT)
#define mtrace_malloc(size)			mtrace_malloc_trace(size, __FILE__, __FUNCTION__, __LINE__, TRACE_MEMSTAT)
#define mtrace_realloc(p, size)		mtrace_realloc_trace(p, size, __FILE__, __FUNCTION__, __LINE__, TRACE_MEMSTAT)
#define mtrace_free(p)				mtrace_free_trace(p, __FILE__, __FUNCTION__, __LINE__, TRACE_MEMSTAT)
#define mtrace_memset(p, val, size)	mtrace_memset_trace(p, val, size, __FILE__, __FUNCTION__, __LINE__, TRACE_MEMSTAT)
#define mtrace_memcpy(dst, src, size)	mtrace_memcpy_trace(dst, src, size, __FILE__, __FUNCTION__, __LINE__, TRACE_MEMSTAT)

#ifndef MTRACE_REPLACE_API
#define  MTRACE_REPLACE_API 1
#endif

#else   //mode 3

extern void *mtrace_calloc(size_t size, int nunit);
extern void *mtrace_malloc(size_t size);
extern void *mtrace_realloc(void *old, size_t size);
extern void mtrace_free(void *pmen);

#ifndef MTRACE_REPLACE_API
#define  MTRACE_REPLACE_API 1
#endif

#endif



#if  (MTRACE_REPLACE_API == 1)
#define calloc(n, size)		mtrace_calloc(n, size)
#define malloc(size)		mtrace_malloc(size)
#define realloc(p, size)	mtrace_realloc(p, size)
#define free(p)				mtrace_free(p)

#define memset	mtrace_memset
#define memcpy	mtrace_memcpy

#endif


#ifdef __cplusplus
}
#endif

#endif
