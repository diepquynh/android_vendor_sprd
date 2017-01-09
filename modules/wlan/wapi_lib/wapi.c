/** @file wapi.c
  * @brief This file contains functions for wapi, main is wai
 *
 *  Copyright (C) 2001-2008, Iwncomm Ltd.
 */

#include "alg_comm.h"
#include "wapi_common.h"
#include "cert.h"
#include "wapi_interface.h"

//#define REKEY 1

static int waigroup_cert_1_3(struct wapi_asue_st* wpa_s, u8* payload, int len);
static int waigroup_cert_2_3_send(struct wapi_asue_st* wpa_s, u8* payload, int len);
static int waigroup_cert_3_3(struct wapi_asue_st* wpa_s, u8* payload, int len);

static int waigroup_unicast_1_3(struct wapi_asue_st* wpa_s, u8* payload, int len);
static int waigroup_unicast_2_3_send(struct wapi_asue_st* wpa_s, u8* payload, int len_in);
static int waigroup_unicast_3_3(struct wapi_asue_st* wpa_s, u8* payload, int len);

static int waigroup_multicast_1_2(struct wapi_asue_st* wpa_s, u8* payload, int len);
static int waigroup_multicast_2_2_send(struct wapi_asue_st* wpa_s, u8* payload, int len);

static int wapi_install_usk(struct wapi_asue_st *wpa_s);
static int wapi_install_msk(struct wapi_asue_st *wpa_s, unsigned char *key_sc);

void wlan_deauth_for_otherc(void);

struct eloop_data iwn_eloop;


static void* MEMCPY(void *dbuf, const void *srcbuf, int len) 
{
	memcpy(dbuf, srcbuf, len); 
	return (char*)dbuf+len;
}

static struct wapi_rxfrag  *malloc_rxfrag(int maxlen)
{
	struct wapi_rxfrag *frag = NULL;
	frag  = (struct wapi_rxfrag *)iwn_get_buffer(sizeof(struct wapi_rxfrag ));
	if(frag != NULL){ 
		frag->maxlen = maxlen;
/*		frag->rxfragstamp = time(NULL);*/
		frag->data = (u8*)iwn_get_buffer(frag->maxlen);
		if(frag->data == NULL){
			frag  = (struct wapi_rxfrag *)iwn_free_buffer(frag, sizeof(struct wapi_rxfrag ));
		}
	}
	return frag;	
}
static void *free_rxfrag(struct wapi_rxfrag *frag)
{
	if(frag != NULL){
		iwn_free_buffer((void *)(frag->data), frag->maxlen);
		iwn_free_buffer((void *)frag, sizeof(struct wapi_rxfrag ));
	}
	return NULL;
}
static void wapi_put_frag(struct wapi_rxfrag *frag, u8 *data, int len)
{
	memcpy((unsigned char *)frag->data + frag->data_len, data, len);
	frag->data_len += len;

	{
		u16 tmp = (u16)frag->data_len;
		SETSHORT(((unsigned char *)(frag->data)+6), tmp);
	}
}
struct wapi_rxfrag *iwn_wpa_defrag(struct wapi_asue_st *wpa_s, struct wapi_rxfrag *rxbuf)
{
	u8 *buf =(u8 *)rxbuf->data;
	int len = rxbuf->data_len;
	
	struct wai_hdr *hdr = (struct wai_hdr *)buf;
	u16 rxseq=0, last_rxseq=0;
	u8 fragno, last_fragno;
	u8 more_frag = hdr->more_frag;
	
	struct wapi_rxfrag *wai_frame = NULL;
	GETSHORT(hdr->rxseq, rxseq);
	fragno = hdr->frag_sc;

	/* Quick way out, if there's nothing to defragment */
	if ((!more_frag) && (fragno == 0) && (wpa_s->rxfrag== NULL))
	{
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
		return rxbuf;
	}

	/*
	 * Update the time stamp.  As a side effect, it
	 * also makes sure that the timer will not change
	 * ni->ni_rxfrag[0] for at least 1 second, or in
	 * other words, for the remaining of this function.
	 */
	/*
	 * Validate that fragment is in order and
	 * related to the previous ones.
	 */
	if (wpa_s->rxfrag) {
		struct wai_hdr *hdr1;
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);

		hdr1 = (struct wai_hdr *) wpa_s->rxfrag->data;
		GETSHORT(hdr1->rxseq, last_rxseq);
		last_fragno = hdr1->frag_sc;
		if (rxseq != last_rxseq
		    || fragno != last_fragno + 1
			||(wpa_s->rxfrag->maxlen - wpa_s->rxfrag->data_len< len)
			/*||(time(NULL) - wpa_s->rxfragstamp >1)*/
			)
		{
			/*
			 * Unrelated fragment or no space for it,
			 * clear current fragments
			 */
			wpa_s->rxfrag = free_rxfrag(wpa_s->rxfrag); 
		}
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	}
	/* If this is the first fragment */
 	if (wpa_s->rxfrag == NULL && fragno == 0) {
		/*ni->ni_rxfrag[0] = skb;*/
		wpa_s->rxfrag = malloc_rxfrag(PAGE_LEN);
		/* If more frags are coming */
		if (more_frag) {
			wapi_put_frag(wpa_s->rxfrag, buf, len);
			iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
		
		}
	}
	else {
			if (wpa_s->rxfrag) {
			struct wai_hdr *lhdr = (struct wai_hdr *)wpa_s->rxfrag->data;

			/*
			 * We know we have enough space to copy,
			 * we've verified that before
			 */
			/* Copy current fragment at end of previous one */
			/* Update tail and length */
			wapi_put_frag(wpa_s->rxfrag , buf + WAI_HDR, len - WAI_HDR);
			
			/* Keep a copy of last sequence and fragno */
			*(u16 *)lhdr->rxseq = *(u16*)hdr->rxseq;
			lhdr->frag_sc = hdr->frag_sc;
		}
	}
		
	if (more_frag) {
		/* More to come */
		wai_frame = NULL;
	} else {
		/* Last fragment received, we're done! */
		wai_frame = wpa_s->rxfrag;
	}
/*	wpa_s->rxfragstamp = time(0);*/
	return wai_frame;
}


static int check_wai_frame(struct wapi_asue_st *wpa_s, const unsigned char *buf, size_t len)
{
	struct wai_hdr *hdr = (struct wai_hdr *)buf;
	u16 version = 0;
	u16 rxseq = 0;
	u16 frmlen = 0;
	if(len < sizeof(struct wai_hdr)){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame too short, len %d",  len);
		return -1;
	}
	version = ((hdr->version[0]<<8)| hdr->version[1]);
	GETSHORT(hdr->version, version);
	if( version != WAI_VERSION) {
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame Version(%u) is wrong", version);
		return -1;
	}

	if(hdr->type != WAI_TYPE){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame type(%u) is wrong", hdr->type);
		return -1;
	}
	
	if(((wpa_s->ap_type == AUTH_TYPE_WAPI_PSK) && (hdr->stype < WAI_USK_NEGOTIATION_REQUEST))
		|| ((hdr->stype < WAI_AUTHACTIVE))){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame stype(%u) is wrong ",hdr->stype);
		return -1;
	}
	
	GETSHORT((buf+6), frmlen);
	if(len != frmlen){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame length(%u) is wrong",  frmlen);
		return -1;
	}

	GETSHORT(hdr->rxseq, rxseq);
	if(rxseq < wpa_s->rxseq){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame packets_sc(%u) is wrong",  rxseq);
		return -1;
	}
	return 0;
}
static int check_challege(u8 *challenge_a , u8 *challenge_b)
{
	 if(memcmp(challenge_a, challenge_b, CHALLENGE_LEN))
	 {
		iwn_wpa_hexdump(MSG_DEBUG, "challenge_a", challenge_a, CHALLENGE_LEN);
		iwn_wpa_hexdump(MSG_DEBUG, "challenge_a", challenge_b, CHALLENGE_LEN);
	 	return -1;
	 }
	 return 0;
}
static int check_addid(struct wapi_asue_st *wpa_s, u8 *addid)
{
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
	if(memcmp(wpa_s->bssid, addid, ETH_ALEN) || 
		memcmp(wpa_s->own_addr, addid+ETH_ALEN, ETH_ALEN)){
		iwn_wpa_hexdump(MSG_DEBUG, "addid", addid ,2*ETH_ALEN);
		iwn_wpa_printf(MSG_DEBUG, "bssid="MACSTR"; ownaddr"MACSTR"",
			MAC2STR(wpa_s->bssid), MAC2STR(wpa_s->own_addr));
		return -1;
	}
	else
		return 0;
}
static int check_iv(struct wapi_msksa *msksa, u8 *msk_ann_id)
{
	return memcmp(msk_ann_id, msksa->msk_ann_id, 16);
}

