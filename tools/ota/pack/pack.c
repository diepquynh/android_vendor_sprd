/* Created by Spreadst */

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include<stdint.h>

#include "pack.h"

#define PATH_BUF_LEN  4096

#define WRITE_DATA(fp, data, size) \
  if (write_all(fd, data, size) < 0){ \
    pack_error("write error:%s.\n", strerror(errno)); \
    res = 1; \
    goto ret_p; \
  }

#define PACK_MAGIC "SPRDPACK"
#define PACK_VER   1

#define OFF_OF_ITEM(type, item) \
  (intptr_t)(((type*)0) -> item)

typedef struct {
  char magic[8];
  int ver;
  long pac_size;
  long offset;
  int items_count;
  int header_len;
  int crc;
} pack_header;

typedef struct {
  int item_header_len;
  int item_len;  //length of item header and item data
  unsigned long atim;
  unsigned long mtim;
  unsigned long ctim;
  unsigned short dev;
  unsigned short rdev;
  mode_t mode;
  uid_t uid;
  gid_t gid;
  int has_se;
  int crc;
  int path_size;
  int scon_size;
  int data_size;
  int size2; //item data size except real data
  uint64_t capabilities;
} pack_item_header;

typedef struct {
  int size;  //size of item data
  int path_off;
  int scon_off;
  int data_off;
  char data[1];
} pack_item_data;

enum {
  MODE_INVALID,
  MODE_PACK,
  MODE_UNPACK,
};

typedef struct f_i_s {
  char *prefix;
  char *prefix_abs_path;
  int pre_len;
  char *prefix_with_slash;
  int pre_len2;
  int has_pack;
  struct f_i_s *next;
} filter_item;

static filter_item *filters = NULL;
static int filter_mode = NO_FILTER;
static int verbose = 0;
static char path_buffer[PATH_BUF_LEN];
static char tmp_buffer[PATH_BUF_LEN];

int pack_info(const char *format, ...) {
  int n = 0;
  va_list ap;
  va_start(ap, format);
  n = vfprintf(stdout, format, ap);
  va_end(ap);
  return n;
}

int pack_error(const char *format, ...) {
  int n = 0;
  va_list ap;
  va_start(ap, format);
  n = vfprintf(stderr, format, ap);
  va_end(ap);
  return n;
}

static int read_all(int fd, void *buf, size_t len) {
  size_t total = 0;
  int ret;
  char *ptr = buf;

  while (total < len) {
    ret = read(fd, ptr, len - total);

    if (ret < 0)
      return ret;

    if (ret == 0)
      return total;

    ptr += ret;
    total += ret;
  }

  return total;
}

static int write_all(int fd, void *buf, size_t len) {
  size_t total = 0;
  int ret;
  char *ptr = buf;

  while (total < len) {
    ret = write(fd, ptr, len - total);

    if (ret < 0)
      return ret;

    if (ret == 0)
      return total;

    ptr += ret;
    total += ret;
  }

  return total;
}

char *get_abs_path(char *path) {
  if (path[0] == '/') {
    strcpy(path_buffer, path);
  } else {
    getcwd(path_buffer, PATH_BUF_LEN);
    strcat(path_buffer, "/");
    strcat(path_buffer, path);
  }
  if (verbose) {
    pack_info("get_abs_path of \'%s\' is \'%s\'.\n", path, path_buffer);
  }
  return path_buffer;
}

static char *get_path_with_slash(char *path) {
  int path_len = strlen(path);
  char *new_path = (char *)malloc(path_len + 2);
  strcpy(new_path, path);
  if (path[path_len - 1] != '/') {
    strcat(new_path, "/");
  }
  return new_path;
}

void set_verbose_on() {
  verbose = 1;
}

int get_verbose() {
  return verbose;
}

/*********for*pack*********/

static void real_add_filter(char *prefix_key, char *prefix_abs_path) {
  int filter_len = strlen(prefix_key);
  int filter_abs_len = strlen(prefix_abs_path);
  filter_item *filter = (filter_item *) malloc(sizeof(filter_item) + filter_len + 1 + filter_abs_len*2 + 3);
  char *prefix = (char *)filter + sizeof(filter_item);
  char *prefix1 = prefix + filter_len + 1;
  char *prefix2 = prefix1 + filter_abs_len + 1;
  int filter_len1, filter_len2;

  strcpy(prefix, prefix_key);
  strcpy(prefix1, prefix_abs_path);
  strcpy(prefix2, prefix_abs_path);
  filter_len1 = filter_len2 = filter_abs_len;
  if (prefix[filter_len - 1] == '/') {
    prefix1[filter_len - 1] = '\0';
    filter_len1 = filter_abs_len - 1;
  } else {
    strcat(prefix2, "/");
    filter_len2 = filter_abs_len + 1;
  }

  filter->prefix = prefix;
  filter->prefix_abs_path = prefix1;
  filter->pre_len = filter_len1;
  filter->prefix_with_slash = prefix2;
  filter->pre_len2 = filter_len2;
  filter->next = NULL;
  filter->has_pack = 0;

  if (filters == NULL) {
    filters = filter;
  } else {
    filter->next = filters;
    filters = filter;
  }
}

