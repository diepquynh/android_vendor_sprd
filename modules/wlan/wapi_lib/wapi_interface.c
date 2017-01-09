/** @file interface.c
  * @brief This file contains functions for interface of wai lib
 *
 *  Copyright (C) 2001-2008, Iwncomm Ltd.
 *
 */


#include <stdarg.h>
#include <assert.h>

#include "alg_comm.h"
#include "wapi_common.h"
#include "cert.h"
#include "wapi_interface.h"

extern void iwn_wapi_sm_rx_wai(struct wapi_asue_st *wpa_s, const unsigned char *src_addr, const unsigned char *buf, size_t len);
struct wapi_rxfrag* iwn_wpa_defrag(struct wapi_asue_st *wpa_s, struct wapi_rxfrag *rxbuf);

extern int init_cert(const void* param);
extern int cleanup_cert(void);

static struct wapi_asue_st *s_asue = NULL;

static int s_init = 0;
static int s_l_endian = 0;


static void timer_run(const int arg);
static void timer_create(void);
static void timer_destory(void);

static void wlan_deauth(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	wpa_s->wapi_state = WAPISM_N0_ASSOC;
	wpa_s->wapi_sm->usksa.uskid = 1;
	if (memcmp(wpa_s->bssid, "\0\0\0\0\0\0", ETH_ALEN) != 0)
	{
		WIFI_Action_Deauth();
		memcpy(wpa_s->bssid, "\0\0\0\0\0\0", ETH_ALEN);
	}
}

void wlan_deauth_for_otherc(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	struct timer_dispose* ptm = wpa_s->ptm;

	wlan_deauth();
	timer_destory();
	ptm->t = 0;
}

static void timer_run(const int arg)
{
	struct wapi_asue_st* wpa_s = s_asue;
	struct timer_dispose* ptm = wpa_s->ptm;
	int t = ptm->t;
	int timeout[4] = {0, 31, 1, 1};
	int n = 4;

	if (t>3 || t<0)
	{
		return;
	}
	if (0 == t)
	{
		return;
	}
	ptm->ctm++;
	if (ptm->ctm >= timeout[t]*n)
	{
		ptm->ctx++;
		if (ptm->ctx >= 3)
		{
			if (1==t || 2==t)
			{
				wlan_deauth();
			}
			timer_destory();
			ptm->t = 0;
			return;
		}
		iwn_wpa_ether_send(ptm->dat, ptm->l);
		ptm->ctm = 0;
	}
}

static void timer_create(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	wpa_s->ptm->pt = OS_timer_setup(500, 500, timer_run, NULL);
	timer_reset();
}

static void timer_destory(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	if ((NULL != wpa_s) && (NULL != wpa_s->ptm->pt))
	{
		OS_timer_clean(wpa_s->ptm->pt);
		wpa_s->ptm->pt = NULL;
		timer_reset();
	}
}

void timer_set(int t, const u8* dat, int l)
{
	struct wapi_asue_st* wpa_s = s_asue;
	struct timer_dispose* ptm = wpa_s->ptm;

	ptm->t = t;
	ptm->l = l;
	ptm->ctm = 0;
	ptm->ctx = 0;
	memcpy(ptm->dat, dat, l);
}

void timer_reset(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	struct timer_dispose* ptm = wpa_s->ptm;

	ptm->t = 0;
}

void timer_resend(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	struct timer_dispose* ptm = wpa_s->ptm;
	int t = ptm->t;
	if (1==t || 2==t)
	{
		ptm->ctx++;
		if (ptm->ctx >= 3)
		{
			wlan_deauth();
			timer_destory();
			ptm->t = 0;
			return;
		}
		iwn_wpa_ether_send(ptm->dat, ptm->l);
		ptm->ctm = 0;
	}
}

void *iwn_get_buffer(int len)
{
	char *buffer=NULL;
	buffer = (char *)malloc(len);
	if(buffer)
		memset(buffer, 0, len);
	else
		buffer = NULL;
	return buffer;
}

void *iwn_free_buffer(void *buffer, int len)
{
	char *tmpbuf = (char *)buffer;

	if(tmpbuf != NULL)
	{
		memset(tmpbuf, 0, len);
		free(tmpbuf);
		return NULL;
	}
	else
		return NULL;
}


