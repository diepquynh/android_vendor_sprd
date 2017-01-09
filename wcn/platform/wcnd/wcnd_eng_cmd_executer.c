#define LOG_TAG 	"WCND_ENG"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>

#include "wcnd.h"


#define IWNPI_SUBMODULE "iwnpi"
#define BT_SUBMODULE "bt"
#define WIFI_SUBMODULE "wifi"
#define FM_SUBMODULE "fm"

extern int iwnpi_runcommand(int client_fd, int argc, char **argv);
extern int bt_runcommand(int client_fd, int argc, char **argv);
extern int wifi_runcommand(int client_fd, int argc, char **argv);
extern int fm_runcommand(int client_fd, int argc, char **argv);

/**
* pre-define static API.
*/
static int eng_runcommand(int client_fd, int argc, char* argv[]);

/*
* wcn eng cmd executer to executer cmd that relate to eng mode
* such as:
* (1) iwnpi xxx  //iwnpi related cmd
*/
const WcnCmdExecuter wcn_eng_cmdexecuter = {
	.name = "eng",
	.runcommand = eng_runcommand,
};


static int eng_runcommand(int client_fd, int argc, char* argv[])
{
	if(argc < 1)
	{
		wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
		return 0;
	}

#if 1
	int k = 0;
	for (k = 0; k < argc; k++) 
	{
		WCND_LOGD("%s: arg[%d] = '%s'", __FUNCTION__, k, argv[k]);
	}
#endif

	if (!strcmp(argv[0], IWNPI_SUBMODULE)) 
	{
		if(argc < 2)
		{
			wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
			return 0;
		}

		//TODO: call iwnapi run command
		WCND_LOGD("%s: CALL IWNPI CMD = '%s'", __FUNCTION__, argv[1]);
		iwnpi_runcommand(client_fd, argc-1, &argv[1]);
	}
	else if(!strcmp(argv[0], BT_SUBMODULE))
	{
		if(argc < 2)
		{
			wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
			return 0;
		}

		//TODO: call bt run command
		WCND_LOGD("%s: CALL BT CMD = '%s'", __FUNCTION__, argv[1]);
		bt_runcommand(client_fd, argc-1, &argv[1]);
	}
	else if(!strcmp(argv[0], WIFI_SUBMODULE))
	{
		if(argc < 2)
		{
			wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
			return 0;
		}

		//TODO: call wifi run command
		WCND_LOGD("%s: CALL WIFI CMD = '%s'", __FUNCTION__, argv[1]);
		wifi_runcommand(client_fd, argc-1, &argv[1]);
	}
	else if(!strcmp(argv[0], FM_SUBMODULE))
	{
		if(argc < 2)
		{
			wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
			return 0;
		}

		//TODO: call bt run command
		WCND_LOGD("%s: CALL FM CMD = '%s'", __FUNCTION__, argv[1]);
#ifdef FM_ENG_DEBUG
		fm_runcommand(client_fd, argc-1, &argv[1]);
#else
		bt_runcommand(client_fd, argc-1, &argv[1]);
#endif
	}
		
	else
		wcnd_send_back_cmd_result(client_fd, "Not support cmd", 0);


	return 0;
}

