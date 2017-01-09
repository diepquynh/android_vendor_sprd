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

//#define DEBUG


#define max_args (50)
static int gFd = -1;
static int bt_ts_quit = 0;
static pthread_t read_thread = NULL;

static     unsigned char   HCI_SCAN_ENABLE[] = {0x01, 0x1A, 0x0C, 0x01, 0x03};             //   Write Scan Enable Command
static     unsigned char   mode_seEvent[]    = {0x04, 0x0E, 0x04, 0x01, 0x1A, 0x0C};
    
static     unsigned char   HCI_SET_FILER[]   = {0x01, 0x05, 0x0C, 0x03, 0x02, 0x00, 0x02}; //   Set Event Filter Command
static     unsigned char   mode_sfEvent[]    = {0x04, 0x0E, 0x04, 0x01, 0x05, 0x0C};

static     unsigned char   HCI_EUT_ENABLE[]  = {0x01, 0x03, 0x18, 0x00};                   //   Enable Device Under Test Mode Command
static     unsigned char   mode_eeEvent[]    = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x18};
unsigned char HCI_SET_FIXED_FREQUENCY_SEND[]={0x01,0x10,0xfd,0x18,0x00,0x00,0x00,0x00,0x04,0x00,0x0f,0x00,0x33,0x1a,0x3a,0xe2,0x4e,0x7a,0x2c,0xce,0xff,0x01,0x55,0x55,0x00,0x00,0x00,0x00};
unsigned char HCI_SET_SEND_STOP[]={0x01,0x12,0xFD,0x00};
unsigned char HCI_SET_CONTINUE_SEND[]={0x01,0x11,0xfd,0x08,0xff,0x01,0xaa,0xaa,0x00,0x05,0x00,0x0f};
unsigned char HCI_SET_FIXED_FREQUENCY__8809e_SEND[]={0x01,0x10,0xfd,0x14,0x33,0x1A,0x3A,0xE2,0x4E,0x7A,0x2C,0xCE,0x12,0x00,0x00,0x00,0x02,0x04,0x00,0x0f,0x00,0x00,0x00,0x00};
static unsigned char  speed=0x00;
static unsigned char packet_type=0x00;
static unsigned char packet_length_lowbyte=0x00;
static unsigned char packet_length_highbyte=0x00;
static unsigned char frequency=0x00;
static unsigned char powerlevel=0x0f;



#define DH1 "DH1"
#define DH3 "DH3"
#define DH5 "DH5"

#define TWODH1 "2DH1"
#define TWODH3 "2DH3"
#define TWODH5 "2DH5"

#define THREEDH1 "3DH1"
#define THREEDH3 "3DH3"
#define THREEDH5 "3DH5"


#define HIGH  "high"
#define MIDDLE  "middle"
#define LOW  "low"

#define HIGH_FREQUNCY  0x4e
#define MIDDLE_FREQUNCY  0x27
#define LOW_FREQUNCY   0x0
extern unsigned int rdabt_chip_version;
extern int init_uart_fcc_ce(void);


    int Packet_Type_find(char *p)
	{
		int ret=0;
		if(!memcmp(p, DH1, strlen(DH1)))
		{
			speed=0x0;
			packet_type=0x04;
			packet_length_lowbyte=0x1B;
			packet_length_highbyte=0x00;
			#ifdef DEBUG
			printf("equal DH1 \n");
			#endif

		}
		else if(!memcmp(p, DH3, strlen(DH3)))
		{
			speed=0;
			packet_type=0x0b;
			packet_length_lowbyte=0xb7;
			packet_length_highbyte=0x00;
			#ifdef DEBUG
			xx
			printf(" equal DH3 \n");
			#endif

		}
		else if(!memcmp(p, DH5, strlen(DH5)))
		{
			speed=0x0;
			packet_type=0x0f;
			packet_length_lowbyte=0x53;
			packet_length_highbyte=0x01;
			#ifdef DEBUG

			printf(" nequal DH5 \n");
			#endif

		}
		else if(!memcmp(p, TWODH1, strlen(TWODH1)))
		{
			speed=0x01;
			packet_type=0x4;
			packet_length_lowbyte=0x36;
			packet_length_highbyte=0x00;
			#ifdef DEBUG

			printf(" equal 2DH1  \n");
			#endif

		}
		else if(!memcmp(p, TWODH3, strlen(TWODH3)))
		{
			speed=0x01;
			packet_type=0x0a;
			packet_length_lowbyte=0x6f;
			packet_length_highbyte=0x01;
			#ifdef DEBUG
			printf(" equal 2DH3  \n");
			#endif

		}
		else if(!memcmp(p, TWODH5, strlen(TWODH5)))
		{
			speed=0x01;
			packet_type=0x0e;
			packet_length_lowbyte=0xa7;
			packet_length_highbyte=0x02;
			#ifdef DEBUG
			printf(" equal 2DH5 \n");
			#endif

		}
		else if(!memcmp(p, THREEDH1, strlen(THREEDH1)))
		{
			speed=0x02;
			packet_type=0x08;
			packet_length_lowbyte=0x53;
			packet_length_highbyte=0x00;
			#ifdef DEBUG
			printf(" equal 3DH1  \n");
			#endif

		}
		else if(!memcmp(p, THREEDH3, strlen(THREEDH3)))
		{
			speed=0x02;
			packet_type=0x0b;
			packet_length_lowbyte=0x28;
			packet_length_highbyte=0x02;
			#ifdef DEBUG

			printf(" equal 3DH3  \n");
			#endif

		}
		else if(!memcmp(p, THREEDH5, strlen(THREEDH5)))
		{
			speed=0x02;
			packet_type=0x0f;
			packet_length_lowbyte=0xfd;
			packet_length_highbyte=0x03;
			#ifdef DEBUG

			printf(" equal 3DH5  \n");
			#endif

		}
		else
		{
			printf(" illegal packet type,input again \n");

			ret=-1;
		}
		return ret;
	}


