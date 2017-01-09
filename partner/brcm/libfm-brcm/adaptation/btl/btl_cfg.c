/*****************************************************************************
 **                                                                           *
 **  Name:          btl_cfg.c                                                 *
 **                                                                           *
 **  Description:   Get runtime configuration from file                       *
 **                                                                           *
 **  Copyright (c) 2009 - 2012, Broadcom Corp., All Rights Reserved.          *
 ******************************************************************************/
#include "bt_target.h"
#include "gki.h"
#include "bd.h"
#include "bte_appl.h"
#include "hcilp.h"
#include "btu.h"
#include "btld_prop.h"
#include "btl_cfg.h"
#include <stdlib.h>
#include <stdio.h>

#define LOG_TAG "BTL_CFG"

#ifndef LINUX_NATIVE
#include <cutils/properties.h>
#include <cutils/log.h>
#else
/* should match properties.h */
#define PROPERTY_KEY_MAX   32
#define PROPERTY_VALUE_MAX 92

#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)

#endif

/* override this in btld.txt! */
#ifndef BTL_CFG_USE_CONF_FILE
#define BTL_CFG_USE_CONF_FILE FALSE
#endif

/* override this in btld.txt! */
#ifndef BTAPP_FM_SNR
#define BTAPP_FM_SNR (BTA_FM_SNR_MAX + 1)
#endif

/* ignore lines starting with # */
#define BTL_CONF_COMMENT '#'
#define BTL_CONF_ASSIGN_CHAR "="
#define BTL_CONF_MAX_LINE_LEN 255

#ifndef BTL_CFG_CONF_BT_PREFIX
#define BTL_CFG_CONF_BT_PREFIX "bt."
#endif
#ifndef BTL_CFG_CONF_FM_PREFIX
#define BTL_CFG_CONF_FM_PREFIX "fm."
#endif
#ifndef BTL_CFG_CONF_PATH
#define BTL_CFG_CONF_PATH "/data/bcm_cust.conf"
#endif
#define BTL_CFG_BT_CONF(property)  BTL_CFG_CONF_BT_PREFIX property
#define BTL_CFG_FM_CONF(property)  BTL_CFG_CONF_FM_PREFIX property

#ifndef BTL_CFG_BRCM_PROP_PREFIX
#define BTL_CFG_BRCM_PROP_PREFIX "service.brcm.bt."
#endif

/* prefix: typically above string, property: commen base property name without prefix */
#define BCM_CFG_BRCM_PROP(property) BTL_CFG_BRCM_PROP_PREFIX property
#define BCM_CFG_PERSIST_BRCM_PROP(property) "persist." BTL_CFG_BRCM_PROP_PREFIX property

#ifndef BRCM_PROPERTY_FM_AUDIO_PATH
#define BRCM_PROPERTY_FM_AUDIO_PATH "service.brcm.fm.audiopath"
#endif

#define BTL_CFG_LINE_MAXLEN (256)
#define BTL_CFG_BTTRC_NAME_MAX (48)    /* 48 bytes for ril.BTTRC_NAME should be fine */
#define BTL_CFG_BTTRC_NAME_OFFSET (sizeof(BTL_CFG_BRCM_PROP_PREFIX)-1)  /* adjust for 0 */
#define SKIP_WHITESPACE(p) {while ((*p==' ') || (*p=='\t')) p++;}

/* Reserved addr: if ril.macaddr set to this, then generate a random BDA to use */
const BD_ADDR BTL_CFG_BDA_RESERVED_INVALID = { 0x00, 0x12, 0x47, 0x01, 0x23, 0x45 };

/* Property keys for Android Property System */
typedef enum {
    BTL_CFG_KEY_BDADDR = 0,
    BTL_CFG_KEY_BDADDR_AUTOGEN,
    BTL_CFG_KEY_PATCHFILE,
    BTL_CFG_KEY_BT_WAKE_POLARITY,
    BTL_CFG_KEY_HOST_WAKE_POLARITY,
    BTL_CFG_KEY_3WAY_CONF_SUPPORT,
    BTL_CFG_KEY_PCM_CLK,
    BTL_CFG_KEY_PCM_MS_ROLE,
    BTL_CFG_KEY_FM_SNR,
    BTL_CFG_KEY_CONTACTS_DB,
    BTL_CFG_KEY_HWTUN_PORT,
    BTL_CFG_KEY_MAX
} tBTL_CFG_KEY;

