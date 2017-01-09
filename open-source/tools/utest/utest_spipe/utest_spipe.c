/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ERR(x...) fprintf(stderr, x)
#define INFO(x...) fprintf(stdout, x)

#define MS_IN_SEC 1000
#define NS_IN_MS  1000000

#define DEFAULT_DATA_SIZE (10 * 1024 * 1024)
#define BYTES_LIMIT (8 * 1024)
#define BYTES_DEFAULT 1000

typedef enum{
    CMD_VERIFY_ID = 0,
    CMD_THROUGHPUT_ID = 1,
    CMD_LATENCY_ID = 2,
    CMD_MAX,
}cmd_id;

typedef struct{
    cmd_id ID;
    char* name;
    char* optcfg;
    int (*func)(char*, int, int);
}cmd_tag;

/*32 bytes*/
static char pattern[] = {
            0x01, 0x10, 0x11,
            0x23, 0x32, 0x33,
            0x45, 0x54, 0x55,
            0x67, 0x76, 0x77,
            0x89, 0x98, 0x99,
            0xab, 0xba, 0xbb,
            0xcd, 0xdc, 0xdd,
            0xef, 0xfe, 0xff,
            0xa1, 0xb2, 0xc3,
            0xd4, 0xe5, 0xf6,
            0xaa, 0xee,
};


static int verify(char* dev_path, int bytes, int count);
static int throughput(char* dev_path, int bytes, int count);
static int latency(char* dev_path, int bytes, int count);

cmd_tag cmd_container[CMD_MAX] = {
                {CMD_VERIFY_ID, "verify", ":d:b:t:", verify},
                {CMD_THROUGHPUT_ID, "throughput", ":d:b:t:", throughput},
                {CMD_LATENCY_ID, "latency", ":d:b:t:", latency},
};

/*
 * usage --  print the usage of utest_spipe
 * @return: none
 */
static void usage(void)
{
    INFO("Usage:\n");
    INFO("    utest_spipe verify | [-d dev] | [-b unit_size] | [-t total_size]\n");
    INFO("    utest_spipe throughput | [-d dev] | [-b unit_size] | [-t total_size]\n");
    INFO("    utest_spipe latency | [-d dev] | [-b unit_size] | [-t total_size]\n");
}

/*
 * clean_spipe --  clean the pipe to ensure no residual data existing
 * @dev_path: path of device node
 * @return: error value, 0 means successful
 */
static int clean_spipe(char* dev_path)
{
    int fd;
    int ret_val;
    char buf[128];
    if(-1 == (fd = open(dev_path, O_RDONLY | O_NONBLOCK))){
        return -EINVAL;
    }
    do{
        ret_val = read(fd, buf, 128);
    }while((0 < ret_val) || (ret_val == -1 && errno == EINTR));
    if(-1 == ret_val){
        strerror(errno);
        return -EINVAL;
    }

    close(fd);
    return 0;
}

/*
* delta_miliseconds --  calculate the delta value of time
 * @begin: begin time, first input variable
 * @end:   end time, second input variable
 * @return: miliseconds
 */
static unsigned int delta_miliseconds(struct timespec *begin, struct timespec *end)
{
    int carrier;
    long ns;
    unsigned int ms;
    time_t sec;

    if(NULL == begin || NULL == end){
        return 0;
    }

    ns = end->tv_nsec - begin->tv_nsec;

    sec = end->tv_sec - begin->tv_sec;

    return sec * MS_IN_SEC + ns / NS_IN_MS;
}

/*
 * buf_init --  initialize rxbuf & txbuf, and clean spipe
 * @rxbuf: pointer to receive buff
 * @rxbuf_len: length of receive buff
 * @txbuf: pointer to transmit buff
 * @rxbuf: length of transmit buff
 * @return: error value, 0 means successful
 */
static int buf_init(char **rxbuf, unsigned int rxbuf_len, char **txbuf, unsigned int txbuf_len)
{
    if(rxbuf_len <= 0 || txbuf_len <= 0){
        ERR("buf_init: invalid input parameters\n");
        return -EINVAL;
    }

    /*initialize txbuf*/
    *txbuf = malloc(txbuf_len);
    if(NULL == *txbuf){
        ERR("buf_init: allocate memory failed");
        return -ENOMEM;
    }
    memset(*txbuf, 0, txbuf_len);

    /*initialize rxbuf*/
    *rxbuf = malloc(rxbuf_len);
    if(NULL == *rxbuf){
        free(*txbuf);
        ERR("buf_init: allocate memory failed");
        return -ENOMEM;
    }
    memset(*rxbuf, 0, rxbuf_len);

    return 0;
}


/*
 * verify --  operation on spipe0, and print the result
 * @dev_path: path of device node, first input variable
 * @bytes: size of data, second input variable
 * @count: operation counts, third input variable
 * @return: error value, 0 means successful
 */
