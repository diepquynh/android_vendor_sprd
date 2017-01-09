/****************************************************************************
**
**  Name:          btui_android.c
**
**  Description:   Platform specific functions:
**                      - paired devices management
**                      - simple pairing
**
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
******************************************************************************/
#include "bt_target.h"
#include "gki.h"
#include "btui.h"
#include "btui_int.h"

#ifndef BTUI_DEFAULT_LOCAL_NAME
#define BTUI_DEFAULT_LOCAL_NAME "BRCM Bluetooth Device"
#endif

tBTUI_CB btui_cb;
tBTUI_CFG btui_cfg;

#if (!defined(BLUEZ_INCLUDED) || BLUEZ_INCLUDED == FALSE)
/*******************************************************************************
**
** Function         btui_init_device_db
**
** Description      Initialises the device data base
**
**
** Returns          void
*******************************************************************************/
void btui_init_device_db (void)
{


    memset(&btui_device_db,0x00,sizeof(btui_device_db));

    btui_device_db.bt_enabled = TRUE;

    /* set visibility to true by default. If visibility setting
    was stored in nvram previously it will get overwritten when
    all the parameters are read from nvram */
    btui_device_db.visibility = TRUE;

    /* Set the local device name to default name */
    strcpy(btui_device_db.local_device_name, BTUI_DEFAULT_LOCAL_NAME);

    /* Phone needs to store in nvram some information about
    itself and other bluetooth devices with which it regularly communicates
    The information that has to be stored typically are
    local bluetooth name, visbility setting bdaddr, name, link_key,
    trust relationship etc.  */

    /* read device data base
    stored in nv-ram */
    btui_nv_init_device_db();

#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)
    btui_nv_init_fm_db();
#endif
#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
    btui_nv_init_hs_db();
#endif

}

/*******************************************************************************
**
** Function         btui_store_device
**
** Description      stores peer device to NVRAM
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btui_store_device( tBTUI_REM_DEVICE * p_rem_device)

{
    UINT8 i;


    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(btui_device_db.device[i].in_use
            && memcmp(&btui_device_db.device[i].bd_addr, p_rem_device->bd_addr, BD_ADDR_LEN))
            continue;
        memcpy(&btui_device_db.device[i], p_rem_device, sizeof(btui_device_db.device[i]));
        btui_device_db.device[i].in_use = TRUE;

        APPL_TRACE_EVENT2("store device %s, services = %08X", p_rem_device->name, p_rem_device->services);
        break;

    }

    if(i == BTUI_NUM_REM_DEVICE)
        return FALSE;

    /* update data base in nvram */
    btui_nv_store_device_db();
    return TRUE;

}

/*******************************************************************************
**
** Function         btui_get_device_record
**
** Description      gets the device record of a stored device
**
**
** Returns          void
*******************************************************************************/
tBTUI_REM_DEVICE * btui_get_device_record(BD_ADDR bd_addr)
{

    UINT8 i;

    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(!btui_device_db.device[i].in_use
            || memcmp(btui_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return &btui_device_db.device[i];

    }

    return NULL;


}

/*******************************************************************************
**
** Function         btui_alloc_device_record
**
** Description      gets the device record of a stored device
**
**
** Returns          void
*******************************************************************************/
tBTUI_REM_DEVICE * btui_alloc_device_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(!btui_device_db.device[i].in_use)
        {
            /* not in use */
            memset(&btui_device_db.device[i], 0, sizeof(btui_device_db.device[i]));
            btui_device_db.device[i].in_use = TRUE;
            memcpy(btui_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN);
            return &btui_device_db.device[i];
        }
    }

    return NULL;
}

