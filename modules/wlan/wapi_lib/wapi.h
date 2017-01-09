/** @file wapi_asue.h
 *  @brief This header file contains data structures and function declarations of wapi
 *
 *  Copyright (C) 2001-2008, Iwncomm Ltd.
 */

#ifndef _WAPI_ASUE_H_
#define _WAPI_ASUE_H_

#define BK_LEN 16

#define PAGE_LEN	4096
#define BKID_LEN	16
#define CHALLENGE_LEN	32
#define PAIRKEY_LEN	16
#define USKSA_LEN	4*PAIRKEY_LEN + CHALLENGE_LEN
#define ADDID_LEN	ETH_ALEN*2
#define WAI_MIC_LEN	20
#define WAI_IV_LEN	16
#define WAI_KEY_AN_ID_LEN	16
#define ISUSK		1
#define WAI_HDR	12

#define MSK_TEXT "multicast or station key expansion for station unicast and multicast and broadcast"
#define USK_TEXT "pairwise key expansion for unicast and additional keys and nonce"


typedef enum
{
	WAPISM_N0_ASSOC=0,		/*not association(pre association)*/
	WAPISM_AL_ASSOC=1,		/*already association*/
	WAPISM_CNT_REQ=2,		/*(already send 4)*/
	WAPISM_CNT_RESP=3,		/*(already recv 5)*/
	WAPISM_UNI_RESP=4,		/*(already send 9)*/
	WAPISM_UNI_ACK=5,		/*(already recv 10)*/
	WAPISM_MUL_RESP=6,		/*(already send 12)*/
	WAPISM_FINISH=7,		/*(finished and opened port)*/
	WAPISM_UPDATE_BK,
	WAPISM_UPDATE_USK,
	WAPISM_UPDATE_MSK,
}wapi_states;


struct wai_hdr
{
	u8 version[2];
	u8 type;
	u8 stype;
	u8 reserve[2]; 
	u16 length;
	u8 rxseq[2];
	u8 frag_sc;
	u8 more_frag;
	/* followed octets of data */
};

struct wapi_bksa_cache
{
	u8 bkid[BKID_LEN];
	u8 bk[BK_LEN];
	u8 asue_auth_flag[32];
	size_t bk_len;
	int akmp; /* WAPI_KEY_MGMT_* */
	u8 ae_mac[ETH_ALEN];
	u8 pad[2];
};
	
struct wapi_usk
{
	u8 uek[PAIRKEY_LEN]; /* Unicast Encryption Key */
	u8 uck[PAIRKEY_LEN]; /* Unicast Integrity check Key (UCK) */
	u8 mak[PAIRKEY_LEN]; /* Message Authentication Key (MAK)*/
	u8 kek[PAIRKEY_LEN]; /*Key Encryption Key (KEK) */
	u8 ae_challenge[CHALLENGE_LEN];
	u8 asue_challenge[CHALLENGE_LEN];
};
struct wapi_usksa
{
	u8 uskid;
	u8 usk_pad[3];
	struct wapi_usk usk[2];
	int ucast_suite;
	u8 ae_mac[ETH_ALEN];
	u8 mac_pad[2];
};

struct wapi_msksa
{
	u8 direction;
	u8 mskid;
	u8 msk_pad[2];
	u8 msk[32];
	u8 msk_ann_id[16];
	int ucast_suite;
	u8 ae_mac[ETH_ALEN];
	u8 mac_pad[2];
};


struct wapi_state_machine
{
	struct wapi_usksa usksa;
	struct wapi_msksa msksa;
	struct wapi_bksa_cache bksa; /* PMKSA cache */
	int bksa_count; /* number of entries in PMKSA cache */

	u8 own_addr[ETH_ALEN];
       u8 own_addr_pad[2];
	const char *ifname;
	u8 bssid[ETH_ALEN];
	u8 bssid_pad[2];

	/* Selected configuration (based on Beacon/ProbeResp WAPI IE) */
	unsigned int key_mgmt;

	u8 assoc_wapi_ie[256]; /* Own WAPI/RSN IE from (Re)AssocReq */
	u8 ap_wapi_ie[256];
	u8 ap_wapi_ie_len;
	u8 assoc_wapi_ie_len;
       u8 len_pad[2];
};


struct wapi_rxfrag
{
	const u8 *data;
	int data_len;
	int maxlen;
};
typedef struct __cert_id
{
	u16 cert_flag;
	u16 length;
	u8 data[2048];
}cert_id;

typedef struct byte_data_
{
	u8 length;
	u8 pad[3];
	u8 data[256];
}byte_data;
struct wpa_certs
{
	int  type;
	int status;
	byte_data		*serial_no;
	byte_data		*as_name;	
	byte_data		*user_name;
};

typedef struct _wai_fixdata_id
{
	u16	id_flag;
	u16	id_len;
	u8 	id_data[1000];     
}wai_fixdata_id;

struct cert_bin_t
{
	unsigned short length;
	unsigned short pad;
	unsigned char *data;
};

