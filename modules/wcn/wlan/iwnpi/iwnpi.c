/*
 * nlnpi userspace tool
 *
 * Wenjie.Zhang <Wenjie.Zhang@spreadtrum.com>
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <utils/Log.h>
#include <android/log.h>
/* Callback Handler */
#include <netlink/object-api.h>
#include <linux/socket.h>
#include <linux/pkt_sched.h>
#include <netlink-types.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nlnpi.h"
#include "iwnpi.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "IWNPI"

static const char *argv0;
int iwnpi_debug = 0;

static int cmd_size;
extern char ret_result_buf[WLNPI_RES_BUF_LEN + 1];

extern struct cmd __start___cmd;
extern struct cmd __stop___cmd;

#define for_each_cmd(_cmd)                          \
  for (_cmd = &__start___cmd; _cmd < &__stop___cmd; \
       _cmd = (const struct cmd *)((char *)_cmd + cmd_size))

static void __usage_cmd(const struct cmd *cmd, char *indent, bool full) {
  const char *start, *lend, *end;

  printf("%s", indent);
  printf("%s", cmd->name);

  if (cmd->args) {
    /* print line by line */
    start = cmd->args;
    end = strchr(start, '\0');
    printf(" ");
    do {
      lend = strchr(start, '\n');
      if (!lend) lend = end;
      if (start != cmd->args) {
        printf("\t");
        printf("%s ", cmd->name);
      }
      printf("%.*s\n", (int)(lend - start), start);
      start = lend + 1;
    } while (end != lend);
  } else
    printf("\n");

  if (!full || !cmd->help) return;

  /* hack */
  if (strlen(indent))
    indent = "\t\t";
  else
    printf("\n");

  /* print line by line */
  start = cmd->help;
  end = strchr(start, '\0');
  do {
    lend = strchr(start, '\n');
    if (!lend) lend = end;
    printf("%s", indent);
    printf("%.*s\n", (int)(lend - start), start);
    start = lend + 1;
  } while (end != lend);

  printf("\n");
}

static void usage_options(void) {
  printf("Options:\n");
  printf("\t--debug\t\tenable netlink debugging\n");
}

static void usage(int argc, char **argv) {
  const struct cmd *cmd;
  bool full = argc >= 0;
  const char *sect_filt = NULL;
  const char *cmd_filt = NULL;

  if (argc > 0) sect_filt = argv[0];

  if (argc > 1) cmd_filt = argv[1];

  printf("Usage:\t%s [wlanX] command\n", argv0);
  usage_options();
  printf("Commands:\n");
  for_each_cmd(cmd) {
    if (sect_filt && strcmp(cmd->name, sect_filt)) continue;

    if (cmd->handler) __usage_cmd(cmd, "\t", full);
  }
}

static void usage_cmd(const struct cmd *cmd) {
  printf("Usage:\t%s [options] ", argv0);
  __usage_cmd(cmd, "", true);
  usage_options();
}

/* Callback Handler */
/*void nl_socket_set_cb(struct nl_sock *sk, struct nl_cb *cb)
{
        nl_cb_put(sk->s_cb);
        sk->s_cb = nl_cb_get(cb);
}*/

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                         void *arg) {
  int *ret = arg;
  *ret = err->error;

  return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg) {
  int *ret = arg;
  *ret = 0;

  return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg) {
  int *ret = arg;
  *ret = 0;

  return NL_STOP;
}