int   frequency_find(char *p)
 {
	int ret=0;
	unsigned int fre_tmp=0;
	if(!memcmp(p, HIGH, strlen(HIGH)))
	{
			frequency=HIGH_FREQUNCY;
			#ifdef DEBUG
			printf(" frequency high  \n");
			#endif

	}
	else if(!memcmp(p, MIDDLE, strlen(MIDDLE)))
	{
			frequency=MIDDLE_FREQUNCY;
			#ifdef DEBUG
			printf(" frequency middle  \n");
			#endif

	}
	else if(!memcmp(p, LOW, strlen(LOW)))
	{
			frequency=LOW_FREQUNCY;
			#ifdef DEBUG
			printf(" frequency low  \n");
			#endif

	}
	else        //input detail frequency
	{
		fre_tmp=atoi(p);
		printf(" frequency:  %d\n",fre_tmp);
		fre_tmp=fre_tmp-2402;
		if((fre_tmp<0)||(fre_tmp>78))
		printf(" illegal frequency,input again \n");

	    frequency=fre_tmp;
		ret=-1;

	}
	return ret;
 }


int  find_powerlevel(char *p)
{
	int ret=0;
	unsigned int fre_tmp=0;
	if(!memcmp(p, HIGH, strlen(HIGH)))
	{
			powerlevel=0x0f;
			printf(" powerlevel high  \n");

	}
	else if(!memcmp(p, MIDDLE, strlen(MIDDLE)))
	{
			powerlevel=0x0e;
			printf(" powerlevel middle  \n");

	}
	else if(!memcmp(p, LOW, strlen(LOW)))
	{
			powerlevel=0x0d;
			printf(" powerlevel low  \n");

	}
	else        //input detail frequency
	{
		ret=-1;
		printf(" illegal frequency,input again \n");

	}
	return 0;
 }


void continue_send(void)
{
	printf(" continue_send  \n");

	write(gFd, HCI_SET_CONTINUE_SEND, sizeof(HCI_SET_CONTINUE_SEND));

    sleep(1);

}


void fixed_frequency_8809e_send(void)
{
	#ifdef DEBUG
	printf(" 8809e fixed_frequency_send  \n");
	#endif
	//stop send
	write(gFd, HCI_SET_SEND_STOP, sizeof(HCI_SET_SEND_STOP));
	sleep(1);
	//config send
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[14]=0;         //hop diable
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[15]=frequency;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[17]=packet_type;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[18]=speed;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[19]=powerlevel;       //0xd ~0xf

	HCI_SET_FIXED_FREQUENCY__8809e_SEND[12]=packet_length_lowbyte;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[13]=packet_length_highbyte;
	write(gFd, HCI_SET_FIXED_FREQUENCY__8809e_SEND, sizeof(HCI_SET_FIXED_FREQUENCY__8809e_SEND));

    sleep(1);

}




