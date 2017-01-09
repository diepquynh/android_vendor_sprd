
#define  MTRACE_REPLACE_API 0

#include <stdio.h>
#include "mtrace.h"

#define CONFIG_ALIGN  1

DEFINE_MEMSTAT(default_memstat);            //total mem trace for all modules

#if (CONFIG_ALIGN == 1)
#define ALIGN(t, align) ((t+align-1) & (~(align-1)))
#else
#define ALIGN(t, align) (t)
#endif

#if (TRACE_MEM_MODE == 0)

#elif ((TRACE_MEM_MODE>0) && (TRACE_MEM_MODE<3))	//mode 1,2

#define  MB_TYPE_MASK     (int)0x00000003

#define  MB_TYPE_C 0
#define  MB_TYPE_M 1
#define  MB_TYPE_R 2
#define  MB_TYPE_F 3

#define  HEAD_MAGIC_MASK  (int)0xCCCCCCCC

typedef struct
{
	int magic;           //HEAD_MAGIC_MASK|MB_TYPE_*
	int size;

#if (TRACE_MEM_MODE&2)
	int logindex;
#endif
}headinfo;

#define  TAIL_MAGIC_MASK  (int)0xa55a0ff0

typedef struct
{
	int magic;           //TAIL_MAGIC_MASK|MB_TYPE_*
	int size;
}tailinfo;

#define  headinfo_len  sizeof(headinfo)
#define  tailinfo_len  sizeof(tailinfo)


#if (TRACE_MEM_MODE&2)

#define MAX_ALLOC_FILNAMELEN	(64*2)
#define memstat_logflag_inited   0x01
#define memstat_logflag_allused  0x02
#define memstat_logindex_invalid  -1

static void init_memlog(memstat_t *stat)
{
	int i;
	if (stat->log.flag & memstat_logflag_inited)
		return ;

#if 0
	char *log =(char *) raw_calloc(sizeof(char *), stat->log.limit);
	if (!log) {
		mtrace_log_error("Failed to calloc %d logs\n", (long)stat->log.limit);
		return ;
	}

	stat->log.logstr = (char **)log;
#endif

	for (i = 0; i < stat->log.limit; i++) {
		stat->log.logstr[i] = NULL;
	}
	stat->log.max_index   = -1;
	stat->log.free_index  = 0;
	stat->log.alloc_count = 0;
	stat->log.flag |= memstat_logflag_inited;

	stat->log.alloc_p = raw_calloc(4096,1);
	if(stat->log.alloc_p){
		stat->log.alloc_p_buffersize = 4096;
	} else {
		stat->log.alloc_p_buffersize = 0;
		mtrace_log_error("register buffer calloc fail!!!\n");
	}
	stat->log.alloc_p_count = 0;
	if (stat != &default_memstat)
		init_memlog(&default_memstat);

}


#define logfmt "%d,%d,%s,%d,%d"
static size_t o_size;
static char o_file[MAX_ALLOC_FILNAMELEN];
static int o_line;
static int o_times;
static int o_alloc_id	= 0;

