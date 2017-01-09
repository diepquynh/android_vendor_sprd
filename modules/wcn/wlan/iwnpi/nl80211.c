#include <errno.h>

#include "common.h"

extern unsigned int g_channelist[];

#ifndef CONFIG_LIBNL20
/*
 * libnl 1.1 has a bug, it tries to allocate socket numbers densely
 * but when you free a socket again it will mess up its bitmap and
 * and use the wrong number the next time it needs a socket ID.
 * Therefore, we wrap the handle alloc/destroy and add our own pid
 * accounting.
 */
uint32_t port_bitmap[32] = { 0 };
inline struct nl_handle *nl80211_handle_alloc(void *cb)
{
	struct nl_handle *handle;
	uint32_t pid = getpid() & 0x3FFFFF;
	int i;

	handle = nl_handle_alloc_cb(cb);

	for (i = 0; i < 1024; i++) {
		if (port_bitmap[i / 32] & (1 << (i % 32)))
			continue;
		port_bitmap[i / 32] |= 1 << (i % 32);
		pid += i << 22;
		break;
	}

	nl_socket_set_local_port(handle, pid);

	return handle;
}

inline void nl80211_handle_destroy(struct nl_handle *handle)
{
	uint32_t port = nl_socket_get_local_port(handle);

	port >>= 22;
	port_bitmap[port / 32] &= ~(1 << (port % 32));

	nl_handle_destroy(handle);
}
#endif
/* nl80211 code */
static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *err = arg;

	msg = msg;
	*err = 0;

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;

	msg = msg;
	*ret = 0;

	return NL_SKIP;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;

	nla = nla;
	*ret = err->error;
	if(*ret)
		SSPRINT( "%s-> %d -> (%s)\n",__func__,*ret, strerror(-(*ret)));

	return NL_SKIP;
}


static int no_seq_check(struct nl_msg *msg, void *arg)
{
	msg = msg;
	arg = arg;

	return NL_OK;
}
void send_scan_event(struct nlattr *tb[])
{
	struct nlattr *nl;
	int rem;
	int num_ssids = 0,num_freqs = 0;
	int freqs[MAX_REPORT_FREQS];
	int ssid_len;
	u8 *ssid;

	if (tb[NL80211_ATTR_SCAN_SSIDS]) {
		nla_for_each_nested(nl, tb[NL80211_ATTR_SCAN_SSIDS], rem) {
			ssid_len = nla_len(nl);
			ssid = nla_data(nl);
			num_ssids++;
			if (num_ssids == WPAS_MAX_SCAN_SSIDS)
				break;
		}
	}
	if (tb[NL80211_ATTR_SCAN_FREQUENCIES]) {
		nla_for_each_nested(nl, tb[NL80211_ATTR_SCAN_FREQUENCIES], rem)
		{
			freqs[num_freqs] = nla_get_u32(nl);
			num_freqs++;
			if (num_freqs == MAX_REPORT_FREQS - 1)
				break;
		}
	}
}
static struct nl_handle * nl_create_handle(struct nl_cb *cb, const char *dbg)
{
	struct nl_handle *handle;

	handle = nl80211_handle_alloc(cb);
	if (handle == NULL) {
		SSPRINT( "nl80211: Failed to allocate netlink "
			   "callbacks (%s)", dbg);
		return NULL;
	}

	if (genl_connect(handle)) {
		SSPRINT( "nl80211: Failed to connect to generic "
			   "netlink (%s)", dbg);
		nl80211_handle_destroy(handle);
		return NULL;
	}

	return handle;
}


static void nl_destroy_handles(struct nl_handle **handle)
{
	if (*handle == NULL)
		return;
	nl80211_handle_destroy(*handle);
	*handle = NULL;
}

static int send_and_recv(struct nl80211_global *global,
			 struct nl_handle *nl_handle, struct nl_msg *msg,
			 int (*valid_handler)(struct nl_msg *, void *),
			 void *valid_data)
{
	struct nl_cb *cb;
	int err = -1;

	cb = nl_cb_clone(global->nl_cb);
	if (!cb)
		goto out;

	err = nl_send_auto_complete(nl_handle, msg);
	if (err < 0){
		SSPRINT("send cmd error\n");
		goto out;
	}
	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	if (valid_handler)
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
			  valid_handler, valid_data);

	while (err > 0)
		nl_recvmsgs(nl_handle, cb);
	if(err){
		SSPRINT("receive reply error\n");
	}
 out:
	nl_cb_put(cb);
	nlmsg_free(msg);
	return err;
}
static int send_and_recv_msgs_global(struct nl80211_global *global,
				     struct nl_msg *msg,
				     int (*valid_handler)(struct nl_msg *, void *),
				     void *valid_data)
{
	return send_and_recv(global, global->nl, msg, valid_handler,
			     valid_data);
}

