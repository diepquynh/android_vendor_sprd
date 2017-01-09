// 
// anli   2012-12-28
//

#include "debug.h"
#include <stdlib.h>

#include <sys/time.h>

//-----------------------------------------------------------------------------
#if 1
#define LOG_PATH "/data"
#else
#define LOG_PATH "/mnt/sdcard"
#endif 
//-----------------------------------------------------------------------------
static FILE * s_pf = NULL;
static int    sMsg2File = 0;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void dbgMsg2FileEnable( int enb )
{
	sMsg2File = enb;
}

int dbgMsg2FileOpen( const char * filename )
{
    AT_ASSERT( NULL == s_pf );
    
    s_pf = fopen(filename, "a+");
	if( NULL != s_pf ) {
		fseek(s_pf, 0, SEEK_END);
		long size = ftell(s_pf);
		if( size > 512 * 1024 ) {
			fclose(s_pf);
			s_pf = fopen(filename, "w");
		}
	}
    return 0;
}

int dbgMsg2File( const char * fmt, ... )
{
	if( !sMsg2File ) {
		return 0;
	}
	
    if( NULL == s_pf ) {
		char name[128];
		//struct timeval tv;
		//gettimeofday(&tv, NULL);
		//snprintf(name, 128, "%s/autotst_%u_%u.log", LOG_PATH, (uint)tv.tv_sec, (uint)tv.tv_usec);
		snprintf(name, 128, "%s/%s", LOG_PATH, DBG_FILE_NAME);
        dbgMsg2FileOpen(name);
    }
	
    if( NULL != s_pf ) {
        int len;
		va_list vl;
        va_start(vl, fmt);
        
        static char buf[4096];
        vsnprintf(buf, 4090, fmt, vl);
        va_end(vl);
        
		len = strlen(buf);
		if( '\r' != buf[len - 1] && '\n' != buf[len - 1] ) {
			buf[len] = '\n';
			len++;
		}
        fwrite(buf, 1, len, s_pf);
        fflush(s_pf);
    }
    return 0;
}

void dbgMsg2FileClose( void )
{
    if( NULL != s_pf ) {
        fclose(s_pf);
        s_pf = NULL;
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static char logcat_file[64+1];

static void * logcatThread( void * param )
{
	char cmd[128];

	snprintf(cmd, 128, "rm %s", logcat_file);
	system(cmd);

	snprintf(cmd, 128, "logcat -v time -f %s", logcat_file);
	system(cmd);

	return NULL;
}

void dbgLogcat2File( const char * filename )
{
	logcat_file[64] = 0;
	strncpy(logcat_file, filename, 64);

	pthread_t      tid;
	pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, logcatThread, NULL);
}