void add_filter(char *base_dir, char *prefix) {
  filter_item *filter = filters;
  int filter_len = strlen(prefix);
  prefix = strdup(prefix);
  if (prefix[filter_len - 1] == '/') {
    prefix[filter_len - 1] = '\0';
  }
  while (filter) {
    if (strcmp(filter->prefix, prefix) == 0) {
      goto ret_p;
    }
    filter = filter->next;
  }
  if (prefix[0] == '/') {
    strncpy(tmp_buffer, prefix, PATH_BUF_LEN);
  } else {
    if (base_dir) {
      strncpy(tmp_buffer, base_dir, PATH_BUF_LEN);
    } else {
      getcwd(tmp_buffer, PATH_BUF_LEN);
    }
    strncat(tmp_buffer, "/", PATH_BUF_LEN-strlen(tmp_buffer));
    strncat(tmp_buffer, prefix, PATH_BUF_LEN-strlen(tmp_buffer));
  }
  real_add_filter(prefix, tmp_buffer);
ret_p:
  free(prefix);
}

void destory_filters() {
  filter_item *filter = filters;
  filter_item *next_filter = NULL;

  if (filters == NULL) {
    return;
  }

  while (filter) {
    next_filter = filter->next;
    free(filter);
    filter = next_filter;
  }
  filters = NULL;
}

static int filter_path(char *path) {
  int res = 1;
  char *path_with_slash = get_path_with_slash(path);
  if (filter_mode == WHITE_LIST) {
    int path_len = strlen(path);
    int path_len2 = strlen(path_with_slash);
    filter_item *filter = filters;
    while (filter) {
      int cmp_len = (path_len > filter->pre_len) ? (filter->pre_len) : (path_len);
      int cmp_len2 = (path_len2 > filter->pre_len2) ? (filter->pre_len2) : (path_len2);
      if (strncmp(filter->prefix_abs_path, path, cmp_len) == 0
        && strncmp(filter->prefix_with_slash, path_with_slash, cmp_len2) == 0) {
        filter->has_pack = 1;
        res = 1;
        goto ret_p;
      }
      filter = filter->next;
    }
    res = 0;
    goto ret_p;
  } else if (filter_mode == BLACK_LIST) {
    filter_item *filter = filters;
    while (filter) {
      if (strncmp(filter->prefix_abs_path, path, filter->pre_len) == 0
        && strncmp(filter->prefix_with_slash, path_with_slash, filter->pre_len2) == 0) {
        res = 0;
        goto ret_p;
      }
      filter = filter->next;
    }
    goto ret_p;
  }

ret_p:
  free(path_with_slash);
  return res;
}

void set_filter_mode(int mode) {
  switch (mode) {
    case NO_FILTER:
    case WHITE_LIST:
    case BLACK_LIST:
      filter_mode = mode;
      break;
    default:
      filter_mode = NO_FILTER;
      pack_error("invalid filter mode: %d\n", mode);
  }
}

static const char *filter_mode_str() {
  switch (filter_mode) {
    case NO_FILTER: return "NO_FILTER";
    case WHITE_LIST: return "WHITE_LIST";
    case BLACK_LIST: return "BLACK_LIST";
    default: return "ERR_MODE";
  }
}

void dump_filter_info() {
  pack_info("filter mode is %s, list as follow:\n", filter_mode_str());
  filter_item *filter = filters;
  int i = 0;
  while (filter) {
    pack_info("%d -> [%s,%s]\n", i++, filter->prefix, filter->prefix_abs_path);
    filter = filter->next;
  }
}

int check_pack_file() {
  int res = 0;
  filter_item *filter = filters;
  if (filter_mode == WHITE_LIST) {
    while (filter) {
      if (filter->has_pack == 0) {
        pack_error("path [%s] have not packed!!\n", filter->prefix);
        res = 1;
      }
      filter = filter->next;
    }
  }
  return res;
}