void show_wapi_pack(const void* buf, int len)
{
	u8* p = (u8*)buf;
	u8 t = p[3];

	return;

	if (3 == t)
	{
		/*get_random(p+13, 32);*/
		print_buf("3_wailib", p, len);
	}
	else if (4 == t)
	{
		print_buf("4_wailib", p, len);
	}
	else if (t>=1 && t<=12)
	{
		print_buf("1,12_wailib", p, len);
	}
	else
	{
		print_buf("error_wailib", p, len);
	}
}

int iwn_wpa_ether_send(const u8 *buf, size_t len)
{
	unsigned long res = 0;
	//show_wapi_pack(buf, len);
	res = WIFI_TX_packet((const char*)buf, len);
 	return res;
}

static void wapi_asue_rx_wai(const u8* src_addr, const u8* buf, size_t len)
{
	struct wapi_asue_st* wpa_s = s_asue;
	int t = wpa_s->ap_type;

	if (AUTH_TYPE_WAPI!=t && AUTH_TYPE_WAPI_PSK!=t)
	{
		return;
	}

	{
		struct wapi_rxfrag rxbuf ,*temp_rxbuf= NULL;
		rxbuf.data = buf;
		rxbuf.data_len = len;
		temp_rxbuf = iwn_wpa_defrag(wpa_s, &rxbuf);
		if (temp_rxbuf)
		{
			show_wapi_pack(temp_rxbuf->data, temp_rxbuf->data_len);
			iwn_wapi_sm_rx_wai(wpa_s, src_addr, temp_rxbuf->data, temp_rxbuf->data_len);
		}
	}
}





#if 0
u16 iwn_ntohs(u16 v)
{
	if (s_l_endian)
	{
		u16 ret = v;
		char* p = (char*)&ret;
		char* q = (char*)&v;
		p[0] = q[1];
		p[1] = q[0];
		return ret;
	}
	return v;
}

u16 iwn_htons(u16 v)
{
	return iwn_ntohs(v);
}
#endif

void iwn_getshort(const void* p, void* v)
{
	if (s_l_endian)
	{
		u8* pp = (u8*)p;
		u8* pv = (u8*)v;
		pv[0] = pp[1];
		pv[1] = pp[0];
	}
	else
	{
		memcpy(v, p, 2);
	}
}

void iwn_setshort(void* p, u16 v)
{
	if (s_l_endian)
	{
		u8* pp = (u8*)p;
		u8* pv = (u8*)&v;
		pp[0] = pv[1];
		pp[1] = pv[0];
	}
	else
	{
		u8* pp = (u8*)p;
		pp[0] = v/256;
		pp[1] = v%256;
	}
}


int lib_get_wapi_state(void)
{
	struct wapi_asue_st* wpa_s = s_asue;
	int ret = (int)wpa_s->wapi_state;

	if (wpa_s->usk_updated)
	{
		/* cert  */
		if (wpa_s->ap_type == AUTH_TYPE_WAPI)
		{
			if (wpa_s->wapi_state >= WAPISM_AL_ASSOC && wpa_s->wapi_state < WAPISM_UNI_ACK )
			{
				ret = WAPISM_UPDATE_BK;/* bk update */
			}
			else
			{
				if (wpa_s->wapi_state == WAPISM_UNI_ACK)
					ret = WAPISM_FINISH;
			}
		}
		else/* PSK */
		{
			if (wpa_s->ap_type == AUTH_TYPE_WAPI_PSK)
			{
				if (wpa_s->wapi_state >= WAPISM_AL_ASSOC && wpa_s->wapi_state < WAPISM_UNI_ACK )
				{
					ret = WAPISM_UPDATE_USK;/* usk update */
				}
				else
				{
					if (wpa_s->wapi_state == WAPISM_UNI_ACK)
						ret = WAPISM_FINISH;
				}

			}
		}

	}
	else
	{
	}

	return ret;
}