/* keep this table and above enum's in sync! */
const char * property_names[] = {
    BCM_CFG_BRCM_PROP("mac"),
    BCM_CFG_PERSIST_BRCM_PROP("mac"),
    BCM_CFG_BRCM_PROP("patchfile"),
    BCM_CFG_BRCM_PROP("bt_wake_polar"),
    BCM_CFG_BRCM_PROP("host_wake_polar"),
    BCM_CFG_BRCM_PROP("3way_support"),
    BCM_CFG_BRCM_PROP("pcm_clk"), /* 0 for 128 KHz, 1 for 256, 2 for 512, 3 for 1024, 4 for 2048 */
    BCM_CFG_BRCM_PROP("pcm_ms_role"), /* slave: 0, master: 1 */
    BCM_CFG_BRCM_PROP("fm_snr"),
    BCM_CFG_BRCM_PROP("contacts_db"), /* contacts path */
    BCM_CFG_BRCM_PROP("hwtun_port"),
};


#if (BTL_CFG_USE_CONF_FILE==TRUE)
typedef int ( tCONF_ACTION)(int idx, const char *p_prop_name, const char *p_prop_value);
typedef struct tCONF_PROP_ENTRY_tag {
    const char *conf_name;
    const tCONF_ACTION *p_action;
} tCONF_PROP_ENTRY;

int btl_cfg_afh_first(int idx, const char *p_prop_name, const char *p_prop_value);
int btl_cfg_afh_last(int idx, const char *p_prop_name, const char *p_prop_value);

enum {
    BTL_CFG_CONF_AHF_FIRST = 0,
    BTL_CFG_CONF_AHF_LAST,
    BTL_CFG_CONF_END
};

/* table with param name and action function to execute if found */
const tCONF_PROP_ENTRY conf_table[] = {
        { BTL_CFG_BT_CONF("afh_first"), btl_cfg_afh_first },
        { BTL_CFG_BT_CONF("afh_last"), btl_cfg_afh_last },
        { (const char *) NULL, NULL } };

#endif

extern void scru_ascii_2_bdaddr(char *p_ascii, UINT8 *p_bd);
extern void scru_bdaddr_2_ascii(UINT8 *p_bd, char *p_ascii);
extern void scru_ascii_2_uint(char *p_ascii, int n_bytes, void *p);

BOOLEAN btl_cfg_get_trace_prop(void);

/* fetches property and performs some sanity checks */
int get_property_by_name(const char* propname, char *value, char *default_value)
{
    int len;
    int ret = 0;

    /* check for too long property names (needs to be < 32) */
    len = strlen(propname);
    if (len >= PROPERTY_KEY_MAX)
    {
        LOGE("property [%s] name is too long (%d), must be < %d", propname, len, PROPERTY_KEY_MAX);
        return ret;
    }

    ret = property_get((char *) propname, value, default_value);
    if (ret <= 0)
    {
        LOGE("WARNING : [%s] property_get failed (%d)", propname, ret);
        ret = 0;
    }

    return ret;
}


int get_property_by_key(tBTL_CFG_KEY key, char *value, char *default_value)
{
    /* check for invalid keys */
    if (key >= BTL_CFG_KEY_MAX)
    {
        LOGE("invalid property key");
        return 0;
    }

    return get_property_by_name(property_names[key], value, default_value);
}


#ifdef LINUX_NATIVE
int property_get(char *prop, char *value, char *default_value)
{
    int pos = 0;
    int fd;

    fd = fopen(prop, "r");
    if (!fd)
    {
        value = default_value;
        return 0;
    }

    while (fread(value, 1, value[pos], fd) == 1)
    pos++;

    return 1;
}
int property_set(char *prop, char *value)
{
    /* create file in linux */
    int fd = fopen(prop, "wb");

    LOGI("property_set : %s = %s\n", prop, value);

    if (fd)
        return fwrite(value, 1, strlen(value), fd);
    return 0;
}

#endif

/*******************************************************************************
 **
 ** Function         btl_cfg_init_rand
 **
 ** Description      Get runtime configuration from Android Property System
 **
 ** Returns          Nothing
 **
 *******************************************************************************/
void btl_cfg_init_rand(void)
{
    srand((unsigned int) (time(0)));
}

