#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <time.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#ifdef ANDROID
#include <hardware_legacy/power.h>
#endif

#undef ANDROID_SYNC
#undef ANDROID_WRITE_LOCK
#undef HAVE_INOTIFY
#define HAVE_INOTIFY    1
#define lprogram_name "vhub"
#define log_out_fileno stderr // STDERR_FILENO
/*
 * 0 --- pts
 * 1 --- pipe是阻塞读,阻塞写,可以安全使用cat, echo和dd等
 */
#define local_ipc_method 0
// vhub -p android -p rild -p kernel -d /dev -o luther.logs. -s 10m -m 5
//
// then you can see a symbol link under /dev, like these
//
// /dev/vhub.android
// /dev/vhub.rild
// /dev/vhub.kernel
//
#if HAVE_INOTIFY
#include <sys/inotify.h>
#endif
struct vhubc {
    int  fd;
    char pts[64];
    char vname[64];
    int vname_len;
};

struct filter {
    struct timeval tv;
    int timeout;
    const char *list;
    const char *exec;
    __u32 flags;
    int opts;
    int opts2;
    struct filter *next;
};

struct vhub {
    struct vhubc *vhc;
    char lpath[1024];
    int  vhc_max;
    struct filter *ft;
    struct filter *observer;
#if HAVE_INOTIFY
    int fd_inotify;
#endif
    int use_ft;
    int use_observer;
};

static struct vhub vhb;
static void unlink_ipc(void);
static void setup_signals();
static void create_pts(int i);
static int save_data(char *buf, int count);

#define TCMD_ROOT_USER_RLIM_SET     0
#if TCMD_ROOT_USER_RLIM_SET
#define TCMD_CLIENT_MAX             (4096/*RLIM_INFINITY*/)
#else
#ifdef ANDROID
#define TCMD_CLIENT_MAX             (16)//(1024) // Application default maximum number of open fd is 1024
#else
#define TCMD_CLIENT_MAX             (128)//(1024) // Application default maximum number of open fd is 1024
#endif
#endif
#define TCMD_COMMON_UNIX_PATH       "/tmp/luther.vhub.tcmd"

struct tcmd {
    struct pollfd pfds[TCMD_CLIENT_MAX];
    time_t stime[TCMD_CLIENT_MAX]; // client start time
    time_t etime[TCMD_CLIENT_MAX]; // client quit time
    char *buf;
    int buf_size;
    int len;
    int fd_terminal;
    int fd_listen;
    int fd_net;
    char fd_out_name[1024];
    int fd_out;
    int fd_out_data_max;
    int fd_out_data_pointer;
    int fd_out_data_segment;
    int fd_out_data_segment_max;
};
static struct tcmd tcmd;

static int vhub_open_new(char *name, int index, int max, int *fd, int flags)
{
    char tmp[2048];
    int oflags = 0;
    index %= max;
    sprintf(tmp, "%s%d", name, index);
    if (*fd) close(*fd);
#ifdef ANDROID_SYNC
    oflags = O_SYNC;
#endif
    *fd = open(tmp, O_CREAT | O_RDWR | flags | oflags, 0777);
    return 0;
}

