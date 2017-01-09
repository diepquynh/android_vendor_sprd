/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <cutils/str_parms.h>
#include <sys/eventfd.h>
#include <assert.h>


#include "slog.h"

#define BT_CONF_PATH "/data/misc/bluedroid/bt_stack.conf"
#define BT_LOG_STATUS "BtSnoopLogOutput="
#define BT_LOG_PATH "BtSnoopFileName="

typedef uint8_t BOOLEAN;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef LOG_TAG
#define LOG_TAG "SLOG"
#endif

//#define BT_SNOOP_V
#define BT_SNOOP_D


#ifdef BT_SNOOP_V
#define HCIV(param, ...) ALOGD("%s " param, __FUNCTION__, ## __VA_ARGS__)
#else
#define HCIV(param, ...) {}
#endif

#ifdef BT_SNOOP_D
#define HCID(param, ...) ALOGD("%s " param, __FUNCTION__, ## __VA_ARGS__)
#else
#define HCID(param, ...) {}
#endif

#define HCII(param, ...) ALOGI("%s " param, __FUNCTION__, ## __VA_ARGS__)
#define HCIE(param, ...) ALOGE("%s " param, __FUNCTION__, ## __VA_ARGS__)

#define BTSNOOP_SOCKET "bt_snoop_slog_socket"
#define HCI_EDR3_DH5_PACKET_SIZE    1021
#define BTSNOOP_HEAD_SIZE  25
#define HCI_PIIL_SVAE_BUF_SIZE 5
#define HCI_POLL_SIZE (HCI_EDR3_DH5_PACKET_SIZE + BTSNOOP_HEAD_SIZE + HCI_PIIL_SVAE_BUF_SIZE)

#define HCI_FILE_SIZE_MAX (10 * 1024 * 1024)

#define UNUSED(x) (void)(x)

#define INVALID_FD (-1)
#define EFD_SEMAPHORE (1 << 0)


typedef enum {
    BTLOG_ON,
    BTLOG_OFF,
} LOG_CTRL_T;

static int server_socket = -1;
static int file_fd = -1;

static pthread_t thread_id_t;

static pthread_t recv_thread;
static pthread_t pop_thread;

pthread_mutex_t lock;

typedef struct {
    uint8_t buf[HCI_POLL_SIZE];
    uint8_t data[HCI_POLL_SIZE * 2];
    uint32_t len;
    BOOLEAN completion;
} HCI_POOL_T;

typedef struct list_node_t{
    struct list_node_t *next;
    void *data;
    uint32_t len;
    uint64_t timestamp;
}list_node_t;

typedef struct {
    list_node_t *head;
    list_node_t *tail;
    uint32_t count;
    uint64_t size;
} list_t;

static list_t hcidata_queue = {
    .head = NULL,
    .tail = NULL,
    .count = 0,
    .size = 0
};

static HCI_POOL_T hci_pool;

static uint64_t hci_log_size = 0;

static int semaphore_fd = INVALID_FD;

static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

BOOLEAN list_append_node(list_t *list, list_node_t *node);
static list_node_t *list_pop_node(list_t *list);
static inline void semaphore_new(void);
static inline void semaphore_free(void);
static inline void semaphore_wait(void);
static inline void semaphore_post(void);

static uint64_t btsnoop_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Timestamp is in microseconds.
    uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
    timestamp += tv.tv_usec;
    return timestamp;
}

void operate_bt_status(char *status, char* path)
{
    FILE *fp;
    char line[MAX_NAME_LEN], buffer[MAX_LINE_LEN];
    int len = 0;

    HCID(": %s", status);

    fp = fopen(BT_CONF_PATH, "r+");
    if(fp == NULL) {
        HCIE("open %s failed!", BT_CONF_PATH);
        return;
    }

    memset(buffer, 0, MAX_LINE_LEN);
    while (fgets(line, MAX_NAME_LEN, fp)) {

    if(!strncmp(BT_LOG_STATUS, line, strlen(BT_LOG_STATUS))) {
        sprintf(line, "%s%s\n", BT_LOG_STATUS, status);
    }

    if(!strncmp(BT_LOG_PATH, line, strlen(BT_LOG_PATH))) {
        if(path != NULL)
            sprintf(line, "%s%s\n", BT_LOG_PATH, path);
    }

    len += sprintf(buffer + len, "%s", line);
    }

    fclose(fp);

    fp = fopen(BT_CONF_PATH, "w");
    if(fp == NULL) {
        HCIE("open %s failed!", BT_CONF_PATH);
        return;
    }

    fprintf(fp, "%s", buffer);
    fclose(fp);
    return;
}