static int cmp_var_struct(const void* remote,  const void* local, int len)
{
	 u16 remote_val_type = 0;
	 u16 remote_val_len = 0;
	 const u8 *p = NULL;
	
	wai_fixdata_id * id = (wai_fixdata_id *)local;
	
	p = remote;
	/*  identifier */
	GETSHORT(p, remote_val_type); p += sizeof(u16);
	/* data length */
	GETSHORT(p, remote_val_len); p += sizeof(u16);
	
	if ((remote_val_type != id->id_flag)
	     || (remote_val_len != id->id_len)
	     || memcmp(p, id->id_data, len))
	{
		return -1;
	}
	
	return 0;
}

static int asue_initialize_alg(struct wapi_asue_st *wpa_s)
{
	char alg_para_oid_der[16] = {0x06, 0x09,0x2a,(char)0x81,0x1c, (char)0xd7,0x63,0x01,0x01,0x02,0x01};
	
	memset((u8 *)&(wpa_s->sign_alg), 0, sizeof(wai_fixdata_alg));
	wpa_s->sign_alg.alg_length = 16;
	wpa_s->sign_alg.sha256_flag = 1;
	wpa_s->sign_alg.sign_alg = 1;
	wpa_s->sign_alg.sign_para.para_flag = 1;
	wpa_s->sign_alg.sign_para.para_len = 11;
	memcpy(wpa_s->sign_alg.sign_para.para_data, alg_para_oid_der, 11);
	return 0;
}

static void key_derivation(u8 *inkey, int inkey_len, u8 *text, int text_len, u8 *outkey, int outkey_len, u8 isusk)
{

	KD_hmac_sha256(text, text_len, inkey, inkey_len, outkey, outkey_len);
	/* iwn_wpa_hexdump(MSG_DEBUG, "text", text, text_len); */
	/* iwn_wpa_hexdump(MSG_DEBUG, "inkey", inkey, inkey_len); */
	
	if(isusk)
		mhash_sha256(outkey+outkey_len - CHALLENGE_LEN, CHALLENGE_LEN, outkey+outkey_len - CHALLENGE_LEN);
}

int iwn_wai_fixdata_id_by_ident(void *cert_st, wai_fixdata_id *fixdata_id, u16 index)
{
	u8 *temp ;
	byte_data		*subject_name = NULL;
	byte_data		*issure_name = NULL;
	byte_data		*serial_no = NULL;
	const struct cert_obj_st_t *cert_obj = NULL;

	if(fixdata_id == NULL || cert_st == NULL) 
		return -1;
	
	temp= fixdata_id->id_data;
	fixdata_id->id_flag = index;

	cert_obj = get_cert_obj(index);
	
	if((cert_obj == NULL)
		||(cert_obj->get_public_key == NULL)
		||(cert_obj->get_subject_name == NULL)
		||(cert_obj->get_issuer_name == NULL)
		||(cert_obj->get_serial_number == NULL)
		||(cert_obj->verify_key == NULL)
		||(cert_obj->sign == NULL)
		||(cert_obj->verify == NULL))
	{
		return -4;
	}
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	
	subject_name = (*cert_obj->get_subject_name)(cert_st);
	issure_name = (*cert_obj->get_issuer_name)(cert_st);
	serial_no = (*cert_obj->get_serial_number)(cert_st);
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	
	if((subject_name == NULL) || (issure_name == NULL) || (serial_no == NULL))
	{
		return -2;
	}

	iwn_wpa_hexdump(MSG_DEBUG, "AE ID subject: ", subject_name->data, subject_name->length);
	iwn_wpa_hexdump(MSG_DEBUG, "AE ID: issure", issure_name->data, issure_name->length);
	iwn_wpa_hexdump(MSG_DEBUG, "AE ID: Number", serial_no->data, serial_no->length);

	memcpy(temp, subject_name->data, subject_name->length);
	temp += subject_name->length;

	memcpy(temp, issure_name->data, issure_name->length);
	temp += issure_name->length;

	memcpy(temp, serial_no->data, serial_no->length);
	temp +=serial_no->length;

	fixdata_id->id_len = temp - fixdata_id->id_data;
	iwn_free_buffer(subject_name,sizeof(byte_data));
	iwn_free_buffer(issure_name,sizeof(byte_data));
	iwn_free_buffer(serial_no,sizeof(byte_data));
	
	return 0;
}


static int asue_x_x_p_derivation(struct wapi_asue_st *wpa_s)
{
       if (wpa_s == NULL)
	   	return -1;

       /* clear buffer */
	wpa_s->asue_eck.length = 0;
	memset(wpa_s->asue_eck.data, 0, sizeof(wpa_s->asue_eck.data));
	wpa_s->asue_key_data.length = 0;
	memset(wpa_s->asue_key_data.data, 0, sizeof(wpa_s->asue_key_data.data));

       /* get the public key and the private key for ECC */
	if (ecc192_genkey(wpa_s->asue_eck.data, wpa_s->asue_key_data.data) != 0)
	{
	     return -1;
	}

	wpa_s->asue_eck.length          = SECKEY_LEN;
	wpa_s->asue_key_data.length  = PUBKEY2_LEN;

	return 0;
}