static int register_log(headinfo *p, int line, const char *file, memstat_t *stat)
{

	int i,j;
	int  retry = 0, ret;
	size_t size = p->size;

	init_memlog(stat);

	for (i=0; (stat->log.alloc_p_buffersize != 0) && (i < (int)(stat->log.alloc_p_buffersize/sizeof(void *))); i++) {
		if(stat->log.alloc_p[i] == NULL){
			stat->log.alloc_p[i] = (long *)p;
			stat->log.alloc_p_count++;
			break;
		}
	}
	if(i >= (int)(stat->log.alloc_p_buffersize/sizeof(void *))){
		mtrace_log_error("register buffer is too small!!!\n");
	}

	p->logindex = memstat_logindex_invalid;

	//updata already exist log entry
	for (i=0;(stat->log.max_index != -1) && (i <= stat->log.max_index); i++) {

		if (stat->log.logstr[i] == NULL)
			continue;

		sscanf(stat->log.logstr[i], logfmt, &o_times, &o_size, o_file, &o_line, &j);
		if ((o_line == line) && (strcmp(o_file, file) == 0)&& o_times>0) {
			ret = snprintf((char*)stat->log.logstr[i], MAX_ALLOC_FILNAMELEN,logfmt, o_times+1, size+o_size, file, line, j);
			if (ret<0) {
				mtrace_log_warn("snprintf error\n");
			}
			p->logindex = i;
			return 0;
		}
	}

	//create new log entry
	if (stat->log.flag & memstat_logflag_allused) {
		mtrace_log_warn("all used out now\n");
		return -1;
	}

	j = stat->log.free_index;
	if (j<0 || j>=stat->log.limit) {
		mtrace_log_warn("free_index error\n");
		for (i = 0 ;i<stat->log.limit; i++) {
			if (stat->log.logstr[i] == NULL)
				break;
		}
		if (i == stat->log.limit) {
			stat->log.flag |= memstat_logflag_allused;
			mtrace_log_warn("all used out now\n");
			return -1;
		}
		j = i;
	} else if (stat->log.logstr[j]) {
		mtrace_log_warn("already used now\n");
		for (i = 0; i<stat->log.limit; i++) {
			if (stat->log.logstr[i] == NULL)
				break;
		}
		if (i == stat->log.limit) {
			stat->log.flag |= memstat_logflag_allused;
			mtrace_log_warn("all used out now\n");
			return -1;
		}
		j = i;
	}

	stat->log.logstr[j] = (char *)raw_calloc(1, MAX_ALLOC_FILNAMELEN);
	if (!stat->log.logstr[j]) {
		mtrace_log_warn("Failed to malloc %d bytes\n", size);
		stat->log.free_index = j;
		return -1;
	}
	ret = snprintf((char*)stat->log.logstr[j], MAX_ALLOC_FILNAMELEN, logfmt, 1, size, file, line, o_alloc_id++);
	if (ret<0) {
		mtrace_log_warn("snprintf error\n");
	}

	stat->log.alloc_count++;
	if (stat->log.max_alloc_count < stat->log.alloc_count)
		stat->log.max_alloc_count = stat->log.alloc_count;

	if (stat->log.max_index < j)
		stat->log.max_index = j;

	i = j+1 ;
again:
	if (stat->log.alloc_count < stat->log.limit) {   //find next free log entry
		for ( ; i<stat->log.limit; i++) {
			if (stat->log.logstr[i] == NULL)
				break;
		}
		if (i == stat->log.limit) {
			if (!retry) {
				i = 0 ;
				retry = 1;
				goto again;
			}
			stat->log.flag |= memstat_logflag_allused;
		}else{
			stat->log.free_index = i;
		}
	}else{
		stat->log.flag |= memstat_logflag_allused;
	}
	p->logindex = j;
	return  0;
}

static int unregister_log(headinfo *p, memstat_t *stat)
{
	int i,j;

	for (i=0; (stat->log.alloc_p_buffersize != 0) && (i < (int)(stat->log.alloc_p_buffersize/sizeof(void *))); i++) {
		if(stat->log.alloc_p[i] != NULL && stat->log.alloc_p[i] == (long *)p){
			stat->log.alloc_p[i] = NULL;
			stat->log.alloc_p_count--;
			break;
		}
	}
	if(i >= (int)(stat->log.alloc_p_buffersize/sizeof(void *))){
		mtrace_log_warn("not register by this module\n");
		return -EEXIST;
	}

	j = p->logindex;
	if (j<0 || j>stat->log.limit)
		return 0;

	if (stat->log.logstr[j] == NULL) {
		mtrace_log_warn("logstr is not appear \n");
		return 0;
	}

	sscanf(stat->log.logstr[j], logfmt, &o_times, &o_size, o_file, &o_line, &i);
	if (o_times > 1) {
		snprintf((char*)stat->log.logstr[j], MAX_ALLOC_FILNAMELEN, logfmt, --o_times, o_size-p->size, o_file, o_line, i);
		return 0;
	}

	raw_free(stat->log.logstr[j]);
	stat->log.logstr[j] = NULL;
	if ((stat->log.flag & memstat_logflag_allused) || (j < stat->log.free_index))
		stat->log.free_index = j;
	stat->log.alloc_count--;

	if (stat->log.max_index == j) {
		for  ( ;j>=0 && stat->log.logstr[j] == NULL; j--)
			stat->log.max_index --;
	}

	stat->log.flag &= ~memstat_logflag_allused;
	return 0;
}