static int family_handler(struct nl_msg *msg, void *arg)
{
	struct family_data *res = arg;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *mcgrp;
	int i;

	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i) {
		struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
		nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, nla_data(mcgrp),
			  nla_len(mcgrp), NULL);
		if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb2[CTRL_ATTR_MCAST_GRP_ID] ||
		    strncmp(nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]),
			       res->group,
			       nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME])) != 0)
			continue;
		res->id = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	};

	return NL_SKIP;
}
static int nl_get_multicast_id(struct nl80211_global *global,
			       const char *family, const char *group)
{
	struct nl_msg *msg;
	int ret = -1;
	struct family_data res = { group, -1 };

	msg = nlmsg_alloc();
	if (!msg)
		return -1;
	genlmsg_put(msg, 0, 0, genl_ctrl_resolve(global->nl, "nlctrl"),
		    0, 0, CTRL_CMD_GETFAMILY, 0);
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = send_and_recv_msgs_global(global, msg, family_handler, &res);
	msg = NULL;
	if (ret == 0)
		ret = res.id;

nla_put_failure:
	nlmsg_free(msg);
	return ret;
}
#ifdef ANDROID
static int android_genl_ctrl_resolve(struct nl_handle *handle,
				     const char *name)
{
	/*
	 * Android ICS has very minimal genl_ctrl_resolve() implementation, so
	 * need to work around that.
	 */
	struct nl_cache *cache = NULL;
	struct genl_family *nl80211 = NULL;
	int id = -1;

	if (genl_ctrl_alloc_cache(handle, &cache) < 0) {
		SSPRINT( "nl80211: Failed to allocate generic "
			   "netlink cache");
		goto fail;
	}

	nl80211 = genl_ctrl_search_by_name(cache, name);
	if (nl80211 == NULL)
		goto fail;

	id = genl_family_get_id(nl80211);

fail:
	if (nl80211)
		genl_family_put(nl80211);
	if (cache)
		nl_cache_free(cache);

	return id;
}
#define genl_ctrl_resolve android_genl_ctrl_resolve
#endif /* ANDROID */
int linux_set_iface_flags(int sock, const char *ifname, int dev_up)
{
	struct ifreq ifr;
	int ret;

	if (sock < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
#ifdef TIZEN_EXT
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
#else
	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
#endif

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		SSPRINT("Could not read interface %s flags: %s",
			   ifname, strerror(errno));
		return -1;
	}

	if (dev_up) {
		if (ifr.ifr_flags & IFF_UP)
			return 0;
		ifr.ifr_flags |= IFF_UP;
	} else {
		if (!(ifr.ifr_flags & IFF_UP))
			return 0;
		ifr.ifr_flags &= ~IFF_UP;
	}

	if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
		SSPRINT("Could not set interface %s flags (%s): "
			   "%s",
			   ifname, dev_up ? "UP" : "DOWN", strerror(errno));
		return -1;
	}

	return 0;
}
int iw_nl80211_init(struct nl80211_global *global)
{
	int ret;

	memset(global,0,sizeof(*global));

	global->ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (global->ioctl_sock < 0) {
		SSPRINT("socket(PF_INET,SOCK_DGRAM)");
		return -1;
	}
	global->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (global->nl_cb == NULL) {
		SSPRINT( "nl80211: Failed to allocate netlink "
			   "callbacks");
		return -1;
	}

	global->nl = nl_create_handle(global->nl_cb, "nl");
	if (global->nl == NULL)
		goto err;

	global->nl80211_id = genl_ctrl_resolve(global->nl, "nl80211");
	if (global->nl80211_id < 0) {
		SSPRINT( "nl80211: 'nl80211' generic netlink not "
			   "found");
		goto err;
	}

	global->nl_event = nl_create_handle(global->nl_cb, "event");
	if (global->nl_event == NULL)
		goto err;

	ret = nl_get_multicast_id(global, "nl80211", "scan");
	if (ret >= 0)
		ret = nl_socket_add_membership(global->nl_event, ret);
	if (ret < 0) {
		SSPRINT( "nl80211: Could not add multicast "
			   "membership for scan events: %d (%s)",
			   ret, strerror(-ret));
		goto err;
	}
	ret = nl_get_multicast_id(global, "nl80211", "mlme");
	if (ret >= 0)
		ret = nl_socket_add_membership(global->nl_event, ret);
	if (ret < 0) {
		SSPRINT( "nl80211: Could not add multicast "
			   "membership for mlme events: %d (%s)",
			   ret, strerror(-ret));
		goto err;
	}

	ret = nl_get_multicast_id(global, "nl80211", "regulatory");
	if (ret >= 0)
		ret = nl_socket_add_membership(global->nl_event, ret);
	if (ret < 0) {
		SSPRINT("nl80211: Could not add multicast "
			   "membership for regulatory events: %d (%s)",
			   ret, strerror(-ret));
		/* Continue without regulatory events */
	}
	nl_cb_set(global->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM,
		  no_seq_check, NULL);
	nl_cb_set(global->nl_cb, NL_CB_VALID, NL_CB_CUSTOM,
		  process_global_event, global);
	/*get index*/
	global->ifindex = if_nametoindex("wlan0");

	nl_socket_set_cb(global->nl_event,global->nl_cb);

	return 0;

err:
	nl_destroy_handles(&global->nl_event);
	nl_destroy_handles(&global->nl);
	nl_cb_put(global->nl_cb);
	global->nl_cb = NULL;
	return -1;
}
void nl80211_iw_cleanup(struct nl80211_global *global)
{
	nl_destroy_handles(&global->nl_event);
	nl_destroy_handles(&global->nl);
	nl_cb_put(global->nl_cb);
	global->nl_cb = NULL;

	if (global->ioctl_sock >= 0)
		close(global->ioctl_sock);

	memset(global,0,sizeof(*global));
}
void set_join_ssid(struct nl80211_global *global ,const char *ssid)
{
	global->find = false;
	global->ssid_len = strlen(ssid);
	memcpy(global->ssid,ssid,global->ssid_len);
}
static int send_and_recv_msgs(struct nl80211_global *global,
		struct nl_msg *msg,
		int (*valid_handler)(struct nl_msg *, void *),
		void *valid_data)
{
	return send_and_recv(global, global->nl, msg,
			valid_handler, valid_data);
}

