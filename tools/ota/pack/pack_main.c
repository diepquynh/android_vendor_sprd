/* Created by Spreadst */

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "pack.h"

typedef struct i_f_i_s {
  char *prefix;
  struct i_f_i_s *next;
} init_filter_item;

static const struct option OPTIONS[] = {
  { "white_list", no_argument, NULL, 'w' },
  { "black_list", no_argument, NULL, 'b' },
  { "filter_path", required_argument, NULL, 'P' },
  { "with_selinux", no_argument, NULL, 'S' },
  { "source_path", required_argument, NULL, 's' },
  { "pack_file", required_argument, NULL, 'p' },
  { "verbose", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 },
};

static init_filter_item *init_filters = NULL;

static void add_init_filter(char *prefix) {
  init_filter_item *filter = (init_filter_item *) malloc(sizeof(init_filter_item));
  filter->prefix = prefix;
  filter->next = NULL;
  if (init_filters == NULL) {
    init_filters = filter;
  } else {
    filter->next = init_filters;
    init_filters = filter;
  }
}

static void process_init_filters(char *base_dir) {
  init_filter_item *filter = init_filters;
  init_filter_item *next_filter = NULL;
  while (filter) {
    next_filter = filter->next;
    add_filter(base_dir, filter->prefix);
    filter = next_filter;
  }
}

static void destory_init_filters() {
  init_filter_item *filter = init_filters;
  init_filter_item *next_filter = NULL;

  if (init_filters == NULL) {
    return;
  }

  while (filter) {
    next_filter = filter->next;
    free(filter);
    filter = next_filter;
  }
  init_filters = NULL;
}

static char *base_name(char *path) {
  char *dup_path = strdup(path);
  char *p = strrchr(dup_path, '/');
  if (p == (dup_path + strlen(dup_path) - 1)) {
    *p = 0;
    p = strrchr(dup_path, '/');
  }
  if (p) {
    char *name = strdup(p + 1);
    free(dup_path);
    return name;
  } else {
    return dup_path;
  }
}

/*********for*pack*args********/

static int pack_usage()
{
  pack_error("usage: pack [OPTIONS] <-s src_dir> <-p pak_file>\n"
        "    -w: use while list\n"
        "    -b: use black list\n"
        "    -P: filter path prefix\n"
        "    -S: enable selinux info\n"
        "    -s: source directory\n"
        "    -p: package file path\n");
    return 1;
}

int pack_main(int argc, char** argv) {
  int res = 0;
  int se_info = 0;
  char *pak_file = NULL;
  char *src_dir = NULL;
  char *abs_dir = NULL;

  int arg;
  while ((arg = getopt_long(argc, argv, "wbSP:s:p:v", OPTIONS, NULL)) != -1) {
    switch (arg) {
    case 'w': set_filter_mode(WHITE_LIST); break;
    case 'b': set_filter_mode(BLACK_LIST); break;
    case 'S': se_info = 1; break;
    case 'P': add_init_filter(optarg); break;
    case 's': src_dir = optarg; break;
    case 'p': pak_file = optarg; break;
    case 'v': set_verbose_on(); break;
    case '?':
    default:
      pack_error("Invalid command argument \'%s\'.\n", optarg);
      res = 1;
      continue;
    }
  }
  if (res) {
    pack_usage();
    goto ret_p;
  }

  if (pak_file == NULL || src_dir == NULL) {
    pack_error("no pack_file or src_dir.\n");
    pack_usage();
    res = 1;
    goto ret_p;
  }

  abs_dir = get_abs_path(src_dir);
  process_init_filters(abs_dir);
  destory_init_filters();

  if (get_verbose()) {
    pack_info("pack \'%s\' to \'%s\' %s selinux info\n", src_dir, pak_file, (se_info?"with":"no"));
    dump_filter_info();
  }

  res = pack(pak_file, abs_dir, se_info);
  check_pack_file();

  destory_filters();
ret_p:
  return res;
}

/*********for*unpack*args********/

static int unpack_usage()
{
  pack_error("usage: unpack [OPTIONS] <-p pak_file>\n"
        "    -S: enable selinux info\n"
        "    -p: package file path\n"
        "    -o: overlay mode, if path exist, just remove\n");
    return 1;
}

int unpack_main(int argc, char** argv) {
  int res = 0;
  int se_info = 0;
  int overlay = 0;
  char *pak_file = NULL;
  char *dest_dir = strdup("/");

  int arg;
  while ((arg = getopt_long(argc, argv, "Sp:d:ov", OPTIONS, NULL)) != -1) {
    switch (arg) {
    case 'S': se_info = 1; break;
    case 'p': pak_file = optarg; break;
    case 'd': dest_dir = strdup(optarg); break;
    case 'o': overlay = 1; break;
    case 'v': set_verbose_on(); break;
    case '?':
    default:
      pack_error("Invalid command argument \'%s\'.\n", optarg);
      res = 1;
      continue;
    }
  }
  if (res) {
    unpack_usage();
    goto ret_p;
  }

  if (pak_file == NULL) {
    unpack_usage();
    res = 1;
    goto ret_p;
  }

  if (get_verbose()) {
    pack_info("unpack \'%s\' to \'%s\' %s selinux info\n", pak_file, dest_dir, (se_info?"with":"no"));
  }

  res = unpack(pak_file, dest_dir, se_info, overlay);

ret_p:
  free(dest_dir);
  return res;
}

int main(int argc, char** argv) {
  int res = 0;
  char *arg0_base = NULL;

  uid_t uid;
  gid_t gid;
  printf("umask:%04o\n", umask(0000));
  uid=getuid();
  gid=getgid();
  printf("uid:%u gid:%u\n", uid, gid);

  arg0_base = base_name(argv[0]);
  if (strcmp(arg0_base, "pack") == 0) {
    res = pack_main(argc, argv);
  } else if (strcmp(arg0_base, "unpack") == 0) {
    res = unpack_main(argc, argv);
  } else {
    if (argc < 2) {
      pack_error("please given at least one parementer: pack or unpack!\n");
      res = 1;
    } else if (strcmp(argv[1], "pack") == 0) {
      res = pack_main(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "unpack") == 0) {
      res = unpack_main(argc - 1, argv + 1);
    } else {
      pack_error("arguments error!\n");
      res = 1;
    }
  }

  free(arg0_base);

  return res;
}