static void wpa_init_iv(struct wapi_msksa *msksa)
{
	u8 init_iv[]={	0x5c,0x36,0x5c,0x36,0x5c,0x36,0x5c,0x36,
				0x5c,0x36,0x5c,0x36,0x5c,0x36,0x5c,0x35};
	memcpy(msksa->msk_ann_id, init_iv, WAI_IV_LEN);
}


static int hex2int(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	return -1;
}

static int str2byte(const unsigned char *str, int len,  char *byte_out)
{
	int i, val, val2;
	const char* pos = (const char*)str;
	for (i = 0; i < len/2; i++) {
		val = hex2int(*pos++);
		if (val < 0)
			return -1;
		val2 = hex2int(*pos++);
		if (val2 < 0)
			return -1;
		byte_out[i] = (val * 16 + val2) & 0xff;
	}

	return 0;
}


static int iwn_password_bk_derivation(u8 *password,unsigned password_len, u8*out_bk)
{
	u8 input_text[] = "preshared key expansion for authentication and key negotiation";
	
	if(password == NULL || password_len == 0 ||out_bk == NULL)
		return -1;
	
	KD_hmac_sha256(input_text, strlen((char*)input_text), password, password_len, out_bk, 16);
	return 0;
}


int WAI_CNTAPPARA_SET(const CNTAP_PARA* pPar)
{
	struct wapi_asue_st* wpa_s = s_asue;
	unsigned char buff[128];
	int ret = -1;

	/* check initialized and param */
	if (0 == s_init || NULL == pPar)
	{
		return ret;
	}

	/* save and parse client auth type */
	wpa_s->ap_type = pPar->authType;
	if (AUTH_TYPE_NONE_WAPI != pPar->authType)
	{
		/* guoxd 20081210 modify start */
		unsigned char buf_ie[64] = {0x44,22,1,0, 1,0,0,0x14,0x72,1, 1,0,0,0x14,0x72,1, 0,0x14,0x72,1, 0, 0, 0, 0};/* little endian*/
		/* guoxd 20081210 modify end */

		if (AUTH_TYPE_WAPI_PSK == pPar->authType)
		{
			buf_ie[9] = 2;
		}

		/* set wapi IE field with the output interface */
		WIFI_WAI_IE_set(buf_ie, 2+buf_ie[1]);

		memcpy(wpa_s->wapi_sm->assoc_wapi_ie, buf_ie, 2+buf_ie[1]);
		wpa_s->wapi_sm->assoc_wapi_ie_len = (u8)(2+buf_ie[1]);
	}

	/* cert security mode*/
	if (AUTH_TYPE_WAPI == pPar->authType)
	{
		ret = init_cert((void *)pPar);
	}
	else
	{
		/* PSK security mode*/
		if (AUTH_TYPE_WAPI_PSK == pPar->authType)
		{

			/* ASCII mode for PSK */
			if (pPar->para.kt == KEY_TYPE_ASCII)
			{
			    iwn_wpa_printf(MSG_DEBUG, "%s: KEY_TYPE_ASCII..\n", __func__);
#if 1
				/* get BK value */
				ret = iwn_password_bk_derivation((u8*)pPar->para.kv, pPar->para.kl, wpa_s->psk_bk);
#else
				memcpy(wpa_s->psk_bk, pPar->para.kv, 16);
				ret = 0;
#endif
			}
			else		/* HEX mode for PSK */
			{
			    iwn_wpa_printf(MSG_DEBUG, "%s: KEY_TYPE_HEX..\n", __func__);
#if 1
				/* get BK value */
				memset(buff, 0, sizeof(buff));
				if (pPar->para.kl % 2) {
				    iwn_wpa_printf(MSG_DEBUG, "%s: pPar->para.kl % 2. ret (%d)\n", __func__, ret);
				    return ret;
				}
				iwn_wpa_printf(MSG_DEBUG, "%s: pPar->para.kv=%s\tpPar->para.kl=%d\n", 
					       __func__, pPar->para.kv, pPar->para.kl);
				str2byte(pPar->para.kv, pPar->para.kl, (char*)buff);
				iwn_wpa_printf(MSG_DEBUG, "%s: buff=%s\n",__func__, buff); 
				ret = iwn_password_bk_derivation(buff, pPar->para.kl /2, wpa_s->psk_bk);
#else
				memcpy(wpa_s->psk_bk, pPar->para.kv, 16);
				ret = 0;
#endif
			}
			iwn_wpa_hexdump(MSG_DEBUG, "WAI_CNTAPPARA_SET: WAI: wpa_s->psk_bk", wpa_s->psk_bk, 16);
		}
		else	/* open mode*/
		{
			ret = 0;
		}
	}

	return ret;
}