void fixed_frequency_send(void)
{
	#ifdef DEBUG
	printf(" fixed_frequency_send  \n");
	#endif
	HCI_SET_FIXED_FREQUENCY_SEND[4]=frequency;
	HCI_SET_FIXED_FREQUENCY_SEND[8]=packet_type;
	HCI_SET_FIXED_FREQUENCY_SEND[9]=speed;
	HCI_SET_FIXED_FREQUENCY_SEND[10]=powerlevel;

	HCI_SET_FIXED_FREQUENCY_SEND[26]=packet_length_lowbyte;
	HCI_SET_FIXED_FREQUENCY_SEND[27]=packet_length_highbyte;
	#ifdef DEBUG
    printf(" frequency = %x  \n",frequency);
	printf(" packet_type = %x  \n",packet_type);
	printf(" speed = %x  \n",speed);
	printf(" packet_length_lowbyte = %x  \n",packet_length_lowbyte);
	printf(" packet_length_highbyte = %x  \n",packet_length_highbyte);
	#endif


	//stop send
	write(gFd, HCI_SET_SEND_STOP, sizeof(HCI_SET_SEND_STOP));
	sleep(1);
	//config send

	write(gFd, HCI_SET_FIXED_FREQUENCY_SEND, sizeof(HCI_SET_FIXED_FREQUENCY_SEND));

    sleep(1);

}


void frequency_rule_jump(void)
{
	unsigned int fre=0;
	#ifdef DEBUG
	printf(" frequency_rule_jump  \n");
	#endif
	HCI_SET_FIXED_FREQUENCY_SEND[4]=0;
	HCI_SET_FIXED_FREQUENCY_SEND[8]=0x00;
	HCI_SET_FIXED_FREQUENCY_SEND[9]=0x00;
	HCI_SET_FIXED_FREQUENCY_SEND[10]=powerlevel;

	HCI_SET_FIXED_FREQUENCY_SEND[26]=0x1b;;
	HCI_SET_FIXED_FREQUENCY_SEND[27]=0x00;

	while(1)
	{
		write(gFd, HCI_SET_SEND_STOP, sizeof(HCI_SET_SEND_STOP));
		sleep(1);
		write(gFd, HCI_SET_FIXED_FREQUENCY_SEND, sizeof(HCI_SET_FIXED_FREQUENCY_SEND));

		fre++;
		if(fre > 78)
		fre=0;
		HCI_SET_FIXED_FREQUENCY_SEND[4]=fre;
	}
}

void frequency_norule_jump(void)
{
	unsigned int fre=0;
	#ifdef DEBUG
	printf(" frequency_rule_jump  \n");
	#endif
	//default DH1
	frequency=0;
	speed=0x0;
	packet_type=0x04;
	packet_length_lowbyte=0x1B;
	packet_length_highbyte=0x00;


	HCI_SET_FIXED_FREQUENCY_SEND[4]=frequency;
	HCI_SET_FIXED_FREQUENCY_SEND[8]=packet_type;
	HCI_SET_FIXED_FREQUENCY_SEND[9]=speed;
	HCI_SET_FIXED_FREQUENCY_SEND[10]=powerlevel;

	HCI_SET_FIXED_FREQUENCY_SEND[26]=packet_length_lowbyte;
	HCI_SET_FIXED_FREQUENCY_SEND[27]=packet_length_highbyte;


	while(1)
	{
		write(gFd, HCI_SET_SEND_STOP, sizeof(HCI_SET_SEND_STOP));
		sleep(1);
		write(gFd, HCI_SET_FIXED_FREQUENCY_SEND, sizeof(HCI_SET_FIXED_FREQUENCY_SEND));

		fre=rand()%79;
		HCI_SET_FIXED_FREQUENCY_SEND[4]=fre;
	}
}

void frequency_norule_jump_8809e(void)
{
	unsigned int fre=0;
	#ifdef DEBUG
	printf(" 8809e frequency_norule_jump  \n");
	#endif
	//config send

	frequency=0x0;
	speed=0x0;
	packet_type=0x04;
	packet_length_lowbyte=0x1B;
	packet_length_highbyte=0x00;


	HCI_SET_FIXED_FREQUENCY__8809e_SEND[14]=0;         //hop diable
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[15]=frequency;         //from 0...78...0...78
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[17]=packet_type;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[18]=speed;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[19]=powerlevel;       //0xd ~0xf

	HCI_SET_FIXED_FREQUENCY__8809e_SEND[12]=packet_length_lowbyte;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[13]=packet_length_highbyte;

    #if 1
	while(1)
	{
		write(gFd, HCI_SET_SEND_STOP, sizeof(HCI_SET_SEND_STOP));
		sleep(1);
		write(gFd, HCI_SET_FIXED_FREQUENCY__8809e_SEND, sizeof(HCI_SET_FIXED_FREQUENCY__8809e_SEND));
		sleep(1);
		fre=rand()%79;
		HCI_SET_FIXED_FREQUENCY__8809e_SEND[15]=fre;
	}
	#endif
}