static void show_log(headinfo *p, memstat_t *stat)
{
	int j = p->logindex;

	if (j<0 || j>stat->log.limit) {
		return ;
	}

	if (stat->log.logstr[j] == NULL) {
		mtrace_log_warn("freed\n");
	}else{
		mtrace_log_warn("meminfo %s\n", stat->log.logstr[j]);
	}
}
#else
static void show_log(headinfo *p, memstat_t *stat)
{
}

#endif

static DECLEAR_KEY(memstat_sem);  //run on multiple task env supporting
static int  updata_meminfo(char *pmem, int type, int size, memstat_t *stat, int line, const char *file)
{
	int ret = 0;
	headinfo *p = (headinfo *)pmem;

	if (!memstat_sem) {
		INIT_THREAD_MUTEX(memstat_sem);
		if (!memstat_sem) {
			mtrace_log_warn("memstat_sem create fail\n");
			return -1;
		}
	}

	THREAD_LOCK(memstat_sem);
	if (size>0) {
		tailinfo *ptail = (tailinfo *)(pmem + headinfo_len + ALIGN(size,4));

		p->magic = HEAD_MAGIC_MASK|type;
		p->size  = size;
		ptail->magic = TAIL_MAGIC_MASK|type;
		ptail->size = size;

		if (type==MB_TYPE_C) {
			stat->calloc += size;
			stat->calloc_cnt++;
		} else if (type==MB_TYPE_M) {
			stat->malloc += size ;
			stat->malloc_cnt++;
		} else if (type==MB_TYPE_R) {
			stat->realloc += size;
			stat->realloc_cnt++;
		} else {
		}

		stat->total += size;
		stat->total_cnt++;

		if (stat->max_once < size)
			stat->max_once = size;
		if (stat->max_total < stat->total)
			stat->max_total = stat->total;
		if (stat->max_total_cnt < stat->total_cnt)
			stat->max_total_cnt = stat->total_cnt;

		if (stat != &default_memstat) {
			if (type == MB_TYPE_C) {
				default_memstat.calloc += size;
				default_memstat.calloc_cnt++;
			} else if (type == MB_TYPE_M) {
				default_memstat.malloc += size ;
				default_memstat.malloc_cnt++;
			} else if (type == MB_TYPE_R) {
				default_memstat.realloc += size;
				default_memstat.realloc_cnt++;
			}else{
			}

			default_memstat.total += size;
			default_memstat.total_cnt++;

			if (default_memstat.max_once < size)
				default_memstat.max_once = size;
			if (default_memstat.max_total < default_memstat.total)
				default_memstat.max_total = default_memstat.total;

			if (default_memstat.max_total_cnt < default_memstat.total_cnt)
				default_memstat.max_total_cnt = default_memstat.total_cnt;
		}

#if (TRACE_MEM_MODE&2)
		ret = register_log(p, line, file, stat);
#endif

	} else	{
		int error = 0;
		tailinfo *ptail = (tailinfo *)(pmem + headinfo_len + ALIGN(p->size,4));

#if (TRACE_MEM_MODE&2)
		ret = unregister_log(p,stat);
#endif
		if(ret){
			THREAD_UNLOCK(memstat_sem);
			return ret;
		}

		type = p->magic & MB_TYPE_MASK;
		size = p->size;

		if ((p->magic != (HEAD_MAGIC_MASK|type))) {
			mtrace_log_error("head overwite at %p,0x%x,0x%x\n", p, p->magic, p->size);
			error = 1;
		}
		if (ptail->magic != (TAIL_MAGIC_MASK|type)) {
			mtrace_log_error("tail overwite at %p,0x%x,0x%x\n", p, ptail->magic, ptail->size);
			error ++;
		}
		if (error != 0) {
			show_log(p, stat);
			THREAD_UNLOCK(memstat_sem);
			return -2;
		} else if(type == MB_TYPE_F || type == MB_TYPE_F) {
			mtrace_log_error("freed %p again\n", p);
			THREAD_UNLOCK(memstat_sem);
			return -3;
		}

		p->magic = HEAD_MAGIC_MASK|MB_TYPE_F;             //avoid  free again
		p->size = 0;
		ptail->magic = TAIL_MAGIC_MASK|MB_TYPE_F;
		ptail->size = 0;
		if (type == MB_TYPE_C) {
			stat->calloc -= size ;
			stat->calloc_cnt--;
		}
		else if (type == MB_TYPE_M) {
			stat->malloc -= size ;
			stat->malloc_cnt--;
		}
		else if (type == MB_TYPE_R) {
			stat->realloc -= size ;
			stat->realloc_cnt--;
		}
		stat->total -= size ;
		stat->total_cnt--;

		if (stat!= &default_memstat) {
			if (type == MB_TYPE_C) {
				default_memstat.calloc -= size ;
				default_memstat.calloc_cnt--;
			}
			else if (type == MB_TYPE_M) {
				default_memstat.malloc -= size ;
				default_memstat.malloc_cnt--;
			}
			else if (type == MB_TYPE_R) {
				default_memstat.realloc -= size ;
				default_memstat.realloc_cnt--;
			}
			default_memstat.total -= size ;
			default_memstat.total_cnt--;
		}
	}
#if 0
	mtrace_log_info("total:0x%x,max_once:0x%x,max_total:0x%x,max_total_cnt:0x%x\n",
		stat->total, stat->max_once, stat->max_total, stat->max_total_cnt);
	mtrace_log_info("calloc_cnt:0x%x,malloc_cnt:0x%x,realloc_cnt:0x%x,total_cnt:0x%x\n",
		stat->calloc_cnt, stat->malloc_cnt, stat->realloc_cnt, stat->total_cnt);
#endif
	THREAD_UNLOCK(memstat_sem);
	return ret;
}