int write_header(int fd, pack_header **p_header) {
  int res = 0;
  pack_header *header;

  if (p_header == NULL || *p_header == NULL) {
    header = (pack_header *)malloc(sizeof(pack_header));

    memset(header, 0, sizeof(pack_header));
    memcpy(header->magic, PACK_MAGIC, sizeof(header->magic));
    header->ver = PACK_VER;
    header->offset = header->header_len = sizeof(pack_header);
    header->pac_size = header->header_len;
    if (p_header) {
      *p_header = header;
    }
  } else {
    header = *p_header;
  }
  lseek(fd, 0, SEEK_SET);
  WRITE_DATA(fd, header, header->header_len);
ret_p:
  return res;
}

pack_item_header *fill_item_header(char *path, int se_info, struct stat *p_st, char **p_selabel) {
  pack_item_header *item_h = NULL;
  char *selabel = NULL;
  int res = 0;

  item_h = (pack_item_header *)malloc(sizeof(pack_item_header));
  memset(item_h, 0, sizeof(pack_item_header));
  item_h->item_header_len = sizeof(pack_item_header);
  item_h->atim = p_st->st_atime;
  item_h->mtim = p_st->st_mtime;
  item_h->ctim = p_st->st_ctime;
  item_h->mode = p_st->st_mode;
  item_h->dev = p_st->st_dev;
  item_h->rdev = p_st->st_rdev;
  item_h->uid = p_st->st_uid;
  item_h->gid = p_st->st_gid;
  item_h->path_size = strlen(path) + 1;
  item_h->data_size = p_st->st_blksize;
  item_h->data_size = item_h->data_size * p_st->st_blocks;

  if (se_info) {
    item_h->has_se = 1;
    res = lgetfilecon(path, &selabel);
    if (res > 0) {
      item_h->scon_size = strlen(selabel) + 1;
      if (p_selabel) {
        *p_selabel = selabel;
      }
      if (verbose) {
        pack_info("[%s]'s selabel is %s.\n", path, selabel);
      }
    }
    if (S_ISREG(p_st->st_mode)) {
      struct vfs_cap_data cap_data;
      memset(&cap_data, 0, sizeof(cap_data));
      if (lgetxattr(path, XATTR_NAME_CAPS, &cap_data, sizeof(cap_data)) == 0) {
        item_h->capabilities = cap_data.data[1].permitted;
        item_h->capabilities = item_h->capabilities << 32;
        item_h->capabilities |= cap_data.data[0].permitted;
      }
      if (verbose) {
        pack_info("[%s]'s capabilities is %lld.\n", path, item_h->capabilities);
      }
    }
  }

  return item_h;
}

int get_data_len(char *path, struct stat *p_st) {
  int res = 0;

  if (S_ISDIR(p_st->st_mode)) {
    return 0;
  }

  if (S_ISREG(p_st->st_mode)) {
    int fd_file = open(path, O_RDONLY);
    if (fd_file < 0) {
      pack_error("open file \'%s\' error:%s.\n", path, strerror(errno));
      res = -1;
      goto ret_p;
    }
    res = lseek(fd_file, 0, SEEK_END);
    close(fd_file);
  }
  if (S_ISLNK(p_st->st_mode)) {
    ssize_t path_len = readlink(path, tmp_buffer, PATH_BUF_LEN);
    if (path_len >= PATH_BUF_LEN) {
      pack_error("link path of \'%s\' too long!\n", path);
      res = -1;
      goto ret_p;
    }
    res = path_len + 1;
  }

ret_p:
  return res;
}

pack_item_data *fill_item_data(char *path, struct stat *p_st, char *selabel, pack_item_header *item_h) {
  pack_item_data *item_d = NULL;
  int data_len = 0;
  int data_off = 0;
  int item_size2 = 0;

  if (item_h == NULL) {
    pack_error("internal error! item_h is NULL\n");
    return NULL;
  }

  data_len = get_data_len(path, p_st);
  if (data_len < 0) {
    return NULL;
  }

  item_h->data_size = data_len;
  data_off = item_h->path_size + item_h->scon_size;
  item_size2 = OFF_OF_ITEM(pack_item_data, data) + data_off;
  item_d = (pack_item_data *)malloc(item_size2);
  item_h->size2 = item_size2;
  item_d->size = item_size2 + data_len;
  item_h->item_len = item_h->item_header_len + item_d->size;
  item_d->path_off = 0;
  item_d->scon_off = item_h->path_size;
  item_d->data_off = data_off;
  memcpy(item_d->data, path, item_h->path_size);
  if (selabel && item_h->scon_size) {
    memcpy(item_d->data + item_h->path_size, selabel, item_h->scon_size);
  }

  return item_d;
}