static int __handle_cmd(struct nlnpi_state *state, int argc, char **argv,
                        const struct cmd **cmdout) {
  const struct cmd *cmd, *match = NULL;
  struct nl_cb *cb;
  struct nl_cb *s_cb;
  struct nl_msg *msg;
  int err, o_argc;
  const char *command;
  char **o_argv;
  signed long long devidx = 0;

  if (argc < 1) return 1;

  o_argc = argc;
  o_argv = argv;

  devidx = if_nametoindex(*argv);
  if (devidx == 0) devidx = -1;
  argc--;
  argv++;

  command = *argv;
  argc--;
  argv++;

  for_each_cmd(cmd) {
    if (match) continue;
    if (strcmp(cmd->name, command) == 0) match = cmd;
  }

  cmd = match;
  if (!cmd) return 1;
  if (argc && !cmd->args) return 1;
  if (!cmd->handler) return 1;

  if (cmdout) *cmdout = cmd;

  if (!cmd->cmd) {
    argc = o_argc;
    argv = o_argv;
    return cmd->handler(state, NULL, NULL, argc, argv);
  }

  msg = nlmsg_alloc();
  if (!msg) {
    fprintf(stderr, "failed to allocate netlink message\n");
    return 2;
  }

  cb = nl_cb_alloc(iwnpi_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
  s_cb = nl_cb_alloc(iwnpi_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
  if (!cb || !s_cb) {
    fprintf(stderr, "failed to allocate netlink callbacks\n");
    err = 2;
    goto out_free_msg;
  }

  genlmsg_put(msg, 0, 0, state->nlnpi_id, 0, cmd->nl_msg_flags, cmd->cmd, 0);
  NLA_PUT_U32(msg, NLNPI_ATTR_IFINDEX, devidx);

  err = cmd->handler(state, cb, msg, argc, argv);
  if (err) goto out;

  nl_socket_set_cb(state->nl_sock, s_cb);

  err = nl_send_auto_complete(state->nl_sock, msg);
  if (err < 0) goto out;

  err = 1;

  nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
  nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
  nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

  while (err > 0) nl_recvmsgs(state->nl_sock, cb);
out:
  nl_cb_put(cb);
out_free_msg:
  nlmsg_free(msg);
  return err;
nla_put_failure:
  fprintf(stderr, "building message failed\n");
  return 2;
}

static int nlnpi_init(struct nlnpi_state *state) {
  int err;

  state->nl_sock = nl_socket_alloc();
  if (!state->nl_sock) {
    fprintf(stderr, "Failed to allocate netlink socket.\n");
    return -ENOMEM;
  }

  nl_socket_set_buffer_size(state->nl_sock, 4096, 4096);

  if (genl_connect(state->nl_sock)) {
    fprintf(stderr, "Failed to connect to generic netlink.\n");
    err = -ENOLINK;
    goto out_handle_destroy;
  }
  /* Android does not support genl_strl_resolve */
  /*	state->nlnpi_id = genl_ctrl_resolve(state->nl_sock, "nlnpi");*/
  state->nlnpi_id = NL_GENERAL_NPI_ID;
  if (state->nlnpi_id < 0) {
    fprintf(stderr, "nlnpi not found.\n");
    err = -ENOENT;
    goto out_handle_destroy;
  }

  return 0;

out_handle_destroy:
  nl_socket_free(state->nl_sock);
  return err;
}

static void nlnpi_cleanup(struct nlnpi_state *state) {
  nl_socket_free(state->nl_sock);
}

int main(int argc, char **argv) {
  struct nlnpi_state nlstate;
  int err;
  const struct cmd *cmd = NULL;
  int fd = -1;

  /* calculate command size including padding */
  cmd_size = abs((long)&__section_set - (long)&__section_get);
  /* strip off self */
  argc--;
  argv0 = *argv++;

  if (argc > 0 && strcmp(*argv, "--debug") == 0) {
    iwnpi_debug = 1;
    argc--;
    argv++;
  }

  /* need to treat "help" command specially so it works w/o nl80211 */
  if (argc == 0 || strcmp(*argv, "help") == 0) {
    usage(argc - 1, argv + 1);
    return 0;
  }

  err = nlnpi_init(&nlstate);
  if (err) return 1;

  if (strncmp(*argv, "wlan", 4) == 0 && argc > 1) {
    err = __handle_cmd(&nlstate, argc, argv, &cmd);
  } else {
    fprintf(stderr, "wireless dev start with wlan\n");
    err = 1;
  }

  if (NULL !=
      (fd = open(IWNPI_EXEC_TMP_FILE, O_RDWR | O_CREAT | O_SYNC | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))) {
    int write_cnt = 0;
    write_cnt = write(fd, ret_result_buf, sizeof(ret_result_buf));
    close(fd);
  } else {
    ALOGD("open error=%d, %s\n", errno, strerror(errno));
  }

  if (err == 1) {
    if (cmd)
      usage_cmd(cmd);
    else
      usage(0, NULL);
  } else if (err < 0)
    fprintf(stderr, "command failed: %s (%d)\n", strerror(-err), err);

  nlnpi_cleanup(&nlstate);

  return err;
}