static int asue_certauthbk_derivation(struct wapi_asue_st *wpa_s)
{
	char input_text[] = "base key expansion for key and additional nonce";
	u8 text[256] = {0,};
	u8 temp_out[48] = {0,};
	u8  ecdhkey[24] = {0,};
	int  ecdhkeyl = sizeof(ecdhkey);
	int ret = -1;

	iwn_wpa_hexdump(MSG_DEBUG, "asue_eck", wpa_s->asue_eck.data,wpa_s->asue_eck.length);
	iwn_wpa_hexdump(MSG_DEBUG, "ae_key_data", wpa_s->ae_key_data.data,wpa_s->ae_key_data.length);

	ret = ecc192_ecdh(wpa_s->asue_eck.data, wpa_s->ae_key_data.data, ecdhkey);

	if (!ret)
	{
		iwn_wpa_printf(MSG_DEBUG, "asue_certauthbk_derivation ECHD fail : in %s:%d", __func__, __LINE__);
		ret = -1;
		return ret;
	}

	iwn_wpa_hexdump(MSG_DEBUG, "ecdhkey", ecdhkey,ecdhkeyl);
		

	memset(text, 0, sizeof(text));
	memcpy(text, wpa_s->Nae, 32);
	memcpy(text + 32, wpa_s->Nasue, 32);
	memcpy(text + 32 + 32, input_text, strlen(input_text));
	KD_hmac_sha256(text, 32+32+strlen(input_text), 
						ecdhkey, 24,
						temp_out, 16 + 32);
	iwn_wpa_hexdump(MSG_DEBUG, "text", text,32+32+strlen(input_text));
	iwn_wpa_hexdump(MSG_DEBUG, "temp_out",temp_out,48);
	
	memcpy(wpa_s->wapi_sm->bksa.bk, temp_out, 16);
	
	memset(text, 0, sizeof(text));
	memcpy(text, wpa_s->bssid, ETH_ALEN);
	memcpy(text + ETH_ALEN, wpa_s->own_addr, ETH_ALEN);
	iwn_wpa_hexdump(MSG_DEBUG, "text1", text,32+32+strlen(input_text));
	
	KD_hmac_sha256(text, 12,
						wpa_s->wapi_sm->bksa.bk, 16, 
						wpa_s->wapi_sm->bksa.bkid, 16);
	
	mhash_sha256(temp_out + 16, 32, wpa_s->ae_auth_flag);
	iwn_wpa_hexdump(MSG_DEBUG, "bk", wpa_s->wapi_sm->bksa.bk, 16);
	iwn_wpa_hexdump(MSG_DEBUG, "bkid", wpa_s->wapi_sm->bksa.bkid, 16);
	ret = 0;

	return ret;
}

static struct wapi_usk *get_usk(struct wapi_state_machine *sm, u8 uskid)
{
	if(uskid == sm->usksa.uskid)
		return &(sm->usksa.usk[uskid]);
	else
		return NULL;
}

static u8 * wapi_build_hdr(u8 *pos, u16 txseq, u8 stype)
{
	struct wai_hdr *hdr = (struct wai_hdr *)pos;
	
	SETSHORT(hdr->version, WAI_VERSION);
	hdr->type = WAI_TYPE;
	hdr->stype= stype;
	SETSHORT(hdr->reserve, 0);
	hdr->length = 0x0000;
	SETSHORT(hdr->rxseq, txseq);
	hdr->frag_sc = 0;
	hdr->more_frag = 0;
	return (u8 *)(hdr+1);
}
static void wapi_set_length(u8 *pos, u16 length)
{
	SETSHORT((pos+6), length);
	//struct wai_hdr *hdr = (struct wai_hdr *)pos;
	//hdr->length = iwn_htons(length);
}



void iwn_wapi_sm_rx_wai(struct wapi_asue_st *wpa_s, const unsigned char *src_addr, const unsigned char *buf, size_t len)
{
	size_t plen;
	struct wai_hdr *hdr;
	u8 *wai_payload ;
	int res = 0;
	int frmlen = 0;
	
	src_addr = src_addr;/*disable warnning*/

	if(wpa_s->wapi_state <WAPISM_AL_ASSOC)
	{
		iwn_wpa_printf(MSG_DEBUG, "wpa_s->wapi_state <WAPISM_AL_ASSOC");
		wpa_s->rxfrag = free_rxfrag(wpa_s->rxfrag);
		return ;
	}
	else if((wpa_s->ap_type == AUTH_TYPE_WAPI) && (iwn_eloop.has_cert == 0))
	{
		iwn_wpa_printf(MSG_DEBUG, "No cert");
		wpa_s->rxfrag = free_rxfrag(wpa_s->rxfrag);
		return ;
	}

	hdr = (struct wai_hdr *) buf;
	if(check_wai_frame(wpa_s, buf, len) != 0){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: WAI frame is wrong");
		wpa_s->rxfrag = free_rxfrag(wpa_s->rxfrag);
		return;
	}
	wai_payload = (u8 *) (hdr + 1);
	
	GETSHORT((buf+6), frmlen);
	//plen = iwn_ntohs(hdr->length) - sizeof(*hdr);
	plen = frmlen - sizeof(*hdr);
	iwn_wpa_printf(MSG_DEBUG, "iwn_wapi_sm_rx_wai  plen = '%d', hdr->length = '%d'. \n", plen, frmlen);

	switch(hdr->stype)
	{
		case WAI_AUTHACTIVE:
		    iwn_wpa_printf(MSG_DEBUG, "%s: received WAI_AUTHACTIVE\n", __func__);
			res = waigroup_cert_1_3(wpa_s, wai_payload, plen);
			break;
		case WAI_ACCESS_AUTH_RESPONSE:
		    iwn_wpa_printf(MSG_DEBUG, "%s: received WAI_ACCESS_AUTH_RESPONSE\n", __func__);
			res = waigroup_cert_3_3(wpa_s, wai_payload, plen);
			if (0 == res)
			{
				timer_reset();
			}
			else
				if (-1 == res)
				{
					timer_resend();
				}
				else
					if (-2 == res)
					{
						wlan_deauth_for_otherc();
					}
			break;
		case WAI_USK_NEGOTIATION_REQUEST:
		    iwn_wpa_printf(MSG_DEBUG, "%s: received WAI_USK_NEGOTIATION_REQUEST\n", __func__);
			res = waigroup_unicast_1_3(wpa_s, wai_payload, plen);
			//SPRD: bug 571144 - Report result when failed to connect.
			if (-1 == res)
			{
				wlan_deauth_for_otherc();
			}
			break;
		case WAI_USK_NEGOTIATION_CONFIRMATION:	
		    iwn_wpa_printf(MSG_DEBUG, "%s: received WAI_USK_NEGOTIATION_CONFIRMATION\n", __func__);
			res = waigroup_unicast_3_3(wpa_s, wai_payload, plen);
			if (0 == res)
			{
				timer_reset();
			}
			else
				if (-1 == res)
				{
					timer_resend();
				}
			break;
		case WAI_MSK_ANNOUNCEMENT:
		    iwn_wpa_printf(MSG_DEBUG, "%s: received WAI_MSK_ANNOUNCEMENT\n", __func__);
			res = waigroup_multicast_1_2(wpa_s, wai_payload,plen);
			if(res)
				wlan_deauth_for_otherc();
			break;

		case WAI_STAKEY_REQUEST:
		    iwn_wpa_printf(MSG_DEBUG, "%s: received WAI_STAKEY_REQUEST\n", __func__);
			iwn_wpa_printf(MSG_DEBUG, "WAPILib: receive ignore frame stype %u", hdr->stype);
			break;

		case WAI_PREAUTH_START:
		case WAI_ACCESS_AUTH_REQUEST:
		case WAI_CERT_AUTH_REQUEST:
		case WAI_CERT_AUTH_RESPONSE:
		case WAI_USK_NEGOTIATION_RESPONSE:
		case WAI_MSK_ANNOUNCEMENT_RESPONSE:
			iwn_wpa_printf(MSG_DEBUG, "WAPILib: receive error frame stype %u", hdr->stype);
			break;

		default:
			iwn_wpa_printf(MSG_DEBUG, "WAPILib: receive unknown frame stype %u", hdr->stype);
			break;
	}
	wpa_s->rxfrag = free_rxfrag(wpa_s->rxfrag); 
}