typedef struct _wai_fixdata_cert
{
	u16	cert_flag;
	u16  pad;
	struct cert_bin_t cert_bin;
}wai_fixdata_cert;

typedef struct _para_alg
{
	u8	para_flag;
	u16	para_len;
	u8	pad;
	u8	para_data[256];
}para_alg, *ppara_alg;

typedef struct _comm_data
{
	u16 length;
	u16 pad_value;
	u8 data[2048];
}comm_data, *pcomm_data,
tkey, *ptkey,
tsign;

/*signature algorithm*/
typedef struct _wai_fixdata_alg
{
	u16	alg_length;
	u8	sha256_flag;
	u8	sign_alg;
	para_alg	sign_para;
}wai_fixdata_alg;

typedef struct _resendbuf_st
{
	u16 	cur_count;
	u16		len;
	void	*data;
}resendbuf_st;


#define TIMER_DATA_MAXLEN    8192


struct timer_dispose{
	void* pt;	/*timer*/
	int t;		/*type, 0-none, 1-wait assoc, 2-wait unicast, 3-wait finish*/
	int ctm;	/*count of into*/
	int ctx;	/*count of tx*/
	int l;
	u8 dat[TIMER_DATA_MAXLEN];
};

/**
 * struct wpa_ssid - Network configuration data
 *
 * This structure includes all the configuration variables for a network. This
 * data is included in the per-interface configuration data as an element of
 * the network list, struct wpa_config::ssid. Each network block in the
 * configuration is mapped to a struct wpa_ssid instance.
 */
 struct wapi_asue_st {
	u8 own_addr[ETH_ALEN];
	u8 own_addr_pad[2];
	u8 bssid[ETH_ALEN];
	u8 ssid_pad[2];
	int reassociate; /* reassociation requested */
	int disconnected; 
	int wai_received;		   	
	/* Selected configuration (based on Beacon/ProbeResp WPA IE) */
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;	
	struct wapi_state_machine *wapi_sm;
	int rxfragstamp;
	struct wapi_rxfrag *rxfrag;
	u16 rxseq;
	u16 txseq;
	wapi_states wapi_state;
	int usk_updated;
	u8 last_wai_src[ETH_ALEN];
	u8 flag;
	u8 flag_pad;
	u8 ae_auth_flag[32];
	u8 Nasue[32];
	u8 Nae[32];
	cert_id asue_cert;
	cert_id ae_cert;
	wai_fixdata_id      ae_asu_id;
	wai_fixdata_id  	ae_id;
	wai_fixdata_id  	asue_id;
	wai_fixdata_alg 	sign_alg;
	byte_data		asue_key_data;
	byte_data		asue_eck;/* ASUE temp private key */
	byte_data		ae_key_data;
	para_alg	ecdh;

	struct timer_dispose* ptm;				
	struct wapi_state_machine vpt_wapi_sm;
	struct timer_dispose vpt_tm;

	int ap_type;/*0-open,1-cert,2-key*/
	u8 psk_bk[16];
};


int iwn_wpa_ether_send(const u8 *buf, size_t len);
int iwn_wai_fixdata_id_by_ident(void *cert_st, wai_fixdata_id *fixdata_id, u16 index);
struct wapi_rxfrag* iwn_wpa_defrag(struct wapi_asue_st *wpa_s, struct wapi_rxfrag *rxbuf);



/*原来在wapi.h中*/

#define BIT(n) (1 << (n))

typedef enum { WPA_ALG_NONE, WPA_ALG_WEP, WPA_ALG_TKIP, WPA_ALG_CCMP,
	       WPA_ALG_IGTK, WPA_ALG_DHV, WAPI_ALG_SMS4 } wpa_alg;

#define WAI_VERSION 1
#define WAI_TYPE 1
#define WAI_FLAG_LEN 1

/*WAI packets type*/
enum
{ 
	WAI_PREAUTH_START	= 0x01,	/*pre-authentication start*/
	WAI_STAKEY_REQUEST = 0x02,	/*STAKey request */
	WAI_AUTHACTIVE	= 0x03,			/*authentication activation*/
	WAI_ACCESS_AUTH_REQUEST = 0x04,	/*access authentication request */
	WAI_ACCESS_AUTH_RESPONSE = 0x05,	/*access authentication response */
	WAI_CERT_AUTH_REQUEST = 0x06,	/*certificate authentication request */
	WAI_CERT_AUTH_RESPONSE = 0x07,	/*certificate authentication response */
	WAI_USK_NEGOTIATION_REQUEST = 0x08,	/*unicast key negotiation request */
	WAI_USK_NEGOTIATION_RESPONSE = 0x09,	/* unicast key negotiation response */
	WAI_USK_NEGOTIATION_CONFIRMATION = 0x0A,/*unicast key negotiation confirmation */
	WAI_MSK_ANNOUNCEMENT = 0x0B, /*multicast key/STAKey announcement */
	WAI_MSK_ANNOUNCEMENT_RESPONSE = 0x0C, /*multicast key/STAKey announcement response */
	//WAI_SENDBK = 0x10, /*BK  for TMC ??*/
};

#endif
