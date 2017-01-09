#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <pthread.h>

#include "bt_cfg.h"
#include "bt_rda.h"



#define max_args (10)
static int gFd = -1;
static int bt_ts_quit = 0;
static pthread_t read_thread = NULL;

static     unsigned char   HCI_SCAN_ENABLE[] = {0x01, 0x1A, 0x0C, 0x01, 0x03};             //   Write Scan Enable Command     
static     unsigned char   mode_seEvent[]    = {0x04, 0x0E, 0x04, 0x01, 0x1A, 0x0C};
    
static     unsigned char   HCI_SET_FILER[]   = {0x01, 0x05, 0x0C, 0x03, 0x02, 0x00, 0x02}; //   Set Event Filter Command  
static     unsigned char   mode_sfEvent[]    = {0x04, 0x0E, 0x04, 0x01, 0x05, 0x0C};

static     unsigned char   HCI_EUT_ENABLE[]  = {0x01, 0x03, 0x18, 0x00};                   //   Enable Device Under Test Mode Command  
static     unsigned char   mode_eeEvent[]    = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x18};



int Rdabt_test_cmd_quit(int argc, char *argv[])
{
    bt_ts_quit = 1;
    
    printf("%s \n", __func__);

    return 0;
}

int Rdabt_test_cmd_ftest(int argc, char *argv[])
{
    write(gFd, HCI_SCAN_ENABLE, sizeof(HCI_SCAN_ENABLE));

    sleep(2);

    write(gFd, HCI_SET_FILER, sizeof(HCI_SET_FILER));

    sleep(2);

    write(gFd, HCI_EUT_ENABLE, sizeof(HCI_EUT_ENABLE));

    sleep(2);
    
    printf("%s \n", __func__);

    return 0;
}


static const struct bt_test_cmd {
	const char *cmd;
	int (*handler)(int argc, char *argv[]);
} bt_test_commands[]= {
    {"f_test",Rdabt_test_cmd_ftest},
    {"quit",Rdabt_test_cmd_quit},
};


static void *event_thread(void *param)
{
    int fd = *((int*)param), rLen = 0;
    unsigned char data_buffer[4096 + 1];
    data_buffer[4096]  = '\0';

    if(fd < 0)
        goto out;
        
    printf("enter %s \n", __func__);

    while(1)
    {
        rLen = read(fd, data_buffer, 4096);
        
        data_buffer[rLen] = '\0';
        
        printf("recev: %s \n", data_buffer);
    }
    
out:
    printf("leave from %s \n", __func__);
    
    return NULL;
}

int main(int argC, char *argV[])
{
    int  argc = 0, i = 0, err;
    char cmdbuf[256], *cmd, *argv[max_args], *pos;
    char uncmdbuf[256];

    printf("f_test to enter eut mode. \n");

    //power on
    set_bluetooth_power(1);

    sleep(5);

    //init serial
    gFd = init_uart();
    
    if(gFd < 0)
    {
        printf("open device failed \n");
        
        goto out;
    }

    err = pthread_create(&read_thread, 0, event_thread, &gFd);
    
    if(err < 0)
        goto out;

    do
    {
        printf("> ");
        
		cmd = fgets(cmdbuf, sizeof(cmdbuf), stdin);

        if (cmd == NULL)
			break;

        memcpy(uncmdbuf, cmdbuf, sizeof(cmdbuf));
        
		pos = cmd;
		
		while (*pos != '\0') 
		{
			if (*pos == '\n')
			{
				*pos = '\0';
				break;
			}
			pos++;
		}
		argc = 0;
		pos  = cmd;
		
		for (;;) 
		{
			while (*pos == ' ')
				pos++;
				
			if (*pos == '\0')
				break;
				
			argv[argc] = pos;
			argc++;
			
			if (argc == max_args)
				break;
				
			if (*pos == '"') 
			{
				char *pos2 = strrchr(pos, '"');
				if (pos2)
					pos = pos2 + 1;
			}
			
			while (*pos != '\0' && *pos != ' ')
				pos++;
			if (*pos == ' ')
				*pos++ = '\0';
		}

        if (argc)
        {
            unsigned char handled = 0;
            
            for(i = 0; i < sizeof(bt_test_commands)/sizeof(struct bt_test_cmd); i ++ )
            {
                if(!memcmp(argv[0], bt_test_commands[i].cmd, strlen(bt_test_commands[i].cmd)))
                {
                    bt_test_commands[i].handler(argc - 1, &argv[1]);
                    
                    handled = 1;
                }
            }
            
            if(!handled)
            {
                write(gFd, HCI_SCAN_ENABLE, sizeof(HCI_SCAN_ENABLE));

                sleep(2);

                write(gFd, HCI_SET_FILER, sizeof(HCI_SET_FILER));

                sleep(2);

                write(gFd, HCI_EUT_ENABLE, sizeof(HCI_EUT_ENABLE));

                sleep(2);
            }
            
        }
    } while(!bt_ts_quit);

out:
    printf("Rdabt_test end\n");

    if(read_thread)
    {
        pthread_exit(read_thread);
        
        read_thread = 0;
    }
    
    if(gFd > 0)
    {
        close(gFd);
        
        gFd = -1;
    }

    set_bluetooth_power(0);
    
    return 0;
}