static int waigroup_cert_1_3(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	u8 flag = 0;
	u8 *ae_auth_flag = NULL;
	u8 *asu_id = NULL;
	u8 *ae_cer = NULL;
	u8 *ecdh = NULL;
	int ret = -1;
	

	u8 auth_act_len = WAI_FLAG_LEN  
		+32
		+2/*ASU ID Identifier*/
		+2/*ASU ID Length*/
		+2/*AE Cert type*/
		+2/*AE Cert length*/
		+1/*ECDH Parameter Identifier*/
		+2/*ECDH Parameter Length */;

	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	
	if(len < auth_act_len)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: WAI frame payload too short");
		return -1;
	}
	

	flag = payload[0];
	ae_auth_flag = payload+1;
	asu_id = payload+1 + 32;
	
	len -= auth_act_len;
	
	if (AUTH_TYPE_WAPI != wpa_s->ap_type)
	{
		return -1;
	}
	else
	{
		int bk_up = flag & BIT(0);
		wapi_states require = bk_up ? WAPISM_FINISH : WAPISM_AL_ASSOC;
		
		if (wpa_s->wapi_state != require)
		{
		       /* skip the usk update check */
		       if (wpa_s->usk_updated && bk_up && wpa_s->wapi_state == WAPISM_UNI_ACK)
			   	;
			else
				return -1;
		}
	}

	if(flag &BIT(0)){
		if(memcmp(ae_auth_flag, wpa_s->ae_auth_flag, 32) !=0)
		{
			iwn_wpa_printf(MSG_ERROR, "WAPILib: ae_auth_flag not same!\n");
			return -1;
		}
	}else{
	
		memcpy(wpa_s->ae_auth_flag, ae_auth_flag, 32);
	}
	
	/*get ASU ID*/	
	GETSHORT(asu_id, wpa_s->ae_asu_id.id_flag); 
	GETSHORT((asu_id+2), wpa_s->ae_asu_id.id_len);

	if(len <wpa_s->ae_asu_id.id_len){
		
		iwn_wpa_printf(MSG_ERROR, "WAPILib: WAI frame payload too short");
		return -1;
	}
	else{
		memcpy(wpa_s->ae_asu_id.id_data, asu_id+4, wpa_s->ae_asu_id.id_len);
	}
	
	len -= wpa_s->ae_asu_id.id_len; 

	/*get AE Certificate*/
	ae_cer = asu_id + 2 + 2 + wpa_s->ae_asu_id.id_len;
	
	GETSHORT(ae_cer, wpa_s->ae_cert.cert_flag); 
	GETSHORT((ae_cer+2), wpa_s->ae_cert.length); 
	
	if(len <wpa_s->ae_cert.length){
		
		iwn_wpa_printf(MSG_ERROR, "WAPILib: WAI frame payload too short");
		return -1;
	}
	else{
		memcpy(wpa_s->ae_cert.data, ae_cer+4, wpa_s->ae_cert.length);
	}
	len -= wpa_s->ae_cert.length;

	/*get AE ID*/
	iwn_wai_fixdata_id_by_ident(&wpa_s->ae_cert, &(wpa_s->ae_id), wpa_s->ae_cert.cert_flag);
	iwn_wpa_hexdump(MSG_DEBUG, "AE ID: ", wpa_s->ae_id.id_data, wpa_s->ae_id.id_len);

	/*get ECDH Parameter*/
	ecdh = ae_cer + 2 + 2 + wpa_s->ae_cert.length;

	wpa_s->ecdh.para_flag = ecdh[0];
	GETSHORT((ecdh + 1), wpa_s->ecdh.para_len);

	if(len <wpa_s->ecdh.para_len){
		
		iwn_wpa_printf(MSG_ERROR, "WAPILib: WAI frame payload too short");
		return -1;
	}
	else{
		memcpy(wpa_s->ecdh.para_data, ecdh+3, wpa_s->ecdh.para_len);
	}
	len -= wpa_s->ecdh.para_len;

	
	if(len != 0) 
	{
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
		return -1;
	}
	
	asue_initialize_alg(wpa_s);
	ret = asue_x_x_p_derivation(wpa_s);
	if(ret == -1)
		return -1;

	get_random(wpa_s->Nasue, 32);

	if (waigroup_cert_2_3_send(wpa_s, payload, len))
	{
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
		return -1;
	}
	return 0;
}

static int waigroup_cert_2_3_send(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	static comm_data data_buff;
	static tsign				sign;
	u8 *sign_len_pos = NULL;
	static u8 tbuf[2048];
	u8 *pos = NULL;
	const struct cert_obj_st_t 	*cert_obj = NULL;

	payload = payload;/*disable warnning*/
	len = len;/*disable warnning*/
	
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	
	pos = tbuf;
	memset(pos, 0, sizeof(tbuf));
	memset(&data_buff, 0, sizeof(data_buff));
	memset(&sign, 0, sizeof(sign));
	
	pos = wapi_build_hdr(pos, ++wpa_s->txseq, WAI_ACCESS_AUTH_REQUEST);
	wpa_s->flag = BIT(2) |wpa_s->flag;
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,wpa_s->flag=%d", __func__, __LINE__,wpa_s->flag);
	*pos = wpa_s->flag;/*flag*/
	pos = pos + 1;
	pos= MEMCPY(pos, &(wpa_s->ae_auth_flag), 32);
	pos= MEMCPY(pos, &(wpa_s->Nasue), 32);
	pos= MEMCPY(pos, &(wpa_s->asue_key_data.length), 1);
	pos= MEMCPY(pos, &(wpa_s->asue_key_data.data), wpa_s->asue_key_data.length);
	
	SETSHORT(pos,wpa_s->ae_id.id_flag); pos += 2;
	SETSHORT(pos,wpa_s->ae_id.id_len); pos += 2;
	pos = MEMCPY(pos, wpa_s->ae_id.id_data, wpa_s->ae_id.id_len);
	#if 0
	{
		
		u16 tmp = wpa_s->ae_id.id_flag;
		pos = MEMCPY(pos, &(tmp), 2);
		tmp = iwn_htons(wpa_s->ae_id.id_len);
		pos = MEMCPY(pos, &(tmp), 2);
		pos = MEMCPY(pos, wpa_s->ae_id.id_data, wpa_s->ae_id.id_len);
	}
	#endif
	
	SETSHORT(pos,iwn_eloop.cert_info.config.used_cert); pos += 2;
	SETSHORT(pos,iwn_eloop.cert_info.asue_cert_obj->cert_bin->length); pos += 2;
	#if 0
	{
		u16 temp_len = iwn_htons(iwn_eloop.cert_info.config.used_cert);
		pos= MEMCPY(pos, &(temp_len), 2);
		temp_len = iwn_htons(iwn_eloop.cert_info.asue_cert_obj->cert_bin->length);
		pos= MEMCPY(pos, &(temp_len), 2);
	}
	#endif
	
	pos= MEMCPY(pos, iwn_eloop.cert_info.asue_cert_obj->cert_bin->data, iwn_eloop.cert_info.asue_cert_obj->cert_bin->length);/*ASUEÖ¤Êé*/
	iwn_wpa_hexdump(MSG_DEBUG, "cert_bin->data", iwn_eloop.cert_info.asue_cert_obj->cert_bin->data, iwn_eloop.cert_info.asue_cert_obj->cert_bin->length);

	*pos = wpa_s->ecdh.para_flag;   pos++;
	//pos= MEMCPY(pos, &(wpa_s->ecdh.para_flag), 1);/*ecdh*/
	SETSHORT(pos,wpa_s->ecdh.para_len); pos += 2;
	pos= MEMCPY(pos, &(wpa_s->ecdh.para_data), wpa_s->ecdh.para_len);/*ecdh*/
#if 0
	{
		u16 temp_len = iwn_htons(wpa_s->ecdh.para_len);
		pos= MEMCPY(pos, &(wpa_s->ecdh.para_flag), 1);/*ecdh*/
		pos= MEMCPY(pos, &temp_len, 2);/*ecdh*/
		pos= MEMCPY(pos, &(wpa_s->ecdh.para_data), wpa_s->ecdh.para_len);/*ecdh*/
	}
#endif
	data_buff.length = pos - tbuf - sizeof(struct wai_hdr);
	memcpy(data_buff.data, tbuf + sizeof(struct wai_hdr), data_buff.length);/*????*/
	cert_obj = get_cert_obj(iwn_eloop.cert_info.config.used_cert);
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,used_cert=%d", __func__, __LINE__,iwn_eloop.cert_info.config.used_cert);
	/*iwn_wpa_hexdump(MSG_DEBUG, "private_key ", cert_obj->private_key->data, cert_obj->private_key->length);*/

	if(!(*cert_obj->sign)(
		cert_obj->private_key->data,
		 cert_obj->private_key->length,
		data_buff.data, 	data_buff.length, sign.data))
	{
		iwn_wpa_printf(MSG_ERROR,"fail to sign data and will exit !!\n");
		return -1;
	}
	*pos ++= 1;
	sign_len_pos = pos;				
	pos += 2;/*length*/
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);

	SETSHORT(pos,iwn_eloop.asue_id.id_flag); pos += 2;
	SETSHORT(pos,iwn_eloop.asue_id.id_len); pos += 2;
	pos= MEMCPY(pos, &(iwn_eloop.asue_id.id_data), iwn_eloop.asue_id.id_len);
	