static int verify(char *dev_path, int bytes, int count)
{
    struct timespec tm_begin, tm_end;
    char *txbuf, *rxbuf, *temp_ptr;
    int i, j, fd, rest, num, ret_val;
    unsigned int mask, ms;

    if(bytes < 0 || count < 0){
        ERR("verify: invalid input parameters\n");
        return -EINVAL;
    }

    mask = sizeof(pattern) - 1;

    /*initialize txbuf & rxbuf*/
    if(0 != (ret_val = buf_init(&rxbuf, bytes + 1, &txbuf, bytes + 1))){
        return ret_val;
    }

    /*clean spipe0*/
    clean_spipe(dev_path);

    /*open spipe0*/
    fd = open(dev_path, O_RDWR);
    if(-1 == fd){
        free(txbuf);
        free(rxbuf);
        ERR("spipe_verify: open spipe failed\n");
        return -EINVAL;
    }

    for(i = 0; i < count; i++){
        for(j = 0; j < bytes; j++){
            txbuf[j] = pattern[(i * bytes + j) & mask];
        }

        /*throuput to spipe0*/
        rest = bytes;
        temp_ptr = txbuf;
        do{
            num = write(fd, temp_ptr, rest);
            if(num != -1){
                rest -= num;
                temp_ptr += num;
            }
        }while((num == - 1 && errno == EINTR) || rest != 0);
        if(num == -1){
            goto FAIL;
        }

        /*loopback from spipe0*/
        rest = bytes;
        temp_ptr = rxbuf;
        do{
            num = read(fd, temp_ptr, rest);
            if(num != -1){
                rest -= num;
                temp_ptr += num;
            }
        }while((num == - 1 && errno == EINTR) || rest != 0);
        if(num == -1){
            goto FAIL;
        }

        if(0 == strcmp(rxbuf, txbuf)){
            continue;
        }else{
            INFO("Round %d: wrong data!\n", i);
            INFO("rxbuf : %s\n", rxbuf);
            INFO("txbuf : %s\n", txbuf);
            goto FAIL;
        }
    }

    INFO("Correct data!\n");
    INFO("\tTotal data size %.3f MB, count %d, unit size %d Bytes.\n", ((float)(count*bytes)/1024/1024), count, bytes);
    ret_val = 0;
    goto SUCCESS;

FAIL :
    /*ret_val = -EINVAL;*/
SUCCESS :
    free(txbuf);
    free(rxbuf);
    close(fd);
    return ret_val;
}

/*
 * throughput --  operation on spipe0, and print the result
 * @dev_path: path of device node, first input variable
 * @bytes: size of data, second input variable
 * @count: operation counts, third input variable
 * @return: error value, 0 means successful
 */
static int throughput(char *dev_path, int bytes, int count)
{
    struct timespec tm_begin, tm_end;
    char *txbuf, *rxbuf, *temp_ptr;
    int i, j, fd, rest, num, ret_val;
    unsigned int mask, ms;

    if(bytes < 0 || count < 0){
        ERR("throughput: invalid input parameters\n");
        return -EINVAL;
    }

    mask = sizeof(pattern) - 1;

    /*initialize txbuf & rxbuf*/
    if(0 != (ret_val = buf_init(&rxbuf, bytes + 1, &txbuf, bytes + 1))){
        return ret_val;
    }
    mask = sizeof(pattern) - 1;
    for(j = 0; j < bytes; j++){
        txbuf[j] = pattern[j & mask];
    }

    /*clean spipe0*/
    clean_spipe(dev_path);
    /*open spipe0*/
    fd = open(dev_path, O_RDWR);
    if(-1 == fd){
        free(txbuf);
        free(rxbuf);
        ERR("throughput: open spipe failed\n");
        return -EINVAL;
    }

    /*timer start*/
    if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_begin)){
            goto FAIL;
    }

    for(i = 0; i < count; i++){
        /*throuput to spipe0*/
        rest = bytes;
        temp_ptr = txbuf;
        do{
            num = write(fd, temp_ptr, rest);
            if(num != -1){
                rest -= num;
                temp_ptr += num;
            }
        }while((num == - 1 && errno == EINTR) || rest != 0);
        if(num == -1){
            goto FAIL;
        }
        /*loopback from spipe0*/
        rest = bytes;
        temp_ptr = rxbuf;
        do{
            num = read(fd, temp_ptr, rest);
            if(num != -1){
                rest -= num;
                temp_ptr += num;
            }
        }while((num == - 1 && errno == EINTR) || rest != 0);
        if(num == -1){
            goto FAIL;
        }
    }

    /*timer end*/
    if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_end)){
        goto FAIL;
    }

    /*calculate interval*/
    ms = delta_miliseconds(&tm_begin, &tm_end);

    /*print result*/
    INFO("Throughput finished\n\tTotal data size %.3f MB, total time cost %u ms, unit size %d Bytes, speed %.3f MB/s.\n",
        ((float)(count*bytes)/1024/1024), ms, bytes, (float)(count * bytes)*MS_IN_SEC/1024/1024/ms);
    ret_val = 0;
    goto SUCCESS;