/**
* mtrace_calloc
*
* Return a pointer to the allocated memory or NULL if the request fails.
*/
void *mtrace_calloc_trace(size_t size, int nunit, const char *file, const char* fun, int line, memstat_t *stat)
{
	char *p;

	if (size <= 0)
		return NULL;
	size *= nunit;

	p = (char *)raw_calloc(1, ALIGN(size, 4)+headinfo_len+tailinfo_len);
	if (!p)	{
		mtrace_log_error("Failed to calloc %d bytes at %s:%s:%d \n", size, file, fun, line);
		errno = ENOMEM;
		return NULL;
	}
	mtrace_log_info("calloc %p,%x at %s:%s:%d \n", p, size, file, fun, line);

	updata_meminfo(p, MB_TYPE_C, size, stat, line, file);

	return (char *)p + headinfo_len;
}

void *mtrace_malloc_trace(size_t size, const char *file, const char* fun, int line, memstat_t *stat)
{
	char *p;

	if (size <= 0)
		return NULL;

	p = (char *)raw_malloc(ALIGN(size,4) + headinfo_len + tailinfo_len);
	if (!p)	{
		mtrace_log_error("Failed to malloc %d bytes at %s:%s:%d \n", size, file, fun, line);
		errno = ENOMEM;
		return NULL;
	}

	mtrace_log_info("malloc %p,%x at %s:%s:%d \n", p, size, file, fun, line);

	updata_meminfo(p, MB_TYPE_M, size, stat, line, file);

	return (char *)p + headinfo_len;
}