void frequency_rule_jump_8809e(void)
{
	unsigned int fre=0;
	#ifdef DEBUG
	printf(" 8809e frequency_rule_jump  \n");
	#endif
	//default DH1
	frequency=79;
	speed=0x0;
	packet_type=0x04;
	packet_length_lowbyte=0x1B;
	packet_length_highbyte=0x00;

	write(gFd, HCI_SET_SEND_STOP, sizeof(HCI_SET_SEND_STOP));
	sleep(1);
	//config send
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[14]=1;         //hop enable
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[15]=frequency;        //jump from 0...78...0...78
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[17]=packet_type;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[18]=speed;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[19]=powerlevel;       //0xd ~0xf

	HCI_SET_FIXED_FREQUENCY__8809e_SEND[12]=packet_length_lowbyte;
	HCI_SET_FIXED_FREQUENCY__8809e_SEND[13]=packet_length_highbyte;
	write(gFd, HCI_SET_FIXED_FREQUENCY__8809e_SEND, sizeof(HCI_SET_FIXED_FREQUENCY__8809e_SEND));
}

int rule_jump(int argc, char *argv[] )
{
	if(rdabt_chip_version == WLAN_VERSION_91 || rdabt_chip_version == WLAN_VERSION_91_E)
			frequency_rule_jump_8809e();
			else if(rdabt_chip_version == WLAN_VERSION_90_D|| rdabt_chip_version == WLAN_VERSION_90_E || rdabt_chip_version==RDABT_5876 )
			frequency_rule_jump();

	return 0;
}

int norule_jump(int argc, char *argv[] )
{
		if(rdabt_chip_version == WLAN_VERSION_91 || rdabt_chip_version == WLAN_VERSION_91_E)
			frequency_norule_jump_8809e();
			else if(rdabt_chip_version == WLAN_VERSION_90_D|| rdabt_chip_version == WLAN_VERSION_90_E || rdabt_chip_version==RDABT_5876 )
			frequency_norule_jump();
    return 0;
}


int frequency_single_hopping(int argc, char *argv[] )
{
	if(Packet_Type_find(argv[0])== 0)
	{
		if(frequency_find(argv[1])== 0)
		{

			if(rdabt_chip_version == WLAN_VERSION_91 || rdabt_chip_version == WLAN_VERSION_91_E)
			fixed_frequency_8809e_send();
			else if(rdabt_chip_version == WLAN_VERSION_90_D|| rdabt_chip_version == WLAN_VERSION_90_E || rdabt_chip_version==RDABT_5876 )
			fixed_frequency_send();
		}
	}
	#ifdef DEBUG
    printf("%s \n", __func__);
	#endif

    return 0;
}


int set_power_level(int argc, char *argv[] )
{

		if(find_powerlevel(argv[1])== 0)
		{
				#ifdef DEBUG
			    printf("set_power_level sucess \n");
				#endif

		}

   

    return 0;
}

int Rdabt_test_cmd_quit(int argc, char *argv[] )
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
	int (*handler)(int argc, char *argv[] );
} bt_test_commands[]= {
    {"f_test",Rdabt_test_cmd_ftest},
	{"DH1",frequency_single_hopping},
    {"DH3",frequency_single_hopping},
	{"DH5",frequency_single_hopping},
	{"2DH1",frequency_single_hopping},
	{"2DH3",frequency_single_hopping},
	{"2DH5",frequency_single_hopping},
	{"3DH1",frequency_single_hopping},
	{"3DH3",frequency_single_hopping},
	{"3DH5",frequency_single_hopping},
    {"rule_hop",rule_jump},
    {"norule_hop",norule_jump},
    {"power",set_power_level},

    {"quit",Rdabt_test_cmd_quit},
};


static void *event_thread(void *param)
{
    int fd = *((int*)param), rLen = 0;
    unsigned char data_buffer[4096 + 1],print_buffer[4096 +1];
    data_buffer[4096]  = '\0';
    print_buffer[4096] = '\0';

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

    printf("enter fcc ce mode,please wait for a moment ,initializing... ... \n");

    //power on
    set_bluetooth_power(1);

    sleep(2);

    //init serial
    #if 1
	gFd = init_uart_fcc_ce();
	#else
	gFd = init_uart();
	#endif

    if(gFd < 0)
    {
        printf("open device failed \n");

        goto out;
    }

    err = pthread_create(&read_thread, 0, event_thread, &gFd);
  
    if(err < 0)
        goto out;
	sleep(2);
	printf("initials finished ,please input command \n");

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




		#if 1
        if (argc)
        {
            unsigned char handled = 0;
            
            for(i = 0; i < sizeof(bt_test_commands)/sizeof(struct bt_test_cmd); i ++ )
            {
                if(!memcmp(argv[0], bt_test_commands[i].cmd, strlen(bt_test_commands[i].cmd)))
                {
                    bt_test_commands[i].handler(argc - 1, argv);
                    
                    handled = 1;
                }
            }
        }
		#endif
    } while(!bt_ts_quit);

out:
    printf("bt fcc ce quit\n");

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