#if 0
	{
		u16 tmp_flag = 0;
		tmp_flag = iwn_htons(iwn_eloop.asue_id.id_flag);
		pos= MEMCPY(pos, &(tmp_flag), 2);
		tmp_flag = iwn_htons(iwn_eloop.asue_id.id_len);
		pos= MEMCPY(pos, &(tmp_flag), 2);
		pos= MEMCPY(pos, &(iwn_eloop.asue_id.id_data), iwn_eloop.asue_id.id_len);
	}
#endif

	SETSHORT(pos,wpa_s->sign_alg.alg_length); pos += 2;
	*pos = wpa_s->sign_alg.sha256_flag; pos++;
	*pos = wpa_s->sign_alg.sign_alg; pos++;
	*pos = wpa_s->sign_alg.sign_para.para_flag; pos++;
	SETSHORT(pos,wpa_s->sign_alg.sign_para.para_len); pos += 2;
	pos= MEMCPY(pos, &(wpa_s->sign_alg.sign_para.para_data), wpa_s->sign_alg.sign_para.para_len);
	SETSHORT(pos,SIGN_LEN); pos += 2;
	pos = MEMCPY(pos, &(sign.data), SIGN_LEN);
#if 0
	{
		u16 tmp_flag = 0;
		tmp_flag = iwn_htons(wpa_s->sign_alg.alg_length);
		pos= MEMCPY(pos, &(tmp_flag), 2);
		pos= MEMCPY(pos, &(wpa_s->sign_alg.sha256_flag), 1);
		pos= MEMCPY(pos, &(wpa_s->sign_alg.sign_alg), 1);
		pos= MEMCPY(pos, &(wpa_s->sign_alg.sign_para.para_flag), 1);
		tmp_flag = iwn_htons(wpa_s->sign_alg.sign_para.para_len);
		pos= MEMCPY(pos, &(tmp_flag), 2);
		pos= MEMCPY(pos, &(wpa_s->sign_alg.sign_para.para_data), wpa_s->sign_alg.sign_para.para_len);
		tmp_flag =iwn_htons( 48);
		pos= MEMCPY(pos, &(tmp_flag), 2);
		pos= MEMCPY(pos, &(sign.data), 48);
	}
#endif
	SETSHORT(sign_len_pos,(pos-sign_len_pos-2));
#if 0
	{
		u16 packet_len = 0;
		packet_len = iwn_htons((short)(pos-sign_len_pos-2));
		memcpy(sign_len_pos , &packet_len , 2);
	}
#endif	
	wapi_set_length(tbuf, (short)(pos-tbuf));
	iwn_wpa_ether_send(tbuf, pos-tbuf);
	timer_set(1, tbuf, pos-tbuf);

	wpa_s->wapi_state = WAPISM_CNT_REQ;

	return 0;
}