int pack_file_data(int fd, char *file, struct stat *p_st) {
  int res = 0;
  int fd_file = -1;
  int write_data = 1;

  if (S_ISDIR(p_st->st_mode)) {
    pack_error("path %s is directory!\n", file);
    res = -1;
    goto ret_p;
  }

  if (S_ISREG(p_st->st_mode)) {
    int read_len = 1;
    fd_file = open(file, O_RDONLY);
    if (fd_file < 0) {
      pack_error("open file \'%s\' error:%s.\n", file, strerror(errno));
      res = -1;
      goto ret_p;
    }
    if (write_data) {
      lseek(fd_file, 0, SEEK_SET);
      while ((read_len = read_all(fd_file, tmp_buffer, PATH_BUF_LEN)) > 0) {
        WRITE_DATA(fd, tmp_buffer, read_len);
      }
    }
    close(fd_file);
  }
  if (S_ISLNK(p_st->st_mode)) {
    ssize_t path_len = readlink(file, tmp_buffer, PATH_BUF_LEN);
    if (path_len >= PATH_BUF_LEN) {
      pack_error("internal error: link path of \'%s\' too long!\n", file);
      res = -1;
      goto ret_p;
    }
    tmp_buffer[path_len] = 0;
    path_len += 1;
    if (write_data) {
      WRITE_DATA(fd, tmp_buffer, path_len);
    }
  }

ret_p:
  return res;
}

int pack_item(int fd, char *path, int se_info, pack_header *header);

int pack_dir(int fd, char *dir, int se_info, pack_header *header) {
  int res = 0;
  int path_len = strlen(dir);
  DIR *d = NULL;
  struct dirent *de;

  d = opendir(dir);
  if(d == 0) {
    pack_error("opendir \'%s\' failed, %s\n", dir, strerror(errno));
    res = -1;
    goto ret_p;
  }

  while((de = readdir(d)) != 0){
    path_buffer[path_len] = '\0';
    if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
    strcat(path_buffer, "/");
    strcat(path_buffer, de->d_name);
    pack_item(fd, path_buffer, se_info, header);
  }

ret_p:
  if (d) {
    closedir(d);
  }
  path_buffer[path_len] = '\0';
  return res;
}

int pack_item(int fd, char *path, int se_info, pack_header *header) {
  int res = 0;
  struct stat st;
  pack_item_header *item_h = NULL;
  pack_item_data *item_d = NULL;
  char *selabel = NULL;
  char *file_data = NULL;
  int data_len = 0;

  if (filter_path(path) == 0) {
    pack_info("do not pack item [%s]\n", path);
    return 1;
  }

  if (verbose) {
    pack_info("pack item [%s]\n", path);
  }

  res = lstat(path, &st);
  if (res < 0) {
    pack_error("stat \'%s\' failed:%s\n", path, strerror(errno));
    res = 1;
    goto ret_p;
  }

  item_h = fill_item_header(path, se_info, &st, &selabel);
  item_d = fill_item_data(path, &st, selabel, item_h);

  if (item_h == NULL || item_d == NULL) {
    pack_error("stat \'%s\' failed:%s\n", path, strerror(errno));
    res = 1;
    goto ret_p;
  }

  header->items_count ++;
  header->pac_size += sizeof(pack_item_header) + item_d->size;

  WRITE_DATA(fd, item_h, sizeof(pack_item_header));
  WRITE_DATA(fd, item_d, item_h->size2);

  if (S_ISDIR(st.st_mode)) {
    res = pack_dir(fd, path, se_info, header);
  } else {
    res = pack_file_data(fd, path, &st);
  }
ret_p:
  if (item_h)
    free(item_h);
  if (item_d)
    free(item_d);
  if (selabel)
    free(selabel);
  return res;
}