/*******************************************************************************
**
** Function         btui_get_inquiry_record
**
** Description      gets the device record from inquery db
**
**
** Returns          void
*******************************************************************************/
tBTUI_REM_DEVICE * btui_get_inquiry_record(BD_ADDR bd_addr)
{

    UINT8 i;

    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(memcmp(btui_inq_db.remote_device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return &btui_inq_db.remote_device[i];

    }
    return NULL;
}

/*******************************************************************************
**
** Function         btui_get_dev_name
**
** Description
**
**
** Returns          NULL, if the device is not found in stored/inquiry DB
*******************************************************************************/
char * btui_get_dev_name(BD_ADDR bd_addr)
{
    tBTUI_REM_DEVICE *p_rec;
    char *p_name = NULL;

    if( NULL == (p_rec = btui_get_device_record(bd_addr)) )
        p_rec = btui_get_inquiry_record(bd_addr);

    if(p_rec)
    {
        if(p_rec->short_name[0])
            p_name = p_rec->short_name;
        else if(p_rec->name[0])
            p_name = p_rec->name;
    }

    return p_name;
}

/*******************************************************************************
**
** Function         btui_delete_device
**
** Description      deletes the device from nvram
**
**
** Returns          void
*******************************************************************************/
void btui_delete_device(BD_ADDR bd_addr)
{

    UINT8 i;

    for(i=0; i<BTUI_NUM_REM_DEVICE; i++)
    {
        if(memcmp(btui_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;

        for(;i<(BTUI_NUM_REM_DEVICE-1) ; i++)
        {
            if(!btui_device_db.device[i+1].in_use)
            {
                memset(&btui_device_db.device[i], 0, sizeof(btui_device_db.device[i+1]));
                break;
            }
            memcpy(&btui_device_db.device[i], &btui_device_db.device[i+1], sizeof(btui_device_db.device[i+1]));

        }

        if(i==(BTUI_NUM_REM_DEVICE-1))
            memset(&btui_device_db.device[i], 0, sizeof(btui_device_db.device[i+1]));
        break;

    }
    /* update data base in nvram */
    btui_nv_store_device_db();


}


/*******************************************************************************
**
** Function         btui_nv_init_device_db
**
** Description      Inits device data base with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_init_device_db(void)
{
}
#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btui_nv_init_hs_db
**
** Description      Inits HS settings with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_init_hs_db(void)
{
}
/*******************************************************************************
**
** Function         btui_nv_store_hs_db
**
** Description      Stores all parameters into nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_store_hs_db(void)
{
}
#endif
#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btui_nv_init_fm_db
**
** Description      Inits FM data base with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_init_fm_db(void)
{
}
/*******************************************************************************
**
** Function         btui_nv_store_fm_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_store_fm_db(void)
{
}
#endif


#if( defined BTA_CT_INCLUDED ) && (BTA_CT_INCLUDED == TRUE)
void btui_nv_init_wug_db(void)
{
}


/*******************************************************************************
**
** Function         btui_nv_store_wug_db
**
** Description      Stores all parameters into nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_store_wug_db(void)
{
}

#endif
/*******************************************************************************
**
** Function         btui_nv_store_device_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btui_nv_store_device_db(void)
{
}




/*******************************************************************************
**
** Function         btui_store_bt_enable_setting
**
** Description      Stores BT enable setting
**
**
** Returns          void
*******************************************************************************/
void btui_store_bt_enable_setting(BOOLEAN enabled)
{
    /* this stores all nvram parameters */
    btui_nv_store_device_db();

}


/*******************************************************************************
**
** Function         btui_store_local_name
**
** Description      Stores local device name into nvram
**
**
** Returns          void
*******************************************************************************/
void btui_store_local_name(char * p_name)
{


    /* this stores all nvram parameters */
    btui_nv_store_device_db();

}

/*******************************************************************************
**
** Function         btui_store_visibility_setting
**
** Description      Stores visibility setting to into nvram
**
**
** Returns          void
*******************************************************************************/
void btui_store_visibility_setting(BOOLEAN visibility)
{

    /* this stores all nvram parameters */
    btui_nv_store_device_db();

}


/*******************************************************************************
**
** Function         btui_platform_startup
**
** Description      Platform specific startup
**
**
** Returns          void
*******************************************************************************/
void btui_platform_startup(void)
{




}


/*******************************************************************************
**
** Function         btui_dm_proc_loc_oob
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btui_dm_proc_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r)
{
}
#endif