/*******************************************************************************
 **
 ** Function         btl_cfg_uint8_rand
 **
 ** Description      Generate a random number between [0,255]
 **
 ** Returns          UINT8
 **
 *******************************************************************************/
UINT8 btl_cfg_uint8_rand(void)
{
    return (UINT8) ((rand() >> 8) & 0xFF);
}

/*******************************************************************************
 **
 ** Function         btl_cfg_get_bdaddr
 **
 ** Description      Get bdaddr from Android Property System
 **
 ** Returns          bdaddr
 **
 *******************************************************************************/
void btl_cfg_get_bdaddr(BD_ADDR local_addr)
{
    char val[BTL_CFG_LINE_MAXLEN];
    BOOLEAN bHaveValidBda = FALSE;

    /* Get local bdaddr */
    if (property_get(property_names[BTL_CFG_KEY_BDADDR], val, NULL))
    {
        LOGI("%s: BDA=%s", __FUNCTION__, val);
        scru_ascii_2_bdaddr(val, local_addr);

        /* If this is not a reserved/special bda, then use it */
        if (bdcmp(local_addr, BTL_CFG_BDA_RESERVED_INVALID) != 0)
        {
            bHaveValidBda = TRUE;
        }
    }

    /* No factory BDADDR found. Look for previously generated random BDA */
    if ((!bHaveValidBda) && (property_get(property_names[BTL_CFG_KEY_BDADDR_AUTOGEN], val, NULL)))
    {
        LOGI("%s: auto BDA=%s", __FUNCTION__, val);

        scru_ascii_2_bdaddr(val, local_addr);
        bHaveValidBda = TRUE;
    }

    /* Generate new BDA if necessary */
    if (!bHaveValidBda)
    {
        /* Seed the random number generator */
        btl_cfg_init_rand();

        /* No autogen BDA. Generate one now. */
        local_addr[0] = 0x43;
        local_addr[1] = 0x25;
        local_addr[2] = btl_cfg_uint8_rand();
        local_addr[3] = btl_cfg_uint8_rand();
        local_addr[4] = btl_cfg_uint8_rand();
        local_addr[5] = btl_cfg_uint8_rand();

        /* Convert to ascii, and store as a persistent property */
        scru_bdaddr_2_ascii(local_addr, val);

        LOGI("%s: No preset BDA. Generating BDA: %s for prop %s",
             __FUNCTION__, val, property_names[BTL_CFG_KEY_BDADDR_AUTOGEN]);
        property_set(property_names[BTL_CFG_KEY_BDADDR_AUTOGEN], val);
    }
} /* btl_cfg_get_bdaddr() */

/*******************************************************************************
 **
 ** Function         btl_cfg_get_patchram_path
 **
 ** Description      Get runtime patch ram path configuration from Android Property System
 **
 ** Input            Pointer to ram variable big enough for path
 **
 ** Returns          patch ram path
 **
 *******************************************************************************/
void btl_cfg_get_patchram_path(char * patchram_path)
{
    char val[BTL_CFG_LINE_MAXLEN];

    if (get_property_by_key(BTL_CFG_KEY_PATCHFILE, val, NULL))
    {
        LOGI("%s: %s=%s", __FUNCTION__, property_names[BTL_CFG_KEY_PATCHFILE], val);
        strcpy(patchram_path, val);
    }
} /* btl_cfg_get_patchram_path() */


/*******************************************************************************
 **
 ** Function         btl_cfg_get_audio_path
 **
 ** Description      Get preset FM audio path
 **
 ** Input            Pointer to ram variable big enough for FM audio path
 **
 ** Returns          void
 **
 *******************************************************************************/
void btl_cfg_get_audio_path(char *path)
{
    char val[BTL_CFG_LINE_MAXLEN];
    if (get_property_by_name(BRCM_PROPERTY_FM_AUDIO_PATH, val, NULL))
    {
        strncpy(path, val, BTL_CFG_LINE_MAXLEN);
    }
} /* btl_cfg_get_audio_path */


/*******************************************************************************
 **
 ** Function         btl_cfg_get_3way_conf_support
 **
 ** Description      Get BT 3-way conference call support config from Android Property System
 **
 ** Input            Pointer to 3-way conf call support flag
 **
 ** Returns          3-way conference call support
 **
 *******************************************************************************/