int pack(char *pak_file, char *src_dir, int se_info) {
  int res = 0;
  pack_header *header = NULL;
  int fd = -1;
  struct stat st;

  res = stat(src_dir, &st);
  if (res < 0) {
    pack_error("stat path \'%s\' failed:%s\n", src_dir, strerror(errno));
    res = 1;
    goto ret_p;
  }
  if (!S_ISDIR(st.st_mode)) {
    pack_error("path \'%s\' is not a directory!!\n", src_dir);
    res = 1;
    goto ret_p;
  }

  fd = open(pak_file, O_CREAT|O_RDWR|O_TRUNC, S_IRWXU|S_IRGRP|S_IROTH);
  if (fd < 0) {
    pack_error("open file \'%s\' failed:%s\n", pak_file, strerror(errno));
    res = 1;
    goto ret_p;
  }

  if (se_info && is_selinux_enabled() <= 0) {
    se_info = 0;
  }

  write_header(fd, &header);

  res = pack_item(fd, src_dir, se_info, header);
  if (res) {
    res = 1;
    goto ret_p;
  }

  write_header(fd, &header);

ret_p:
  if (fd >= 0) {
    close(fd);
  }
  if (header)
    free(header);
  return res;
}

/*********for*unpack*********/
int set_item_permissions(char *path, pack_item_header *item_h, pack_item_data *item_d, int se_info) {
  int res = 0;

  if (verbose) {
    pack_info("lchown(%s, %d, %d)\n", path, item_h->uid, item_h->gid);
  }
  res = lchown(path, item_h->uid, item_h->gid);
  if (res < 0) {
    pack_error("lchown(\'%s\', %d, %d) failed:%s\n", path, item_h->uid, item_h->gid, strerror(errno));
    goto ret_p;
  }
  if (se_info && item_h->has_se) {
    char *item_scon = NULL;
    if (item_h->scon_size) {
      item_scon = item_d->data+item_d->scon_off;
      if (verbose) {
        pack_info("lsetfilecon %s as %s.\n", path, item_scon);
      }
      if (lsetfilecon(path, item_scon) != 0) {
        pack_error("lsetfilecon of %s to %s failed: %s\n\n", path, item_scon, strerror(errno));
        res = -1;
        goto ret_p;
      }
    }
    if (S_ISREG(item_h->mode)) {
      if (item_h->capabilities == 0) {
        if ((lremovexattr(path, XATTR_NAME_CAPS) == -1) && (errno != ENODATA)) {
          // Report failure unless it's ENODATA (attribute not set)
          pack_error("lremovexattr of %s to %lld failed: %s\n",
                 path, item_h->capabilities, strerror(errno));
        }
      } else {
        struct vfs_cap_data cap_data;
        memset(&cap_data, 0, sizeof(cap_data));
        cap_data.magic_etc = VFS_CAP_REVISION | VFS_CAP_FLAGS_EFFECTIVE;
        cap_data.data[0].permitted = (uint32_t) (item_h->capabilities & 0xffffffff);
        cap_data.data[0].inheritable = 0;
        cap_data.data[1].permitted = (uint32_t) (item_h->capabilities >> 32);
        cap_data.data[1].inheritable = 0;
        if (lsetxattr(path, XATTR_NAME_CAPS, &cap_data, sizeof(cap_data), 0) < 0) {
          pack_error("setcap of %s to %lld failed: %s\n",
                 path, item_h->capabilities, strerror(errno));
        }
      }
    }
  }
ret_p:
  return res;
}

int remove_path(char *path, struct stat *p_st) {
  int res = 0;
  if (S_ISDIR(p_st->st_mode)) {
    pack_error("need implament remove directory!!\n");
  } else {
    res = remove(path);
    if (res <0) {
      pack_error("remove \'%s\' failed:%s!!\n", path, strerror(errno));
    }
  }
  return res;
}

static char *info_file(char *buffer, int buf_len, char *path, pack_item_header *item_h, pack_item_data *item_d) {
  mode_t mode = item_h->mode;
  char *out = buffer;
  int remain_len = buf_len;

  if (remain_len > 10) { // mode size is 10
    switch(mode & S_IFMT){
    case S_IFSOCK: *out++ = 's'; break;
    case S_IFLNK: *out++ = 'l'; break;
    case S_IFREG: *out++ = '-'; break;
    case S_IFDIR: *out++ = 'd'; break;
    case S_IFBLK: *out++ = 'b'; break;
    case S_IFCHR: *out++ = 'c'; break;
    case S_IFIFO: *out++ = 'p'; break;
    default: *out++ = '?'; break;
    }

    *out++ = (mode & 0400) ? 'r' : '-';
    *out++ = (mode & 0200) ? 'w' : '-';
    if(mode & 04000) {
        *out++ = (mode & 0100) ? 's' : 'S';
    } else {
        *out++ = (mode & 0100) ? 'x' : '-';
    }
    *out++ = (mode & 040) ? 'r' : '-';
    *out++ = (mode & 020) ? 'w' : '-';
    if(mode & 02000) {
        *out++ = (mode & 010) ? 's' : 'S';
    } else {
        *out++ = (mode & 010) ? 'x' : '-';
    }
    *out++ = (mode & 04) ? 'r' : '-';
    *out++ = (mode & 02) ? 'w' : '-';
    if(mode & 01000) {
        *out++ = (mode & 01) ? 't' : 'T';
    } else {
        *out++ = (mode & 01) ? 'x' : '-';
    }
    remain_len = buf_len - (out - buffer);
  }

  out += snprintf(out, remain_len, " %u", item_h->uid);
  remain_len = buf_len - (out - buffer);
  out += snprintf(out, remain_len, " %u", item_h->gid);
  remain_len = buf_len - (out - buffer);
  out += snprintf(out, remain_len, " S:%s %s",
          item_d->data + item_d->scon_off,
          item_d->data + item_d->path_off);

  return buffer;
}