static inline void hci_log_enable(BOOLEAN enable) {
    if (enable == TRUE) {
        HCID("enable");
        operate_bt_status("true", BTSNOOP_SOCKET);
    } else if (enable == FALSE) {
        HCID("disable");
        operate_bt_status("false", NULL);
    }
}

static inline int create_server_socket(const char* name)
{
    int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(s < 0) {
       return -1;
    }
    HCID("name: %s", name);
    if (socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >= 0) {
        if (listen(s, 5) == 0) {
            HCID("listen socket: %s, fd: %d", name, s);
            return s;
        } else {
            HCIE("listen socket: %s, fd: %d failed, %s (%d)", name, s, strerror(errno), errno);
        }
    } else {
        HCIE("failed: %s fd: %d, %s (%d)", name, s, strerror(errno),errno);
    }
    close(s);
    return -1;
}

static inline int accept_server_socket(int sfd)
{
    struct sockaddr_un remote;
    int fd;
    socklen_t len = sizeof(struct sockaddr_un);

    if ((fd = accept(sfd, (struct sockaddr *)&remote, &len)) == -1) {
         HCIE("sock accept failed (%s)", strerror(errno));
         return -1;
    }
    return fd;
}

static int create_log_file(void) {
    int ret, fd;
    char dir_name[256] = {0};
    char file_name[256] = {0};
    time_t now;
    struct tm *timenow;
    sprintf(dir_name, "%s/%s/%s", current_log_path, top_logdir, "bt");
    ret = mkdir(dir_name, S_IRWXU | S_IRWXG | S_IRWXO);
    if (-1 == ret && (errno != EEXIST)){
        HCIE("mkdir %s failed,%s", dir_name,strerror(errno));
        exit(0);
    }
    time(&now);
    timenow = localtime(&now);
    sprintf(file_name, "%s/btsnoop_%04d-%02d-%02d_%02d-%02d-%02d.log", dir_name,
       timenow->tm_year+1900,  timenow->tm_mon+1, timenow->tm_mday,
        timenow->tm_hour, timenow->tm_min, timenow->tm_sec);
    fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    if(fd < 0) {
        HCIE("open file %s failed,%s", file_name,strerror(errno));
        exit(0);
    }
    HCID("open file %s ok", file_name);
    if (write(fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16) < 0) {
        HCIE("write btsnoop head error,%s",strerror(errno));
        close(fd);
        exit(0);
    }
    hci_log_size = 0;
    return fd;
}

static LOG_CTRL_T bt_log_ctrl_state(void) {
    struct slog_info *info = NULL, *bt = NULL;
    info = stream_log_head;
    while(info){
        if( (info->state == SLOG_STATE_ON) && !strncmp(info->name, "bt", 2) ){
            bt = info;
            break;
        }
        info = info->next;
    }
    if(!bt) {
        return BTLOG_OFF;
    }

    if( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)) ) {
        bt->state = SLOG_STATE_OFF;
        return BTLOG_OFF;
    }
    return BTLOG_ON;
}

static void dump_hex(unsigned char *src, int len) {
    int buf_size = len * 5 +1;
    char *mem = malloc(buf_size);
    memset(mem, 0, buf_size);
    char temp[6] = {0};
    int i;
    for (i = 0; i < len; i++) {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "0x%02X ", src[i]);
        strcat(mem, temp);
    }
    ALOGD("%s", mem);
    free(mem);
    mem = NULL;
}