void WAI_Msg_Input(CONN_STATUS action, const MAC_ADDRESS* pBSSID, const MAC_ADDRESS* pLocalMAC, unsigned char *assoc_ie, unsigned char assoc_ie_len)
{
	struct wapi_asue_st* wpa_s = s_asue;
	int i;
	
	/* check initialized and param */
	if (0 == s_init || NULL == pBSSID || NULL == pLocalMAC
		|| (NULL != assoc_ie && 0 == assoc_ie_len)
		|| (NULL == assoc_ie && 0 != assoc_ie_len))
	{
		iwn_wpa_printf(MSG_ERROR, "WAI_Msg_Input(): premature exit .... \n");
		return;
	}

	if (CONN_ASSOC == action)
	{	
		iwn_wpa_printf(MSG_DEBUG, "WAI_Msg_Input():  CONN_ASSOC == action .... \n");
		memcpy(wpa_s->own_addr, pLocalMAC->v, ETH_ALEN);

		/* save wapi state */
		wpa_s->wapi_state = WAPISM_AL_ASSOC;
		wpa_s->usk_updated = 0;

		/* new ap */
		if (memcmp(wpa_s->bssid, pBSSID->v, ETH_ALEN) != 0)
		{
			memcpy(wpa_s->bssid, pBSSID->v, ETH_ALEN);
			wpa_init_iv(&wpa_s->wapi_sm->msksa);

			if (AUTH_TYPE_WAPI_PSK == wpa_s->ap_type)
			{
				u8 addid[12] = {0};
				u8 bkid[16] = {0};

				iwn_wpa_printf(MSG_DEBUG, "WAI_Msg_Input():  AUTH_TYPE_WAPI_PSK == wpa_s->ap_type.... \n");
				memcpy(addid, wpa_s->bssid, 6);
				memcpy(addid + 6, wpa_s->own_addr, 6);
				
				KD_hmac_sha256(addid,12, wpa_s->psk_bk,16, bkid,BKID_LEN);
				memcpy(wpa_s->wapi_sm->bksa.bk, wpa_s->psk_bk, 16);
				iwn_wpa_hexdump(MSG_DEBUG, "wpa_s->wapi_sm->bksa.bk: ", wpa_s->wapi_sm->bksa.bk, 16);
				memcpy(wpa_s->wapi_sm->bksa.bkid, bkid, 16);
				iwn_wpa_hexdump(MSG_DEBUG, "wpa_s->wapi_sm->bksa.bkid", wpa_s->wapi_sm->bksa.bkid, 16);
			}

			if (AUTH_TYPE_NONE_WAPI != wpa_s->ap_type)
			{
				/* WAPI IE */
				memset(wpa_s->wapi_sm->ap_wapi_ie, 0, sizeof(wpa_s->wapi_sm->ap_wapi_ie));
				if (assoc_ie_len >= sizeof(wpa_s->wapi_sm->ap_wapi_ie)-1)
					assoc_ie_len = sizeof(wpa_s->wapi_sm->ap_wapi_ie)-1;

				memcpy(wpa_s->wapi_sm->ap_wapi_ie, assoc_ie, assoc_ie_len);
				wpa_s->wapi_sm->ap_wapi_ie_len = assoc_ie_len;
				iwn_wpa_printf(MSG_DEBUG, "WAI_Msg_Input(): wpa_s->wapi_sm->ap_wapi_ie_len=%d\n", wpa_s->wapi_sm->ap_wapi_ie_len);
				iwn_wpa_hexdump(MSG_DEBUG, "wpa_s->wapi_sm->ap_wapi_ie", wpa_s->wapi_sm->ap_wapi_ie, assoc_ie_len);
			}
		}
		
		wpa_s->rxseq = 0;
		wpa_s->txseq = 1;

		timer_create();
	}
	else	
		if (CONN_DISASSOC == action)/* DISCONN ASSOC */
		{
		iwn_wpa_printf(MSG_DEBUG,"WAI_Msg_Input():  CONN_DISASSOC == action .... \n");
			wpa_s->wapi_state = WAPISM_N0_ASSOC;
			wpa_s->usk_updated = 0;

			timer_destory();

			memset(wpa_s->own_addr, 0, sizeof(wpa_s->own_addr));
			memset(wpa_s->bssid, 0, sizeof(wpa_s->bssid));
			memset(wpa_s->last_wai_src, 0, sizeof(wpa_s->last_wai_src));
			
			memset(wpa_s->ae_auth_flag, 0, sizeof(wpa_s->ae_auth_flag));
			memset(wpa_s->Nasue, 0, sizeof(wpa_s->Nasue));
			memset(wpa_s->Nae, 0, sizeof(wpa_s->Nae));
			
			memset(&wpa_s->ae_cert, 0, sizeof(wpa_s->ae_cert));
			memset(&wpa_s->sign_alg, 0, sizeof(wpa_s->sign_alg));
			memset(&wpa_s->asue_eck, 0, sizeof(wpa_s->asue_eck));
			memset(&wpa_s->ae_key_data, 0, sizeof(wpa_s->ae_key_data));
			memset(&wpa_s->ecdh, 0, sizeof(wpa_s->ecdh));
			memset(&wpa_s->ae_asu_id, 0, sizeof(wpa_s->ae_asu_id));
			memset(&wpa_s->ae_id, 0, sizeof(wpa_s->ae_id));

			memset(&wpa_s->wapi_sm->usksa, 0, sizeof(wpa_s->wapi_sm->usksa));
			memset(&wpa_s->wapi_sm->msksa, 0, sizeof(wpa_s->wapi_sm->msksa));
			memset(&wpa_s->wapi_sm->bksa, 0, sizeof(wpa_s->wapi_sm->bksa));

			wpa_s->wapi_sm->usksa.uskid = 1;
			wpa_s->wapi_sm->msksa.mskid = 1;
			wpa_s->wapi_sm->bksa.bk_len = BKID_LEN;

			/* WAPI IE */
			memset(wpa_s->wapi_sm->ap_wapi_ie, 0, sizeof(wpa_s->wapi_sm->ap_wapi_ie));
			wpa_s->wapi_sm->ap_wapi_ie_len = 0;
		}
}

