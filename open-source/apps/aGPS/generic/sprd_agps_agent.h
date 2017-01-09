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

#ifndef __SPRD_AGPS_AGENT_H
#define __SPRD_AGPS_AGENT_H

/** handle_NotifyMTLR structure */
typedef struct agps_user_notify_params_t {
	int handle_id;
	int ni_type;
	int notify_type;
	int requestorID_len;
	int requestorID_dcs;
	char *requestorID;
	int clientName_len;
	int clientName_dcs;
	char *clientName;
} agps_user_notify_params_t;

// MTLR request definitions
typedef unsigned long int (* agps_cppayload)(unsigned int ota_type, unsigned int rrc_state, unsigned int sessionPriority,
		unsigned char * p_msg_data, unsigned long int msg_size);
typedef unsigned long int (* agps_measureterminate)(void);
typedef unsigned long int (* agps_rrcstateevent)(unsigned int rrc_state);
typedef unsigned long int (* agps_e911dial_notify)(void);
typedef unsigned long int (* agps_cpreset)(short type);
typedef unsigned long int (* agps_mtlrnotify)(agps_user_notify_params_t agps_user_notify_params);

/** AGPS C-Plane hook structure. */
struct agps_c_plane_hook {
	/** set to sizeof(struct agps_c_plane_hook) */
	size_t size;
	agps_cppayload cppayload_cb;
	agps_measureterminate meas_term_cb;
	agps_rrcstateevent rrc_state_cb;
	agps_e911dial_notify e911notify_cb;
	agps_cpreset cp_reset_cb;
	agps_mtlrnotify mtlr_notify_cb;
};

struct agps_c_plane_interface {
	/** set to sizeof(struct agps_c_plane_interface) */
	size_t size;
	/**
	 * Opens the interface and provides the callback functions.
	 */
	int (*init)(struct agps_c_plane_hook *hook);

	/** Closes the interface. */
	void (*exit)(void);


	/** Send uplink payload request to network. */
	unsigned long int   (*network_ulreq)(unsigned int ota_type,     /* over-the-air type (RRLP / RRC) */
			unsigned int rrc_msg_type,
			int isfinal,      /* is final response? */
			unsigned char *p_msg_data,     /* pointer to uplink payload */
			unsigned long int msg_size );  /* uplink payload size */
	/** Send MTLR verification response to network. */
	unsigned long int (*lcsverificationresp)(unsigned long notifId,     /* Notification ID (unique) */
			unsigned long present,     /* Indicates if verification response is present */
			unsigned long verificationResponse); /* User's verification response value */

};

#define AT_PREFIX_MAX_LEN 20
#define CI_DEV_MAX_APGS_MSG_SIZE 8192

enum {
	SPAGPSDL_INDEX,
	SPAGPSEND_INDEX,
	SPAGPSSTATE_INDEX,
	SPAGPSNILR_INDEX,
	SPAGPSMTLR_INDEX,
	SPAGPSRESET_INDEX,
	ATSPLN_INDEX,
	ATSPAGPSUL_INDEX,
	AT_CMD_INUSE
};

const char AT_PREFIX_table[AT_CMD_INUSE][AT_PREFIX_MAX_LEN]= {
	"+SPAGPSDL:",
	"+SPAGPSEND",
	"+SPAGPSSTATE:",
	"+SPAGPSNILR",
	"+SPAGPSMTLR:",
	"+SPAGPSRESET:",
	"AT+SPLN=",
	"AT+SPAGPSUL="
};

static void register_RecvCommand(void);
static int handle_ReportDownlinkData(const char *pData);
static int handle_NotifyStopSession(void);
static int handle_NotifyStateChange(const char *pData);
static int handle_NotifyNILocateReq(void);
static int handle_NotifyMTLR(const char *pData);
static int handle_ResetPosInfo(const char *pData);
static int my_at_response_cb(const char *at, int len);

#endif