static void * nl80211_cmd(struct nl80211_global *global,
		                          struct nl_msg *msg, int flags, uint8_t cmd)
{
	return genlmsg_put(msg, 0, 0, global->nl80211_id,
				                           0, flags, cmd, 0);

}

void wpa_scan_results_free(struct wpa_scan_results *res)
{
	size_t i;

	if (res == NULL)
		return;

	for (i = 0; i < res->num; i++)
		free(res->res[i]);
	free(res->res);
	free(res);
}
void * os_zalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}
static inline void * os_realloc_array(void *ptr, size_t nmemb, size_t size)
{
	if (size && nmemb > (~(size_t) 0) / size)
		return NULL;
	return realloc(ptr, nmemb * size);
}
static const u8 * nl80211_get_ie(const u8 *ies, size_t ies_len, u8 ie)
{
	const u8 *end, *pos;

	if (ies == NULL)
		return NULL;

	pos = ies;
	end = ies + ies_len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == ie)
			return pos;
		pos += 2 + pos[1];
	}

	return NULL;
}
static int bss_info_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_BSSID] = { .type = NLA_UNSPEC },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { .type = NLA_UNSPEC },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { .type = NLA_UNSPEC },
	};
	struct nl80211_bss_info_arg *_arg = arg;
	struct wpa_scan_results *res = _arg->res;
	struct nl80211_global *global = _arg->global;
	struct wpa_scan_res **tmp;
	struct wpa_scan_res *r;
	const u8 *ie, *beacon_ie;
	size_t ie_len, beacon_ie_len;
	u8 *pos;
	size_t i;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	if (!tb[NL80211_ATTR_BSS])
		return NL_SKIP;
	if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS],
			     bss_policy))
		return NL_SKIP;
	if (bss[NL80211_BSS_STATUS]) {
		enum nl80211_bss_status status;
		status = nla_get_u32(bss[NL80211_BSS_STATUS]);
		if (status == NL80211_BSS_STATUS_ASSOCIATED &&
		    bss[NL80211_BSS_FREQUENCY]) {
			_arg->assoc_freq =
				nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
			SSPRINT( "nl80211: Associated on %u MHz\n",
				   _arg->assoc_freq);
		}
		if (status == NL80211_BSS_STATUS_ASSOCIATED &&
		    bss[NL80211_BSS_BSSID]) {
			memcpy(_arg->assoc_bssid,
				  nla_data(bss[NL80211_BSS_BSSID]), ETH_ALEN);
			SSPRINT("nl80211: Associated with "
				   MACSTR "\n", MAC2STR(_arg->assoc_bssid));
		}
	}
	if (!res)
		return NL_SKIP;
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		ie = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		ie_len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
	} else {
		ie = NULL;
		ie_len = 0;
	}
	if (bss[NL80211_BSS_BEACON_IES]) {
		beacon_ie = nla_data(bss[NL80211_BSS_BEACON_IES]);
		beacon_ie_len = nla_len(bss[NL80211_BSS_BEACON_IES]);
	} else {
		beacon_ie = NULL;
		beacon_ie_len = 0;
	}
	r = os_zalloc(sizeof(*r) + ie_len + beacon_ie_len);
	if (r == NULL)
		return NL_SKIP;
	if (bss[NL80211_BSS_BSSID])
		memcpy(r->bssid, nla_data(bss[NL80211_BSS_BSSID]),
			  ETH_ALEN);
	if (bss[NL80211_BSS_FREQUENCY])
		r->freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
	if (bss[NL80211_BSS_BEACON_INTERVAL])
		r->beacon_int = nla_get_u16(bss[NL80211_BSS_BEACON_INTERVAL]);
	if (bss[NL80211_BSS_CAPABILITY])
		r->caps = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
	r->flags |= WPA_SCAN_NOISE_INVALID;
	if (bss[NL80211_BSS_SIGNAL_MBM]) {
		r->level = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
		r->level /= 100; /* mBm to dBm */
		r->flags |= WPA_SCAN_LEVEL_DBM | WPA_SCAN_QUAL_INVALID;
	} else if (bss[NL80211_BSS_SIGNAL_UNSPEC]) {
		r->level = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
		r->flags |= WPA_SCAN_QUAL_INVALID;
	} else
		r->flags |= WPA_SCAN_LEVEL_INVALID | WPA_SCAN_QUAL_INVALID;
	if (bss[NL80211_BSS_TSF])
		r->tsf = nla_get_u64(bss[NL80211_BSS_TSF]);
	if (bss[NL80211_BSS_SEEN_MS_AGO])
		r->age = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);

	r->ie_len = ie_len;
	pos = (u8 *) (r + 1);
	if (ie) {
		memcpy(pos, ie, ie_len);
		pos += ie_len;
	}
	r->beacon_ie_len = beacon_ie_len;
	if (beacon_ie)
		memcpy(pos, beacon_ie, beacon_ie_len);

	if (bss[NL80211_BSS_STATUS]) {
		enum nl80211_bss_status status;
		status = nla_get_u32(bss[NL80211_BSS_STATUS]);
		switch (status) {
		case NL80211_BSS_STATUS_AUTHENTICATED:
			r->flags |= WPA_SCAN_AUTHENTICATED;
			break;
		case NL80211_BSS_STATUS_ASSOCIATED:
			r->flags |= WPA_SCAN_ASSOCIATED;
			break;
		default:
			break;
		}
	}
	{
		const u8 *sid = nl80211_get_ie((u8 *) (r + 1), r->ie_len, WLAN_EID_SSID);

		if((sid != NULL) && (sid[1] < 33)){

			if((global->ssid_len == sid[1])
					&& (!memcmp(global->ssid, sid + 2, sid[1]))){

				unsigned int ssid_len = sid[1];
				char ssid[ssid_len + 1];
				ssid[ssid_len] = '\0';

				memcpy(ssid,sid + 2,sid[1]);
				global->freq = r->freq;
				memcpy(global->bssid,r->bssid,ETH_ALEN);
				global->find = true;
				SSPRINT("\nYes,I find You\n");
				SSPRINT("bssid: <"MACSTR"> ", MAC2STR(r->bssid));
				SSPRINT("freq: <%d> beacon_int:<%d>" ,r->freq,r->beacon_int);
				SSPRINT("ssid: <%s>\n",ssid);
#if 0
				SSPRINT("caps: <%d> level:<%d> ",r->caps,r->level);
				SSPRINT("flag: <%d> tsf:<%lu> ",r->flags,r->tsf);
				SSPRINT("age: <%u>\n",r->age);
#endif
			}
		}
	}
	/*
	 * cfg80211 maintains separate BSS table entries for APs if the same
	 * BSSID,SSID pair is seen on multiple channels. wpa_supplicant does
	 * not use frequency as a separate key in the BSS table, so filter out
	 * duplicated entries. Prefer associated BSS entry in such a case in
	 * order to get the correct frequency into the BSS table.
	 */
	for (i = 0; i < res->num; i++) {
		const u8 *s1, *s2;
		if (memcmp(res->res[i]->bssid, r->bssid, ETH_ALEN) != 0)
			continue;

		s1 = nl80211_get_ie((u8 *) (res->res[i] + 1),
				    res->res[i]->ie_len, WLAN_EID_SSID);
		s2 = nl80211_get_ie((u8 *) (r + 1), r->ie_len, WLAN_EID_SSID);
		if (s1 == NULL || s2 == NULL || s1[1] != s2[1] ||
		    memcmp(s1, s2, 2 + s1[1]) != 0)
			continue;

		/* Same BSSID,SSID was already included in scan results */
		SSPRINT("nl80211: Remove duplicated scan result "
			   "for " MACSTR, MAC2STR(r->bssid));

		if ((r->flags & WPA_SCAN_ASSOCIATED) &&
		    !(res->res[i]->flags & WPA_SCAN_ASSOCIATED)) {
			free(res->res[i]);//iw how to free
			res->res[i] = r;
		} else
			free(r);
		return NL_SKIP;
	}

	tmp = os_realloc_array(res->res, res->num + 1,
			       sizeof(struct wpa_scan_res *));
	if (tmp == NULL) {
		free(r);
		return NL_SKIP;
	}
	tmp[res->num++] = r;
	res->res = tmp;

	return NL_SKIP;
}
static struct wpa_scan_results *
nl80211_get_scan_results(struct nl80211_global *global)
{
	struct nl_msg *msg;
	struct wpa_scan_results *res;
	int ret;
	struct nl80211_bss_info_arg arg;
	res = os_zalloc(sizeof(*res));
	if (res == NULL)
		return NULL;

	msg = nlmsg_alloc();
	if (!msg)
		goto nla_put_failure;

	nl80211_cmd(global, msg, NLM_F_DUMP, NL80211_CMD_GET_SCAN);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, global->ifindex);

	arg.global = global;//global
	arg.res = res;
	ret = send_and_recv_msgs(global, msg, bss_info_handler, &arg);
	msg = NULL;
	if (ret == 0) {
		global->scan_done = true;
		SSPRINT("nl80211: Received scan results (%lu "
			   "BSSes)\n", (unsigned long) res->num);
		return res;
	}
	SSPRINT("nl80211: Scan result fetch failed: ret=%d "
		   "(%s)\n", ret, strerror(-ret));