FAIL :
    ret_val = -EINVAL;
SUCCESS :
    free(txbuf);
    free(rxbuf);
    close(fd);
    return ret_val;
}

/*
 * latency --  operation on spipe0, and print the result
 * @dev_path: path of device node, first input variable
 * @bytes: size of data, second input variable
 * @count: operation counts, third input variable
 * @return: error value, 0 means successful
 */
static int latency(char *dev_path, int bytes, int count)
{
    struct timespec tm_begin, tm_end;
    char *txbuf, *rxbuf, *temp_ptr;
    int i, j, fd, rest, num, ret_val;
    unsigned int mask, ms;

    if(bytes < 0 || count < 0){
        ERR("latency: invalid input parameters\n");
        return -EINVAL;
    }

    mask = sizeof(pattern) - 1;

    /*initialize txbuf & rxbuf*/
    if(0 != (ret_val = buf_init(&rxbuf, bytes + 1, &txbuf, bytes + 1))){
        return ret_val;
    }
    mask = sizeof(pattern) - 1;
    for(j = 0; j < bytes; j++){
        txbuf[j] = pattern[j & mask];
    }

    /*clean spipe0*/
    clean_spipe(dev_path);
    /*open spipe0*/
    fd = open(dev_path, O_RDWR);
    if(-1 == fd){
        free(txbuf);
        free(rxbuf);
        ERR("latency: open spipe failed\n");
        return -EINVAL;
    }

    /*timer start*/
    if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_begin)){
            goto FAIL;
    }

    for(i = 0; i < count; i++){
        /*throuput to spipe0*/
        rest = bytes;
        temp_ptr = txbuf;
        do{
            num = write(fd, temp_ptr, rest);
            if(num != -1){
                rest -= num;
                temp_ptr += num;
            }
        }while((num == - 1 && errno == EINTR) || rest != 0);
        if(num == -1){
            goto FAIL;
        }
        /*loopback from spipe0*/
        rest = bytes;
        temp_ptr = rxbuf;
        do{
            num = read(fd, temp_ptr, rest);
            if(num != -1){
                rest -= num;
                temp_ptr += num;
            }
        }while((num == - 1 && errno == EINTR) || rest != 0);
        if(num == -1){
            goto FAIL;
        }
    }

    /*timer end*/
    if(-1 == clock_gettime(CLOCK_MONOTONIC, &tm_end)){
        goto FAIL;
    }

    /*calculate interval*/
    ms = delta_miliseconds(&tm_begin, &tm_end);

    /*print result*/
    INFO("Latency finished\n\tTotal count %d, total time cost %u ms, unit size %d Bytes, average time cost %.3f ms per unit.\n", 
         count, ms, bytes, (float)ms/count);
    ret_val = 0;
    goto SUCCESS;

FAIL :
    ret_val = -EINVAL;
SUCCESS :
    free(txbuf);
    free(rxbuf);
    close(fd);
    return ret_val;
}

/*
 * do_parse_cmd --  parse command line
 * @id: command id, first input variable
 * @argc: string number, second input variable
 * @argv: string array, third input variable
 * @return: error value, 0 means successful
 */
static int do_parse_cmd(cmd_id id, int argc, char ** argv)
{
    int opt;
    int bytes = BYTES_DEFAULT;
    int data_size = DEFAULT_DATA_SIZE;
    int count;
    char *dev_path = "/dev/spipe_td0";
    char *optcfg = cmd_container[id].optcfg;

    while ((opt = getopt(argc, argv, optcfg)) != -1){
        switch(opt){
            case 'd':
                dev_path = optarg;
                break;
            case 'b':
                bytes = atoi(optarg);
                break;
            case 't':
                data_size = atoi(optarg);
                break;
            default:
                return -EINVAL;
        }
    }

    if(optind < argc){
        return -EINVAL;
    }

    if(0 >= bytes || BYTES_LIMIT <= bytes){
        bytes = BYTES_DEFAULT;
    }
    count = data_size / bytes;

    return cmd_container[id].func(dev_path, bytes, count);
}

int main(int argc, char ** argv)
{
    char *cmd;
    int ret = -EINVAL;
    int i;

    if(argc < 2){
        usage();
        return ret;
    }

    cmd = argv[1];
    argc--;
    argv++;

    for(i = CMD_VERIFY_ID; i < CMD_MAX; i++){
        if(0 == strcmp(cmd_container[i].name, cmd)){
            ret = do_parse_cmd(i, argc, argv);
            break;
        }
    }

    if(0 != ret){
        usage();
    }

    return 0;
}