void btl_cfg_get_3way_conf_support(BOOLEAN * supported)
{
    char val[BTL_CFG_LINE_MAXLEN];

    *supported = TRUE;
    if (get_property_by_key(BTL_CFG_KEY_3WAY_CONF_SUPPORT, val, "true"))
    {
        LOGI("%s: %s=%s", __FUNCTION__, property_names[BTL_CFG_KEY_3WAY_CONF_SUPPORT], val);
        if (strcmp(val, "false") == 0)
        {
            *supported = FALSE;
        }
    }
} /* btl_cfg_get_3way_conf_support */

/*******************************************************************************
 **
 ** Function         btl_cfg_get_lpm_params
 **
 ** Description      Get runtime lpm config from Android Property System
 **                  make sure HCILP_Init() is not over writing this params
 **
 ** Input            pointers to host and bt wake polarity variables
 **
 ** Returns          sets polarity to property value if found or to compile default value
 **
 *******************************************************************************/
void btl_cfg_get_lpm_params(UINT8 * p_bt_wake_polarity, UINT8 * p_host_wake_polarity)
{
    char val[BTL_CFG_LINE_MAXLEN];

    if (get_property_by_key(BTL_CFG_KEY_BT_WAKE_POLARITY, val, NULL))
    {
        scru_ascii_2_uint(val, 1, (void *) p_bt_wake_polarity);
    }
    else
    {
        /* set compiled in polarity if property is not found */
        *p_bt_wake_polarity = HCILP_BT_WAKE_POLARITY;
    }
    LOGI("%s: %s=%d", __FUNCTION__,
         property_names[BTL_CFG_KEY_BT_WAKE_POLARITY], *p_bt_wake_polarity);

    if (get_property_by_key(BTL_CFG_KEY_HOST_WAKE_POLARITY, val, NULL))
    {
        scru_ascii_2_uint(val, 1, (void *) p_host_wake_polarity);
    }
    else
    {
        *p_host_wake_polarity = HCILP_HOST_WAKE_POLARITY;
    }
    LOGI("%s: %s=%d", __FUNCTION__,
         property_names[BTL_CFG_KEY_HOST_WAKE_POLARITY], *p_host_wake_polarity);
} /* btl_cfg_get_lpm_params()*/

/*******************************************************************************
 **
 ** Function         btl_cfg_get_pcm_params
 **
 ** Description      Get runtime PCM/SCO settings from Android Property System
 **
 ** Input            pointers to 5 byte PCM VSC array
 **
 ** Returns          sets clock frequency and master slave mode (sync and clk together!)
 **
 *******************************************************************************/
void btl_cfg_get_sco_pcm_params(UINT8 * vsc_sco_pcm)
{
    char val[BTL_CFG_LINE_MAXLEN];
    UINT8 pcm_role;

    if (get_property_by_key(BTL_CFG_KEY_PCM_CLK, val, NULL))
    {
        scru_ascii_2_uint(val, 1, (void *) &vsc_sco_pcm[BT_PCM_CLK_IDX]);
    }
    else
    {
        vsc_sco_pcm[BT_PCM_CLK_IDX] = BT_PCM_DEF_CLK;
    }

    if (get_property_by_key(BTL_CFG_KEY_PCM_MS_ROLE, val, NULL))
    {
        scru_ascii_2_uint(val, 1, (void *) &pcm_role);
    }
    else
    {
        pcm_role = BT_PCM_DEF_ROLE;
    }
    /* sync and clk are set to the MS role */
    vsc_sco_pcm[BT_PCM_SYNC_MS_ROLE_IDX] = pcm_role;
    vsc_sco_pcm[BT_PCM_CLK_MS_ROLE_IDX] = pcm_role;
} /* btl_cfg_get_pcm_params() */

/*******************************************************************************
 **
 ** Function         btl_cfg_get_fm_snr
 **
 ** Description      Get FM SNR value setting from Android Property System
 **
 ** Input            pointer to the FM SNR value
 **
 ** Returns          FM SNR setting
 **
 *******************************************************************************/
