/*****************************************************************************
**
**  Name:             btapp_vcard.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_VCARD_H
#define BTAPP_VCARD_H

#ifndef LINUX_NATIVE
#define SQLITE_ENABLED
#endif

#ifdef SQLITE_ENABLED
#include "sqlite3.h"
#include "sqlite3_android.h"
#else
#define sqlite int
#define sqlite3 int
#endif

enum {
    VCARD_STORE_DUP_ACTION_NONE,    // Do Nothing
    VCARD_STORE_DUP_ACTION_ADD_NEW, // Add as new
    VCARD_STORE_DUP_ACTION_REPLACE, // Update the duplicate entry (Add as replacement)
};
typedef UINT8 tVCARD_STORE_DUP_ACTION;

enum {
    VCARD_STORE_STATUS_FAIL,              // vCard store fail
    VCARD_STORE_STATUS_STORED,            // vCard stored
    VCARD_STORE_STATUS_DUP_NOT_STORED,    // Duplicate vCard found; vCard not stored
    VCARD_STORE_STATUS_DUP_STORED_AS_NEW, // Duplicate vCard found; vCard stored as new
    VCARD_STORE_STATUS_DUP_REPLACED,      // Duplicate vCard found; vCard stored as replacement
};
typedef UINT8 tVCARD_STORE_STATUS;

enum {
    VCARD_SET_INDEX_PEOPLE,
    VCARD_SET_INDEX_SIM_PEOPLE,
    VCARD_SET_INDEX_MYPROFILE_PEOPLE,
    VCARD_SET_INDEX_MAX
};
typedef UINT8 tVCARD_SET_INDEX;

typedef struct {
    tVCARD_STORE_DUP_ACTION dup_action;
    BOOLEAN found_dup_entry;
    UINT32 dup_id;
    UINT32 person_id;
    DTUN_PATH name;
    tBTA_OP_STATUS status;
} tDUP_ENTRY_INFO;
tBTA_OP_STATUS btapp_vcard_read_contact_entry (sqlite3 *db,
    tVCARD_SET_INDEX vcard_set_index, UINT32 key, UINT8 *p_card, UINT16 *p_len);
BOOLEAN btapp_vcard_build_vcard (DTUN_PATH vcard_key, char *p_path);
tDUP_ENTRY_INFO *btapp_vcard_store_vcard(char *p_path, tVCARD_STORE_DUP_ACTION dup_action);

#endif