void *mtrace_realloc_trace(void *old, size_t size, const char *file, const char* fun, int line, memstat_t *stat)
{
	char *p;
	int type ;

	if (size<=0)
		return NULL;

	mtrace_log_info("realloc at %s:%s:%d \n", file, fun, line);
	if (old) {
		p =(char *)((char *)old - headinfo_len) ;

		if (updata_meminfo(p, -1, -1, stat, line,file) != 0)	{
			return NULL;
		}
		type = MB_TYPE_R ;
	} else {
		p = NULL;
		type = MB_TYPE_M ;
	}
	p = (char *)raw_realloc(p, ALIGN(size, 4) + headinfo_len + tailinfo_len);
	if (!p)
	{
		mtrace_log_error("Failed to realloc %d bytes at %s:%s:%d \n", size, file, fun, line);
		errno = ENOMEM;
		return NULL;
	}

	updata_meminfo(p, type, size, stat, line, file);

	return (char *)p + headinfo_len;
}

void mtrace_free_trace(void *pmen, const char *file, const char* fun, int line, memstat_t *stat)
{
	if (pmen) {

		char *p = (char *)((char *)pmen - headinfo_len) ;
		int  ret;
		ret = updata_meminfo(p, -1, -1, stat, line, file);
		if (ret == 0) {
			mtrace_log_info("free %p at %s:%s:%d \n", p, file, fun, line);
			raw_free(p);
		} else if(ret == -EEXIST) {
			mtrace_log_info("err free %p at %s:%s:%d \n", pmen, file, fun, line);
			raw_free(pmen);
		}
	}
}


static int checkregister(void *pbuf, size_t num, int line, const char *file, memstat_t *stat)
{
	int i;
	int ret = 0 ;

	THREAD_LOCK(memstat_sem);

	for (i=0; (stat->log.alloc_p_buffersize != 0) && (i < (int)(stat->log.alloc_p_buffersize/sizeof(void *))); i++) {
		if(stat->log.alloc_p[i] != NULL ){

			headinfo *p = (headinfo *)stat->log.alloc_p[i];
			tailinfo *ptail = (tailinfo *)((char *)p + headinfo_len + ALIGN(p->size,4));
			char *ptr = (char *)(stat->log.alloc_p[i] + headinfo_len)  ;

			int type = p->magic & MB_TYPE_MASK;
			int size = p->size;
			int error = 0;

			if ((p->magic != (HEAD_MAGIC_MASK|type))) {
				mtrace_log_error("head overwite at %p,0x%x,0x%x\n", p, p->magic, p->size);
				error = 1;
			}
			if (ptail->magic != (TAIL_MAGIC_MASK|type)) {
				mtrace_log_error("tail overwite at %p,0x%x,0x%x\n", p, ptail->magic, ptail->size);
				error ++;
			}
			if (error != 0) {
			} else if((type == MB_TYPE_F) || (type == MB_TYPE_F)) {
				mtrace_log_error("already free %p \n", p);
			} else if( ((char *)pbuf <= (char *)p && ((char *)pbuf + num) > (char *)p)
				|| ((char *)pbuf < ((char *)p + headinfo_len) && ((char *)pbuf + num) >= ((char *)p + headinfo_len))) {
				mtrace_log_error("will overwite head %p by %s:%d\n", p, file, line);
				show_log(p, stat);
				ret = -1;
			} else if( ((char *)pbuf <= (char *)ptail  && ((char *)pbuf + num) > (char *)ptail)
				|| ((char *)pbuf < ((char *)ptail + tailinfo_len) && ((char *)pbuf + num) >= ((char *)ptail + tailinfo_len))) {
				mtrace_log_error("will overwite tail %p by %s:%d\n", p, file, line );
				show_log(p, stat);
				ret = -1;
			} else if( ((char *)pbuf >= ((char *)p + headinfo_len)) && ((char *)pbuf + num) <= (char *)ptail) {
				mtrace_log_info("normal wite %p by %s:%d\n", p, file, line );
			}
		}
	}

	THREAD_UNLOCK(memstat_sem);
	return ret;
}