nla_put_failure:
	nlmsg_free(msg);
	wpa_scan_results_free(res);
	return NULL;
}
static struct nl_msg *
nl80211_scan_common(struct nl80211_global *global,unsigned char cmd)
{
	struct nl_msg *msg;
	unsigned int i,num_ssids = 1;

	msg = nlmsg_alloc();
	if (!msg)
		return NULL;

	nl80211_cmd(global, msg, 0, cmd);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, global->ifindex);

	if (num_ssids) {
		struct nlattr *ssids;

		ssids = nla_nest_start(msg, NL80211_ATTR_SCAN_SSIDS);
		if (ssids == NULL)
			goto fail;
		for (i = 0; i < num_ssids; i++) {
			if (nla_put(msg, i + 1,0,NULL) < 0)
				goto fail;
		}
		nla_nest_end(msg, ssids);
	}
	return msg;
fail:
nla_put_failure:
	nlmsg_free(msg);
	return NULL;
}
static const char * nl80211_iftype_str(enum nl80211_iftype mode)
{
	switch (mode) {
		case NL80211_IFTYPE_ADHOC:
			return "ADHOC";
		case NL80211_IFTYPE_STATION:
			return "STATION";
		case NL80211_IFTYPE_AP:
			return "AP";
		case NL80211_IFTYPE_AP_VLAN:
			return "AP_VLAN";
		case NL80211_IFTYPE_WDS:
			return "WDS";
		case NL80211_IFTYPE_MONITOR:
			return "MONITOR";
		case NL80211_IFTYPE_MESH_POINT:
			return "MESH_POINT";
		case NL80211_IFTYPE_P2P_CLIENT:
			return "P2P_CLIENT";
		case NL80211_IFTYPE_P2P_GO:
			return "P2P_GO";
		case NL80211_IFTYPE_P2P_DEVICE:
			return "P2P_DEVICE";
		default:
			return "unknown";
	}
}