static int waigroup_cert_3_3(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	u8 flag = 0 ;
	u8 *Nasue = NULL, *Nae = NULL, *acc_res = NULL;
	u8 *asue_key_data = NULL,*ae_key_data = NULL;
	u8 *ae_id = NULL, *asue_id = NULL, *cert_res = NULL;
	u8 *ae_sign = NULL;
	int request_len = 0;
	const struct cert_obj_st_t  *cert_obj = NULL;
	
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);

	if (wpa_s->wapi_state != WAPISM_CNT_REQ)
	{
		return -1;
	}

	/*flag*/
	flag = payload[0];
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,flag=%d", __func__, __LINE__,flag);
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,wpa_s->flag=%d,(flag & BIT(0) )=%d", __func__, __LINE__,flag,(flag & BIT(0) ));
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,wpa_s->flag=%d,(flag & BIT(0) )=%d", __func__, __LINE__,flag,(wpa_s->flag& BIT(0) ));
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,wpa_s->flag=%d,(flag & BIT(1) )=%d", __func__, __LINE__,flag,(flag & BIT(1) ));
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,wpa_s->flag=%d,(flag & BIT(1) )=%d", __func__, __LINE__,flag,(wpa_s->flag & BIT(1) ));

	if((flag &0x03)!= (wpa_s->flag & 0x03))
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: not same flag bit 0,1!\n");
		return -1;
	}
	request_len +=1;
	Nasue = payload+1;
	if(Nasue == NULL) return -1;
	if(memcmp(Nasue, wpa_s->Nasue , 32)!=0)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: not same Nasue!\n");
		return -1;
	}
	request_len +=32;
	Nae = Nasue + 32;
	if(Nae== NULL) return -1;
	memcpy(wpa_s->Nae, Nae, 32);
	request_len +=32;
	acc_res = Nae + 32;
	if(acc_res == NULL) return -1;
	if(acc_res[0] != 0) return -2;
	request_len +=1;
	asue_key_data = acc_res + 1;
	if(asue_key_data == NULL) return -1;
	if((asue_key_data[0]!=wpa_s->asue_key_data.length)
		||memcmp((char *)asue_key_data+1, &(wpa_s->asue_key_data.data), wpa_s->asue_key_data.length )!=0)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: not same asue key data!,asue_len =%d,wpa_s->asue_key_data.length=%d\n",asue_key_data[0],wpa_s->asue_key_data.length);

		return -1;
	}
	request_len +=wpa_s->asue_key_data.length + 1;
	ae_key_data = asue_key_data + 	wpa_s->asue_key_data.length + 1;
	if(ae_key_data == NULL) return -1;
	memcpy(wpa_s->ae_key_data.data, ae_key_data+1,ae_key_data[0]);
	wpa_s->ae_key_data.length = ae_key_data[0];
	request_len +=ae_key_data[0] + 1;
	ae_id = ae_key_data + ae_key_data[0] + 1;
	if(ae_id == NULL) return -1;
	if (cmp_var_struct(ae_id, &(wpa_s->ae_id), wpa_s->ae_id.id_len))
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: not same ae id!\n");
		iwn_wpa_hexdump(MSG_ERROR, "ae_id", ae_id,wpa_s->ae_id.id_len+ 4);
		iwn_wpa_hexdump(MSG_ERROR, "wpa_s->ae_id", (unsigned char *)&(wpa_s->ae_id),wpa_s->ae_id.id_len+ 4);
		return -1;
	}
	request_len +=wpa_s->ae_id.id_len+ 4;
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,request_len=%d,wpa_s->ae_id.id_len=%d", __func__, __LINE__,request_len,wpa_s->ae_id.id_len);
	asue_id = ae_id + wpa_s->ae_id.id_len+ 4;
	if(asue_id == NULL) return -1;
	if (cmp_var_struct(asue_id, &(iwn_eloop.asue_id), iwn_eloop.asue_id.id_len))
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: not same asue id!\n");
		return -1;
	}
	request_len +=iwn_eloop.asue_id.id_len+ 4;
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,request_len=%d,wpa_s->asue_id.id_len=%d", __func__, __LINE__,request_len,iwn_eloop.asue_id.id_len);


	if(flag & BIT(3))
	{
		u8 *cert_pos = NULL, *asu_sign = NULL;
		u16 fix_data_len=0, sign_len=0; 
		tkey *pubkey = NULL;
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,iwn_eloop.asue_id.id_len=%d", __func__, __LINE__,iwn_eloop.asue_id.id_len);

		cert_res = asue_id +iwn_eloop.asue_id.id_len+ 4;
		if(cert_res[0] != 2)
		{
			iwn_wpa_printf(MSG_ERROR, "cert result flag is not 2!\n");
			return -1;
		}
		cert_pos = cert_res + 1 +2 + 32 + 32 + 1 + 2 ;
		GETSHORT((cert_pos), fix_data_len);
		cert_pos = cert_pos + fix_data_len + 2;
		if(cert_pos[0] != 0)
		{
			iwn_wpa_printf(MSG_ERROR, "cert result  is not ok!\n");
			return -2;
		}
		GETSHORT((cert_pos + 1 + 2), fix_data_len);
		asu_sign = cert_pos + fix_data_len + 1 + 2 + 2;
		GETSHORT((asu_sign + 1), sign_len);
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);

		GETSHORT((cert_res + 1), fix_data_len);
		cert_obj = get_cert_obj(iwn_eloop.cert_info.config.used_cert);
		pubkey = cert_obj->asu_pubkey;
		if(!(*cert_obj->verify)(pubkey->data, pubkey->length,
								cert_res,fix_data_len + 3,
								asu_sign + 1 + 2 + sign_len - 48, 
								48))
		{
			iwn_wpa_hexdump(MSG_ERROR, "pubkey->data", pubkey->data,pubkey->length);
			iwn_wpa_hexdump(MSG_ERROR, "cert_res", cert_res,fix_data_len + 3);
			iwn_wpa_hexdump(MSG_ERROR, "asu_sign", asu_sign + 1 + 2 + sign_len - 48,48);
			iwn_wpa_printf(MSG_DEBUG, "ASU sign error!!!\n");
			return -1;
		}
		request_len +=fix_data_len + 3+sign_len+3 ;
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,request_len=%d,len=%d", __func__, __LINE__,request_len,len);

	}
	{
		tkey *pubkey = NULL;
		u16 fix_data_len = 0;
		u16 cert_flag = wpa_s->ae_cert.cert_flag;//iwn_ntohs(wpa_s->ae_cert.cert_flag);

		ae_sign = payload + len - 48;
		cert_obj = get_cert_obj(cert_flag);
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,wpa_s->ae_cert.len=%d", __func__, __LINE__,wpa_s->ae_cert.length);
		pubkey = (*cert_obj->get_public_key)((void *)&wpa_s->ae_cert);
		if(!(*cert_obj->verify)(pubkey->data, pubkey->length,
							payload,request_len,
							ae_sign, 
							48))
					
		{
			pubkey = iwn_free_buffer(pubkey, sizeof(tkey));
			printf("AE sign error!!!\n");
			return -1;
		}
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
		pubkey = iwn_free_buffer(pubkey, sizeof(tkey));
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
		GETSHORT((payload+request_len+1), fix_data_len);
		request_len += fix_data_len + 3;
	}
	if(len != request_len)
	{
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d,request_len=%d,len=%d", __func__, __LINE__,request_len,len);
		return -1;
	}

	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	asue_certauthbk_derivation(wpa_s);

	wpa_s->wapi_state = WAPISM_CNT_RESP;

	return 0;
}