void btl_cfg_get_fm_snr(UINT8 *fm_snr)
{
    char val[BTL_CFG_LINE_MAXLEN];

    if (get_property_by_key(BTL_CFG_KEY_FM_SNR, val, NULL))
    {
        scru_ascii_2_uint(val, 1, (void *) fm_snr);
        LOGI("%s: SNR property: fm_snr = 0x%x", __FUNCTION__, *fm_snr);
    }
    else if (BTAPP_FM_SNR != FM_SNR_INVALID)
    {
        *fm_snr = BTAPP_FM_SNR;
        LOGI("%s: SNR btld: fm_snr = 0x%x", __FUNCTION__, *fm_snr);
    }
    else
    {
        LOGI("%s: SNR default or command line: fm_snr = 0x%x", __FUNCTION__, *fm_snr);
    }
    LOGI("%s: fm_snr = 0x%x", __FUNCTION__, *fm_snr);
} /* btl_cfg_get_fm_snr() */

/*******************************************************************************
 **
 ** Function         btl_cfg_get_trace_prop
 **
 ** Description      Get runtime trace configuration from Android Property System
 **
 ** Returns          TRUE if found and set, FALSE if no trace settings found
 **
 ** DEPENDS on bttrc_set_level_map containing the textual strings
 **
 *******************************************************************************/
#define BTL_CFG_TRACE_SET_RIL BCM_CFG_BRCM_PROP( BTL_GLOBAL_PROP_TRC_FLAG )    /* only if this prefix.BTTRC_BTAPP x is present, it will
                                                  search for the rest of traces! */
BOOLEAN btl_cfg_get_trace_prop(void)
{
    BOOLEAN return_val;
#if ( BT_USE_TRACES==TRUE )
    char val[BTL_CFG_LINE_MAXLEN];
    char trace_name[BTL_CFG_BTTRC_NAME_MAX];
    tBTTRC_LEVEL levels[bttrc_map_size]; /* may use malloc if a lot of traces */
    int lvl_idx;
    const tBTTRC_FUNC_MAP *p_map;

    if (get_property_by_name(BTL_CFG_TRACE_SET_RIL, val, NULL))
    {
        memset(levels, sizeof(levels), 0);
        strcpy(trace_name, BTL_CFG_BRCM_PROP_PREFIX); /* ril. or what is the defined prefix */
        lvl_idx = 0;
        p_map = &bttrc_set_level_map[0];

        /* loop over the whole trace map. this may take a while! */
        while (p_map->layer_id_start != 0)
        {
            strncpy(&trace_name[BTL_CFG_BTTRC_NAME_OFFSET], p_map->trc_name,
                    (BTL_CFG_BTTRC_NAME_MAX - BTL_CFG_BTTRC_NAME_OFFSET - 1));
            trace_name[BTL_CFG_BTTRC_NAME_MAX - 1] = 0x00; /* safety null termination */

            if (get_property_by_name((const char *) trace_name, val, NULL))
            {
                levels[lvl_idx].layer_id = p_map->layer_id_start;
                scru_ascii_2_uint(val, 1, (void *) &levels[lvl_idx].type);
                /* LOGD( "[bttrc] setting trace: %s (%d), type: %d",
                 trace_name, levels[lvl_idx].layer_id, levels[lvl_idx].type ); */
                lvl_idx++;
            }
            p_map++;
        }
        BTA_SysSetTraceLevel(levels);
        LOGI( "[bttrc] Set Trace Levels for %d modules. map size: %d", lvl_idx,
                bttrc_map_size );
        return_val = TRUE;
    }
    else
#endif
    {
        return_val = FALSE;
    }
    return return_val;
} /* btl_cfg_get_trace_prop() */

/*******************************************************************************
 **
 ** Function         btl_cfg_get_contacts_db
 **
 ** Description      Get the contacts databases file path
 **
 ** Returns          Nothing
 **
 *******************************************************************************/
void btl_cfg_get_contacts_db(char* db_path)
{
    char val[BTE_APPL_CONTACTS_DB_PATH];

    if (property_get(property_names[BTL_CFG_KEY_CONTACTS_DB], val, NULL))
    {
        strcpy(db_path, val);
    }
    LOGI("%s: %s=%s", __FUNCTION__, property_names[BTL_CFG_KEY_CONTACTS_DB], db_path);
}

/*******************************************************************************
 **
 ** Function         btl_cfg_getproperties
 **
 ** Description      Get runtime configuration from Android Property System
 **
 ** Returns          Nothing
 **
 *******************************************************************************/
