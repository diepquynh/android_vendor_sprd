/*
 * Authors:<jinglong.chen@spreadtrum.com>
 * Owner:
 */


#include "wlnpi.h"

#include <android/log.h>
#include <utils/Log.h>

#define WLAN_NL_GENERAL_SOCK_ID 1022
#define WLNPI_WLAN0_NAME            ("wlan0")

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG     "WLNPI"

#define ENG_LOG  ALOGD

wlnpi_t g_wlnpi = {0};
static unsigned char  *g_r_buf = NULL;
static unsigned int   *g_r_len = NULL;

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,  void *arg)
{
    int *ret = arg;
    *ret = err->error;
    return NL_STOP;
}
static int finish_handler(struct nl_msg *msg, void *arg)
{
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}
static int ack_handler(struct nl_msg *msg, void *arg)
{
    int *ret = arg;
    *ret = 0;
    return NL_STOP;
}

static int nl_reply_handler(struct nl_msg *msg, void *arg)
{
    unsigned char str[64] = {0};
    unsigned short len;
    unsigned char  *data;
    struct nlattr *tb_msg[WLAN_NL_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    if( (NULL == g_r_buf)  ||  (NULL == g_r_len)  )
    {
        printf("[%s][%d][ERR]\n", __func__, __LINE__);
    }
    nla_parse(tb_msg, WLAN_NL_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if (tb_msg[WLAN_NL_ATTR_COMMON_DRV_TO_USR])
    {
        len =  nla_len(tb_msg[WLAN_NL_ATTR_COMMON_DRV_TO_USR]);
        data = (unsigned char *)nla_data(tb_msg[WLAN_NL_ATTR_COMMON_DRV_TO_USR]);
        if(len > *g_r_len)
        {
            printf("[%s][%d][%d][ERR]\n", __func__, __LINE__, len);
            *g_r_len = -1;
            return NL_SKIP;
        }
        *g_r_len = len;         
        memcpy(g_r_buf,data, *g_r_len );
    }
    else
    {
        printf("%s faild\n", __func__);
    }
    return NL_SKIP;
}

static int nl_send_recv_msg(wlnpi_t *wlnpi, unsigned char *s_buf, unsigned short s_len, unsigned char *r_buf, unsigned int *r_len )
{
    struct nl_cb *cb;
    struct nl_cb *s_cb;
    struct nl_msg *msg;
    int ret;
    msg = nlmsg_alloc();
    if (!msg)
    {
        printf("failed to allocate netlink message\n");
        return 2;
    }
    cb = nl_cb_alloc(NL_CB_DEBUG );//NL_CB_DEFAULT
    s_cb = nl_cb_alloc(NL_CB_DEBUG);//NL_CB_DEFAULT
    if (!cb || !s_cb)
    {
        printf("failed to allocate netlink callbacks\n");
        ret = 2;
        goto out_free_msg;
    }
    genlmsg_put(msg, 0, 0, wlnpi->nl_id, 0, 0, /*WLAN_NL_CMD_NPI*/wlnpi->nl_cmd_id, 0);
    NLA_PUT(msg, WLAN_NL_ATTR_COMMON_USR_TO_DRV, s_len, s_buf);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl_reply_handler, NULL);

    nl_socket_set_cb(wlnpi->sock, s_cb);
    ret = nl_send_auto_complete(wlnpi->sock, msg);
    if (ret < 0)
    {
        goto out;
    }
    ret = 1;
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
    g_r_buf = r_buf;
    g_r_len = r_len;

	while (ret > 0)
		nl_recvmsgs(wlnpi->sock, cb);
out:
    nl_cb_put(cb);
out_free_msg:
    nlmsg_free(msg);
	free(cb);
    free(s_cb);
    return ret;
nla_put_failure:
    printf("building message failed\n");

    free(cb);
    free(s_cb);
    return 2;
}

static int wlnpi_handle_special_cmd(struct wlnpi_cmd_t *cmd)
{
    ENG_LOG("ADL entry %s(), cmd = %s", __func__, cmd->name);
    if (0 == strcmp(cmd->name, WLNPI_CMD_CONN_STATUS_STR))
    {
        printf("not connected AP\n");
        ENG_LOG("ADL leval %s(), return 1", __func__);

        return 1;
    }

    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}
static int get_drv_info(wlnpi_t *wlnpi, struct wlnpi_cmd_t *cmd)
{
    int ret;
    unsigned char  s_buf[4]  ={0};
    unsigned short s_len     = 4;
    unsigned char  r_buf[32] ={0};
    unsigned short r_len     = 0;
    memset(s_buf, 0xFF, 4);
    wlnpi->nl_cmd_id = WLAN_NL_CMD_GET_INFO;
    ret = nl_send_recv_msg(wlnpi, s_buf, s_len, r_buf, &r_len );
    wlnpi->nl_cmd_id = WLAN_NL_CMD_NPI;
    if(6 != r_len)
    {
        if (1 == wlnpi_handle_special_cmd(cmd))
        {
            ENG_LOG("ADL levaling %s(), return -2", __func__);
            return -2;
        }

        printf("get drv info err\n");
        return -1;
    }
    memcpy(&(wlnpi->mac[0]), r_buf, 6 );
    return 0;
}

static int wlan_nl_init(wlnpi_t *wlnpi)
{
    int ret;
    wlnpi->sock = nl_socket_alloc();
    if (!wlnpi->sock)
    {
        printf("Failed to allocate netlink socket.\n");
        return -ENOMEM;
    }
    nl_socket_set_buffer_size(wlnpi->sock, 4096, 4096);
    if (genl_connect(wlnpi->sock))
    {
        printf("Failed to connect to generic netlink. drv not insmod\n");
        ret = -ENOLINK;
        goto out_handle_destroy;
    }
    wlnpi->nl_id = WLAN_NL_GENERAL_SOCK_ID;
    wlnpi->nl_cmd_id = WLAN_NL_CMD_NPI;
    return 0;

out_handle_destroy:
    nl_socket_free(wlnpi->sock);
    return ret;
}

static void wlan_nl_deinit(wlnpi_t *wlnpi)
{
    nl_socket_free(wlnpi->sock);
}

extern void do_help(void);

int main(int argc, char **argv)
{
    int i, ret;
    unsigned short       s_len = 0;
    unsigned short       r_len = 0; 
    unsigned char        s_buf[1024] = {0};
    unsigned char        r_buf[1024] = {0};
    struct wlnpi_cmd_t  *cmd = NULL;
    WLNPI_CMD_HDR_T     *msg = NULL;
    wlnpi_t             *wlnpi = &g_wlnpi;
    int                 *status = NULL;

    argc--;
    argv++;

    if (0 == strcmp(argv[0], WLNPI_WLAN0_NAME))
    {
        /* skip "wlan0" */
        argc--;
        argv++;
    }

    if(argc < 1)
    {
        do_help();
        return -1;
    }

    cmd = match_cmd_table(argv[0]);
    if(NULL == cmd)
    {
        printf("[%s][not match]\n", argv[0]);
        return -1;
    }
    
    ret = wlan_nl_init(wlnpi);
    if(0 != ret)
        return -1;

    ret = get_drv_info(wlnpi, cmd);
    if(0 != ret)
        goto EXIT;

    argc--;
    argv++;
    if(NULL == cmd->parse)
    {
        printf("func null\n");
    }
    
    ret = cmd->parse(argc, argv, s_buf + sizeof(WLNPI_CMD_HDR_T), &s_len );
    if(0 != ret)
    {
        printf("%s\n", cmd->help);
        goto EXIT;
    }   
    
    msg = (WLNPI_CMD_HDR_T *)s_buf;
    msg->type = HOST_TO_MARLIN_CMD;
    msg->subtype = cmd->id; 
    msg->len = s_len;
    s_len = s_len + sizeof(WLNPI_CMD_HDR_T);
    ret = nl_send_recv_msg(wlnpi, s_buf, s_len, r_buf, &r_len);
    msg = r_buf;
    
    if( (MARLIN_TO_HOST_REPLY != msg->type) || (cmd->id != msg->subtype) || (r_len < sizeof(WLNPI_CMD_HDR_T) + sizeof(int) ) )
    {
        printf("communication error\n");
        printf("msg->type = %d, cmd->id = %d, subtype = %d, r_len = %d\n", msg->type, cmd->id, msg->subtype, r_len);
        goto EXIT;
    }
    
    status = (int *)( r_buf + sizeof(WLNPI_CMD_HDR_T) );
    if(-100 == *status)
    {
        printf("marlin not realize\n");
        goto EXIT;
    }

    printf("ret: status %d :end\n",  *status);
    cmd->show(cmd, r_buf + sizeof(WLNPI_CMD_HDR_T) + sizeof(int ), r_len - sizeof(WLNPI_CMD_HDR_T) - sizeof(int ) );
    
EXIT:
    wlan_nl_deinit(wlnpi);
    return 0;
}