static int waigroup_unicast_1_3(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	u8 flag = 0;
	u8 *bkid = NULL;
	u8 uskid = 0;
	u8 *addid = NULL;
	u8 *ae_challenge = NULL;
	u8 *text = NULL;
	u8 *pos = NULL;
	int text_len = 0;
	
	struct wapi_state_machine *wapi_sm = wpa_s->wapi_sm;

	u8 usk_request_len = WAI_FLAG_LEN +1 +BKID_LEN +ADDID_LEN + CHALLENGE_LEN;


	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);

	if(len < usk_request_len)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: WAI frame payload too short, len(%u)", len);
		return -1;
	}
	flag = payload[0];
	bkid = payload+1;
	uskid = (bkid+BK_LEN)[0] ;  
	addid = (bkid+BK_LEN+1);
	ae_challenge = addid +ETH_ALEN*2;

	{
		int usk_up = flag & BIT(4);
		wapi_states require = WAPISM_FINISH;
		if (usk_up)
		{
		    iwn_wpa_printf(MSG_DEBUG, "~~~~~ WAPILib: %s USK rekeying == 1 ~~~~~", __func__);
		    iwn_wpa_printf(MSG_DEBUG, "WAPILib: new uskid=%d, wapi_sm->usksa.uskid=%d", 
				   uskid, wapi_sm->usksa.uskid);
		    require = WAPISM_FINISH;
		}
		else
		{
			int psk = (AUTH_TYPE_WAPI_PSK==wpa_s->ap_type);
			require = psk ? WAPISM_AL_ASSOC : WAPISM_CNT_RESP;
		}

		if (wpa_s->wapi_state != require)
		{
			/* skip the usk update */
			if (wpa_s->usk_updated 
				&& usk_up 
				&& (wpa_s->wapi_state     == WAPISM_FINISH 
				      || wpa_s->wapi_state == WAPISM_UNI_ACK
				      || wpa_s->wapi_state == WAPISM_CNT_RESP))
			    iwn_wpa_printf(MSG_DEBUG, "WAPILib: %s skipped the usk update", __func__);
			else {
			    iwn_wpa_printf(MSG_ERROR, "WAPILib: %s exiting -1", __func__);
			    return -1;
			}
		}
	}

	if(memcmp(bkid, wapi_sm->bksa.bkid, BKID_LEN) != 0)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: BKID is wrong");
		iwn_wpa_hexdump(MSG_ERROR, "AE BKID", bkid, BKID_LEN);
		iwn_wpa_hexdump(MSG_ERROR, "ASUE BKID", wapi_sm->bksa.bkid, BKID_LEN);
		iwn_wpa_hexdump(MSG_ERROR, "ASUE BK", wapi_sm->bksa.bk, BKID_LEN);
		return -1;
	}
	else {
	    iwn_wpa_printf(MSG_DEBUG, "WAPILib: BKID matched \n");
	    iwn_wpa_hexdump(MSG_DEBUG, "AE BKID", bkid, BKID_LEN);
	    iwn_wpa_hexdump(MSG_DEBUG, "ASUE BKID", wapi_sm->bksa.bkid, BKID_LEN);
	    iwn_wpa_hexdump(MSG_DEBUG, "ASUE BK", wapi_sm->bksa.bk, BKID_LEN);
	}
	
	if((flag & BIT(4)) && (uskid != !wapi_sm->usksa.uskid))
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: USKID is not invalid");
		return -1;
	}

	if((flag & BIT(4)) && check_challege(wapi_sm->usksa.usk[wapi_sm->usksa.uskid].ae_challenge, ae_challenge))
	{
		iwn_wpa_hexdump(MSG_ERROR, "ae_challenge", ae_challenge, CHALLENGE_LEN);
		return -1;
		
	}

	wapi_sm->usksa.uskid = uskid;

	text_len = strlen(USK_TEXT) + ADDID_LEN + CHALLENGE_LEN*2;
	text = iwn_get_buffer(text_len);
	if(text == NULL)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: malloc failure");
		return -1;
	}
	pos = text;
	get_random(wapi_sm->usksa.usk[uskid].asue_challenge, CHALLENGE_LEN);
	
	pos= MEMCPY(pos, addid, ADDID_LEN);
	pos= MEMCPY(pos, ae_challenge, CHALLENGE_LEN);
	iwn_wpa_hexdump(MSG_DEBUG, "asue_challenge", wapi_sm->usksa.usk[uskid].asue_challenge, 32);
	iwn_wpa_hexdump(MSG_DEBUG, "ae_challenge", ae_challenge, 32);
	pos= MEMCPY(pos, wapi_sm->usksa.usk[uskid].asue_challenge, CHALLENGE_LEN);
	pos= MEMCPY(pos, USK_TEXT, strlen(USK_TEXT));
	
	key_derivation(wapi_sm->bksa.bk, wapi_sm->bksa.bk_len, text, text_len, wapi_sm->usksa.usk[uskid].uek, USKSA_LEN, ISUSK);
	iwn_free_buffer(text, text_len);
	/*iwn_wpa_hexdump(MSG_DEBUG, "outkey", wapi_sm->usksa.usk[uskid].uek, USKSA_LEN);*/
	if (waigroup_unicast_2_3_send(wpa_s, payload, len))
		return -1;
	return 0;
}
static int waigroup_unicast_2_3_send(struct wapi_asue_st* wpa_s, u8* payload, int len_in)
{
	struct wapi_state_machine *wapi_sm = wpa_s->wapi_sm;
	struct wapi_usk *usk = &wapi_sm->usksa.usk[wapi_sm->usksa.uskid];

	len_in = len_in;/*disable warnning*/
	
	u8 *tbuf = NULL;
	u8 *pos = NULL;
	int len = WAI_FLAG_LEN+BKID_LEN+1+ADDID_LEN;
	int tbuflen =WAI_HDR+ len+2*CHALLENGE_LEN + wapi_sm->assoc_wapi_ie_len+ WAI_MIC_LEN;

	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	tbuf = iwn_get_buffer(tbuflen);
	if(tbuf == NULL)
		return -1;
	/*iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: tbuf", tbuf, tbuflen);*/
	pos = tbuf;
	pos = wapi_build_hdr(pos, ++wpa_s->txseq, WAI_USK_NEGOTIATION_RESPONSE);
	pos= MEMCPY(pos, payload, len);
	pos= MEMCPY(pos, usk->asue_challenge, CHALLENGE_LEN);
	iwn_wpa_hexdump(MSG_DEBUG, "asue_challenge", usk->asue_challenge, 32);
	pos= MEMCPY(pos, payload+len, CHALLENGE_LEN);
	iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: tbuf", tbuf, tbuflen);

	pos= MEMCPY(pos, wapi_sm->assoc_wapi_ie, wapi_sm->assoc_wapi_ie_len);
	wapi_hmac_sha256(tbuf+WAI_HDR, tbuflen-WAI_MIC_LEN-WAI_HDR, usk->mak,PAIRKEY_LEN,pos, WAI_MIC_LEN);
	/*iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: mic", pos, WAI_MIC_LEN);*/
	pos += WAI_MIC_LEN;
	wapi_set_length(tbuf, (short)(pos-tbuf));
	iwn_wpa_ether_send(tbuf, tbuflen);
	timer_set(2, tbuf, tbuflen);
	iwn_free_buffer(tbuf, tbuflen);

	wpa_s->wapi_state = WAPISM_UNI_RESP;

	return 0;
}
static int waigroup_unicast_3_3(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	struct wapi_state_machine*wapi_sm = wpa_s->wapi_sm;
	int uskid = wpa_s->wapi_sm->usksa.uskid;
	struct wapi_usk *usk = &wapi_sm->usksa.usk[uskid];

	
	u8  *pos = payload+WAI_FLAG_LEN+BKID_LEN+1+ADDID_LEN;
	u8 *wie = NULL;
	u8  wie_len = 0;
	u8 mic[WAI_MIC_LEN] = {0,};
	u8 *rxmic = payload+len -WAI_MIC_LEN;
	
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);

	if (wpa_s->wapi_state != WAPISM_UNI_RESP)
	{
		return -1;
	}

	if(check_challege(wapi_sm->usksa.usk[uskid].asue_challenge, pos))
	{
		return -1;
	}
	wapi_hmac_sha256(payload, len-WAI_MIC_LEN, usk->mak,PAIRKEY_LEN,mic, WAI_MIC_LEN);
	
	if(memcmp(mic, rxmic, WAI_MIC_LEN))
	{
		iwn_wpa_hexdump(MSG_DEBUG, "receive ap's mic", rxmic, WAI_MIC_LEN);
		iwn_wpa_hexdump(MSG_DEBUG, "own mic", mic, WAI_MIC_LEN);
		return -1;
	}
	pos += CHALLENGE_LEN;
	wie = pos; wie_len = wie[1]+2;
	iwn_wpa_hexdump(MSG_DEBUG, "AE's IE ", wie, wie_len);

	/* not unicast update  */
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s: payload[0]=%x.\n", __func__, payload[0]);
	if(!(payload[0] & BIT(4))){
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s: wie_len=%d, wapi_sm->ap_wapi_ie_len=%d\n", __func__, wie_len, wapi_sm->ap_wapi_ie_len);
		if((wie_len != wapi_sm->ap_wapi_ie_len)||
			memcmp(wie, wapi_sm->ap_wapi_ie, wapi_sm->ap_wapi_ie_len))
		{
			iwn_wpa_hexdump(MSG_DEBUG, "AE's IE ", wie, wie_len);
			iwn_wpa_hexdump(MSG_DEBUG, "ASUE's IE ", wapi_sm->ap_wapi_ie, wapi_sm->ap_wapi_ie_len);
			return -1;
		}
	}
	wapi_sm->usksa.uskid = payload[1+BKID_LEN];
	if (wapi_install_usk(wpa_s))
	{
		iwn_wpa_printf(MSG_ERROR,"in %s install usk failure\n", __func__);
		return -1;
	}
	else
		printf("wapi_install_usk() succeeded\n");
	
	wpa_s->wapi_state = WAPISM_UNI_ACK;
	if (!wpa_s->usk_updated && (payload[0] & BIT(4)))
	{
		wpa_s->usk_updated = 1;
	}

	return 0;
}

