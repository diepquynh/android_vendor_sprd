/*****************************************************************************
**                                                                           *
**  Name:          btl_cfg.h                                                 *
**                                                                           *
**  Description:   Get runtime configuration from file                       *
**                                                                           *
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.                 *
******************************************************************************/
#ifndef BTL_CFG_H
#define BTL_CFG_H

/* this trace serves as global flag in local.prop (in property space) to scan for more trace
 * settings.
 */
#define BTL_GLOBAL_PROP_TRC_FLAG "TRC_BTAPP"

/* this setting provides a config option to set filter on bdaddr for inquiry
 *
 */
#define BTL_GLOBAL_PROP_INQ_BDA_FILTER		"INQ_FILTER_BDA"

/*******************************************************************************
**
** Function         btl_cfg_getproperties
**
** Description      Get runtime configuration from Android Property System
**
** Returns          Nothing
**
*******************************************************************************/
void btl_cfg_getproperties(void);

/*******************************************************************************
 **
 ** Function         btl_cfg_get_trace_prop
 **
 ** Description      loads and sets runtime btld trace settings from Android Property System
 **
 ** Returns          TRUE if found, FALSE if nothing present
 **                  requires ril.BTTRC_BTAPP to be always present!
 **
 *******************************************************************************/
extern BOOLEAN btl_cfg_get_trace_prop( void );

/*******************************************************************************
**
** Function         btl_cfg_get_android_bt_status
**
** Description      Get Android BT status
**
** Returns          0 if Android BT is not active; 1 if active
**
*******************************************************************************/
int btl_cfg_get_android_bt_status(void);
/*******************************************************************************
**
** Function         btl_cfg_get_btld_status
**
** Description      Get btld status from Android property system
**
** Returns          0 if btld not active yet; 1 if btld is active
**
*******************************************************************************/
int btl_cfg_get_btld_status(void);

/*******************************************************************************
**
** Function         btl_cfg_set_btld_status
**
** Description      Set btld status in Android property system
**
** Returns          0 if not successful
**
*******************************************************************************/
int btl_cfg_set_btld_status(int iNewStatus);

/*******************************************************************************
 **
 ** Function         btl_cfg_is_soft_onoff_enabled
 **
 ** Description     Check BT Software ON/OFF status from Android property system
 **
 ** Returns          1 if soft on/off is enabled; 0 otherwise
 **
 *******************************************************************************/
int btl_cfg_is_soft_onoff_enabled(void);

/*******************************************************************************
 **
 ** Function         btl_cfg_set_btld_pid
 **
 ** Description     Sets the PID of the BTLD process to the Android property system
 **
 ** Returns          non-zero if btld is active; 0 otherwise
 **
 *******************************************************************************/
int btl_cfg_set_btld_pid(int pid);

/*******************************************************************************
**
** Function         btl_cfg_get_call_active_status
**
** Description      Get call active status from Android property system
**
** Returns          0 if call not active yet; 1 if call is active
**
*******************************************************************************/
int btl_cfg_get_call_active_status();

/*******************************************************************************
**
** Function         btl_cfg_get_avrcp_pass_thru_status
**
** Description      Get AVRCP pass through status from Android property system
**
** Returns          0 if AVRCP pass through not active;
**                  1 if AVRCP pass through is active
**
*******************************************************************************/
int btl_cfg_get_avrcp_pass_thru_status (void);


/*******************************************************************************
**
** Function         btl_cfg_get_btport_redirection_enable
**
** Description      Get BTPORT redirection enable
**
** Returns          0 if redirection not active yet; 1 if redirection is active
**
*******************************************************************************/
int btl_cfg_get_btport_redirection_enable();

/*******************************************************************************
**
** Function         btl_cfg_getBDAFilterCond
**
** Description      Gets the filter condition for DiscoverDevices if one exists
**
** Returns          int
**			non-zero if a property is configured to filter on bdaddr
*******************************************************************************/
int btl_cfg_getBDAFilterCond(BD_ADDR bd_addr);

#if defined(BTL_CFG_USE_CONF_FILE) && (BTL_CFG_USE_CONF_FILE==TRUE)
/*******************************************************************************
 **
 ** Function        btl_cfg_read_conf
 **
 ** Description     reads configuration parameters from a config file
 **
 ** Parameters      path to config file including filename itself
 **
 ** Returns         -1: unable to open file, 0 success
 **
 *******************************************************************************/
extern int btl_cfg_read_conf(const char * p_path);
#endif

#endif /* BTL_CFG_H */