unsigned long WAI_RX_packets_indication(const u8* pbuf, int length)
{
	struct wapi_asue_st* wpa_s = s_asue;
	wapi_asue_rx_wai(wpa_s->bssid, (const u8*)pbuf, length);
	return 0;
}

int WIFI_lib_initialized()
{
    return s_init;
}

int WIFI_lib_init()
{
	struct wapi_asue_st* wpa_s = NULL;
	int ret = -1;

	/* check initialized */
	if (!s_init)
	{
		/* check the cpu mode(little and big) */
		short v_test = 1;
		char* p = (char*)&v_test;
		if (p[0])
		{
			s_l_endian = 1;
		}

		/* get memory for library main body */
		s_asue = iwn_get_buffer(sizeof(struct wapi_asue_st));
		if (s_asue == NULL)
			return ret;

		wpa_s = s_asue;

		/* ECC Lib init */
		ECC_Init();
		
		wpa_s->ptm = &wpa_s->vpt_tm;
		wpa_s->wapi_sm = &wpa_s->vpt_wapi_sm;

		wpa_s->wapi_sm->usksa.uskid = 1;
		wpa_s->wapi_sm->msksa.mskid = 1;
		wpa_s->wapi_sm->bksa.bk_len = BKID_LEN;

		/* X509 cert  initialize*/
		X509_init();

		/* initialized */
		s_init = 1;

		ret = 0;
	}

	return ret;
}

int WIFI_lib_exit()
{
	/* cleanup resource */
	timer_destory();
	cleanup_cert();
	X509_exit();

	s_asue = iwn_free_buffer(s_asue, sizeof(struct wapi_asue_st));

	/* not initialized */
	s_init = 0;	
	
	return 0;
}