static int nl80211_set_mode(struct nl80211_global *global, enum nl80211_iftype mode)
{
	struct nl_msg *msg;
	int ret = -1;

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	nl80211_cmd(global, msg, 0, NL80211_CMD_SET_INTERFACE);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, global->ifindex);
	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, mode);

	ret = send_and_recv_msgs(global, msg, NULL, NULL);
	msg = NULL;
	if (!ret)
		return 0;
nla_put_failure:
	nlmsg_free(msg);
	SSPRINT("nl80211: Failed to set interface %d to mode %d:"
			" %d (%s)", global->ifindex, mode, ret, strerror(-ret));
	return ret;
}
int nl80211_create_iface_once(struct nl80211_global *global,
				     const char *ifname,
				     enum nl80211_iftype iftype,
				     const u8 *addr, int wds,
				     int (*handler)(struct nl_msg *, void *),
				     void *arg)
{
	struct nl_msg *msg;
	int ifidx;
	int ret = -1;

	addr = addr;
	SSPRINT("%s\n",__func__);
	SSPRINT("nl80211: Create interface iftype %d (%s)\n",
		   iftype, nl80211_iftype_str(iftype));

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	SSPRINT("set up cmd\n");
	nl80211_cmd(global, msg, 0, NL80211_CMD_NEW_INTERFACE);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, global->ifindex);
	NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, ifname);
	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, iftype);

	if (iftype == NL80211_IFTYPE_MONITOR) {
		struct nlattr *flags;

		flags = nla_nest_start(msg, NL80211_ATTR_MNTR_FLAGS);
		if (!flags)
			goto nla_put_failure;

		NLA_PUT_FLAG(msg, NL80211_MNTR_FLAG_COOK_FRAMES);

		nla_nest_end(msg, flags);
	} else if (wds) {
		NLA_PUT_U8(msg, NL80211_ATTR_4ADDR, wds);
	}

	SSPRINT("send cmd\n");
	ret = send_and_recv_msgs(global, msg, handler, arg);
	msg = NULL;
	if (ret) {
 nla_put_failure:
		nlmsg_free(msg);
		SSPRINT("Failed to create interface %s: %d (%s)\n",
			   ifname, ret, strerror(-ret));
		return ret;
	}
	if (iftype == NL80211_IFTYPE_P2P_DEVICE)
		return 0;

	ifidx = if_nametoindex(ifname);
	SSPRINT("nl80211: New interface %s created: ifindex=%d",
		   ifname, ifidx);

	if (ifidx <= 0)
		return -1;

