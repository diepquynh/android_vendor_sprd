#ifndef __OPROFILE_DAEMON__H__
#define __OPROFILE_DAEMON__H__
typedef enum
{
   OPROFILE_START = 0,
   OPROFILE_CMD_MAX
}profilecmd;

typedef struct profile_info
{
    profilecmd cmd;
    unsigned long profiletime;
}profileinfo;

#ifdef __cplusplus
extern "C"
{
#endif
    void* oprofile_daemon(void *);
    int start_oprofile(unsigned long time);
#ifdef __cplusplus
}
#endif


#define OPROFILE_SOCKET_PATH           "/data/local/tmp/oprofile/"
#define OPROFILE_SOCKET_NAME           OPROFILE_SOCKET_PATH "socket"
#define OPROFILE_DEBUG_SWITCHER        "debug.oprofile.value"

#endif