void btl_cfg_getproperties(void)
{
    /* get/create local bd address */
    btl_cfg_get_bdaddr(bte_appl_cfg.local_addr);

    /* Get patchfile path */
    btl_cfg_get_patchram_path(bte_appl_cfg.patchram_path);

    /* Check BT 3-way phone conference support */
    btl_cfg_get_3way_conf_support(&bte_appl_cfg.ag_enable_3way_conf);

    /* load LPM polarities */
    btl_cfg_get_lpm_params(&bte_appl_cfg.bt_wake_polarity, &bte_appl_cfg.host_wake_polarity);

    /* set PCM clk and slave/master role */
    btl_cfg_get_sco_pcm_params(bte_appl_cfg.ag_vsc_sco_pcm);

#if defined(BTA_FM_INCLUDED) && (BTA_FM_INCLUDED == TRUE)
    btl_cfg_get_fm_snr(&bte_appl_cfg.fm_snr);
#endif

#if 0
    /* DO NOT READ IT here. it is  done at the end of BTE_InitStack() as otherwise the
     * trace level are reset to 0!!!!!
     */
#error "NOT ALLOWED here!!!!!"
    /* Get trace properties */
    btl_cfg_get_trace_prop();
#endif

    /* Get vendor Contacts Databases location */
    btl_cfg_get_contacts_db(bte_appl_cfg.contacts_db);
}

/*******************************************************************************
 **
 ** Function         btl_cfg_get_android_bt_status
 **
 ** Description      Get Android BT status
 **
 ** Returns          0 if Android BT is not active; 1 if active
 **
 *******************************************************************************/
int btl_cfg_get_android_bt_status(void)
{
    char value[PROPERTY_VALUE_MAX];

    /* default if not set it 0 */
    property_get(BRCM_PROPERTY_BT_ACTIVATION, value, "0");

    //LOGV("%s: %s = %s\n", __FUNCTION__, BRCM_PROPERTY_BT_ACTIVATION, value);

#ifndef LINUX_NATIVE
    if (strcmp(value, "1") == 0)
    {
        return 1;
    }
    return 0;
#else
    return 1;
#endif
}

/*******************************************************************************
**
** Function         btl_cfg_get_btport_redirection_enable
**
** Description      Get BTPORT redirection enable
**
** Returns          0 if redirection not active yet; 1 if redirection is active
**
*******************************************************************************/
int btl_cfg_get_btport_redirection_enable(void)
{
    char value[8];

    /* default if not set it 0 */
    property_get(BRCM_PROPERTY_DUN_REDIRECTION, value, NULL);

    LOGI("%s: %s=%s\n", __FUNCTION__, BRCM_PROPERTY_DUN_REDIRECTION, value);

#ifndef LINUX_NATIVE
    if (strcmp(value, "1") == 0) {
        LOGD("%s: %s=1\n", __FUNCTION__, BRCM_PROPERTY_DUN_REDIRECTION);
        return 1;
    }
    return 0;
#else
    return 1;
#endif
}

