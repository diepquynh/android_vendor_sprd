/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
//#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
// GMS push need fts to calculate file size ->
#include <fts.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
// GMS push <-
#include <cutils/properties.h>
#include "slog.h"
char external_stor[MAX_NAME_LEN];

int send_socket(int sockfd, void* buffer, int size)
{
        int result = -1;
        int ioffset = 0;
        while(sockfd > 0 && ioffset < size) {
                result = send(sockfd, (char *)buffer + ioffset, size - ioffset, MSG_NOSIGNAL);
                if (result > 0) {
                        ioffset += result;
                } else {
                        break;
                }
        }
        return result;

}

int recv_socket(int sockfd, void* buffer, int size)
{
        int received = 0, result;
        while(buffer && (received < size))
        {
                result = recv(sockfd, (char *)buffer + received, size - received, MSG_NOSIGNAL);
                if (result > 0) {
                        received += result;
                } else {
                        received = result;
                        break;
                }
        }
        return received;
}

char *parse_string(char *src, char c, char *token)
{
	char *results;
	results = strchr(src, c);
	if(results == NULL) {
		return NULL;
	}
	*results++ = 0;
	while(results[0]== c)
		*results++ = 0;
	return results;
}

void update_5_entries(const char *keyword, const char *status, char *line)
{
	char *name, *pos3, *pos4, *pos5;
	char buffer[MAX_NAME_LEN];

	/* sanity check */
	if(line == NULL) {
		printf("type is null!");
		return;
	}

	strcpy(buffer, line);
	/* fetch each field */
	if((name = parse_string(buffer, '\t', "name")) == NULL) return;
	if((pos3 = parse_string(name, '\t', "pos3")) == NULL) return;
	if((pos4 = parse_string(pos3, '\t', "pos4")) == NULL) return;
	if((pos5 = parse_string(pos4, '\t', "pos5")) == NULL) return;

	if ( !strncmp("android", keyword, 7) ) {
		if ( !strncmp("main", name, 4) || !strncmp("system", name, 6) || !strncmp("radio", name, 5)
		|| !strncmp("events", name, 6) || !strncmp("kernel", name, 6) )
			sprintf(line, "%s\t%s\t%s\t%s\t%s", "stream", name, status, pos4, pos5);
	} else if  ( !strncmp("modem", keyword, 5) ) {
		if  ( !strncmp("modem", name, 5) )
			sprintf(line, "%s\t%s\t%s\t%s\t%s", "stream", name, status, pos4, pos5);
	} else if  ( !strncmp("tcp", keyword, 3) ) {
		if  ( !strncmp("tcp", name, 3) )
			sprintf(line, "%s\t%s\t%s\t%s\t%s", "stream", name, status, pos4, pos5);
	} else if  ( !strncmp("bt", keyword, 2) ) {
		if  ( !strncmp("bt", name, 2) )
			sprintf(line, "%s\t%s\t%s\t%s\t%s", "stream", name, status, pos4, pos5);
	}
}

