#define LOG_TAG "AUDIO_TESTER"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <cutils/properties.h>
#include <cutils/log.h>
#include "audiotester_server.h"

#define  SERVER_PORT 9997 //  define the defualt connect port id
#define  CLIENT_PORT ((20001+rand()))  //  define the defualt client port as a random port

static int gfd;
uint8_t cmd_buf[65536];
void  usage(char *name)
{
    printf( " usage: %s IpAddr\n " , name);
}


#define MAX_SOCKT_LEN 65535

static char g_send_buf[MAX_SOCKT_LEN];
static char g_receive_buf[MAX_SOCKT_LEN];

int g_diag_seq = 0;
int cur_len = 0;


void send_error(const char *error)
{

    int index = 0;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;
    *(g_send_buf + index) = 0x7e;
    index += 1;
    m_head.seq_num = g_diag_seq++;
    m_head.type = 0x99;//spec
    m_head.subtype = 1;
    if (error != NULL && strcmp(error, "null") != 0) {
        m_head.len = sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) + strlen(error);
    } else {
        m_head.len = sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T);
    }
    printf("m_head.len=%d\n", m_head.len);
    memcpy(g_send_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    data_head.data_state = DATA_STATUS_ERROR;
    memcpy(g_send_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);
    if (error != NULL && strcmp(error, "null") != 0) {
        strcpy(g_send_buf + index, error);
        index += strlen(error);
    }
    *(g_send_buf + index) = 0x7e;
    index += 1;

    write(gfd, g_send_buf, index);

    //save the response
    FILE *fp;
    if ((fp = fopen("data/result.txt", "rw+")) == NULL) {
        printf("can not open the data/result.txt\n");
        return;
    }
    int len = read(gfd, g_receive_buf, MAX_SOCKT_LEN);
    cur_len = len - 30;
    printf("%s\n", g_receive_buf + 29);
    printf("len =%d\n", len);
    fwrite("123", 3, 1, fp);
    fwrite(g_receive_buf, len, 1, fp);
    fclose(fp);

}

void read_response()
{
    int len = read(gfd, g_receive_buf, MAX_SOCKT_LEN);
    //printf("%s\n", g_receive_buf + 29);
    //printf("len =%d\n", len);
}

void send_command(int sub_cmd, int data_state)
{
    //cur_len = 64000;

    int index = 0;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;
    *(g_send_buf + index) = 0x7e;
    index += 1;
    m_head.seq_num = g_diag_seq++;
    m_head.type = 0x99;//spec
    m_head.subtype = sub_cmd;
    m_head.len = sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) + cur_len;
    printf("m_head.len=%d\n", m_head.len);
    memcpy(g_send_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    data_head.data_state = DATA_STATUS_ERROR;
    memcpy(g_send_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);

    //copy data
    memcpy(g_send_buf + index, g_receive_buf + 29, cur_len);
    index += cur_len;

    *(g_send_buf + index) = 0x7e;
    index += 1;

    int len = write(gfd, g_send_buf, index);
    printf("zzj   len = %d   index=%d\n", len, index);

}

void send_fake_cmd(const char *cmd)
{
    FILE *fp;
    int len;
    if ((fp = fopen(cmd, "r")) == NULL) {
        ALOGE("can not open the cmd file\n");
        return;
    }

    ALOGE("send fake cmd");
    len = fread(cmd_buf, 1, 65536, fp);
    send(gfd, cmd_buf, len, 0);
}


int  main(int argc, char **argv)
{
    int  servfd, clifd, length = 0;
    struct  sockaddr_in servaddr, cliaddr;
    socklen_t socklen  =   sizeof (servaddr);
    char value[PROPERTY_VALUE_MAX];

    if (argc < 2 ) {
        usage(argv[ 0 ]);
        exit( 1 );
    }

    if ((clifd  =  socket(AF_INET, SOCK_STREAM, 0 ))  <   0 ) {
        printf( " create socket error!\n " );
        exit( 1 );
    }

    srand(time(NULL)); // initialize random generator

    bzero( & cliaddr, sizeof (cliaddr));
    cliaddr.sin_family  =  AF_INET;
    cliaddr.sin_port  =  htons(CLIENT_PORT);
    cliaddr.sin_addr.s_addr  =  htons(INADDR_ANY);

    bzero( & servaddr, sizeof (servaddr));
    servaddr.sin_family  =  AF_INET;
    inet_aton(argv[ 1 ], & servaddr.sin_addr);
    servaddr.sin_port  =  htons(SERVER_PORT);
    // servaddr.sin_addr.s_addr = htons(INADDR_ANY);

    if  (bind(clifd, (struct sockaddr * ) &cliaddr, sizeof (cliaddr)) < 0 ) {
        printf( " bind to port %d failure!\n " , CLIENT_PORT);
        exit( 1 );
    }

    if (connect(clifd, ( struct  sockaddr * ) & servaddr, socklen)  <   0 ) {
        printf( " can't connect to %s!\n ", argv[ 1 ]);
        exit( 1 );
    }

    gfd = clifd;
    printf("connected\n");
    char lastcmd[30];
    int curval = 0;
    while(1) {
        usleep(1000000);
        property_get("audiotester.cmd", value, "0");
        if (strcmp(lastcmd, value)) {
            printf("send the cmd = %s\n", value);
            send_error(NULL);
            send_command(2, 1);
            memcpy(lastcmd, value, strlen(value) + 1);
            read_response();
            read_response();
            read_response();
            read_response();
        }
    }

    close(clifd);
    return 0;
}