int upack_file(int fd, char *path, pack_item_header *item_h, pack_item_data *item_d) {
  int res = 0;
  int read_len = 0;
  int fd_file = -1;

  if (S_ISREG(item_h->mode)) {
    int file_len = item_h->data_size;
    int this_len, remain_len = file_len;
    int all_read_len = 0;
    int write_len = 0;
    fd_file = open(path, O_WRONLY|O_TRUNC);
    if (fd_file < 0) {
      pack_error("open %s error:%s!!\n", path, strerror(errno));
      res = -1;
      goto ret_p;
    }
    do {
      if (remain_len > PATH_BUF_LEN) {
        this_len = PATH_BUF_LEN;
      } else {
        this_len = remain_len;
      }
      remain_len -= this_len;
      read_len = read_all(fd, tmp_buffer, this_len);
      if (read_len < 0) {
        pack_error("read data for %s error:%s!!\n", path, strerror(errno));
        res = -1;
        goto ret_p;
      }
      all_read_len += read_len;
      if (read_len > 0) {
        write_len = write_all(fd_file, tmp_buffer, read_len);
        if (read_len < 0) {
          pack_error("write data to %s error:%s!!\n", path, strerror(errno));
          res = -1;
          goto ret_p;
        }
      }
    } while (read_len > 0 && remain_len > 0);
    if (all_read_len != file_len) {
      if (all_read_len < file_len) {
        pack_error("can not get enough data for file %s, read size:%d!!\n", item_d->data + item_d->path_off, all_read_len);
      } else {
        pack_error("read too much data for file %s, read size:%d!!\n", item_d->data + item_d->path_off, all_read_len);
      }
      res = -1;
      goto ret_p;
    }
  } else if (S_ISLNK(item_h->mode)) {
    read_len = read_all(fd, tmp_buffer, item_h->data_size);
    if (read_len != item_h->data_size) {
      pack_error("read size is %d.\n", read_len);
      if (read_len < 0) {
        pack_error("read data for symlink %s error:%s!!\n", path, strerror(errno));
      } else {
        pack_error("can not get enough data for symlink \'%s\'!!\n", path);
      }
      res = -1;
      goto ret_p;
    }
    res = symlink(tmp_buffer, path);
    if (res < 0) {
      pack_error("symlink %s->%s error:%s!!\n", path, tmp_buffer, strerror(errno));
      goto ret_p;
    }
    /*res = chmod(path, item_h->mode & 07777);
    if (res < 0) {
      pack_error("chmod %s as %05o error:%s!!\n", path, item_h->mode & 07777, strerror(errno));
      goto ret_p;
    }*/
  } else {
    pack_info("special file:[%s]\n\t[atim:%lu,mtim:%lu,ctim:%lu,dev:%08x,rdev:%08x]\n",
          info_file(tmp_buffer, PATH_BUF_LEN, path, item_h, item_d),
          item_h->atim, item_h->mtim, item_h->ctim, item_h->dev, item_h->rdev);
  }

ret_p:
  if (fd_file >= 0)
    close(fd_file);
  return res;
}

static char mode2kind(mode_t mode)
{

    switch(mode & S_IFMT){
    case S_IFSOCK: return 's';
    case S_IFLNK: return 'l';
    case S_IFREG: return '-';
    case S_IFDIR: return 'd';
    case S_IFBLK: return 'b';
    case S_IFCHR: return 'c';
    case S_IFIFO: return 'p';
    default: return '?';
    }
}

