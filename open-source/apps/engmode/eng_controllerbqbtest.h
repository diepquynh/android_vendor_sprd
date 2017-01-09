#ifndef ENG_CONTROLLERBQBTEST
#define ENG_CONTROLLERBQBTEST

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */



#define MAX_PORTS 4
#define DEBUG 1


struct PortInfo{
	int busy;
	char name[32];
	int handle;
};

int eng_controller_bqb_start(void);
void eng_send_data(char * data, int data_len);
int eng_controller_bqb_stop(void);

#endif