void* mtrace_memset_trace(void * ptr, int value, size_t num, const char *file, const char* fun, int line, memstat_t *stat)
{
	mtrace_log_info("memset %p,%x at %s:%s:%d \n", ptr, num, file, fun, line);

	checkregister(ptr ,num, line, file ,stat);

	return memset(ptr,value,num);
}

void* mtrace_memcpy_trace(void * destination, const void * source, size_t num, const char *file, const char* fun, int line, memstat_t *stat)
{
	mtrace_log_info("memcpy %p,%x at %s:%s:%d \n", destination, num, file, fun, line);

	checkregister(destination ,num, line, file ,stat);

	return memcpy(destination,source,num);
}

#else   //mode 3

void *mtrace_calloc(size_t size, int nunit)
{

	void *p;

	if (size <= 0)
		return NULL;

	size *= nunit;

	p = raw_calloc(1, ALIGN(size, 4));
	if (!p) {
		mtrace_log_error("Failed to calloc %d bytes\n", (long)size);
		errno = ENOMEM;
	}

	return p;
}

void *mtrace_malloc(size_t size)
{
	void *p;

	if (size <= 0)
		return NULL;

	p = raw_malloc(ALIGN(size, 4));
	if (!p) {
		mtrace_log_error("Failed to malloc %d bytes\n", (long)size);
		errno = ENOMEM;
	}

	return p;
}

void *mtrace_realloc(void *old, size_t size)
{

	void *p;

	if (size <= 0)
		return NULL;

	p = raw_realloc(old, ALIGN(size, 4));
	if (!p)	{
		mtrace_log_error("Failed to realloc %d bytes\n", (long)size);
		errno = ENOMEM;
	}

	return p;
}

void mtrace_free(void *pmen)
{
	if (pmen) {
		raw_free(pmen);
	}
}

#endif

#if 0 //TRACE_MEM_PRINT_ENABLE


extern int  OSTime ;
void mtrace_log_redirect(const char *function, const char *file, int line,
						 const char *FORMAT, ...)
{
//	OS_CPU_SR cpu_sr;
	//  INT32U  tick = AVTimeGet();           //avoid  OS_EXIT_CRITICAL in printf
	va_list args;

//	OS_ENTER_CRITICAL();
//	printf("\n%s (%s,line=%d,tick=%d)\n--" ,function,file,line,OSTime);
	va_start (args, FORMAT);
	vprintf (FORMAT, args);
	va_end (args);
//	OS_EXIT_CRITICAL();

}
#endif

void mtrace_print_alllog(memstat_t *stat)
{

#if (TRACE_MEM_MODE == 2)
	int i;

	THREAD_LOCK(memstat_sem);
	mtrace_log_info("mtrace_print_alllog E\n");
	if (!stat){
		mtrace_log_error("!stat\n");
		goto exit;
	}
	if (!(stat->log.flag & memstat_logflag_inited)){
		mtrace_log_error("!(stat->log.flag & memstat_logflag_inited)\n");
		goto exit;
	}

	mtrace_log_warn("max_index=%d,free_index=%d\n", stat->log.max_index, stat->log.free_index);
	if (MAX_ALLOC_ENTRYS != stat->log.limit) {
		mtrace_log_warn("limit=%d,%d\n",stat->log.limit,MAX_ALLOC_ENTRYS);
	}

	mtrace_log_info("total:0x%x, max_once:0x%x, max_total:0x%x, max_total_cnt:0x%x\n",
		stat->total, stat->max_once, stat->max_total, stat->max_total_cnt);
	mtrace_log_info("calloc_cnt:0x%x, malloc_cnt:0x%x, realloc_cnt:0x%x, total_cnt:0x%x\n",
		stat->calloc_cnt, stat->malloc_cnt, stat->realloc_cnt, stat->total_cnt);

	for  (i = 0; i < stat->log.limit; i++) {
		if (stat->log.logstr[i] != NULL) {
			mtrace_log_warn("logindex = %d,log = %s\n", i, stat->log.logstr[i]);
		}
	}

exit:
	mtrace_log_info("mtrace_print_alllog X\n");
	THREAD_UNLOCK(memstat_sem);

#endif
	return;
}