static int parse_and_save(void) {
    int ret, type;
    uint32_t length_a, length_b;

    ret = recv(server_socket, hci_pool.buf, sizeof(hci_pool.buf), MSG_NOSIGNAL);
    if (ret <= 0) {
        HCIE("recv error: %d", ret);
        semaphore_post();
        return -1;
    }

#ifdef BT_SNOOP_V
    dump_hex(hci_pool.buf, ret);
#endif

    if ((uint32_t)ret > (uint32_t)sizeof(hci_pool.data) - hci_pool.len) {
        HCIE("out of memory");
        exit(0);
    } else {
        memcpy(hci_pool.data + hci_pool.len, hci_pool.buf, ret);
        hci_pool.len += ret;
    }

    do {
        if (hci_pool.len < BTSNOOP_HEAD_SIZE) {
            HCIV("not reach head size");
            break;
        }
        length_a = ((uint32_t)(hci_pool.data[0] & 0xFF)) << 24
                 | ((uint32_t)(hci_pool.data[1] & 0xFF)) << 16
                 | ((uint32_t)(hci_pool.data[2] & 0xFF)) << 8
                 | ((uint32_t)(hci_pool.data[3] & 0xFF)) << 0;
        length_b = ((uint32_t)(hci_pool.data[4] & 0xFF)) << 24
                 | ((uint32_t)(hci_pool.data[5] & 0xFF)) << 16
                 | ((uint32_t)(hci_pool.data[6] & 0xFF)) << 8
                 | ((uint32_t)(hci_pool.data[7] & 0xFF)) << 0;
        type = hci_pool.data[24];
        if (length_a == length_b) {
            HCIV("length: %d", length_a);
            if (length_a > (uint32_t)sizeof(hci_pool.data)) {
                HCIE("unknow head");
                return -1;
            }
            if (hci_pool.len < length_a + BTSNOOP_HEAD_SIZE - 1) {
                HCIV("incomplete");
                break;
            }
            switch (type) {
                case 1:
                    if (length_a == (uint32_t)hci_pool.data[27] + 4) {
                        hci_pool.completion = 1;
                        HCIV("COMMAND");
                    }
                    break;
                case 2:
                    if (length_a == (uint32_t)hci_pool.data[27] + 5 + (((uint32_t)hci_pool.data[28]) << 8)) {
                        hci_pool.completion = 1;
                        HCIV("ACL");
                    }
                    break;
                case 3:
                    if (length_a == (uint32_t)hci_pool.data[27] + 4) {
                        hci_pool.completion = 1;
                        HCIV("SCO");
                    }
                    break;
                case 4:
                    if (length_a == (uint32_t)hci_pool.data[26] + 3) {
                        hci_pool.completion = 1;
                        HCIV("EVENT");
                    }
                    break;
                default:
                    HCIE("unknow type: %d", type);
                break;
            }
        }
        if (hci_pool.completion) {
            list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t) + (length_a + BTSNOOP_HEAD_SIZE -1));
            node->data = (uint8_t*)(node +1);
            memcpy(node->data, hci_pool.data, (length_a + BTSNOOP_HEAD_SIZE -1));
            node->next = NULL;
            node->len = (length_a + BTSNOOP_HEAD_SIZE -1);
            node->timestamp = ((uint64_t)(hci_pool.data[16] & 0xFF)) << 56
                            | ((uint64_t)(hci_pool.data[17] & 0xFF)) << 48
                            | ((uint64_t)(hci_pool.data[18] & 0xFF)) << 40
                            | ((uint64_t)(hci_pool.data[19] & 0xFF)) << 32
                            | ((uint64_t)(hci_pool.data[20] & 0xFF)) << 24
                            | ((uint64_t)(hci_pool.data[21] & 0xFF)) << 16
                            | ((uint64_t)(hci_pool.data[22] & 0xFF)) << 8
                            | ((uint64_t)(hci_pool.data[23] & 0xFF)) << 0;
            pthread_mutex_lock(&lock);
            list_append_node(&hcidata_queue, node);
            pthread_mutex_unlock(&lock);
            semaphore_post();
            hci_pool.len -= (length_a + BTSNOOP_HEAD_SIZE -1);
            HCIV("len: %d, save: %d", hci_pool.len, (length_a + BTSNOOP_HEAD_SIZE -1));
            if (hci_pool.len) {
                memcpy(hci_pool.data, &hci_pool.data[length_a + BTSNOOP_HEAD_SIZE -1], hci_pool.len);
            }
            HCIV("copy complete");
            hci_pool.completion = 0;
            continue;
        } else {
            break;
        }
    } while(1);
    return 0;
}

static void btsnoop_save_thread(void* arg) {
    UNUSED(arg);
    if (file_fd != -1) {
        close(file_fd);
    }
    file_fd = create_log_file();
    memset(&hci_pool, 0, sizeof(hci_pool));
    while (parse_and_save() == 0);
}

static inline void semaphore_new(void) {
    if (semaphore_fd != INVALID_FD) {
        close(semaphore_fd);
    }
    semaphore_fd = eventfd(0, EFD_SEMAPHORE);
    if (semaphore_fd == INVALID_FD) {
        HCIE("error");
    }
}

static inline void semaphore_free(void) {
    if (semaphore_fd != INVALID_FD)
        close(semaphore_fd);
}

