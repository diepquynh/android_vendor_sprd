/* Created by Spreadst */

#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <selinux/selinux.h>
#include <sys/capability.h>
#include <sys/xattr.h>
#include <linux/xattr.h>
#include <inttypes.h>

#ifndef __SPRD_PACK_H__
#define __SPRD_PACK_H__

enum {
  NO_FILTER,
  WHITE_LIST,
  BLACK_LIST,
};

int pack_info(const char *format, ...);
int pack_error(const char *format, ...);

void set_filter_mode(int mode);
void add_filter(char *base_dir, char *prefix);
void destory_filters();

// used for dump all filter info, just for debug
void dump_filter_info();
// used for check if all files in white list is packed, and just display information
int check_pack_file();

char *get_abs_path(char *path);

void set_verbose_on();
int get_verbose();

int pack(char *pak_file, char *src_dir, int se_info);
int unpack(char *pak_file, char *dest_dir, int se_info, int overlay);

#endif /* #ifndef __SPRD_PACK_H__ */