/*******************************************************************************
**
** Function         btl_cfg_get_call_active_status
**
** Description      Get call active status from Android property system
**
** Returns          0 if call not active yet; 1 if call is active
**
*******************************************************************************/
int btl_cfg_get_call_active_status()
{
    char value[8];

    /* default if not set it 0 */
    property_get(BRCM_PROPERTY_CALL_ACTIVE, value, NULL);

    LOGD("%s: %s=%s\n", __FUNCTION__, BRCM_PROPERTY_CALL_ACTIVE, value);

    if (strcmp(value, "1") == 0) {
        LOGD("%s: %s=1\n", __FUNCTION__, BRCM_PROPERTY_CALL_ACTIVE);
        return 1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         btl_cfg_getBDAFilterCond
**
** Description      Gets the filter condition for DiscoverDevices if one exists
**
** Returns          non-zero if a property is configured to filter on bdaddr
**
*******************************************************************************/

#define BTL_CFG_GET_BDA_FILT_COND  BCM_CFG_BRCM_PROP(BTL_GLOBAL_PROP_INQ_BDA_FILTER)
int btl_cfg_getBDAFilterCond(BD_ADDR bd_addr)
{
    int  nRet = 0;
    char bdaFormatted[18] = {0}; // buffer to hold formatted bd_addr
    int  ntokens = 0;

    LOGD("+btl_cfg_getBDAFilterCond getting property %s", BTL_CFG_GET_BDA_FILT_COND);

    if (get_property_by_name(BTL_CFG_GET_BDA_FILT_COND, bdaFormatted, "") != 0)
    {
        ntokens = sscanf(bdaFormatted,
                         "%02x:%02x:%02x:%02x:%02x:%02x",
                         &bd_addr[0],
                         &bd_addr[1],
                         &bd_addr[2],
                         &bd_addr[3],
                         &bd_addr[4],
                         &bd_addr[5]);

        if (ntokens == 6)
        {
            nRet = 1;
        }
    }

    LOGD("-btl_cfg_getBDAFilterCond bdaddr=%s, nRet=%d",bdaFormatted,nRet);
    return nRet;
}

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
int btl_cfg_get_avrcp_pass_thru_status (void)
{
    char value[3];

    /* default if not set it "1" */
    property_get(BRCM_PROPERTY_AVRCP_PASS_THROUGH_ACTIVE, value, "1");

    LOGD("%s: %s=%s", __FUNCTION__, BRCM_PROPERTY_AVRCP_PASS_THROUGH_ACTIVE, value);

    if (strcmp(value, "1") == 0) {
        return 1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         btl_cfg_get_btld_status
 **
 ** Description      Get btld status from Android property system
 **
 ** Returns          0 if btld not active yet; 1 if btld is active
 **
 *******************************************************************************/
int btl_cfg_get_btld_status()
{
    char value[PROPERTY_VALUE_MAX];

    /* default if not set it 0 */
    property_get(BRCM_PROPERTY_BTLD_ACTIVE, value, "0");

    LOGV("%s: %s=%s\n", __FUNCTION__, BRCM_PROPERTY_BTLD_ACTIVE, value);

    if (strcmp(value, "1") == 0)
    {
        return 1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         btl_cfg_set_btld_status
 **
 ** Description      Set btld status in Android property system
 **
 ** Returns          0 if not successful
 **
 *******************************************************************************/
int btl_cfg_set_btld_status(int iNewBtldStatus)
{
    char value[PROPERTY_VALUE_MAX];
    int ret;

    if (iNewBtldStatus)
        strcpy(value, "1");
    else
        strcpy(value, "0");

    ret = property_set(BRCM_PROPERTY_BTLD_ACTIVE, value);
    LOGV("%s: %s=%s, ret=%d\n", __FUNCTION__, BRCM_PROPERTY_BTLD_ACTIVE, value, ret);
    return ret;
}

/*******************************************************************************
 **
 ** Function         btl_cfg_is_soft_onoff_enabled
 **
 ** Description     Check BT Software ON/OFF status from Android property system
 **
 ** Returns          1 if soft on/off is enabled; 0 otherwise
 **
 *******************************************************************************/
int btl_cfg_is_soft_onoff_enabled(void)
{
    char value[PROPERTY_VALUE_MAX];
    int enabled = 0;

    /* default if not set it 0 */
    property_get(BRCM_PROPERTY_SOFT_ONOFF_ENABLE, value, "0");

    LOGV("btl_cfg_get_soft_onoff_enabled : %s = %s\n", BRCM_PROPERTY_SOFT_ONOFF_ENABLE, value);
    sscanf(value, "%d", &enabled);

    return enabled;
}

/*******************************************************************************
 **
 ** Function         btl_cfg_set_btld_pid
 **
 ** Description     Sets the PID of the BTLD process to the Android property system
 **
 ** Returns          non-zero if btld is active; 0 otherwise
 **
 *******************************************************************************/
int btl_cfg_set_btld_pid(int pid)
{
    int ret;
    char value[PROPERTY_VALUE_MAX];
    sprintf(value, "%d", pid);

    LOGV("btld_cfg_btld_pid : %s = %s\n", BRCM_PROPERTY_BTLD_PID, value);

    ret = property_set(BRCM_PROPERTY_BTLD_PID, value);

    if (ret)
        LOGE("btl_cfg_set_btld_pid failed : %s = %s, ret = %d\n", BRCM_PROPERTY_BTLD_PID, value, ret);
    else
        LOGV("btl_cfg_set_btld_pid success : %s = %s, ret = %d\n", BRCM_PROPERTY_BTLD_PID, value, ret);

    return ret;
}

/*******************************************************************************
**
** Function         btl_cfg_get_hwtun_port
**
** Description      Get runtime patch ram path configuration from Android Property System
**
** Input            Pointer to ram variable big enough for path
**
** Returns          patch ram path
**
*******************************************************************************/

int btl_cfg_get_hwtun_port(void)
{
    char val[BTL_CFG_LINE_MAXLEN];

#ifndef LINUX_NATIVE
    if (property_get(property_names[BTL_CFG_KEY_HWTUN_PORT], val, "10000"))
    {
        LOGI("%s: %s=%s", __FUNCTION__, property_names[BTL_CFG_KEY_HWTUN_PORT], val);
        return atoi(val);
    }
    return 10000;
#else
   {
       extern int bthwtun_port;
       return bthwtun_port;
    }
#endif
}

#if (BTL_CFG_USE_CONF_FILE==TRUE)
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
int btl_cfg_read_conf(const char * p_path)
{
    FILE                *fd;
    int                 result = -1;
    char                *p_name;
    char                *p_value;
    tCONF_PROP_ENTRY    *p_entry;
    int                 index;
    char                line[BTL_CONF_MAX_LINE_LEN+1]; /* add 1 for 0 char */

    APPL_TRACE_DEBUG1( "btl_cfg_read_conf( reading file >%s< )", p_path );
    if ( (FILE *)NULL != (fd = fopen(p_path, "r")) )
    {
        result = 1;
        /* read line by line */
        while ( NULL != fgets( line, BTL_CONF_MAX_LINE_LEN+1, fd ) )
        {
            if ( BTL_CONF_COMMENT == line[0] )
                continue;
            p_name = strtok( line, BTL_CONF_ASSIGN_CHAR );
            if ( NULL == p_name )
            {
                APPL_TRACE_WARNING1("btl_cfg_read_conf(): NO assignement in this line: >%s<", line );
                continue;
            }
            p_value = strtok( NULL, BTL_CONF_ASSIGN_CHAR );
            if ( NULL==p_value )
            {
                APPL_TRACE_WARNING1("btl_cfg_read_conf(): no value found for name: >%s<", p_name );
                continue;
            }
            APPL_TRACE_API2("btl_cfg_read_conf(): prop_name: >%s<, value: >%s<", p_name, p_value );
            p_entry = (tCONF_PROP_ENTRY *)&conf_table[0];
            index = 0;
            while ( NULL != p_entry->conf_name )
            {
                if ( 0 == strcmp( p_entry->conf_name, (const char *)p_name ) )
                {
                    /* execute first entry found */
                    if ( 0 > p_entry->p_action( index, (const char *)p_name, (const char *)p_value ) )
                    {
                        APPL_TRACE_WARNING0( "btl_cfg_read_conf() setting failed" );
                    }
                    result++;
                    break;
                }
                index++;
                p_entry++;
            }
        }
        fclose(fd);
    }
    else
    {
        /* just for debugging purpose. this is not failure as the file may not be present on
         * purpose.
         */
        APPL_TRACE_DEBUG1( "btl_cfg_read_conf( file: >%s< ) not found", p_path );
    }
    return result;
} /* btl_cfg_read_conf() */


int btl_cfg_afh_first(int idx, const char *p_prop_name, const char *p_prop_value)
{
    int     ahf_first;
    int     res = -1;

    APPL_TRACE_API3( "btl_cfg_ahf_first(idx:%d, name:>%s<, value:>%s< )", idx, p_prop_name,
            p_prop_value );
    ahf_first = atoi( p_prop_value );
    if ( ((0 <= ahf_first) && (79 > ahf_first)) || (255==ahf_first) )
    {
        bte_appl_cfg.afh_first = ahf_first;
        res = 1;
    }
    else
    {
        APPL_TRACE_WARNING1( "btl_cfg_ahf_first(): unsupported value %d", ahf_first );
    }
    return res;
}

int btl_cfg_afh_last(int idx, const char *p_prop_name, const char *p_prop_value)
{
    int     ahf_last;
    int     res = -1;

    APPL_TRACE_API3( "btl_cfg_ahf_last(idx:%d, name:>%s<, value:>%s< )", idx, p_prop_name,
            p_prop_value );
    ahf_last = atoi( p_prop_value );
    if ( ((0 <= ahf_last) && (79 > ahf_last)) || (255==ahf_last) )
    {
        bte_appl_cfg.afh_last = ahf_last;
        res = 1;
    }
    else
    {
        APPL_TRACE_WARNING1( "btl_cfg_ahf_last(): unsupported value %d", ahf_last );
    }
    return res;
}

#endif