#if 0
	/* start listening for EAPOL on this interface */
	add_ifidx(drv, ifidx);

	if (addr && iftype != NL80211_IFTYPE_MONITOR &&
	    linux_set_ifhwaddr(drv->global->ioctl_sock, ifname, addr)) {
		nl80211_remove_iface(drv, ifidx);
		return -1;
	}
#endif
	return ifidx;
}
int nl80211_remove_iface(struct nl80211_global *global, const char *ifname)
{
	struct nl_msg *msg;
	int ifindex ;

	ifindex = if_nametoindex(ifname);
#if 0
	if (ifindex <= 0){
		SSPRINT("Interface name error\n");
		return -1;
	}
	SSPRINT("nl80211: Remove interface ifindex=%d", ifindex);
#endif
	msg = nlmsg_alloc();
	if (!msg)
		goto nla_put_failure;

	nl80211_cmd(global, msg, 0, NL80211_CMD_DEL_INTERFACE);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ifindex);

	if (send_and_recv_msgs(global, msg, NULL, NULL) == 0)
		return 0;
	msg = NULL;
 nla_put_failure:
	nlmsg_free(msg);
	SSPRINT("Failed to remove interface (ifidx=%d)", ifindex);
	return -1;
}
/**
 * wpa_driver_nl80211_scan - Request the driver to initiate scan
 * @bss: Pointer to private driver data from wpa_driver_nl80211_init()
 * @params: Scan parameters
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_nl80211_scan(struct nl80211_global *global)
{
	int ret = -1;
	struct nl_msg *msg = NULL;
	if(global == NULL){
		SSPRINT("nl80211_global is null\n");
		return ret;
	}

	msg = nl80211_scan_common(global,NL80211_CMD_TRIGGER_SCAN);
	if (!msg)
		return -1;
	ret = send_and_recv_msgs(global, msg, NULL, NULL);
	msg = NULL;
	if (ret) {
		pr_err("nl80211: Scan trigger failed: ret=%d "
			   "(%s)\n", ret, strerror(-ret));
		goto nla_put_failure;
	}

	SSPRINT("Tigger scan cmd send successfully\n");
nla_put_failure:
	nlmsg_free(msg);
	return ret;
}
int android_priv_cmd(struct nl80211_global *global, const char *cmd)
{
	struct ifreq ifr;
	android_wifi_priv_cmd priv_cmd;
	char buf[MAX_DRV_CMD_SIZE];
	int ret;

	memset(&ifr, 0, sizeof(ifr));
	memset(&priv_cmd, 0, sizeof(priv_cmd));
#ifdef TIZEN_EXT
	strncpy(ifr.ifr_name, IFNAME, IFNAMSIZ);
#else
	strlcpy(ifr.ifr_name, IFNAME, IFNAMSIZ);
#endif

	memset(buf, 0, sizeof(buf));
#ifdef TIZEN_EXT
	strncpy(buf, cmd, sizeof(buf));
#else
	strlcpy(buf, cmd, sizeof(buf));
#endif

	priv_cmd.buf = buf;
	priv_cmd.used_len = sizeof(buf);
	priv_cmd.total_len = sizeof(buf);
	ifr.ifr_data = &priv_cmd;

	ret = ioctl(global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr);
	if (ret < 0) {
		SSPRINT("%s: failed to issue private commands\n",
			   __func__);
		return ret;
	}

	return 0;
}
static void nl80211_reg_rule_sec(struct nlattr *tb[],
				 void *arg)
{
	u32 start, end, max_bw, i;
	struct nl80211_global *global = arg;

	if (tb[NL80211_ATTR_FREQ_RANGE_START] == NULL ||
	    tb[NL80211_ATTR_FREQ_RANGE_END] == NULL ||
	    tb[NL80211_ATTR_FREQ_RANGE_MAX_BW] == NULL)
		return;

	start = nla_get_u32(tb[NL80211_ATTR_FREQ_RANGE_START]) / 1000;
	end = nla_get_u32(tb[NL80211_ATTR_FREQ_RANGE_END]) / 1000;
	max_bw = nla_get_u32(tb[NL80211_ATTR_FREQ_RANGE_MAX_BW]) / 1000;
	/*SSPRINT("start = %d, end = %d, max_bw = %d\n",
	 	start, end, max_bw);*/

	for(i = 1; i < 15; i++){
		if(g_channelist[i] > start && g_channelist[i] < end)
			global->chan_bitmap |= CHANNEL(i);
	}
	return;
}
static int nl80211_get_reg(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *nl_rule;
	struct nlattr *tb_rule[NL80211_FREQUENCY_ATTR_MAX + 1];
	int rem_rule;
	static struct nla_policy reg_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
		[NL80211_ATTR_REG_RULE_FLAGS] = { .type = NLA_U32 },
		[NL80211_ATTR_FREQ_RANGE_START] = { .type = NLA_U32 },
		[NL80211_ATTR_FREQ_RANGE_END] = { .type = NLA_U32 },
		[NL80211_ATTR_FREQ_RANGE_MAX_BW] = { .type = NLA_U32 },
		[NL80211_ATTR_POWER_RULE_MAX_ANT_GAIN] = { .type = NLA_U32 },
		[NL80211_ATTR_POWER_RULE_MAX_EIRP] = { .type = NLA_U32 },
	};
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	if (!tb_msg[NL80211_ATTR_REG_ALPHA2] ||
	    !tb_msg[NL80211_ATTR_REG_RULES]) {
		SSPRINT("nl80211: No regulatory information "
			   "available\n");
		return NL_SKIP;
	}

	SSPRINT("nl80211: Regulatory information - country=%s\n",
		   (char *) nla_data(tb_msg[NL80211_ATTR_REG_ALPHA2]));

	nla_for_each_nested(nl_rule, tb_msg[NL80211_ATTR_REG_RULES], rem_rule)
	{
		nla_parse(tb_rule, NL80211_FREQUENCY_ATTR_MAX,
			  nla_data(nl_rule), nla_len(nl_rule), reg_policy);
		nl80211_reg_rule_sec(tb_rule, arg);
	}

	return NL_SKIP;
}
int nl80211_send_get_reg(struct nl80211_global *global)
{
	struct nl_msg *msg;
	int ret;
	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	nl80211_cmd(global, msg, 0, NL80211_CMD_GET_REG);
	ret = send_and_recv_msgs(global, msg, nl80211_get_reg, global);
	if (ret) {
		SSPRINT("nl80211: set country code failed, ret=%d (%s)\n",
			ret, strerror(-ret));
	}
	return ret;
}
int wpa_driver_nl80211_set_country(struct nl80211_global *global, const char *alpha2_arg)
{
	char alpha2[3];
	struct nl_msg *msg;
	int ret;
	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	alpha2[0] = alpha2_arg[0];
	alpha2[1] = alpha2_arg[1];
	alpha2[2] = '\0';

	nl80211_cmd(global, msg, 0, NL80211_CMD_REQ_SET_REG);

	NLA_PUT_STRING(msg, NL80211_ATTR_REG_ALPHA2, alpha2);
	ret = send_and_recv_msgs(global, msg, NULL, NULL);
	if (ret) {
		SSPRINT("nl80211: set country code failed, ret=%d (%s)\n",
			ret, strerror(-ret));
		goto nla_put_failure;
	}
	return 0;
nla_put_failure:
	nlmsg_free(msg);
	return -1;
}
static int wpa_driver_nl80211_mlme(struct nl80211_global *global,
				   const u8 *addr, int cmd, u16 reason_code,
				   int local_state_change)
{
	int ret = -1;
	struct nl_msg *msg;
	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	nl80211_cmd(global, msg, 0, cmd);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, global->ifindex);
	NLA_PUT_U16(msg, NL80211_ATTR_REASON_CODE, reason_code);
	if (addr)
		NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, addr);
	if (local_state_change)
		NLA_PUT_FLAG(msg, NL80211_ATTR_LOCAL_STATE_CHANGE);

	ret = send_and_recv_msgs(global, msg, NULL, NULL);
	msg = NULL;
	if (ret) {
		pr_err("nl80211: disconnect command failed: reason=%u ret=%d (%s)\n",
			reason_code, ret, strerror(-ret));
		goto nla_put_failure;
	}
	ret = 0;
	SSPRINT("nl80211:disconnect command send successfully\n");
