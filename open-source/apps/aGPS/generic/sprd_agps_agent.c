/*
 *  Generic aGPS Stack Customer Interface
 *
 *  Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <atd.h>
#include "sprd_agps_agent.h"
#include <parser.h>

struct agps_c_plane_hook c_plane_stack_hook;

static int agps_c_plane_stack_init(struct agps_c_plane_hook *hook)
{
	int fdm = -1;
	int rc = 0;
	struct at_channel *atc = NULL;

	dprintf("[%s]: Entry point", __FUNCTION__);

	atc = atc_init(ATC_DEVICE_DEFAULT, O_RDWR, fdm);
	if(atc == NULL){
		dprintf("[%s] GPS get at channel failed!\n",__FUNCTION__);
		return -1;
	}

	if (hook && hook->size == sizeof(c_plane_stack_hook)) {
		memset(&c_plane_stack_hook, 0, sizeof(c_plane_stack_hook));
		c_plane_stack_hook = *hook;
	} else {
		dprintf("[%s] GPS callback structure mismatch!\n", __FUNCTION__);
		rc = -2;
	}

	register_RecvCommand();

	dprintf("[%s]: Exit point. rc = %d", __FUNCTION__, rc);
	return rc;
}

static void agps_c_plane_stack_exit(void)
{

}

/**
 * ota_type - over-the-air type (RRLP / RRC)
 * isfinal - is final response
 * p_msg_data - pointer to uplink payload
 * msg_size - uplink payload size
 */
static unsigned long int agps_c_plane_stack_network_ulreq(unsigned int ota_type, unsigned int rrc_msg_type,
		int isfinal, unsigned char *p_msg_data, unsigned long int msg_size)
{
	char ATcmd[16*1024];
	char *p = ATcmd;

	dprintf("[%s]: Entry point", __FUNCTION__);

	p += sprintf(p, "AT+SPAGPSUL=");
	p += sprintf(p, "%d,%d,%d,%ld,", isfinal, ota_type, rrc_msg_type, msg_size);
	p += sprintf(p, "\"");
	while (msg_size) {
		p += sprintf(p, "%02x", *p_msg_data++);
		msg_size--;
	}
	p += sprintf(p, "\"");

	if (isfinal) {
		handle_NotifyStopSession();
	}

	return at_send(ATcmd, p - ATcmd, 5000);
}

/**
 * notifId - Notification ID (unique)
 * present - Indicates if verification response is present
 * verificationResponse - User's verification response value
 */
static unsigned long int agps_c_plane_stack_lcsverificationresp(unsigned long notifId,
		unsigned long present, unsigned long verificationResponse)
{
	char ATcmd[50];
	char *p = ATcmd;

	dprintf("[%s]: Entry point", __FUNCTION__);

	if (present) {
		p += sprintf(p, "AT+CMTLRA=");
		p += sprintf(p, "%lu, %lu", verificationResponse, notifId);
	}

	return at_send(ATcmd, p - ATcmd, 5000);
}

/**********************************************
 * name: register_RecvCommand
 * function: register AT command needed to be received
 ***********************************************/
static void register_RecvCommand(void)
{
	register_unsolicited("+SPAGPSDL:", my_at_response_cb);
	register_unsolicited("+SPAGPSEND", my_at_response_cb);
	register_unsolicited("+SPAGPSSTATE:", my_at_response_cb);
	register_unsolicited("+SPAGPSNILR", my_at_response_cb);
	register_unsolicited("+SPAGPSMTLR:", my_at_response_cb);
	register_unsolicited("+SPAGPSRESET:", my_at_response_cb);
}

/**********************************************
 * name: handle_ReportDownlinkData
 * function: report dow nlink AGPS control plane data
 * command format: +SPAGPSDL: type, rrc_state, session_priority, pdu_len, pdu_data
 ***********************************************/