static int vhub_init_unix(char *path, int queue)
{
    int fd;
    struct sockaddr_un addr;

    if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        printf("socket: %s\n", strerror(errno));
        return -1;
    }

    memset(&addr, '\0', sizeof (addr));
    addr.sun_family = AF_UNIX;

    addr.sun_path[0] = '\0';
    strncpy(&addr.sun_path[1], path, strlen(path)+1);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("bind: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (listen(fd, queue) < 0) {
        printf("listen: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static inline int pfd_set(int fd)
{
    int ifd = fd;

    tcmd.pfds[ifd].fd = fd;
    tcmd.pfds[ifd].events = POLLIN;

    time(&tcmd.stime[ifd]);

    return ifd;
}

static inline int pfd_get(int ifd)
{
    return tcmd.pfds[ifd].fd;
}

static inline void pfd_clr(int ifd)
{
    tcmd.pfds[ifd].fd = 0;
    tcmd.pfds[ifd].events = 0;
}

static inline int pfd_isset(int ifd)
{
    return tcmd.pfds[ifd].revents;
}

#if HAVE_INOTIFY
static void tcmd_inotfiy_file_observer_handler(int ifd, int fd)
{
    struct inotify_event *event;
    int res;
    int event_pos = 0;
    char event_buf[4096];

    res = read(fd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) {
        if(errno == EINTR)
            return;
        fprintf(stderr, "could not get event, %s\n", strerror(errno));
        return; // 1;
    }

    while(res >= (int)sizeof(*event)) {
        int event_size;
        struct filter *observer = vhb.observer;
        event = (struct inotify_event *)(event_buf + event_pos);

        printf("<vhub observer> event : 0x%08x \"%s\"\n", event->mask, event->len ? event->name : "");

        while (observer) {
            if (observer->opts == event->wd) {
                int subdir;
                if (observer->opts2 & 0x80000000) {
                    if (event->len && !strcmp(observer->list, event->name))
                        subdir = 0;
                    else subdir = 1;
                } else subdir = 0;
                if (subdir == 0) {
                    if (observer->opts2 & ~0x80000000) {
                        if (observer->flags == event->mask)
                            break;
                    } else if (observer->flags & event->mask) break;
                }
            }
            observer = observer->next;
        }

        if (observer) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            if (tv.tv_sec > (observer->tv.tv_sec + observer->timeout)) {
                printf("<vhub observer> %s timeout=%d\n", observer->list, observer->timeout);
                if (fork() == 0) {
                    execl(observer->exec, observer->list, "VHUB OBSERVER", NULL);
                    exit(0);
                }
                observer->tv = tv;
            } else printf("<vhub observer> %s left=%lds\n", observer->list, observer->tv.tv_sec  + observer->timeout - tv.tv_sec);
        } else fprintf(stderr, "<vhub observer> not find observer\n");

        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
}
#endif

static void tcmd_terminal_handler(int ifd, int fd)
{
    tcmd.len = read(fd, tcmd.buf, tcmd.buf_size);
    save_data(tcmd.buf, tcmd.len);
}

static void tcmd_pts_handler(int ifd, int fd)
{
    // struct iovec iov[2];
    int index = fd - vhb.vhc[0].fd;

    if ((tcmd.len = read(fd, tcmd.buf, tcmd.buf_size)) <= 0) {
        close(fd);
        pfd_clr(ifd);
#if (local_ipc_method == 0)
        create_pts(index);
#endif
        return;
    }
#if 0
    iov[0].iov_base = vhb.vhc[index].vname;
    iov[0].iov_len = vhb.vhc[index].vname_len;
    iov[1].iov_base = tcmd.buf;
    iov[1].iov_len = tcmd.len;

    writev(tcmd.fd_out, iov, 2);
#endif
    // save_data(vhb.vhc[index].vname, vhb.vhc[index].vname_len);
    save_data(tcmd.buf, tcmd.len);
}

static void tcmd_listen_handler(int ifd, int fd)
{
    // char name[64];
    int fd_client;
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;

    if ((fd_client = accept(fd, &addr, &addrlen)) < 0)
        return;

    ifd = pfd_set(fd_client);
    // snprintf(name, sizeof name, "%d", fd_client);
    // tcmd_client_name_set(ifd, name, &tcmd.list_name);
}

static void tcmd_client_handler(int ifd, int fd)
{
    char *data;

    if ((tcmd.len = read(fd, tcmd.buf, tcmd.buf_size)) <= 0) {
        time(&tcmd.etime[ifd]);
        close(fd);
        pfd_clr(ifd);
#if TCMD_DEBUG
        printf("[%s][%ld][%s -- %s]\n",
                tcmd.name[ifd],
                tcmd.etime[ifd] - tcmd.stime[ifd],
                strtok(ctime(&tcmd.stime[ifd]), "\n"),
                strtok(ctime(&tcmd.etime[ifd]), "\n")
              );
#endif
#if 0
        if (!tcmd_client_name_free(ifd, &tcmd.list_net)) {
            tcmd_client_status_change_notifier("remove", ifd);
            tcmd_client_name_free(ifd, &tcmd.list_name);
        }
#endif
        return;
    }
    save_data(tcmd.buf, tcmd.len);
#if 0
    tcmd.buf[tcmd.len] = 0;

    if (strncmp(tcmd.buf, "data", 4))
        return do_command(ifd, fd, tcmd.buf, tcmd.len);

    data = tcmd.buf + 4;

    if (tcmd.push_log_channels & TCMD_PUSH_LOG2_TERMINAL)
        printf("<%s> %s", tcmd.name[ifd], data);
    if (tcmd.push_log_channels & TCMD_PUSH_LOG2_NET)
        send2net(tcmd.name[ifd], data, strlen(data));
#endif
}

static int init_net_ipc(void)
{
    tcmd.fd_listen = vhub_init_unix(TCMD_COMMON_UNIX_PATH, 30);
    if (tcmd.fd_listen > 0) {
        pfd_set(tcmd.fd_listen);
        return 0;
    }
    return tcmd.fd_listen;
}

#ifdef ANDROID
/* Construct a string according to a format. Caller frees. */
char *
format(const char *format, ...)
{
	va_list ap;
	char *ptr = NULL;

	va_start(ap, format);
	if (vasprintf(&ptr, format, ap) == -1) {
		perror("Out of memory in format");
	}
	va_end(ap);

	if (!*ptr) perror("Internal error in format");
	return ptr;
}

static inline unsigned long long rdclock(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

static int save_data(char *buf, int count)
{
    int ret = 0;
    unsigned long long t0 = 0, t1, diff_ms;
#ifdef ANDROID_WRITE_LOCK
    static char *lock_name = NULL;
    static int depth = 0;
    if (lock_name == NULL) lock_name = format("vhub:%d", getpid());
    if (depth++ == 0 && lock_name) {
        acquire_wake_lock(PARTIAL_WAKE_LOCK, lock_name);
#endif
        t0 = rdclock();
#ifdef ANDROID_WRITE_LOCK
    }
#endif
    if (count > 0) {
        ssize_t m, left = 0;
        int *n = &tcmd.fd_out_data_pointer;
        if (vhb.use_ft && vhb.ft) {
            struct filter *ft = vhb.ft;
            buf[count] = 0;
            while (ft) {
                if (strstr(buf, ft->list)) {
                    struct timeval tv;
                    gettimeofday(&tv, NULL);
                    if (tv.tv_sec > (ft->tv.tv_sec + ft->timeout)) {
                        printf("<vhub> %s timeout=%d\n", ft->list, ft->timeout);
                        if (fork() == 0) {
                            execl(ft->exec, ft->list, "VHUB", NULL);
                            exit(0);
                        }
                        ft->tv = tv;
                    }
                }
                ft = ft->next;
            }
        }
        /* n = data_max;
        while (n) */ {
            // m = count < *n ? count:*n; // read(fd, buf, count > n ? count:n);
            if (count < *n) {
                m = count;
            } else {
                m = *n;
                left = count - m;
            }

            if (m > 0) {
                size_t w, tryn = 0;
                *n -= m;
               do {
                    w = write(tcmd.fd_out, buf, m);
                    if (w > 0) {
                        m -= w;
                    } else {
                        if (++tryn > 5) {
                           ret = w;
                           goto _exit;
                        }
                        fprintf(log_out_fileno, "%s trying...[%d]\n", lprogram_name, tryn);
                        usleep(10*1000);
                    }
                } while (m);
            } else {
                fprintf(log_out_fileno, "\n\nall flowed data number is %d\n\n", tcmd.fd_out_data_max);
                // goto _exit;
            }
        }
        if (*n <= 0) {
            // it's full
            tcmd.fd_out_data_pointer = tcmd.fd_out_data_max;
            vhub_open_new(tcmd.fd_out_name, tcmd.fd_out_data_segment++, tcmd.fd_out_data_segment_max, &tcmd.fd_out, O_TRUNC);
            if (left)
                save_data(&buf[count - left], left);
        }
        // goto _exit;
    }
_exit:
#ifdef ANDROID_WRITE_LOCK
    if (--depth == 0 && lock_name) {
        release_wake_lock(lock_name);
#endif
        t1 = rdclock();
        diff_ms = (t1 - t0) / (1000 * 1000);
        if (diff_ms > 100) {
            fprintf(stderr, "Warning : vhub sync write %d bytes cost %lld ms\n", count, diff_ms);
        }
#ifdef ANDROID_WRITE_LOCK
    }
#endif
    return ret;
}

#if (local_ipc_method == 0)
static void create_pts(int i)
{
    char *slavename;
    extern char *ptsname();
    int fdm;
    char *tpath = malloc(sizeof(vhb.lpath) + sizeof(vhb.vhc[0].vname) + 2 + sizeof(lprogram_name) + 1);
    if(tpath == NULL)
        return;

    fdm = open("/dev/ptmx", O_RDWR);
    if(fdm < 0){
        free(tpath);
        return;
    }

    grantpt(fdm);
    unlockpt(fdm);
    slavename = ptsname(fdm);
    strncpy(vhb.vhc[i].pts, slavename, sizeof vhb.vhc[0].pts);
#if 1
    {
        // 必须设置为raw,否则一些特殊字符会被tty解释[luther.ge]
        struct termios termbuf;
        tcgetattr(fdm, &termbuf);
        cfmakeraw(&termbuf);
        tcsetattr(fdm, TCSANOW, &termbuf);
    }
#endif
    vhb.vhc[i].fd = fdm; // vhb.vhc[i].fd = open(slavename, O_RDWR, 0777);
    strcpy(tpath, vhb.lpath);
    strcat(tpath, "/");
    strcat(tpath, lprogram_name);
    strcat(tpath, ".");
    strncat(tpath, vhb.vhc[i].vname+2, vhb.vhc[i].vname_len - 3);
    unlink(tpath);
    printf("[%d]%s --> %s\n", vhb.vhc[i].fd, vhb.vhc[i].pts, tpath);
    symlink(vhb.vhc[i].pts, tpath);
    chmod(vhb.vhc[i].pts, 0777);

    pfd_set(vhb.vhc[i].fd);
    free(tpath);
}
#endif

#if (local_ipc_method == 1)
int create_pipe(int i)
{
    char *tpath = malloc(sizeof(vhb.lpath) + sizeof(vhb.vhc[0].vname) + 2 + sizeof(lprogram_name) + 1);

    strncpy(vhb.vhc[i].pts, "pipe", sizeof vhb.vhc[0].pts);
    strcpy(tpath, vhb.lpath);
    strcat(tpath, "/");
    strcat(tpath, lprogram_name);
    strcat(tpath, ".");
    strncat(tpath, vhb.vhc[i].vname+2, vhb.vhc[i].vname_len - 3);
    unlink(tpath);
    mkfifo(tpath, 0666);
    vhb.vhc[i].fd = open(tpath, O_RDWR);
    printf("[%d]%s%d --> %s\n", vhb.vhc[i].fd, vhb.vhc[i].pts, i, tpath);

    pfd_set(vhb.vhc[i].fd);
    free(tpath);
}
#endif

int convert_size_format(void *fmt)
{
    char buf[64];
    char *num;
    char type;
    int result = 0;
    int mult = 1;
    strcpy(buf, fmt);
    // num = strtok(buf, "kKmM");
    num = strpbrk(buf, "kKmM");
    if (num) {
        type = *num;
        *num = 0;
        num = buf;
        // printf("%s->%c\n", num, type);
        // printf("%s\n", (char*)fmt);
        switch (tolower(type)) {
            case 'k': mult = 1024; break;
            case 'm': mult = 1024*1024; break;
        }
    } else num = buf;
    result = strtol(num, NULL, 0);
    return result * mult;
}

int main(int argc, char *argv[])
{
    int arg;
    int ret = 0;
    int fd;
    int count = 1024*256;
    int wcount;
    int flags = 0;
    char *fd_out_name=lprogram_name;
    int data_max = 1024*100*1024;
    int i;
    int fds_max = 0;
    struct filter *ft, *observer;

#if HAVE_INOTIFY
    vhb.use_ft = 0;
    vhb.use_observer = 1;
#else
    vhb.use_ft = 1;
    vhb.use_observer = 0;
#endif
    vhb.vhc_max = 0;
    vhb.vhc = calloc(64, sizeof(struct vhubc));
    strcpy(vhb.lpath, "/dev");
    tcmd.fd_out_data_segment_max = 2;

#if HAVE_INOTIFY
    vhb.fd_inotify = inotify_init();
    if (vhb.fd_inotify < 0)
        fprintf(stderr, "inotify_init failed, %s\n", strerror(errno));
#endif

    while ((arg = getopt(argc, argv, "t:p:s:d:o:m:hf:e:T:SF:Z:E:M:Y:")) != EOF) {
        switch (arg) {
            case 't': {
                fds_max = strtol(optarg, NULL, 0);
            } break;
            case 'p': {
                vhb.vhc[vhb.vhc_max].vname[0] = '\n';
                vhb.vhc[vhb.vhc_max].vname[1] = '<';
                strncpy(vhb.vhc[vhb.vhc_max].vname + 2, optarg, sizeof(vhb.vhc[0].vname) - 3);
                strcat(vhb.vhc[vhb.vhc_max].vname, ">");
                vhb.vhc[vhb.vhc_max].vname_len = strlen(vhb.vhc[vhb.vhc_max].vname);
                vhb.vhc_max++;
            } break;
            case 's': {
                data_max = convert_size_format(optarg);
            } break;
            case 'o': {
                fd_out_name = optarg;
            } break;
            case 'm': {
                tcmd.fd_out_data_segment_max = strtol(optarg, NULL, 0);
            } break;
            case 'd': {
                strncpy(vhb.lpath, optarg, sizeof(vhb.lpath));
            } break;
            case 'S': {
                vhb.use_ft = 1;
                vhb.use_observer = 0;
            } break;
            case 'f': {
                ft = calloc(1, sizeof(struct filter));
#if HAVE_INOTIFY
                ft->flags = IN_CLOSE_WRITE | IN_CLOSE_NOWRITE;
#endif
                ft->list = optarg;
                ft->next = vhb.ft;
                vhb.ft = ft;
            } break;
            case 'F': {
                observer = calloc(1, sizeof(struct filter));
#if HAVE_INOTIFY
                if (optarg[0] == ':') // watch this dir which contains this file [luther.ge]
                    observer->flags = IN_ACCESS; // IN_CREATE | IN_ACCESS; // subfile was created, /data/anr/traces.txt will do IN_ACCESS finally
                else
                    observer->flags = IN_CLOSE_WRITE | IN_CLOSE_NOWRITE;
#endif
                observer->list = optarg;
                observer->next = vhb.observer;
                vhb.observer = observer;
            } break;
            case 'Y': {
                 if (vhb.observer)
                     vhb.observer->opts2 = strtol(optarg, NULL, 0);
            } break;
            case 'M': {
                if (vhb.observer) {
                    vhb.observer->flags = strtol(optarg, NULL, 0);
                }
                if (vhb.ft) {
                    vhb.ft->flags = strtol(optarg, NULL, 0);
                }
            } break;
            case 'T': {
                if (vhb.ft) {
                    vhb.ft->timeout = strtol(optarg, NULL, 0);
                }
            } break;
            case 'Z': {
                if (vhb.observer) {
                    vhb.observer->timeout = strtol(optarg, NULL, 0);
                }
            } break;
            case 'e': {
                if (vhb.ft) {
                    vhb.ft->exec = optarg;
                    gettimeofday(&vhb.ft->tv, NULL);
                    printf("[ vhub ] %s %s\n", vhb.ft->exec, vhb.ft->list);
                }
            } break;
            case 'E': {
                if (vhb.observer) {
                    vhb.observer->exec = optarg;
                    gettimeofday(&vhb.observer->tv, NULL);
                    printf("[ vhub observer ] %s %s\n", vhb.observer->exec, vhb.observer->list);
                }
            } break;
            case 'h': {
                fprintf(log_out_fileno, "vhub -p android -p rild -p kernel -d /dev -o luther.logs. -s 10m -m 5\n");  
                return 0;
            } break;
            default: break;
        }
    }

    setup_signals();

#if HAVE_INOTIFY
    if (vhb.use_observer && vhb.observer) {
        int lcount = 0;
        struct filter *observer = vhb.observer;
        while (vhb.fd_inotify >= 0 && observer) {
            char *file;
            char *pos = NULL;
            if (observer->list[0] == ':') {
                file = (char*)observer->list + 1;
                pos = strrchr(observer->list, '/');
                *pos++ = 0;
                observer->opts2 |= 0x80000000;
                observer->list = pos;
            } else file = (char*)observer->list;
            printf("<vhub observer> add 0x%08x \"%s\"%s%s\n", observer->flags, file, pos ? " --> ":"", pos ? pos:"");
            ret = inotify_add_watch(vhb.fd_inotify, file, observer->flags);
            if (ret < 0)
                fprintf(stderr, "inotify_add_watch failed for %s, %s\n", observer->list, strerror(errno));
            else observer->opts = ret; // as wd
            observer = observer->next;
            ++lcount;
        }
        if (lcount) pfd_set(vhb.fd_inotify);
    }
#endif

    if (vhb.vhc_max) {
        for (i = 0; i < vhb.vhc_max; i++) {
#if (local_ipc_method == 0)
            create_pts(i);
#endif
#if (local_ipc_method == 1)
            create_pipe(i);
#endif
        }
    }

#if 0
    snprintf(tcmd.fd_out_name, sizeof(tcmd.fd_out_name),\
             "%s%d", fd_out_name, tcmd.fd_out_data_segment++);
#else
    strcpy(tcmd.fd_out_name, fd_out_name);
#endif
    tcmd.fd_out_data_segment = 0;
    vhub_open_new(tcmd.fd_out_name, tcmd.fd_out_data_segment++, tcmd.fd_out_data_segment_max, &tcmd.fd_out, O_TRUNC);
    tcmd.fd_out_data_max = tcmd.fd_out_data_pointer = data_max;
    tcmd.buf = malloc(count);
    tcmd.buf_size = count;
    tcmd.fd_terminal =
#ifdef ANDROID
                        888;
#else
                        STDIN_FILENO;
    pfd_set(tcmd.fd_terminal);
#endif

    init_net_ipc();

    for (;;) {
        int ifd, fd;

        if (poll(tcmd.pfds, TCMD_CLIENT_MAX, -1) <= 0) {
            perror("poll");
            continue;
        }

        for (ifd = 0; ifd < TCMD_CLIENT_MAX; ifd++) {
            if (pfd_isset(ifd)) {
                fd = pfd_get(ifd);
                if (fd >= vhb.vhc[0].fd && fd <= vhb.vhc[vhb.vhc_max-1].fd)
                    tcmd_pts_handler(ifd, fd);
                else if (fd == tcmd.fd_terminal)
                    tcmd_terminal_handler(ifd, fd);
                else if (fd == tcmd.fd_listen)
                    tcmd_listen_handler(ifd, fd);
#if HAVE_INOTIFY
                else if (fd == vhb.fd_inotify)
                    tcmd_inotfiy_file_observer_handler(ifd, fd);
#endif
                /*else if (fd == fd_net)
                    tcmd_net_handler(ifd, fd);*/
                else tcmd_client_handler(ifd, fd);
            }
        }
    }

_exit:
    unlink_ipc();
    return ret;
}

#include <signal.h>
static sigset_t signals_handled;
static void term __P((int));
/*
 * setup_signals - initialize signal handling.
 */
static void setup_signals()
{
    struct sigaction sa;

    /*
     * Compute mask of all interesting signals and install signal handlers
     * for each.  Only one signal handler may be active at a time.  Therefore,
     * all other signals should be masked when any handler is executing.
     */
    sigemptyset(&signals_handled);
//    sigaddset(&signals_handled, SIGHUP);
    sigaddset(&signals_handled, SIGINT);
    sigaddset(&signals_handled, SIGTERM);
//    sigaddset(&signals_handled, SIGCHLD);
//    sigaddset(&signals_handled, SIGUSR2);

#define SIGNAL(s, handler)	do { \
	sa.sa_handler = handler; \
	if (sigaction(s, &sa, NULL) < 0) \
        {}/* fatal("Couldn't establish signal handler (%d): %m", s); */ \
    } while (0)

    sa.sa_mask = signals_handled;
    sa.sa_flags = 0;
//    SIGNAL(SIGHUP, hup);		/* Hangup */
    SIGNAL(SIGINT, term);		/* Interrupt */
    SIGNAL(SIGTERM, term);		/* Terminate */
//    SIGNAL(SIGCHLD, chld);
//
//    SIGNAL(SIGUSR1, toggle_debug);	/* Toggle debug flag */
//    SIGNAL(SIGUSR2, open_ccp);		/* Reopen CCP */
//
//    /*
//     * Install a handler for other signals which would otherwise
//     * cause pppd to exit without cleaning up.
//     */
//    SIGNAL(SIGABRT, bad_signal);
//    SIGNAL(SIGALRM, bad_signal);
//    SIGNAL(SIGFPE, bad_signal);
//    SIGNAL(SIGILL, bad_signal);
//    SIGNAL(SIGPIPE, bad_signal);
//    SIGNAL(SIGQUIT, bad_signal);
//    SIGNAL(SIGSEGV, bad_signal);
//#ifdef SIGBUS
//    SIGNAL(SIGBUS, bad_signal);
//#endif
//#ifdef SIGEMT
//    SIGNAL(SIGEMT, bad_signal);
//#endif
//#ifdef SIGPOLL
//    SIGNAL(SIGPOLL, bad_signal);
//#endif
//#ifdef SIGPROF
//    SIGNAL(SIGPROF, bad_signal);
//#endif
//#ifdef SIGSYS
//    SIGNAL(SIGSYS, bad_signal);
//#endif
//#ifdef SIGTRAP
//    SIGNAL(SIGTRAP, bad_signal);
//#endif
//#ifdef SIGVTALRM
//    SIGNAL(SIGVTALRM, bad_signal);
//#endif
//#ifdef SIGXCPU
//    SIGNAL(SIGXCPU, bad_signal);
//#endif
//#ifdef SIGXFSZ
//    SIGNAL(SIGXFSZ, bad_signal);
//#endif
//
//    /*
//     * Apparently we can get a SIGPIPE when we call syslog, if
//     * syslogd has died and been restarted.  Ignoring it seems
//     * be sufficient.
//     */
//    signal(SIGPIPE, SIG_IGN);
}

/*
 * term - Catch SIGTERM signal and SIGINT signal (^C/del).
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
/*ARGSUSED*/
static void
term(sig)
    int sig;
{
//    /* can't log a message here, it can deadlock */
//    got_sigterm = sig;
//    if (conn_running)
//	/* Send the signal to the [dis]connector process(es) also */
//	kill_my_pg(sig);
//    notify(sigreceived, sig);
//    if (waiting)
//	siglongjmp(sigjmp, 1);
    unlink_ipc();
    exit(0);
}

static void unlink_ipc(void)
{
    int i;

    if (vhb.vhc_max) {
        char *tpath = malloc(sizeof(vhb.lpath) + sizeof(vhb.vhc[0].vname) + 1 + sizeof(lprogram_name) + 1);
        for (i = 0; i < vhb.vhc_max; i++) {
            if (vhb.vhc[i].vname_len) {
                strcpy(tpath, vhb.lpath);
                strcat(tpath, "/");
                strcat(tpath, lprogram_name);
                strcat(tpath, ".");
                strncat(tpath, vhb.vhc[i].vname+2, vhb.vhc[i].vname_len - 3);
                unlink(tpath);
#if (local_ipc_method == 0)
                printf("delete %s --> %s\n", vhb.vhc[i].pts, tpath);
#endif
#if (local_ipc_method == 1)
                printf("delete %s%d --> %s\n", vhb.vhc[i].pts, i, tpath);
#endif
            }
        }
        free(tpath);
    }
}