nla_put_failure:
	nlmsg_free(msg);
	return ret;
}
static int wpa_driver_nl80211_disconnect(struct nl80211_global *global)
{
	int ret;

	ret = wpa_driver_nl80211_mlme(global, NULL, NL80211_CMD_DISCONNECT,
				      1, 0);
	return ret;
}
static int wpa_driver_nl80211_connect(struct nl80211_global *params)
{
	int ret = -1;
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg)
		return -1;
	nl80211_cmd(params, msg, 0, NL80211_CMD_CONNECT);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, params->ifindex);
	SSPRINT("AP->bssid=" MACSTR"\n", MAC2STR(params->bssid));
	NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, params->bssid);

	if (params->freq) {
		SSPRINT("AP->freq=%d\n", params->freq);
		NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, params->freq);
	}

	NLA_PUT(msg, NL80211_ATTR_SSID, params->ssid_len,
			params->ssid);
	if (params->ssid_len > 32) //changed
		goto nla_put_failure;

	NLA_PUT_U32(msg, NL80211_ATTR_AUTH_TYPE,0/*NL80211_AUTHTYPE_OPEN_SYSTEM*/);

	NLA_PUT_U32(msg, NL80211_ATTR_WPA_VERSIONS, 0);

	ret = send_and_recv_msgs(params, msg, NULL, NULL);
	msg = NULL;
	if (ret) {
		pr_err("nl80211: MLME command failed (assoc): ret=%d (%s)\n",
			ret, strerror(-ret));
		goto nla_put_failure;
	}
	ret = 0;
	SSPRINT("nl80211: connect request send successfully\n");