void update_conf(const char *keyword, const char *status)
{
	FILE *fp;
	int len = 0;
	char buffer[MAX_LINE_LEN *2], line[MAX_NAME_LEN];

	fp = fopen(TMP_SLOG_CONFIG, "r");
	if(fp == NULL) {
		perror("open conf failed!\n");
		return;
	}

#ifdef LOW_POWER_MODE
	if (!strncmp("enable", keyword, 6) || !strncmp("disable", keyword, 7) || !strncmp("low_power", keyword, 8)) {
#else
	if (!strncmp("enable", keyword, 6) || !strncmp("disable", keyword, 7)) {
#endif
		while (fgets(line, MAX_NAME_LEN, fp) != NULL) {
#ifdef LOW_POWER_MODE
			if(!strncmp("enable", line, 6) || !strncmp("disable", line, 7) || !strncmp("low_power", line, 8)) {
#else
			if(!strncmp("enable", line, 6) || !strncmp("disable", line, 7)) {
#endif
				sprintf(line, "%s\n",  keyword);
			}
			len += sprintf(buffer + len, "%s", line);
		}
	} else if ( !strncmp("android", keyword, 6) || !strncmp("modem", keyword, 5) || !strncmp("bt", keyword, 2) || !strncmp("tcp", keyword, 3)) {
		while (fgets(line, MAX_NAME_LEN, fp) != NULL) {
			if (!strncmp("stream", line, 6)) {
				update_5_entries(keyword, status, line);
			}

			len += sprintf(buffer + len, "%s", line);
		}
	}
	fclose(fp);
	fp = fopen(TMP_SLOG_CONFIG, "w");
	if(fp == NULL) {
		perror("open conf failed!\n");
		return;
	}

	fprintf(fp, "%s", buffer);
	fclose(fp);
	return;
}

void usage(const char *name)
{
	printf("Usage: %s <operation> [arguments]\n", name);
	printf("Operation:\n"
               "\tenable             update config file and enable slog\n"
               "\tdisable            update config file and disable slog\n"
               "\tlow_power          update config file and make slog in low_power state\n"
               "\tandroid [on/off]   update config file and enable/disable android log\n"
               "\tmodem [on/off]     update config file and enable/disable modem log\n"
               "\ttcp [on/off]       update config file and enable/disable cap log\n"
               "\tbt  [on/off]       update config file and enable/disable bluetooth log\n"
               "\treload             reboot slog and parse config file.\n"
               "\tsnap [arg]         catch certain snapshot log, catch all snapshot without any arg\n"
               "\texec <arg>         through the slogctl to run a command.\n"
               "\ton                 start slog.\n"
               "\toff                pause slog.\n"
               "\tclear              delete all log.\n"
               "\tdump [file]        dump all log to a tar file.\n"
               "\tscreen [file]      screen shot, if no file given, will be put into misc dir\n"
               "\tsync               sync current android log to file.\n"
               "\thook_modem         dump current modem log to /data/log\n"
               "\tquery              print the current slog configuration.\n");
	return;
}

static char* init_external_stor()
{
	char *p;
	int type;
	char value[PROPERTY_VALUE_MAX];

	p = getenv("SECOND_STORAGE_TYPE");
	if(p){
		type = atoi(p);
		p = NULL;
		if(type == 0 || type == 1){
			p = getenv("EXTERNAL_STORAGE");
		} else if(type == 2) {
			p = getenv("SECONDARY_STORAGE");
		}

		if(p){
			sprintf(external_stor, "%s/slog", p);
			err_log("the external storage 1: %s", external_stor);
			return external_stor;
		} else {
			err_log("SECOND_STORAGE_TYPE is %d, but can't find the external storage environment", type);
			exit(0);
		}

	}

	property_get("persist.storage.type", value, "3");
	type = atoi(value);
	if( type == 0 || type == 1 || type == 2) {
		p = NULL;
		if(type == 0 || type == 1){
			p = getenv("EXTERNAL_STORAGE");
		} else if(type == 2) {
			p = getenv("PHYSICAL_STORAGE");
		}

		if(p){
			sprintf(external_stor, "%s/slog", p);
			err_log("the external storage 2: %s", external_stor);
			return external_stor;
		} else {
			err_log("SECOND_STORAGE_TYPE is %d, but can't find the external storage environment", type);
			exit(0);
		}
	}

	p = getenv("SECONDARY_STORAGE");
	if(p == NULL)
		p = getenv("EXTERNAL_STORAGE");
	if(p == NULL){
		err_log("Can't find the external storage environment");
		exit(0);
	}
	sprintf(external_stor, "%s/slog", p);
	err_log("the external storage 3: %s", external_stor);
	return external_stor;
}


int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct slog_cmd cmd;
	struct sockaddr_un address;
	struct timeval tv_out;
    /* GMS push arguments -> */
	struct statfs gmsDiskInfo;
	long gmsAvailableSize;
	int ftsOptions;
	FTSENT *p_ftsent;
	DIR* gms_dir_ptr;
	FTS* fts;
	long blocksize;
	long gmsBlocks;
	int gmsEval;
	const char *gmsPath[2];
    /* GMS push arguments <- */

	/*
	arguments list:
	enable		update config file and enable slog
	disable		update config file and disable slog
        low_power	update config file and make slog in low_power state
	reload		CTRL_CMD_TYPE_RELOAD,
	snap $some	CTRL_CMD_TYPE_SNAP,
	snap 		CTRL_CMD_TYPE_SNAP_ALL,
	exec $some	CTRL_CMD_TYPE_EXEC,
	on		CTRL_CMD_TYPE_ON,
	off		CTRL_CMD_TYPE_OFF,
	query		CTRL_CMD_TYPE_QUERY,
	clear		CTRL_CMD_TYPE_CLEAR,
	dump		CTRL_CMD_TYPE_DUMP,
	screen		CTRL_CMD_TYPE_SCREEN,
	hook_modem      CTRL_CMD_TYPE_HOOK_MODEM,
	*/
	if(argc < 2) {
		usage(argv[0]);
		return 0;
	}

	memset(&cmd, 0, sizeof(cmd));

	if(!strncmp(argv[1], "reload", 6)) {
		cmd.type = CTRL_CMD_TYPE_RELOAD;
	} else if(!strncmp(argv[1], "snap", 4)) {
		if(argc == 2) 
			cmd.type = CTRL_CMD_TYPE_SNAP_ALL;
		else {
			cmd.type = CTRL_CMD_TYPE_SNAP;
			snprintf(cmd.content, MAX_NAME_LEN, "%s", argv[2]);
		}
	} else if(!strncmp(argv[1], "exec", 4)) {
		if(argc == 2)  {
			usage(argv[0]);
			return 0;
		} else {
			cmd.type = CTRL_CMD_TYPE_EXEC;
			snprintf(cmd.content, MAX_NAME_LEN, "%s", argv[2]);
		}
	} else if(!strncmp(argv[1], "gms", 3)) {
	    printf("GMS push will start\nChecking filesystem\n");

		// ensure the dir exist
		if (gms_dir_ptr = (DIR*)opendir("/sdcard/gms/") == NULL) {
			printf("/sdcard/gms/ doesn't exsist or open failed.\n");
			return -1;
		}
		closedir(gms_dir_ptr);

		// Calculate system partition available size
		if (statfs("/system", &gmsDiskInfo) < 0) {
			printf("Get /system usage failed. Operation abandon.\n");
			return -1;
		}
		gmsAvailableSize = gmsDiskInfo.f_bavail * gmsDiskInfo.f_bsize / 1024;

        printf("System partition size: %ld\n", gmsAvailableSize);

		// Calculate gms package size
		ftsOptions = FTS_PHYSICAL;
		gmsPath[0] = "/sdcard/gms";
		gmsPath[1] = NULL;
		if ((fts = fts_open(__UNCONST(gmsPath), FTS_PHYSICAL, NULL)) == NULL) {
			printf("open /sdcard/gms/ failed. Please ensure it exist and can be read.\n");
			return -1;
		}
		for (gmsEval = 0; (p_ftsent = fts_read(fts)) != NULL; ) {
			p_ftsent->fts_parent->fts_number += 
			    p_ftsent->fts_number += p_ftsent->fts_statp->st_blocks;
			/*
			 * If listing each directory, or not listing files
			 * or directories and this is post-order of the
			 * root of a traversal, display the total.
			 */
			if (p_ftsent->fts_level <= 0 || (!p_ftsent->fts_level))
				gmsBlocks = p_ftsent->fts_number / 2;
		}
		fts_close(fts);

		printf("GMS package size: %ld\n\n", gmsBlocks);
        printf("Filesystem checking done.\n");

		if (gmsBlocks > gmsAvailableSize) {
			if (argc == 2) {
				printf("We found available size is not enough\n");
				printf("If still you want to push them, use:\n");
				printf("slogctl gms force\n");
				return -1;
			} else if (argc == 3) {
			    if (strncmp(argv[2], "force", 5)) {
			        printf("Invalid intput.\n");
			        return -1;
			    }
			} else {
			    return -1;
			}
		}
		cmd.type = CTRL_CMD_TYPE_GMS;
	} else if(!strncmp(argv[1], "on", 2)) {
		cmd.type = CTRL_CMD_TYPE_ON;
	} else if(!strncmp(argv[1], "off", 3)) {
		cmd.type = CTRL_CMD_TYPE_OFF;
	} else if(!strncmp(argv[1], "clear", 5)) {
		cmd.type = CTRL_CMD_TYPE_CLEAR;
	} else if(!strncmp(argv[1], "dump", 4)) {
		cmd.type = CTRL_CMD_TYPE_DUMP;
		if(argc == 2)
			snprintf(cmd.content, MAX_NAME_LEN, "%s", DEFAULT_DUMP_FILE_NAME);
		else
			snprintf(cmd.content, MAX_NAME_LEN, "%s.tgz", argv[2]);
	} else if(!strncmp(argv[1], "screen", 6)) {
		cmd.type = CTRL_CMD_TYPE_SCREEN;
		if(argc == 3)
			snprintf(cmd.content, MAX_NAME_LEN, "%s", argv[2]);
	} else if(!strncmp(argv[1], "query", 5)) {
		cmd.type = CTRL_CMD_TYPE_QUERY;
	} else if(!strncmp(argv[1], "sync", 4)) {
		cmd.type = CTRL_CMD_TYPE_SYNC;
	} else if(!strncmp(argv[1], "javacrash", 9)) {
		cmd.type = CTRL_CMD_TYPE_JAVACRASH;
#ifdef LOW_POWER_MODE
	} else if(!strncmp(argv[1], "hook_modem", 10)) {
		cmd.type = CTRL_CMD_TYPE_HOOK_MODEM;
	} else if(!strncmp(argv[1], "low_power", 8)) {
		update_conf("low_power", NULL);
                property_set("debug.slog.enabled", "1");
		cmd.type = CTRL_CMD_TYPE_RELOAD;
#endif
	} else if(!strncmp(argv[1], "enable", 6)) {
		update_conf("enable", NULL);
                property_set("debug.slog.enabled", "1");
		cmd.type = CTRL_CMD_TYPE_RELOAD;
	} else if(!strncmp(argv[1], "disable", 7)) {
		update_conf("disable", NULL);
                property_set("debug.slog.enabled", "0");
		cmd.type = CTRL_CMD_TYPE_RELOAD;
               // The daemon will be destroyed, do not send any commands
                return 0;
	} else if(!strncmp(argv[1], "android", 7)) {
		if(argc == 3 && ( strncmp(argv[2], "on", 2) == 0 || strncmp(argv[2], "off", 3) == 0 )) {
			update_conf("android", argv[2]);
			cmd.type = CTRL_CMD_TYPE_RELOAD;
		} else {
			usage(argv[0]);
			return -1;
		}
	} else if(!strncmp(argv[1], "modem", 5)) {
		if(argc == 3 && ( strncmp(argv[2], "on", 2) == 0 || strncmp(argv[2], "off", 3) == 0 )) {
			update_conf("modem", argv[2]);
			cmd.type = CTRL_CMD_TYPE_RELOAD;
		} else {
			usage(argv[0]);
			return -1;
		}
	} else if(!strncmp(argv[1], "tcp", 3)) {
		if(argc == 3 && ( strncmp(argv[2], "on", 2) == 0 || strncmp(argv[2], "off", 3) == 0 )) {
			update_conf("tcp", argv[2]);
			cmd.type = CTRL_CMD_TYPE_RELOAD;
		} else {
			usage(argv[0]);
			return -1;
		}
	} else if(!strncmp(argv[1], "bt", 2)) {
		if(argc == 3 && ( strncmp(argv[2], "on", 2) == 0 || strncmp(argv[2], "off", 3) == 0 )) {
			update_conf("bt", argv[2]);
			cmd.type = CTRL_CMD_TYPE_RELOAD;
		} else {
			if(argc == 3 && (strncmp(argv[2], "false", 5) == 0)) {
				cmd.type = CTRL_CMD_TYPE_BT_FALSE;
			}else{
				usage(argv[0]);
				return -1;
			}
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	/* init unix domain socket */
	memset(&address, 0, sizeof(address));
	address.sun_family=AF_UNIX; 
	strcpy(address.sun_path, SLOG_SOCKET_FILE);

 	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) {
		perror("create socket failed");
		return -1;
	}
	ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
	if (ret < 0) {
		perror("connect failed,use another way");
		printf("internal storage,%s,\n", INTERNAL_LOG_PATH);
		printf("external storage,%s,\n", init_external_stor());
		return -1;
	}
	ret = send_socket(sockfd, (void *)&cmd, sizeof(cmd));
        if (ret < 0) {
		perror("send failed");
		return -1;
	}

	tv_out.tv_sec = 3600;
	tv_out.tv_usec = 0;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	if (ret < 0) {
		perror("setsockopt failed");
	}
	ret = recv_socket(sockfd, (void *)&cmd, sizeof(cmd));
        if (ret < 0) {
		perror("recv failed");
		return -1;
	}
	if(!strncmp(cmd.content,"FAIL", 4)){
		printf("slogctl cmd fail \n");
		return -1;
	}
	printf("%s\n", cmd.content);

	close(sockfd);
	return 0;
}