static int waigroup_multicast_1_2(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	u8 *pos = payload;
	u8 mic[20] = {0,};
	u8 ct_msk[16]={0,};
	u8 iv[16]={0,};
	u8 *key_an_id = pos+3+ADDID_LEN+WAI_IV_LEN;
	u8 *key_data = key_an_id + WAI_KEY_AN_ID_LEN;
	u8  tmp_msk[16];

	struct wapi_usk *usk = NULL;
	struct wapi_state_machine *sm = wpa_s->wapi_sm;

	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	
	if (WAPISM_UNI_ACK==wpa_s->wapi_state || WAPISM_FINISH==wpa_s->wapi_state)
	{
	}
	else
	{
	    iwn_wpa_printf(MSG_ERROR, "WAPILib: wpa_s->wapi_state = %d (not %d or %d)",
			   wpa_s->wapi_state, WAPISM_UNI_ACK, WAPISM_FINISH);
	    iwn_wpa_printf(MSG_DEBUG, "WAPILib: %s, abnormal exiting", __func__);
		return -1;
	}

	if(payload[0]&BIT(5))
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: STAKEY_NEG  is not supptable by ASUE");
		return -1;
	}
	pos += 1+BKID_LEN;
#ifdef REKEY
	iwn_wpa_printf(MSG_ERROR, "WAPILib: %s, new mskid=%d, sm->msksa.mskid=%d",
		       __func__, payload[1], sm->msksa.mskid);
	if(payload[1] != !sm->msksa.mskid)
	{
		iwn_wpa_printf(MSG_DEBUG, "WAPILib: MSKID  is not invalid");
		return -1;
	}
#endif
	sm->msksa.mskid = payload[1];
	if(check_addid(wpa_s, payload+3) != 0)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: ADDID  is wrong");
		return -1;
	}

	if(check_iv(&sm->msksa, key_an_id) <= 0)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: Key annoucement identfier  is wrong");
		iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: msk_ann_id", sm->msksa.msk_ann_id, WAI_IV_LEN);
		iwn_wpa_hexdump(MSG_DEBUG, "rx WAPILib: Key_an_id", key_an_id, WAI_IV_LEN);
		return -1;
	}
	memcpy(sm->msksa.msk_ann_id, key_an_id, WAI_IV_LEN );
	usk = get_usk(sm, payload[2]);
	if(usk == NULL)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: Key annoucement uskid  is wrong");
		return -1;
	}
	wapi_hmac_sha256(payload, len-WAI_MIC_LEN, usk->mak, PAIRKEY_LEN, mic, WAI_MIC_LEN);

	if(memcmp(mic, payload+len-WAI_MIC_LEN, WAI_MIC_LEN)!=0)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: Multicast  announcement packet mic  is wrong");
		return -1;
	}

	memcpy(ct_msk, &key_data[1], PAIRKEY_LEN);
	memcpy(iv, key_an_id, WAI_IV_LEN);
	
	iwn_wpa_hexdump(MSG_DEBUG, "ct_msk", &key_data[1], PAIRKEY_LEN);
	iwn_wpa_hexdump(MSG_DEBUG, "kek", usk->kek, PAIRKEY_LEN);
	iwn_wpa_hexdump(MSG_DEBUG, "iv", iv, WAI_IV_LEN);

	/* decrypt the multicast key */
       memset(tmp_msk, 0, sizeof(tmp_msk));
       memcpy(tmp_msk, key_data + 1, sizeof(tmp_msk));
	wpi_encrypt(iv, tmp_msk, 16, usk->kek, ct_msk);
	iwn_wpa_hexdump(MSG_DEBUG, "outmkey", ct_msk, PAIRKEY_LEN);

	key_derivation(ct_msk, key_data[0], (u8 *)MSK_TEXT, strlen(MSK_TEXT), sm->msksa.msk, PAIRKEY_LEN*2, 0);

	iwn_wpa_hexdump(MSG_DEBUG, "msk", sm->msksa.msk, PAIRKEY_LEN*2);
	/*wpa_build_bksa(msksa);*/
	if (waigroup_multicast_2_2_send(wpa_s, payload, len))
		return -1;
	return 0;
}

static int waigroup_multicast_2_2_send(struct wapi_asue_st* wpa_s, u8* payload, int len)
{
	u8 *tbuf = NULL;
	u8 *pos = NULL;
	int tlen = WAI_FLAG_LEN +1+1+ADDID_LEN;
	int tbuflen =WAI_HDR+ tlen+WAI_IV_LEN+WAI_MIC_LEN;

	len = len;/*disable warnning*/
	
	struct wapi_usk *usk = NULL;
	iwn_wpa_printf(MSG_DEBUG, "WAPILib: in %s:%d", __func__, __LINE__);
	usk = get_usk(wpa_s->wapi_sm, payload[2]);
	if(usk == NULL)
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: Key annoucement uskid  is wrong");
		return -1;
	}
		
	tbuf = (u8 *)iwn_get_buffer(tbuflen);
	if(tbuf == NULL)
		return -1;
	pos = tbuf;
	pos = wapi_build_hdr(pos, ++wpa_s->txseq, WAI_MSK_ANNOUNCEMENT_RESPONSE);
	pos = MEMCPY(pos, payload, tlen);
	pos = MEMCPY(pos, payload+tlen+WAI_IV_LEN, WAI_IV_LEN);

	iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: mak", usk->mak, PAIRKEY_LEN);
	iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: tbuf", tbuf+WAI_HDR, tbuflen-WAI_MIC_LEN-WAI_HDR);

	wapi_hmac_sha256(tbuf+WAI_HDR, tbuflen-WAI_MIC_LEN-WAI_HDR, usk->mak, PAIRKEY_LEN, pos, WAI_MIC_LEN);

	/* guoxd 20081210 add start */
	/* set wai length field */
	pos += WAI_MIC_LEN;
	wapi_set_length(tbuf, (short)(pos-tbuf));
	/* guoxd 20081210 add end */

	iwn_wpa_ether_send(tbuf, tbuflen);
	timer_set(3, tbuf, tbuflen);
	iwn_free_buffer(tbuf, tbuflen);

	wpa_s->wapi_state = WAPISM_MUL_RESP;
	if (wapi_install_msk(wpa_s, &payload[tlen]))
	{
		iwn_wpa_printf(MSG_ERROR,"in %s install msk failure\n", __func__);
		return -1;
	}
	wpa_s->wapi_state = WAPISM_FINISH;
	
	return 0;
}

static int wapi_install_usk(struct wapi_asue_st *wpa_s)
{
	struct wapi_usksa *usksa = &wpa_s->wapi_sm->usksa;
	int uskid = (int)usksa->uskid;
	struct wapi_usk *usk = &usksa->usk[uskid];

	iwn_wpa_hexdump(MSG_DEBUG, "wapi_install_usk:", (u8*)usk, 32);
	if (0 != WIFI_unicast_key_set((const char*)usk, 32, uskid))
	{
		iwn_wpa_printf(MSG_ERROR, "WAPILib: Failed to set PTK to the driver");
		return -1;
	}

	return 0;
}

static int wapi_install_msk(struct wapi_asue_st *wpa_s, unsigned char *key_sc)
{
	struct wapi_msksa *msksa = &wpa_s->wapi_sm->msksa;
	int mskid = (int)msksa->mskid;
	
	iwn_wpa_hexdump(MSG_DEBUG, "wapi_install_msk:", msksa->msk, 32);
	iwn_wpa_hexdump(MSG_DEBUG, "WAPILib: KEYSC", key_sc, 16);
	if (0 != WIFI_group_key_set(msksa->msk, 32, mskid, key_sc))
	{
		iwn_wpa_printf(MSG_WARNING, "WAPILib: Failed to set MSK to the driver.");
		return -1;
	}

	return 0;
}