static inline void semaphore_wait(void) {
    uint64_t value;
    assert(semaphore_fd != INVALID_FD);
    if (eventfd_read(semaphore_fd, &value) == -1) {
        HCIE("%s unable to wait on semaphore: %s", __func__, strerror(errno));
    }
}

static inline void semaphore_post(void) {
    assert(semaphore_fd != INVALID_FD);
    if (eventfd_write(semaphore_fd, 1ULL) == -1) {
        HCIE("%s unable to post to semaphore: %s", __func__, strerror(errno));
    }
}

BOOLEAN list_append_node(list_t *list, list_node_t *node) {
    assert(list != NULL);
    assert(data != NULL);

    if (!node) {
        HCIE("malloc failed");
        return FALSE;
    }

    if (list->tail == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->size += node->len;
    ++list->count;
    return TRUE;
}

static list_node_t *list_pop_node(list_t *list) {
    assert(list != NULL);

    list_node_t *node = NULL;

    if (list->head != NULL) {
        node = list->head;
        if (list->head == list->tail) {
            list->tail = list->head->next;
        }
        list->head = list->head->next;
        --list->count;
        list->size -= node->len;
    } else {
        HCIE("not found list head");
    }
    return node;
}



static void btsnoop_recv_thread(void* arg) {
    UNUSED(arg);
    memset(&hci_pool, 0, sizeof(hci_pool));
    while (parse_and_save() == 0);
}

static void btsnoop_pop_thread(void* arg) {
    int ret;
    uint64_t write_time, done_time;
    UNUSED(arg);
    if (file_fd != -1) {
        close(file_fd);
    }
    file_fd = create_log_file();
    uint64_t file_size = 0;
    do {
        semaphore_wait();
        if (!hcidata_queue.count) {
            HCID("hcidata_queue.count: %d", (int)hcidata_queue.count);
        }
        pthread_mutex_lock(&lock);
        list_node_t *node = list_pop_node(&hcidata_queue);
        pthread_mutex_unlock(&lock);
        if (node == NULL) {
            HCIE("pop failed");
            return;
        }

        write_time = btsnoop_timestamp();
        ret = write(file_fd, node->data, node->len);
        done_time = btsnoop_timestamp();

        if ((done_time - write_time) > (100 * 1000LL)) {
            ALOGD("write block: (%llu: %llu) us", (done_time - write_time), (done_time - (node->timestamp - BTSNOOP_EPOCH_DELTA)));
        }

        if (ret <= 0) {
            HCIE("write failed: %d", ret);
            free(node);
            node = NULL;
            continue;
        }

        file_size += node->len;
        if (file_size > HCI_FILE_SIZE_MAX) {
            close(file_fd);
            file_fd = create_log_file();
            file_size = 0;
        }

        free(node);
        node = NULL;
    } while (1);
}

static inline void btsnoop_thread_start(void) {
    HCID("start");
    pthread_mutex_init(&lock, NULL);
    semaphore_new();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&recv_thread, &attr,
                       (void*)btsnoop_recv_thread, NULL)!=0) {
      ALOGE("create error");
    }
    if (pthread_create(&pop_thread, &attr,
                       (void*)btsnoop_pop_thread, NULL)!=0) {
      ALOGE("create error");
    }
    HCIV("out");
}

static inline void btsnoop_thread_stop(void) {
    HCID();
}


void* bt_snoop_start(void *arg) {
    int socket_f, socket_s, file_fd;
    LOG_CTRL_T log_ctrl;

    UNUSED(arg);

    log_ctrl = bt_log_ctrl_state();
    if (BTLOG_ON == log_ctrl) {
        hci_log_enable(FALSE);
        hci_log_enable(TRUE);
    } else if (BTLOG_OFF == log_ctrl) {
        hci_log_enable(FALSE);
        return NULL;
    }

    socket_f = create_server_socket(BTSNOOP_SOCKET);
    if (socket_f < 0) {
        exit(0);
    }

    do {
        HCID("waiting for connection");
        socket_s = accept_server_socket(socket_f);
        if (socket_s < 0) {
            continue;
        }
        HCID("accept for connection");
        if (server_socket != -1) {
            HCID("close server_socket");
            close(server_socket);
        }
        server_socket = socket_s;
        btsnoop_thread_start();
    } while (1);
}

void bt_snoop_stop(void) {
    hci_log_enable(FALSE);
}