static int handle_ReportDownlinkData(const char *pData)
{
	char *line, *tmpData;
	int err;
	int type, rrc_state, session_priority, pdu_len;
	unsigned char pdu_data[CI_DEV_MAX_APGS_MSG_SIZE];

	line = (char *)pData;
	err = at_tok_start(&line);
	if (err < 0) {
		dprintf("[%s] +SPAGPSDL start error", __FUNCTION__);
		goto END_DL;
	}
	/* 1. get type */
	err = parse_nextint(&line, &type);
	if (err < 0) {
		dprintf("[%s] +SPAGPSDL get type error", __FUNCTION__);
		goto END_DL;
	}
	/* 2. get RRC state */
	err = parse_nextint(&line, &rrc_state);
	if (err < 0) {
		dprintf("[%s] +SPAGPSDL get RRC state error", __FUNCTION__);
		goto END_DL;
	}
	/* 3. get session priority */
	err = parse_nextint(&line, &session_priority);
	if (err < 0) {
		dprintf("[%s] +SPAGPSDL get session priority error", __FUNCTION__);
		goto END_DL;
	}
	/* 4. get pdu len */
	err = parse_nextint(&line, &pdu_len);
	if (err < 0) {
		dprintf("[%s] +SPAGPSDL get pdu len error", __FUNCTION__);
		goto END_DL;
	}
	/* 5. get pdu data */
	err = parse_nextstr(&line, &tmpData);
	GSMHexToGSM(tmpData, strlen(tmpData), (char *)pdu_data, CI_DEV_MAX_APGS_MSG_SIZE);

	if (!c_plane_stack_hook.cppayload_cb((unsigned int)type, (unsigned int)rrc_state, (unsigned int)session_priority, pdu_data, (unsigned long int)pdu_len)) {
		err = -1;
	}

END_DL:
	return err;
}

/**********************************************
 * name: handle_NotifyStopSession
 * function: indicates that network stops the session
 * command format: +SPAGPSEND
 ***********************************************/
static int handle_NotifyStopSession(void)
{
	dprintf("[%s]: Entry point", __FUNCTION__);

	return c_plane_stack_hook.meas_term_cb();
}


/**********************************************
 * name: handle_NotifyStateChange
 * function: RRC protocol state change notification
 * command format: +SPAGPSSTATE: rrc_state
 ***********************************************/
static int handle_NotifyStateChange(const char *pData)
{
	char *line;
	int err;
	int rrc_state;

	line = (char *)pData;
	err = at_tok_start(&line);
	if (err < 0) {
		dprintf("[%s] +SPAGPSSTATE start error", __FUNCTION__);
		goto END_STATE;
	}
	/* 1. get RRC state */
	err = parse_nextint(&line, &rrc_state);
	if (err < 0) {
		dprintf("[%s] +SPAGPSSTATE get RRC state error", __FUNCTION__);
		goto END_STATE;
	}

	if (!c_plane_stack_hook.rrc_state_cb(rrc_state)) {
		dprintf("[%s] CP_SendRRCStateEvent fail!!", __FUNCTION__);
		err = -1;
	}

END_STATE:
	return err;
}

/**********************************************
 * name: handle_NotifyNILocateReq
 * function: NI emergency location request notification
 * command format: +SPAGPSNILR
 ***********************************************/
static int handle_NotifyNILocateReq(void)
{
	dprintf("[%s]: Entry point", __FUNCTION__);

	return c_plane_stack_hook.e911notify_cb();
}

/**********************************************
 * name: handle_NotifyMTLR
 * function: MTLR notification
 * command format: +SPAGPSMTLR: handle_id, ni_type, notify_type, clientName_len, clientName_dcs,
 clientName, requestorID_len, requestorID_dcs, requestorID
 ***********************************************/