nla_put_failure:
	nlmsg_free(msg);
	return ret;
}
int select_nl80211(struct nl80211_global *global,int time, bool *state,bool expect)
{
	int res;
	fd_set fds;
	struct timeval timeout = {time,0};
	int sock = nl_socket_get_fd(global->nl_event);

	while(1){
		FD_ZERO(&fds);
		FD_SET(sock,&fds);
		res = select(sock + 1,&fds,NULL,NULL,&timeout);

		if(res == -1){//error
			FD_CLR(sock,&fds);
			pr_err("error select\n");
		}
		if(res ==  0){//timeout
			if (time)
				pr_err("select time out\n");
			FD_CLR(sock,&fds);
			return -1;
		}
		if(res > 0 ){
			if(FD_ISSET(sock, &fds)){
				nl_recvmsgs(global->nl_event,global->nl_cb);
				FD_CLR(sock,&fds);
				if(state == NULL)
					return 0;
				else if(*state == expect)
					return 0;
			}
		}
	}
}
int nl80211_trigger_scan(struct nl80211_global *global)
{
	int ret;
	int i,scan_time = 1;

	/*read event non-blocking*/
	select_nl80211(global,0,NULL,false);
	/*scan*/
	for (i = 0; i < scan_time; i++){
		global->scan_done = false;
		ret = wpa_driver_nl80211_scan(global);
		if(ret)
			return ret;//cmd send error
		ret = select_nl80211(global, 3, &global->scan_done, true);
		if(ret == 0)
			break;
		if((ret == -1) && (i == scan_time - 1)) {
			pr_err("waitting scan result timeout\n");
			return -1;//wait result timeout
		}
	}
	return ret;
}
int nl80211_join_ssid(struct nl80211_global *global)
{
	int ret;
	int i, scan_time = 2;

	/*read event non-blocking*/
	select_nl80211(global,0,NULL,false);
	/*disconnect*/
	if(global->connected == true){
		ret = wpa_driver_nl80211_disconnect(global);
		if(ret)
			return ret;//cmd send error
		ret = select_nl80211(global, 2, &global->connected, false);
		if(ret)
			return ret;//wait result timeout
	}
	/*scan*/
	for (i = 0; i < scan_time; i++){
		global->find = false;
		ret = wpa_driver_nl80211_scan(global);
		if(ret)
			return ret;//cmd send error
		ret = select_nl80211(global, 2, &global->find, true);
		if(ret == 0)
			break;//find ideal ap
		if((ret == -1) && (i == scan_time - 1)) {
			pr_err("wait scan result timeout\n");
			return -1;//wait result timeout
		}
	}
	/*connect*/
	if((global->find == true) && (global->connected == false)){
		global->connect_finish = false;
		ret = wpa_driver_nl80211_connect(global);
		if(ret)
			return -1;//cmd send error
		ret = select_nl80211(global, 2, &global->connect_finish, true);
		if((ret == -1) || (!global->connected)) {
			pr_err("wait connect result timeout\n");
			return -1;//wait result timeout
		}
	}
	return ret;
}
int nl80211_set_interface(struct nl80211_global *global, enum nl80211_iftype mode)
{
	return nl80211_set_mode(global, mode);
}
static void do_process_drv_event(struct nl80211_global *global, int cmd,
				 struct nlattr **tb)
{
	struct wpa_scan_results *res;
	int status;

	switch (cmd) {
	case NL80211_CMD_NEW_SCAN_RESULTS:
		SSPRINT("nl80211: New scan results available\n");
		res = nl80211_get_scan_results(global);
		if(res)
			wpa_scan_results_free(res);
		break;
	case NL80211_CMD_CONNECT:
		global->connect_finish = true;
		status = nla_get_u16(tb[NL80211_ATTR_STATUS_CODE]);
		SSPRINT("nl80211: connect(%u):%s\n",status,status? "failed": "success");
		if(!status)
			global->connected = true;
		break;
	case NL80211_CMD_DISCONNECT:
		global->connected = false;
		SSPRINT("nl80211:disconnect Successfully\n");
		break;
	case NL80211_CMD_REG_CHANGE:
		SSPRINT("nl80211: Regulatory domain change\n");
		break;
	case NL80211_CMD_REG_BEACON_HINT:
		SSPRINT("nl80211: Regulatory beacon hint\n");
		break;
	default:
		break;
	}
}
int process_global_event(struct nl_msg *msg,void *arg)
{
	struct nl80211_global *global = arg;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	do_process_drv_event(global , gnlh->cmd, tb);

	return NL_SKIP;
}