char * strmode(mode_t mode, char *strmode)
{
    char *out = strmode;

    *out++ = mode2kind(mode);

    *out++ = (mode & 0400) ? 'r' : '-';
    *out++ = (mode & 0200) ? 'w' : '-';
    if(mode & 04000) {
        *out++ = (mode & 0100) ? 's' : 'S';
    } else {
        *out++ = (mode & 0100) ? 'x' : '-';
    }
    *out++ = (mode & 040) ? 'r' : '-';
    *out++ = (mode & 020) ? 'w' : '-';
    if(mode & 02000) {
        *out++ = (mode & 010) ? 's' : 'S';
    } else {
        *out++ = (mode & 010) ? 'x' : '-';
    }
    *out++ = (mode & 04) ? 'r' : '-';
    *out++ = (mode & 02) ? 'w' : '-';
    if(mode & 01000) {
        *out++ = (mode & 01) ? 't' : 'T';
    } else {
        *out++ = (mode & 01) ? 'x' : '-';
    }
    *out = 0;

    return strmode;
}

int unpack_item(int idx, int fd, pack_header *header, char *buf_item_h, int se_info, int overlay) {
  int res = 0;
  int read_len = 0;
  pack_item_header *item_h = (pack_item_header *)buf_item_h;
  pack_item_data *item_d = NULL;
  int item_size2 = 0;
  char *item_path = NULL;
  struct stat st;
  int need_create = 1;

  read_len = read_all(fd, item_h, sizeof(pack_item_header));
  if (read_len != sizeof(pack_item_header)) {
    pack_error("read size is %d.\n", read_len);
    if (read_len < 0) {
      pack_error("read item[%d] header error:%s!!\n", idx, strerror(errno));
    } else {
      pack_error("can not get enough data for item[%d] header!!\n", idx);
    }
    res = 1;
    goto ret_p;
  }

  lseek(fd, item_h->item_header_len - sizeof(pack_item_header), SEEK_CUR);
  item_size2 = item_h->size2;
  item_d = (pack_item_data *)malloc(item_size2);
  read_len = read_all(fd, item_d, item_size2);
  if (read_len != item_size2) {
    pack_error("read size is %d.\n", read_len);
    if (read_len < 0) {
      pack_error("read item[%d] header error:%s!!\n", idx, strerror(errno));
    } else {
      pack_error("can not get enough data for item[%d] data!!\n", idx);
    }
    res = 1;
    goto ret_p;
  }

  item_path = item_d->data + item_d->path_off;
  //item_scon = item_d->data + item_d->scon_off;
  strcat(path_buffer, item_path);

  if (verbose) {
    pack_info("upack_item [%s], data len is %d\n",
        info_file(tmp_buffer, PATH_BUF_LEN, path_buffer, item_h, item_d), item_h->data_size);
  }
  res = lstat(path_buffer, &st);
  if (res == 0) {
    if (overlay) {
      remove_path(path_buffer, &st);
    } else {
      if ((st.st_mode&S_IFMT) == (item_h->mode&S_IFMT)
        && !S_ISLNK(item_h->mode)) {
        need_create = 0;
      } else {
        remove_path(path_buffer, &st);
      }
    }
  }

  if (need_create) {
    if (S_ISDIR(item_h->mode)) {
      if (verbose) {
        pack_info("create dir %s with mode %05o\n", path_buffer, item_h->mode & 07777);
      }
      res = mkdir(path_buffer, item_h->mode & 07777);
      if (res < 0) {
        pack_error("mkdir %s with %05o error:%s!!\n", path_buffer, item_h->mode & 07777, strerror(errno));
        goto ret_p;
      }
    } else if (S_ISREG(item_h->mode)) {
      if (verbose) {
        pack_info("create file %s with mode %05o\n", path_buffer, item_h->mode & 07777);
      }
      res = creat(path_buffer, item_h->mode & 07777);
      if (res < 0) {
        pack_error("creat %s with %05o error:%s!!\n", path_buffer, item_h->mode & 07777, strerror(errno));
        goto ret_p;
      }
      close(res);
    } else if (S_ISSOCK(item_h->mode) || S_ISFIFO(item_h->mode)
             || S_ISCHR(item_h->mode) || S_ISBLK(item_h->mode)) {
      unsigned short dev_val = 0;
      if (S_ISSOCK(item_h->mode) || S_ISFIFO(item_h->mode)) {
        dev_val = item_h->dev;
      } else if (S_ISCHR(item_h->mode) || S_ISBLK(item_h->mode)) {
        dev_val = item_h->rdev;
      }
      if (verbose) {
        pack_info("mknod %s with mode %08x and dev %08x\n", path_buffer, item_h->mode, dev_val);
      }
      res = mknod(path_buffer, item_h->mode, dev_val);
      if (res < 0) {
        pack_error("mknod %s with mode %08x and dev %08x error:%s!!\n", path_buffer, item_h->mode, dev_val, strerror(errno));
        goto ret_p;
      }
    }

  } else {
    if (verbose) {
      pack_info("no need to create %s!\n", path_buffer);
    }
    res = chmod(path_buffer, item_h->mode & 07777);
    if (res < 0) {
      pack_error("chmod %s as %05o error:%s!!\n", path_buffer, item_h->mode & 07777, strerror(errno));
      goto ret_p;
    }
  }
  if (!S_ISDIR(item_h->mode)) {
    res = upack_file(fd, path_buffer, item_h, item_d);
    if (res < 0) {
      goto ret_p;
    }
  }

  res = set_item_permissions(path_buffer, item_h, item_d, se_info);
  if (res < 0) {
    goto ret_p;
  }

  if( !S_ISLNK(item_h->mode)) {

    res = chmod(path_buffer, item_h->mode & 07777);
    if (res < 0) {
        pack_error("chmod %s as %05o error:%s!!\n", path_buffer, item_h->mode & 07777, strerror(errno));
        goto ret_p;
    }

  }

    char mode[16];
    res = lstat(path_buffer, &st);
    if (res == 0) {
        printf("info file :%s, size: %lld, uid: %u ,gid: %u,mode:%s\n",path_buffer,st.st_size,st.st_uid,st.st_gid,strmode(st.st_mode, mode));

    }

ret_p:
  if (item_d) {
    free(item_d);
  }
  return res;
}