static int handle_NotifyMTLR(const char *pData)
{
	char *line, *tmpData;
	int err;
	int handle_id, ni_type, notify_type, requestorID_len, requestorID_dcs, clientName_len, clientName_dcs;
	char requestorID[CI_DEV_MAX_APGS_MSG_SIZE], clientName[CI_DEV_MAX_APGS_MSG_SIZE];
	agps_user_notify_params_t agps_user_notify_params;

	line = (char*)pData;
	err = at_tok_start(&line);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR start error", __FUNCTION__);
		goto END_MTLR;
	}
	/* 1. get handle id */
	err = parse_nextint(&line, &handle_id);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get handle_id error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.handle_id = handle_id;
	/* 2. get ni type */
	err = parse_nextint(&line, &ni_type);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get ni_type error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.ni_type = ni_type;
	/* 3. get notify type */
	err = parse_nextint(&line, &notify_type);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get notify type error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.notify_type = notify_type;
	/* 4. get clientName len */
	err = parse_nextint(&line, &clientName_len);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get clientName len error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.clientName_len = clientName_len;
	/* 5. get clientName dcs */
	err = parse_nextint(&line, &clientName_dcs);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get clientName dcs error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.clientName_dcs = clientName_dcs;
	/* 6. get clientName */
	err = parse_nextstr(&line, &tmpData);
	GSMHexToGSM(tmpData, strlen(tmpData), (char *)clientName, CI_DEV_MAX_APGS_MSG_SIZE);
	agps_user_notify_params.clientName = clientName;
	/* 7. get requestorID len */
	err = parse_nextint(&line, &requestorID_len);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get requestorID len error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.requestorID_len = requestorID_len;
	/* 8. get requestorID dcs */
	err = parse_nextint(&line, &requestorID_dcs);
	if (err < 0) {
		dprintf("[%s] +SPAGPSMTLR get requestorID dcs error", __FUNCTION__);
		goto END_MTLR;
	}
	agps_user_notify_params.requestorID_dcs = requestorID_dcs;
	/* 9. get requestorID */
	err = parse_nextstr(&line, &tmpData);
	GSMHexToGSM(tmpData, strlen(tmpData), (char *)requestorID, CI_DEV_MAX_APGS_MSG_SIZE);
	agps_user_notify_params.requestorID = requestorID;

	if (!c_plane_stack_hook.mtlr_notify_cb(agps_user_notify_params)) {
		err = -1;
	}

END_MTLR:
	return err;
}

/**********************************************
 * name: handle_ResetPosInfo
 * function: Indicate reset UE positioning stored information
 * command format: +SPAGPSRESET: type
 ***********************************************/
static int handle_ResetPosInfo(const char *pData)
{
	char *line;
	int err;
	short type;

	line = (char *)pData;
	err = at_tok_start(&line);
	if (err < 0) {
		dprintf("[%s] +SPAGPSRESET start error", __FUNCTION__);
		goto END_RESET;
	}
	/* 1. get type */
	err = parse_nextint(&line, (int *)&type);
	if (err < 0) {
		dprintf("[%s] +SPAGPSRESET get tSPAGPSSTATEype error", __FUNCTION__);
		goto END_RESET;
	}

	if (!c_plane_stack_hook.cp_reset_cb(type)) {
		dprintf("[%s] CP_Reset fail!!", __FUNCTION__);
		err = -1;
	}

END_RESET:
	return err;
}

static int my_at_response_cb(const char *at, int len)
{
	if (strStartsWith(at, AT_PREFIX_table[SPAGPSDL_INDEX])) {
		handle_ReportDownlinkData(at);
	} else if (strStartsWith(at, AT_PREFIX_table[SPAGPSEND_INDEX])) {
		handle_NotifyStopSession();
	} else if (strStartsWith(at, AT_PREFIX_table[SPAGPSSTATE_INDEX])) {
		handle_NotifyStateChange(at);
	} else if (strStartsWith(at, AT_PREFIX_table[SPAGPSNILR_INDEX])) {
		handle_NotifyNILocateReq();
	} else if (strStartsWith(at, AT_PREFIX_table[SPAGPSMTLR_INDEX])) {
		handle_NotifyMTLR(at);
	} else if (strStartsWith(at, AT_PREFIX_table[SPAGPSRESET_INDEX])) {
		handle_ResetPosInfo(at);
	} else {
		dprintf("[%s] AT command not match", __FUNCTION__);
	}

	return 0;
}

static const struct agps_c_plane_interface agps_c_plane_stack_inf = {
	sizeof(struct agps_c_plane_interface),
	agps_c_plane_stack_init,
	agps_c_plane_stack_exit,
	agps_c_plane_stack_network_ulreq,
	agps_c_plane_stack_lcsverificationresp,
};

const struct agps_c_plane_interface *get_agps_cp_interface(void)
{
	dprintf("[%s]: is called", __FUNCTION__);
	return &agps_c_plane_stack_inf;
}