int unpack(char *pak_file, char *dest_dir, int se_info, int overlay) {
  int res = 0;
  char *buffer = NULL;
  pack_header *header = NULL;
  int fd = -1;
  struct stat st;
  int read_len = 0;
  long file_len = 0;
  int item_index = 0;
  int path_len = 0;

  res = stat(dest_dir, &st);
  if (res < 0) {
    pack_error("stat path \'%s\' failed:%s\n", dest_dir, strerror(errno));
    res = 1;
    goto ret_p;
  }
  if (!S_ISDIR(st.st_mode)) {
    pack_error("path \'%s\' is not a directory!!\n", dest_dir);
    res = 1;
    goto ret_p;
  }

  fd = open(pak_file, O_RDONLY);
  if (fd < 0) {
    pack_error("open file %s error:%s\n", pak_file, strerror(errno));
    res = 1;
    goto ret_p;
  }

  if (se_info && is_selinux_enabled() <= 0) {
    se_info = 0;
  }

  buffer = (char *)malloc(sizeof(pack_header) + sizeof(pack_item_header));
  header = (pack_header *)buffer;

  file_len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  read_len = read_all(fd, header, sizeof(pack_header));
  if (read_len != sizeof(pack_header)) {
    pack_error("read size is %d.\n", read_len);
    if (read_len < 0) {
      pack_error("read pak_file \'%s\' header error:%s!!\n", pak_file, strerror(errno));
    } else {
      pack_error("can not get enough data for pak_file \'%s\' header!!\n", pak_file);
    }
    res = 1;
    goto ret_p;
  }

  if (memcmp(header->magic, PACK_MAGIC, strlen(PACK_MAGIC)) != 0) {
    pack_error("file \'%s\' is not a valid pak_file!!\n", pak_file);
    res = 1;
    goto ret_p;
  }

  if (file_len != header->pac_size) {
    pack_error("pak_file \'%s\' format error!!\n", pak_file);
    res = 1;
    goto ret_p;
  }

  get_abs_path(dest_dir);
  path_len = strlen(path_buffer);

  lseek(fd, header->header_len - sizeof(pack_header), SEEK_CUR);
  for(item_index = 0; item_index < header->items_count; item_index++) {
    res = unpack_item(item_index, fd, header, buffer+sizeof(pack_header), se_info, overlay);
    if (res != 0) {
      pack_error("unpack the %dth item error!!\n", item_index);
      res = 1;
      goto ret_p;
    }
    path_buffer[path_len] = '\0';
  }

  if (item_index == header->items_count) {
    pack_info("unpack \'%s\' success!!\n", pak_file);
  }

ret_p:
  if (fd >= 0) {
    close(fd);
  }
  if (buffer) {
    free(buffer);
  }
  return res;
}

