/****************************************************************************
**
**  Name:          btapp_vcard.c
**
**  Description:   Contains application level utility functions for vCard conversions
**
**
**  Copyright (c) 2002-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if (defined BTA_OP_INCLUDED) && (BTA_OP_INCLUDED == TRUE)

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "gki.h"
#include "bta_api.h"
#include "bta_op_api.h"
#include "bta_op_fmt.h"
#include "bta_fs_co.h"
#include "bd.h"
#include "btui.h"
#include "btui_int.h"
#include "btapp_dm.h"
#include "dtun_api.h"
#include "bte_appl.h"
#include "btapp_vcard.h"

#ifndef LINUX_NATIVE
#define SQLITE_ENABLED
#endif

#ifdef SQLITE_ENABLED
#include "sqlite3.h"
#include "sqlite3_android.h"
#else
#define sqlite int
#define sqlite3 int
#define sqlite3_stmt int
#endif

#define LOG_TAG "BTAPP_VCARD:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#include "utils/Log.h"
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

#define DEBUG              1
#define debug(format, ...) if (DEBUG) {LOGD (format, ## __VA_ARGS__);}
#define info(format, ...)  LOGI (format, ## __VA_ARGS__)
#define error(format, ...) LOGE (format, ## __VA_ARGS__)

#define UTF16_STORAGE                       0

#define VCARD_MAX_N_FIELDS                  5
#define VCARD_MAX_ADR_FIELDS                7

#define VCARD_PHONES_TYPE_CUSTOM            0
#define VCARD_PHONES_TYPE_HOME              1
#define VCARD_PHONES_TYPE_MOBILE            2
#define VCARD_PHONES_TYPE_WORK              3
#define VCARD_PHONES_TYPE_FAX_WORK          4
#define VCARD_PHONES_TYPE_FAX_HOME          5
#define VCARD_PHONES_TYPE_PAGER             6
#define VCARD_PHONES_TYPE_OTHER             7

#define VCARD_CONTACT_METHODS_KIND_EMAIL    1
#define VCARD_CONTACT_METHODS_KIND_ADDRESS  2

#define VCARD_CONTACT_METHODS_TYPE_CUSTOM   0
#define VCARD_CONTACT_METHODS_TYPE_HOME     1
#define VCARD_CONTACT_METHODS_TYPE_WORK     2
#define VCARD_CONTACT_METHODS_TYPE_OTHER    3

#define VCARD_ORGANIZATIONS_TYPE_CUSTOM     0
#define VCARD_ORGANIZATIONS_TYPE_WORK       1
#define VCARD_ORGANIZATIONS_TYPE_OTHER      2

#define VCARD_MEM_LEN                       8224
#define VCARD_STMT_BUF_SIZE                 300
#define VCARD_MAX_PROP                      25
#define VCARD_VALUE_LEN                     (7*1024)
#define VCARD_PEOPLE_TBL_NAME_SIZE          30
#define VCARD_B64_DEF_LINE_SIZE             60
#define VCARD_N_FIELD_TOKEN_LEN             256

#define PARAMETER(i,m)  ((vcard_sqlite_cb.prop_array[(i)].parameters) & (m))

enum {
    VCARD_CONTACT_TABLE_ID_PEOPLE,
    VCARD_CONTACT_TABLE_ID_PHONES,
    VCARD_CONTACT_TABLE_ID_CONTACT_METHODS,
    VCARD_CONTACT_TABLE_ID_ORGANIZATIONS,
    VCARD_CONTACT_TABLE_ID_PHOTOS,
#if !defined(BTLA_REL_2_X) && !(BTLA_REL_2_X == TRUE)
    VCARD_CONTACT_TABLE_ID_GROUPMEMBERSHIP,
#endif
    VCARD_CONTACT_TABLE_ID_ANDROID_METADATA,
    VCARD_CONTACT_TABLE_ID_MAX
};
typedef UINT8 tVCARD_CONTACT_TABLE_ID;

typedef struct {
    UINT8 op_mem[VCARD_MEM_LEN];
    char stmt_buf[VCARD_STMT_BUF_SIZE];
    char value[VCARD_MAX_PROP][VCARD_VALUE_LEN];
    tBTA_OP_PROP prop_array[VCARD_MAX_PROP];
    UINT8 num_prop;
    char locale[20];
    tVCARD_CONTACT_TABLE_ID tbl_id;
    tVCARD_SET_INDEX vcard_set_index;
    tDUP_ENTRY_INFO dup_entry_info;
    sqlite3 *db;
} tVCARD_SQLITE_CB;

static tVCARD_SQLITE_CB vcard_sqlite_cb = {
    { 0 },
    { 0 },
    { { 0 } },
    { { NULL, 0, 0, 0, NULL, 0 } },
    VCARD_MAX_PROP,
    "",
    VCARD_CONTACT_TABLE_ID_MAX,
    VCARD_SET_INDEX_PEOPLE,
    { VCARD_STORE_DUP_ACTION_NONE, FALSE, 0, 0, "", VCARD_STORE_STATUS_FAIL },
    NULL
};

static char *vcard_tbl_set[VCARD_SET_INDEX_MAX][VCARD_CONTACT_TABLE_ID_MAX] = {
    // VCARD_SET_INDEX_PEOPLE
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    { "view_v1_people", "view_v1_phones", "view_v1_contact_methods",
      "view_v1_organizations", "view_v1_photos", "android_metadata" },
    // VCARD_SET_INDEX_SIM_PEOPLE
    { "sim_people", "sim_phones", "",
      "", "", "android_metadata"  },
    // VCARD_SET_INDEX_MYPROFILE_PEOPLE
    { "myprofile_people", "myprofile_phones", "myprofile_contact_methods",
      "myprofile_organizations", "myprofile_photos", "android_metadata"  },
#else
    { "people", "phones", "contact_methods",
      "organizations", "photos", "groupmembership", "android_metadata" },
    // VCARD_SET_INDEX_SIM_PEOPLE
    { "sim_people", "sim_phones", "",
      "", "", "", "android_metadata"  },
    // VCARD_SET_INDEX_MYPROFILE_PEOPLE
    { "myprofile_people", "myprofile_phones", "myprofile_contact_methods",
      "myprofile_organizations", "myprofile_photos", "myprofile_groupmembership", "android_metadata"  },
#endif
};

// Translation Table as described in RFC1113
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Translation Table to decode
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

extern const tBTA_OP_PROP_TBL bta_op_vcard_tbl[];

/*******************************************************************************
**
** Function         btapp_vcard_b64_encodeblock
**
** Description      Helper function to encode 3 8-bit binary bytes as
**                  4 '6-bit' characters
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_b64_encodeblock (UINT8 in[3], UINT8 out[4], int len)
{
    out[0] = cb64[in[0] >> 2];
    out[1] = cb64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
    out[2] = (UINT8)((len > 1) ? cb64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
    out[3] = (UINT8)((len > 2) ? cb64[in[2] & 0x3f] : '=');
}

/*******************************************************************************
**
** Function         btapp_vcard_b64_encode
**
** Description      Helper function to encode as a BASE64 stream
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_b64_encode (UINT8 *in_buf, UINT8 *out_buf, UINT16 in_len)
{
    UINT8 *inb=in_buf, *outb=out_buf, in[3], out[4];
    int i, len, blocksout=0;
    UINT16 inl=in_len;

    *outb++ = '\r';
    *outb++ = '\n';
    *outb++ = ' ';

    while (inl) {
        len = 0;
        for (i=0; i<3; i++) {
            in[i] = *inb++;
            if (inl) {
                len++;
                inl--;
            } else {
                in[i] = 0;
            }
        }

        if (len) {
            btapp_vcard_b64_encodeblock(in, out, len);
            for (i=0; i<4; i++) {
                *outb++ = out[i];
            }
            blocksout++;
        }

        if ((blocksout >= (VCARD_B64_DEF_LINE_SIZE/4)) || (!inl)) {
            if (blocksout) {
                *outb++ = '\r';
                *outb++ = '\n';
                *outb++ = ' ';
            }
            blocksout = 0;
        }
    }
    *outb = 0;
}

/*******************************************************************************
**
** Function         btapp_vcard_b64_decodeblock
**
** Description      Helper function to decode 4 '6-bit' characters into
**                  3 8-bit binary bytes
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_b64_decodeblock (UINT8 in[4], UINT8 out[3])
{
    out[0] = (UINT8) (in[0] << 2 | in[1] >> 4);
    out[1] = (UINT8) (in[1] << 4 | in[2] >> 2);
    out[2] = (UINT8) (((in[2] << 6) & 0xc0) | in[3]);
}

/*******************************************************************************
**
** Function         btapp_vcard_b64_decode
**
** Description      Helper function to decode a BASE64 encoded stream
**
**
** Returns          void
*******************************************************************************/
static UINT16 btapp_vcard_b64_decode (UINT8 *in_buf, UINT8 *out_buf)
{
    UINT8 *inb=in_buf, *outb=out_buf, in[4], out[3], v;
    UINT16 i, len, out_len=0;

    while (*inb) {
        for (len=0, i=0; (i<4) && *inb; i++) {
            v = 0;
            while (*inb && (v == 0)) {
                v = *inb++;
                v = (UINT8)(((v<43) || (v>122)) ? 0 : cd64[(v-43)]);
                if (v) {
                    v = (UINT8)((v == '$') ? 0 : (v-61));
                }
            }

            if (*inb) {
                len++;
                if (v) {
                    in[i] = (UINT8) (v - 1);
                }
            } else {
                in[i] = 0;
            }
        }

        if (len) {
            btapp_vcard_b64_decodeblock(in, out);
            for (i=0; i<(len-1); i++) {
                *outb++ = out[i];
                out_len++;
            }
        }
    }
    return out_len;
}

/*******************************************************************************
**
** Function         btapp_vcard_cstr_to_utf8
**
** Description      Helper function to convert C string to an UTF-8 encoded
**                  string
**
** Returns          void
*******************************************************************************/
static UINT8 *btapp_vcard_cstr_to_utf8 (UINT8 *str)
{
    UINT16 *utf16, i, len, len16;
    UINT8 *p_utf16;

    if (!str || !(*str)) {
        return NULL;
    }

    len = strlen((char *)str) + 1;
    len16 = len * sizeof(UINT16);
    utf16 = (UINT16 *)GKI_getbuf(len16);
    p_utf16 = (UINT8 *)utf16;

    for (i=0; i<len; i++) {
        p_utf16[sizeof(UINT16)*i] = str[i];
        p_utf16[sizeof(UINT16)*i+1] = 0;
    }

    utfc_16_to_8(str, len16, utf16, len16);
    GKI_freebuf(utf16);
    return str;
}

/*******************************************************************************
**
** Function         btapp_vcard_utf8_to_cstr
**
** Description      Helper function to convert an UTF-8 encoded string to a C
**                  string
**
** Returns          void
*******************************************************************************/
static UINT8 *btapp_vcard_utf8_to_cstr (UINT8 *str)
{
    UINT16 *utf16, i, len16;
    UINT8 *p_utf16;

    if (!str || !(*str)) {
        return NULL;
    }

    len16 = (strlen((char *)str) + 1) * sizeof(UINT16);
    utf16 = (UINT16 *)GKI_getbuf(len16);

    len16 = utfc_8_to_16(utf16, len16, str);

    p_utf16 = (UINT8 *)utf16;
    for (i=0; i<len16; i++) {
        str[i] = p_utf16[sizeof(UINT16)*i];
    }

    GKI_freebuf(utf16);
    return str;
}

/*******************************************************************************
**
** Function         btapp_vcard_convert_encoding
**
** Description      Helper function to convert data as per encoding
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_convert_encoding (UINT8 indx)
{
    char *p_data;
    char encoded_buf[4];

    if (PARAMETER(indx, BTA_OP_ENC_QUOTED_PRINTABLE)) {
        vcard_sqlite_cb.prop_array[indx].len = 0;
        p_data = (char *)vcard_sqlite_cb.prop_array[indx].p_data;
        while(*p_data != '\0') {
            if (*p_data != '=') {
                /* It is possible that tabs and spaces in the middle of the string may not be encoded. */
                vcard_sqlite_cb.prop_array[indx].p_data[vcard_sqlite_cb.prop_array[indx].len++] =
                *p_data++;
                continue;
            }

            if (*(p_data + 1) == '\r' && *(p_data + 2) == '\n') {
                /* = CR LF should be ignored */
                p_data += 3;
                continue;
            }

            /* Printable ascii number can be followed by the encoded byte. It should not be changed by strtoul(). */
            strncpy(encoded_buf, p_data, 3);
            encoded_buf[3] = '\0';

            vcard_sqlite_cb.prop_array[indx].p_data[vcard_sqlite_cb.prop_array[indx].len++] =
                (UINT8)strtoul(encoded_buf+1, NULL, 16);

            p_data += 3;
        }
        vcard_sqlite_cb.prop_array[indx].p_data[vcard_sqlite_cb.prop_array[indx].len] = '\0';
        debug("  QUOTED-PRINTABLE: %s (%d)",
            vcard_sqlite_cb.prop_array[indx].p_data,
            vcard_sqlite_cb.prop_array[indx].len);
    }
}

/*******************************************************************************
**
** Function         btapp_vcard_close_db
**
** Description      Helper function to close database
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_close_db (sqlite3 *db)
{
#ifdef SQLITE_ENABLED
    if (db) {
        if (sqlite3_close(db) != SQLITE_OK) {
            error("%s: Error closing database - %s", __FUNCTION__, bte_appl_cfg.contacts_db);
        }
    }
#endif
}

/*******************************************************************************
**
** Function         btapp_vcard_exec_stmt
**
** Description      Helper function to execute the sqlite command
**
**
** Returns          void
*******************************************************************************/
static BOOLEAN btapp_vcard_exec_stmt (sqlite3 *db, char *stmt, int (*callback)(void*,int,char**,char**))
{
    char *errmsg;

#ifdef SQLITE_ENABLED
    if (sqlite3_exec(db, stmt, callback, NULL, &errmsg) != SQLITE_OK)
    {
        error("%s: Error executing %s - %s", __FUNCTION__, stmt, errmsg);
        btapp_vcard_close_db(db);
        return FALSE;
    }
#endif

    debug("Executed %s", stmt);
    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_vcard_dup_entry_cb
**
** Description      Callback function for "Select" operation to determine duplicate entries
**
**
** Returns          void
*******************************************************************************/
static int btapp_vcard_dup_entry_cb (void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i;

    for (i=0; i<num_fields; i++) debug("Dup Entry[%d]:%s = %s", i+1, p_col_names[i], p_fields[i] ? p_fields[i] : "(null)");
    vcard_sqlite_cb.dup_entry_info.found_dup_entry = TRUE;
    vcard_sqlite_cb.dup_entry_info.dup_id = atoi(p_fields[0]); // _id
    return 0;
}

/*******************************************************************************
**
** Function         btapp_vcard_reverse_number_string
**
** Description      Helper function to reverse number string
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_reverse_number_string (char *src, char *dst)
{
    char *tmp;
    int len;

    if (src == NULL || dst == NULL) {
        return;
    }

    len = strlen(src);
    tmp = src + (len - 1);

    while (tmp >= src) {
        if ((*tmp >= '0') && (*tmp <= '9')) {
            *dst++ = *tmp;
        }
        tmp--;
    }
}

/*******************************************************************************
**
** Function         btapp_vcard_add_SQL_escape_squence
**
** Description     Escape all single quotation mark to use the string in the SQL statement.
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_add_SQL_escape_squence(char *dst, char *src)
{
    if (dst == NULL || src == NULL)
        return;

    while (*src != '\0') {
        if (*src == '\'')
            *dst++ = '\'';

        *dst++ = *src++;
    }

    *dst = '\0';
}

/*******************************************************************************
**
** Function         btapp_vcard_write_contact_table
**
** Description      Writes into Contacts tables
**
**
** Returns          void
*******************************************************************************/
static int btapp_vcard_write_contact_table (sqlite3 *db, tVCARD_CONTACT_TABLE_ID id)
{
    tDTUN_DEVICE_SIGNAL sig;
    int i, j;

    debug("Writing table id: %d", id);

    memset(vcard_sqlite_cb.stmt_buf, 0, sizeof(vcard_sqlite_cb.stmt_buf));
    vcard_sqlite_cb.tbl_id = id;
    switch (id) {
        case VCARD_CONTACT_TABLE_ID_PEOPLE:
        {
            char disp_name[VCARD_STMT_BUF_SIZE], *p_n[VCARD_MAX_N_FIELDS], *n=NULL, *fn=NULL, *name="<No Name>", *notes="", *space=" ";
            char escape_sequence_buf[VCARD_VALUE_LEN];
            // name, notes, primary_phone, primary_organization, primary_email, photo_version, custom_ringtone, phonetic_name

            for (i=0; i<vcard_sqlite_cb.num_prop; i++) {
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_N) {
                    for (j=0; j<VCARD_MAX_N_FIELDS; j++)
                        p_n[j] = NULL;

                    // Family Name;Given Name;Additional Names; Name Prefix;Name Suffix
                    p_n[0] = (char *)vcard_sqlite_cb.prop_array[i].p_data;
                    for (j=1; j<VCARD_MAX_N_FIELDS; j++) {
                        if ((p_n[j] = strchr(p_n[j-1], ';')) == NULL) break;
                        *(p_n[j]++) = '\0';
                    }

                    *disp_name = '\0';
                    if (p_n[3] && strlen(p_n[3])) {
                        btapp_vcard_add_SQL_escape_squence(disp_name + strlen(disp_name), p_n[3]);
                        strcat(disp_name, space);
                    }
                    if (p_n[1] && strlen(p_n[1])) {
                        btapp_vcard_add_SQL_escape_squence(disp_name + strlen(disp_name), p_n[1]);
                        strcat(disp_name, space);
                    }
                    if (p_n[2] && strlen(p_n[2])) {
                        btapp_vcard_add_SQL_escape_squence(disp_name + strlen(disp_name), p_n[2]);
                        strcat(disp_name, space);
                    }
                    if (p_n[0] && strlen(p_n[0])) {
                        btapp_vcard_add_SQL_escape_squence(disp_name + strlen(disp_name), p_n[0]);
                        strcat(disp_name, space);
                    }
                    if (p_n[4] && strlen(p_n[4])) {
                        btapp_vcard_add_SQL_escape_squence(disp_name + strlen(disp_name), p_n[4]);
                        strcat(disp_name, space);
                    }

                    while (disp_name[strlen(disp_name)-1] == *space) {
                        disp_name[strlen(disp_name)-1] = '\0';
                    }

                    n = disp_name;
                    if (!(PARAMETER(i, BTA_OP_CHAR_UTF_8))) {
                        btapp_vcard_cstr_to_utf8((UINT8 *)n);
                    }
                }
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_FN) {
                    memset(escape_sequence_buf, 0, sizeof(escape_sequence_buf));
                    btapp_vcard_add_SQL_escape_squence(escape_sequence_buf, (char *)vcard_sqlite_cb.prop_array[i].p_data);
                    snprintf((char *)vcard_sqlite_cb.prop_array[i].p_data, VCARD_VALUE_LEN, "%s", escape_sequence_buf);
                    fn = (char *)vcard_sqlite_cb.prop_array[i].p_data;
                    if (!(PARAMETER(i, BTA_OP_CHAR_UTF_8))) {
                        btapp_vcard_cstr_to_utf8((UINT8 *)fn);
                    }
                }
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_NOTE) {
                    memset(escape_sequence_buf, 0, sizeof(escape_sequence_buf));
                    btapp_vcard_add_SQL_escape_squence(escape_sequence_buf, (char *)vcard_sqlite_cb.prop_array[i].p_data);
                    snprintf((char *)vcard_sqlite_cb.prop_array[i].p_data, VCARD_VALUE_LEN, "%s", escape_sequence_buf);
                    notes = (char *)vcard_sqlite_cb.prop_array[i].p_data;
                    if (!(PARAMETER(i, BTA_OP_CHAR_UTF_8))) {
                        btapp_vcard_cstr_to_utf8((UINT8 *)notes);
                    }
                }
            }

            name = ((n != NULL) ? n : ((fn != NULL) ? fn : name));
            memcpy(vcard_sqlite_cb.dup_entry_info.name, name,
                sizeof(vcard_sqlite_cb.dup_entry_info.name));

            sprintf(vcard_sqlite_cb.stmt_buf,
                "SELECT _id, name FROM %s WHERE name='%s';",
                vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE], name);
            if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, btapp_vcard_dup_entry_cb)) {
                return VCARD_STORE_STATUS_FAIL;
            }
            if (vcard_sqlite_cb.dup_entry_info.found_dup_entry) {
                if (vcard_sqlite_cb.dup_entry_info.dup_action == VCARD_STORE_DUP_ACTION_ADD_NEW) {
                    // Go ahead and store -- Do nothing here
                } else if (vcard_sqlite_cb.dup_entry_info.dup_action == VCARD_STORE_DUP_ACTION_REPLACE) {
                    sprintf(vcard_sqlite_cb.stmt_buf, "DELETE FROM %s WHERE _id=%u;",
                        vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE],
                        (unsigned int)vcard_sqlite_cb.dup_entry_info.dup_id);
                    if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                        return VCARD_STORE_STATUS_FAIL;
                    }
                } else {
                    return VCARD_STORE_STATUS_DUP_NOT_STORED;
                }
            }
            sprintf(vcard_sqlite_cb.stmt_buf,
                "INSERT INTO %s (name, notes) VALUES ('%s','%s');",
                vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE],
                name, notes);
            if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                return VCARD_STORE_STATUS_FAIL;
            }
#ifdef SQLITE_ENABLED
            vcard_sqlite_cb.dup_entry_info.person_id = sqlite3_last_insert_rowid(db);
#endif
#if !defined(BTLA_REL_2_X) && !(BTLA_REL_2_X == TRUE)
            sprintf(vcard_sqlite_cb.stmt_buf, "INSERT INTO %s (person, group_id) VALUES (%d,%d);",
                vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_GROUPMEMBERSHIP],
                (unsigned int) vcard_sqlite_cb.dup_entry_info.person_id, 1 /* Contacts in groups table */);
            if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                return VCARD_STORE_STATUS_FAIL;
            }
#endif
            break;
        }

        case VCARD_CONTACT_TABLE_ID_PHONES:
        {
            char *number="", *label, *label1="'Custom'";
            char number_key[VCARD_VALUE_LEN];
            int type, isprimary;
            // person, type, number, number_key, label, isprimary

            for (i=0; i<vcard_sqlite_cb.num_prop; i++) {
                label="NULL";
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_TEL) {
                    number = (char *)vcard_sqlite_cb.prop_array[i].p_data;

                    memset(number_key, 0, VCARD_VALUE_LEN);
                    btapp_vcard_reverse_number_string(number, number_key);

                    if (PARAMETER(i, BTA_OP_TEL_PREF)) {
                        isprimary = 1;
                    } else {
                        isprimary = 0;
                    }

                    if (PARAMETER(i, BTA_OP_TEL_WORK) && PARAMETER(i, BTA_OP_TEL_FAX)) {
                        type = VCARD_PHONES_TYPE_FAX_WORK;
                    } else if (PARAMETER(i, BTA_OP_TEL_HOME) && PARAMETER(i, BTA_OP_TEL_FAX)) {
                        type = VCARD_PHONES_TYPE_FAX_HOME;
                    } else if (PARAMETER(i, BTA_OP_TEL_CELL)) {
                        type = VCARD_PHONES_TYPE_MOBILE;
                    } else if (PARAMETER(i, BTA_OP_TEL_HOME)) {
                        type = VCARD_PHONES_TYPE_HOME;
                    } else if (PARAMETER(i, BTA_OP_TEL_WORK)) {
                        type = VCARD_PHONES_TYPE_WORK;
                    } else if (PARAMETER(i, BTA_OP_TEL_PAGER)) {
                        type = VCARD_PHONES_TYPE_PAGER;
                    } else if (PARAMETER(i, BTA_OP_TEL_MSG)) {
                        type = VCARD_PHONES_TYPE_OTHER;
                    } else {
                        type = VCARD_PHONES_TYPE_CUSTOM;
                        label = label1;
                    }

                    sprintf(vcard_sqlite_cb.stmt_buf,
                        "INSERT INTO %s (person, type, number, number_key, label, isprimary) VALUES (%u, %d, '%s', '%s', %s, %d);",
                        vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PHONES],
                        (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id,
                        type, number, number_key, label, isprimary);
                    if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                        return VCARD_STORE_STATUS_FAIL;
                    }

                    if (isprimary == 1) {
                        sprintf(vcard_sqlite_cb.stmt_buf,
                            "UPDATE %s SET primary_phone = (SELECT _id FROM %s WHERE person = %u and isprimary = 1 LIMIT 1) WHERE _id = %u;",
                            vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE],
                            vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PHONES],
                            (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id,
                            (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
                    } else {
                        sprintf(vcard_sqlite_cb.stmt_buf,
                            "UPDATE %s SET primary_phone = (SELECT _id FROM %s WHERE person = %u LIMIT 1) WHERE _id = %u;",
                            vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE],
                            vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PHONES],
                            (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id,
                            (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
                    }
                    if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                        return VCARD_STORE_STATUS_FAIL;
                    }
                }
            }
            break;
        }

        case VCARD_CONTACT_TABLE_ID_CONTACT_METHODS:
        {
            char disp_adr[VCARD_STMT_BUF_SIZE], *p_adr[VCARD_MAX_ADR_FIELDS], *data="", *label, *label1="'Custom'", *space=" ";
            int kind, type, isprimary;
            // person, kind, data, aux_data, type, label, isprimary

            for (i=0; i<vcard_sqlite_cb.num_prop; i++) {
                label="NULL";
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_EMAIL) {
                    kind = VCARD_CONTACT_METHODS_KIND_EMAIL;
                    data = (char *)vcard_sqlite_cb.prop_array[i].p_data;

                    if (PARAMETER(i, BTA_OP_EMAIL_PREF)) {
                        isprimary = 1;
                    } else {
                        isprimary = 0;
                    }

                    if (PARAMETER(i, BTA_OP_EMAIL_INTERNET)) {
                        type = VCARD_CONTACT_METHODS_TYPE_HOME;
                    } else {
                        type = VCARD_CONTACT_METHODS_TYPE_CUSTOM;
                        label = label1;
                    }
                } else if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_ADR) {
                    kind = VCARD_CONTACT_METHODS_KIND_ADDRESS;
                    // Post Office Address;Extended Address;Street;Locality;Region;Postal Code;Country
                    p_adr[0] = (char *)vcard_sqlite_cb.prop_array[i].p_data;
                    for (j=1; j<VCARD_MAX_ADR_FIELDS; j++) {
                        p_adr[j] = NULL;
                        if ((p_adr[j] = strchr(p_adr[j-1], ';')) == NULL) break;
                        *(p_adr[j]++) = '\0';
                    }

                    *disp_adr = '\0';
                    for (j=0; j<VCARD_MAX_ADR_FIELDS; j++) {
                        if (p_adr[j] && strlen(p_adr[j])) {
                            strcat(disp_adr, p_adr[j]);
                            strcat(disp_adr, space);
                        }
                    }

                    while (disp_adr[strlen(disp_adr)-1] == *space) {
                        disp_adr[strlen(disp_adr)-1] = '\0';
                    }
                    data = ((disp_adr != NULL) ? disp_adr : data);

                    if (!(PARAMETER(i, BTA_OP_CHAR_UTF_8))) {
                        btapp_vcard_cstr_to_utf8((UINT8 *)data);
                    }

                    if (PARAMETER(i, BTA_OP_ADR_HOME)) {
                        type = VCARD_CONTACT_METHODS_TYPE_HOME;
                    } else if (PARAMETER(i, BTA_OP_ADR_WORK)) {
                        type = VCARD_CONTACT_METHODS_TYPE_WORK;
                    } else {
                        type = VCARD_CONTACT_METHODS_TYPE_CUSTOM;
                        label = label1;
                    }
                    isprimary = 0;
                } else {
                    continue;
                }

                sprintf(vcard_sqlite_cb.stmt_buf,
                    "INSERT INTO %s (person, kind, data, type, label, isprimary) VALUES (%u, %d, '%s', %d, %s, %d);",
                    vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_CONTACT_METHODS],
                    (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id,
                    kind, data, type, label, isprimary);
                if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                    return VCARD_STORE_STATUS_FAIL;
                }

                if (isprimary == 1) {
                    sprintf(vcard_sqlite_cb.stmt_buf,
                        "UPDATE %s SET primary_email = (SELECT _id FROM %s WHERE person = %u and isprimary = 1 LIMIT 1) WHERE _id = %u;",
                        vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE],
                        vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_CONTACT_METHODS],
                        (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id,
                        (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
                    if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                        return VCARD_STORE_STATUS_FAIL;
                    }
                }
            }
            break;
        }

        case VCARD_CONTACT_TABLE_ID_ORGANIZATIONS:
        {
            char *p, *company="", *title="", *label, *label1="'Custom'";
            int isprimary=0, type=VCARD_ORGANIZATIONS_TYPE_WORK;
            // company, title, isprimary, type, label, person

            for (i=0; i<vcard_sqlite_cb.num_prop; i++) {
                label="NULL";
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_ORG) {
                    p = company = (char *)vcard_sqlite_cb.prop_array[i].p_data;
                    while ((p = strchr(p, ';')) != NULL) {
                        *p = ' ';
                    }
                    if (!(PARAMETER(i, BTA_OP_CHAR_UTF_8))) {
                        btapp_vcard_cstr_to_utf8((UINT8 *)company);
                    }
                }
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_TITLE) {
                    title = (char *)vcard_sqlite_cb.prop_array[i].p_data;
                    if (!(PARAMETER(i, BTA_OP_CHAR_UTF_8))) {
                        btapp_vcard_cstr_to_utf8((UINT8 *)title);
                    }
                }
            }

            if (*company || *title) {
                sprintf(vcard_sqlite_cb.stmt_buf,
                    "INSERT INTO %s (company, title, isprimary, type, person, label) VALUES ('%s', '%s', %d, %d, %u, %s);",
                    vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_ORGANIZATIONS],
                    company, title, isprimary, type,
                    (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id, label);
                if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                    return VCARD_STORE_STATUS_FAIL;
                }
            }
            break;
        }

        case VCARD_CONTACT_TABLE_ID_PHOTOS:
        {
#ifdef SQLITE_ENABLED
            UINT8 b64_buf[VCARD_VALUE_LEN] = { 0 };
            sqlite3_stmt *stmt=NULL;
            int result, len;
            BOOLEAN found=FALSE;

            for (i=0; i<vcard_sqlite_cb.num_prop; i++) {
                if (vcard_sqlite_cb.prop_array[i].name == BTA_OP_VCARD_PHOTO) {
                    found = TRUE;

                    sprintf(vcard_sqlite_cb.stmt_buf, "INSERT INTO %s (data, person) VALUES (?, %u);",
                        vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PHOTOS],
                        (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
                    stmt = NULL;
                    if ((result = sqlite3_prepare_v2(db, vcard_sqlite_cb.stmt_buf,
                        -1, &stmt, NULL)) != SQLITE_OK) {
                        error("Error preparing statement (%d): '%s'", result,
                            sqlite3_errmsg(db));
                        btapp_vcard_close_db(db);
                        return VCARD_STORE_STATUS_FAIL;
                    }
                    len = btapp_vcard_b64_decode(vcard_sqlite_cb.prop_array[i].p_data, b64_buf);
                    if ((result = sqlite3_bind_blob(stmt, 1, b64_buf, len,
                        SQLITE_TRANSIENT)) != SQLITE_OK) {
                        error("Error binding blob (%d): '%s'", result,
                            sqlite3_errmsg(db));
                        btapp_vcard_close_db(db);
                        return VCARD_STORE_STATUS_FAIL;
                    }
                    if ((result = sqlite3_step(stmt)) != SQLITE_DONE) {
                        error("Error executing statement (%d): '%s'", result,
                            sqlite3_errmsg(db));
                        btapp_vcard_close_db(db);
                        return VCARD_STORE_STATUS_FAIL;
                    }
                    if ((result = sqlite3_finalize(stmt)) != SQLITE_OK) {
                        error("Error finalizing statement (%d): '%s'", result,
                            sqlite3_errmsg(db));
                        btapp_vcard_close_db(db);
                        return VCARD_STORE_STATUS_FAIL;
                    }
                    debug("Executed %s", vcard_sqlite_cb.stmt_buf);
                }
            }

            if (found == FALSE) {
                sprintf(vcard_sqlite_cb.stmt_buf, "INSERT INTO %s (person) VALUES (%u);",
                    vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PHOTOS],
                    (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
                if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, NULL)) {
                    return VCARD_STORE_STATUS_FAIL;
                }
            }
            break;
#endif
        }

        default:
            break;
    }

    return VCARD_STORE_STATUS_STORED;
}

/*******************************************************************************
**
** Function         btapp_vcard_fill_prop_array
**
** Description      Helper function to fill vCard properties array
**
**
** Returns          void
*******************************************************************************/
static BOOLEAN btapp_vcard_fill_prop_array (UINT16 name, UINT32 parameters, char *p_data)
{
    if (p_data == NULL) {
        return FALSE;
    }

    if ((vcard_sqlite_cb.num_prop+1) > VCARD_MAX_PROP) {
        error("%s: Number of properties exceeded %d", __FUNCTION__, VCARD_MAX_PROP);
        return FALSE;
    }

    vcard_sqlite_cb.prop_array[vcard_sqlite_cb.num_prop].name = name;
    vcard_sqlite_cb.prop_array[vcard_sqlite_cb.num_prop].parameters = parameters;
    strncpy(vcard_sqlite_cb.value[vcard_sqlite_cb.num_prop], p_data, VCARD_VALUE_LEN);
    vcard_sqlite_cb.prop_array[vcard_sqlite_cb.num_prop].p_data =
        (UINT8 *)vcard_sqlite_cb.value[vcard_sqlite_cb.num_prop];
    vcard_sqlite_cb.prop_array[vcard_sqlite_cb.num_prop].len =
        (UINT16)strlen(vcard_sqlite_cb.value[vcard_sqlite_cb.num_prop]);
    vcard_sqlite_cb.num_prop++;

    return TRUE;
}

static int btapp_vcard_build_N_field(char *src, char *buf, int buflen)
{
    /* Family Name;Given Name;Additional Names; Name Prefix;Name Suffix */
#define VCARD_MAX_N_FIELDS                  5

    char *ptr, *p_s, *p_e;
    char tokens[VCARD_MAX_N_FIELDS][VCARD_N_FIELD_TOKEN_LEN];
    int i, ntoken, len;

    if ((src == NULL) || (buf == NULL))
        return -1;

    memset(tokens, 0, sizeof(tokens));

    len = strlen(src);

    p_s = src;
    p_e = src + len;    /* '\0' */

    //APPL_TRACE_DEBUG2("btapp_vcard_build_N_field: \"%s\" , length: %d", src, len);

    for (i = 0, ntoken = 0; i < VCARD_MAX_N_FIELDS; i++)
    {
        if ((p_s >= p_e) || (*p_s == '\0'))
            break;

        ptr = tokens[i];

        while ((p_s < p_e) && (*p_s != ' ') && (*p_s != '\0'))
            *ptr++ = *p_s++;

        *ptr = '\0';                                            /* null termination */
        ntoken++;

        while ((p_s < p_e) && (*p_s == ' '))      /* skip space */
            p_s++;

        //APPL_TRACE_DEBUG2("btapp_vcard_build_N_field: token \"%s\" , length: %d", tokens[i], strlen(tokens[i]));
    }

    switch (ntoken)
    {
        case 1 :
            snprintf(buf, buflen, "%s", tokens[0]);
            break;
        case 2 :
            snprintf(buf, buflen, "%s;%s", tokens[1], tokens[0]);
            break;
        case 3 :
            snprintf(buf, buflen, "%s;%s;%s",tokens[2], tokens[0], tokens[1]);
            break;
        case 4 :
            snprintf(buf, buflen, "%s;%s;%s;%s",tokens[3], tokens[1], tokens[2], tokens[0]);
            break;
        case 5 :
            snprintf(buf, buflen, "%s;%s;%s;%s;%s",tokens[3], tokens[1], tokens[2], tokens[0], tokens[4]);
            break;
        default :
            snprintf(buf, buflen, "%s", tokens[0]);
            break;
    }

    APPL_TRACE_DEBUG1("btapp_vcard_build_N_field: N: \"%s\"", buf);
    return ntoken;
}

/*******************************************************************************
**
** Function         btapp_vcard_select_cb
**
** Description      Callback function for "Select" operation
**
**
** Returns          void
*******************************************************************************/
static int btapp_vcard_select_cb (void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i, type, kind, isprimary;
    UINT16 length;
    UINT32 parameters=0;
    char value[VCARD_VALUE_LEN];

    if (vcard_sqlite_cb.tbl_id >= VCARD_CONTACT_TABLE_ID_MAX) {
        error("%s: Unknown table id:%d", __FUNCTION__, vcard_sqlite_cb.tbl_id);
        return 0;
    }

    debug("Callback reading table id: %d", vcard_sqlite_cb.tbl_id);
    for (i=0; i<num_fields; i++) debug("[%d]:%s = %s", i+1, p_col_names[i],
        p_fields[i] ? p_fields[i] : "(null)");
    switch (vcard_sqlite_cb.tbl_id) {
        case VCARD_CONTACT_TABLE_ID_PEOPLE:
            vcard_sqlite_cb.dup_entry_info.person_id = atoi(p_fields[0]); // _id
            parameters |= (BTA_OP_CHAR_UTF_8);

            memset(value, 0, sizeof(value));
            /* special handling for N, need to add ; */
            if (strstr(p_fields[1], ";"))
                strncpy(value, p_fields[1], VCARD_VALUE_LEN);
            else
                btapp_vcard_build_N_field(p_fields[1], value, VCARD_VALUE_LEN);

            if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_N, parameters, value)) { // name
                return 0;
            }
            if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_FN, parameters, p_fields[1])) { // name
                return 0;
            }
            if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_NOTE, parameters, p_fields[2])) { // notes
                return 0;
            }
            break;

        case VCARD_CONTACT_TABLE_ID_PHONES:
            type = atoi(p_fields[0]); // type
            if (type == VCARD_PHONES_TYPE_HOME) {
                parameters |= (BTA_OP_TEL_HOME | BTA_OP_TEL_VOICE);
            } else if (type == VCARD_PHONES_TYPE_MOBILE) {
                parameters |= (BTA_OP_TEL_CELL | BTA_OP_TEL_VOICE);
            } else if (type == VCARD_PHONES_TYPE_WORK) {
                parameters |= (BTA_OP_TEL_WORK | BTA_OP_TEL_VOICE);
            } else if (type == VCARD_PHONES_TYPE_FAX_WORK) {
                parameters |= (BTA_OP_TEL_WORK | BTA_OP_TEL_FAX);
            } else if (type == VCARD_PHONES_TYPE_FAX_HOME) {
                parameters |= (BTA_OP_TEL_HOME | BTA_OP_TEL_FAX);
            } else if (type == VCARD_PHONES_TYPE_PAGER) {
                parameters |= (BTA_OP_TEL_PAGER);
            } else if (type == VCARD_PHONES_TYPE_OTHER) {
                parameters |= (BTA_OP_TEL_MSG);
            } else if (type == VCARD_PHONES_TYPE_CUSTOM) {
                parameters |= (0);
            } else {
                parameters |= (BTA_OP_TEL_CELL | BTA_OP_TEL_VOICE);
            }
            if (atoi(p_fields[1]) == 1) { // isprimary
                parameters |= BTA_OP_TEL_PREF;
            }
            if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_TEL, parameters, p_fields[2])) { // number
                return 0;
            }
            break;

        case VCARD_CONTACT_TABLE_ID_CONTACT_METHODS:
            kind = atoi(p_fields[0]); // kind
            type = atoi(p_fields[1]); // type
            isprimary = atoi(p_fields[2]); // isprimary
            if (kind == VCARD_CONTACT_METHODS_KIND_EMAIL) {
                if (type == VCARD_CONTACT_METHODS_TYPE_HOME) {
                    parameters |= (BTA_OP_EMAIL_INTERNET);
                } else if (type == VCARD_CONTACT_METHODS_TYPE_WORK) {
                    parameters |= (BTA_OP_EMAIL_INTERNET);
                } else if (type == VCARD_CONTACT_METHODS_TYPE_OTHER) {
                   parameters |= (BTA_OP_EMAIL_INTERNET);
                } else if (type == VCARD_CONTACT_METHODS_TYPE_CUSTOM) {
                   parameters |= (0);
                }
                if (isprimary == 1) {
                    parameters |= BTA_OP_EMAIL_PREF;
                }
                if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_EMAIL, parameters, p_fields[3])) {
                    return 0;
                }
            } else if (kind == VCARD_CONTACT_METHODS_KIND_ADDRESS) {
                if (type == VCARD_CONTACT_METHODS_TYPE_HOME) {
                    parameters |= (BTA_OP_ADR_HOME);
                } else if (type == VCARD_CONTACT_METHODS_TYPE_WORK) {
                    parameters |= (BTA_OP_ADR_WORK);
                } else if (type == VCARD_CONTACT_METHODS_TYPE_OTHER) {
                    parameters |= (0);
                } else if (type == VCARD_CONTACT_METHODS_TYPE_CUSTOM) {
                    parameters |= (0);
                }
                if (isprimary == 1) {
                    parameters |= 0;
                }
                parameters |= (BTA_OP_CHAR_UTF_8);
                if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_ADR, parameters, p_fields[3])) {
                    return 0;
                }
            }
            break;

        case VCARD_CONTACT_TABLE_ID_ORGANIZATIONS:
            type = atoi(p_fields[0]); // type
            isprimary = atoi(p_fields[1]); // isprimary
            parameters |= (BTA_OP_CHAR_UTF_8);
            if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_ORG, parameters, p_fields[2])) { // company
                return 0;
            }
            if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_TITLE, parameters, p_fields[3])) { // title
                return 0;
            }
            break;

        case VCARD_CONTACT_TABLE_ID_PHOTOS:
            if (p_fields[0]) {
                length = atoi(p_fields[0]); // length(data)

                if (length && (length < VCARD_VALUE_LEN)) {
                    UINT8 b64_buf[VCARD_VALUE_LEN] = { 0 };

                    btapp_vcard_b64_encode((UINT8 *)p_fields[1], b64_buf, length); // data
                    parameters = BTA_OP_ENC_BASE64 | BTA_OP_PHOTO_TYPE_JPEG;
                    if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_PHOTO,
                        parameters, (char *)b64_buf)) {
                        return 0;
                    }
                }
            }
            break;

        case VCARD_CONTACT_TABLE_ID_ANDROID_METADATA:
            strncpy(vcard_sqlite_cb.locale, p_fields[0],
                sizeof(vcard_sqlite_cb.locale)); // locale
            break;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
**
** Function         btapp_vcard_get_locale
**
** Description      Helper function to get Locale
**
**
** Returns          void
*******************************************************************************/
static BOOLEAN btapp_vcard_get_locale (sqlite3 *db)
{
    vcard_sqlite_cb.tbl_id = VCARD_CONTACT_TABLE_ID_ANDROID_METADATA;

    sprintf(vcard_sqlite_cb.stmt_buf, "SELECT locale FROM %s LIMIT 1;",
        vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_ANDROID_METADATA]);
    if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, btapp_vcard_select_cb)) {
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_vcard_write_contact_info
**
** Description      Writes into Contacts database
**
**
** Returns          void
*******************************************************************************/
static int btapp_vcard_write_contact_info (void)
{
    tVCARD_STORE_STATUS status=vcard_sqlite_cb.dup_entry_info.dup_action;
    sqlite3 *db = NULL;

#ifdef SQLITE_ENABLED
    debug("%s: Opening contact database - %s", __FUNCTION__,
        bte_appl_cfg.contacts_db);
    if (sqlite3_open(bte_appl_cfg.contacts_db, &db) != SQLITE_OK) {
        error("%s: Error opening database - %s [%s]",
            __FUNCTION__, bte_appl_cfg.contacts_db, sqlite3_errmsg(db));
        return VCARD_STORE_STATUS_FAIL;
    }
    if (db == NULL) {
        error("%s: Not enough memory to open database - %s [%s]",
            __FUNCTION__, bte_appl_cfg.contacts_db, sqlite3_errmsg(db));
        return VCARD_STORE_STATUS_FAIL;
    }

    if (register_android_functions(db, UTF16_STORAGE) != SQLITE_OK) {
        error("%s: Error registering android functions", __FUNCTION__);
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }

    if (!btapp_vcard_get_locale(db)) {
        error("%s: Error getting locale info", __FUNCTION__);
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }

    if (register_localized_collators(db, vcard_sqlite_cb.locale, UTF16_STORAGE) != SQLITE_OK) {
        error("%s: Error registering localized collators", __FUNCTION__);
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }

    vcard_sqlite_cb.dup_entry_info.found_dup_entry = FALSE;
    vcard_sqlite_cb.dup_entry_info.dup_id = 0;
    vcard_sqlite_cb.dup_entry_info.person_id = 0;
    *(vcard_sqlite_cb.dup_entry_info.name) = 0;

    status = btapp_vcard_write_contact_table(db, VCARD_CONTACT_TABLE_ID_PEOPLE);
    if ((status == VCARD_STORE_STATUS_FAIL) ||
        (status == VCARD_STORE_STATUS_DUP_NOT_STORED)) {
        btapp_vcard_close_db(db);
        return status;
    }
    if (!btapp_vcard_write_contact_table(db, VCARD_CONTACT_TABLE_ID_PHONES)) {
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }
    if (!btapp_vcard_write_contact_table(db, VCARD_CONTACT_TABLE_ID_CONTACT_METHODS)) {
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }
    if (!btapp_vcard_write_contact_table(db, VCARD_CONTACT_TABLE_ID_ORGANIZATIONS)) {
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }
    if (!btapp_vcard_write_contact_table(db, VCARD_CONTACT_TABLE_ID_PHOTOS)) {
        btapp_vcard_close_db(db);
        return VCARD_STORE_STATUS_FAIL;
    }

    btapp_vcard_close_db(db);

    if (vcard_sqlite_cb.dup_entry_info.dup_action == VCARD_STORE_DUP_ACTION_ADD_NEW) {
        return VCARD_STORE_STATUS_DUP_STORED_AS_NEW;
    } else if (vcard_sqlite_cb.dup_entry_info.dup_action == VCARD_STORE_DUP_ACTION_REPLACE) {
        return VCARD_STORE_STATUS_DUP_REPLACED;
    } else {
        return VCARD_STORE_STATUS_STORED;
    }
#endif
}

/*******************************************************************************
**
** Function         btapp_vcard_store_vcard
**
** Description      Stores the vCard in the database
**
**
** Returns          void
*******************************************************************************/
tDUP_ENTRY_INFO *btapp_vcard_store_vcard (char *p_path, tVCARD_STORE_DUP_ACTION dup_action)
{
    int fd = BTA_FS_INVALID_FD;
    size_t file_size;
    UINT8 i;

    if ((fd = open(p_path, O_RDONLY)) < 0) {
        error("Error opening %s (%d)", p_path, fd);
        vcard_sqlite_cb.dup_entry_info.status = VCARD_STORE_STATUS_FAIL;
        return &vcard_sqlite_cb.dup_entry_info;
    }

    if ((file_size = read(fd, vcard_sqlite_cb.op_mem, VCARD_MEM_LEN)) == 0) {
        error("Error reading %s (%d)", p_path, fd);
        vcard_sqlite_cb.dup_entry_info.status = VCARD_STORE_STATUS_FAIL;
        return &vcard_sqlite_cb.dup_entry_info;
    }

    close(fd);

    debug ("Parsing vCard file:%s, size:%d", p_path, file_size);
    vcard_sqlite_cb.num_prop = VCARD_MAX_PROP;
    vcard_sqlite_cb.dup_entry_info.status =
        BTA_OpParseCard(vcard_sqlite_cb.prop_array,
        &vcard_sqlite_cb.num_prop, vcard_sqlite_cb.op_mem, file_size);
    if (vcard_sqlite_cb.dup_entry_info.status != BTA_OP_OK) {
        error("BTA_OpParseCard Failed - Status: %d", vcard_sqlite_cb.dup_entry_info.status);
        vcard_sqlite_cb.dup_entry_info.status = VCARD_STORE_STATUS_FAIL;
        return &vcard_sqlite_cb.dup_entry_info;
    }

    debug("No. of properties: %d", vcard_sqlite_cb.num_prop);
    for (i=0; i<vcard_sqlite_cb.num_prop; i++) {
        vcard_sqlite_cb.prop_array[i].p_data[vcard_sqlite_cb.prop_array[i].len] = '\0';
        if (vcard_sqlite_cb.prop_array[i].p_param == NULL) {
            vcard_sqlite_cb.prop_array[i].param_len = 0;
        } else {
            vcard_sqlite_cb.prop_array[i].p_param[vcard_sqlite_cb.prop_array[i].param_len] = '\0';
        }
        debug("[%d]%s: %s (%d) - 0x%X {%s, (%d)}", i+1,
            bta_op_vcard_tbl[vcard_sqlite_cb.prop_array[i].name].p_name,
            vcard_sqlite_cb.prop_array[i].p_data, vcard_sqlite_cb.prop_array[i].len,
            (unsigned int)vcard_sqlite_cb.prop_array[i].parameters,
            (char *)(vcard_sqlite_cb.prop_array[i].p_param ? vcard_sqlite_cb.prop_array[i].p_param : "(null)"),
            vcard_sqlite_cb.prop_array[i].param_len);
        btapp_vcard_convert_encoding(i);
    }

    vcard_sqlite_cb.dup_entry_info.dup_action = dup_action;
    vcard_sqlite_cb.dup_entry_info.status = btapp_vcard_write_contact_info();
    return &vcard_sqlite_cb.dup_entry_info;
}

/*******************************************************************************
**
** Function         btapp_vcard_read_contact_table
**
** Description      Reads the Contacts table
**
**
** Returns          void
*******************************************************************************/
static BOOLEAN btapp_vcard_read_contact_table (sqlite3 *db, tVCARD_CONTACT_TABLE_ID id, UINT32 key)
{
    debug("Reading table id: %d", id);
    memset(vcard_sqlite_cb.stmt_buf, 0, sizeof(vcard_sqlite_cb.stmt_buf));
    vcard_sqlite_cb.tbl_id = id;
    switch (id) {
        case VCARD_CONTACT_TABLE_ID_PEOPLE:
            sprintf(vcard_sqlite_cb.stmt_buf, "SELECT _id, name, notes FROM %s WHERE _id=%u;",
                vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_PEOPLE],
                (unsigned int)key);
            break;

        case VCARD_CONTACT_TABLE_ID_PHONES:
            sprintf(vcard_sqlite_cb.stmt_buf, "SELECT type, isprimary, number FROM %s WHERE person=%u;",
                vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_PHONES],
                (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
            break;

        case VCARD_CONTACT_TABLE_ID_CONTACT_METHODS:
            sprintf(vcard_sqlite_cb.stmt_buf, "SELECT kind, type, isprimary, data FROM %s WHERE person=%u;",
                vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_CONTACT_METHODS],
                (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
            break;

        case VCARD_CONTACT_TABLE_ID_ORGANIZATIONS:
            sprintf(vcard_sqlite_cb.stmt_buf,
                "SELECT type, isprimary, company, title FROM %s WHERE person=%u;",
                vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_ORGANIZATIONS],
                (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
            break;

        case VCARD_CONTACT_TABLE_ID_PHOTOS:
            sprintf(vcard_sqlite_cb.stmt_buf, "SELECT length(data), data FROM %s WHERE person=%u;",
                vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_PHOTOS],
                (unsigned int)vcard_sqlite_cb.dup_entry_info.person_id);
            break;

        default:
            break;
    }

    if (!btapp_vcard_exec_stmt(db, vcard_sqlite_cb.stmt_buf, btapp_vcard_select_cb)) {
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_vcard_read_contact_info
**
** Description      Reads the Contacts database
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_read_contact_info (sqlite3 *db, UINT32 key)
{
#ifdef SQLITE_ENABLED
    sqlite3 *pdb=db;

    vcard_sqlite_cb.db = pdb;
    if (!pdb) {
        debug("%s: Opening contact database - %s for key=%u",
            __FUNCTION__, bte_appl_cfg.contacts_db, (unsigned int)key);
        if (sqlite3_open(bte_appl_cfg.contacts_db, &pdb) != SQLITE_OK) {
            error("%s: Error opening database - %s [%s]",
                __FUNCTION__, bte_appl_cfg.contacts_db, sqlite3_errmsg(pdb));
            return;
        }
        if (pdb == NULL) {
            error("%s: Not enough memory to open database - %s [%s]",
                __FUNCTION__, bte_appl_cfg.contacts_db, sqlite3_errmsg(pdb));
            return;
        }
    }

    if (*vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_PEOPLE]) {
        if (!btapp_vcard_read_contact_table(pdb, VCARD_CONTACT_TABLE_ID_PEOPLE, key)) {
            return;
        }
    }
    if (*vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_PHONES]) {
        if (!btapp_vcard_read_contact_table(pdb, VCARD_CONTACT_TABLE_ID_PHONES, 0)) {
            return;
        }
    }
    if (*vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_CONTACT_METHODS]) {
        if (!btapp_vcard_read_contact_table(pdb, VCARD_CONTACT_TABLE_ID_CONTACT_METHODS, 0)) {
            return;
        }
    }
    if (*vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_ORGANIZATIONS]) {
        if (!btapp_vcard_read_contact_table(pdb, VCARD_CONTACT_TABLE_ID_ORGANIZATIONS, 0)) {
            return;
        }
    }
    if (*vcard_tbl_set[vcard_sqlite_cb.vcard_set_index][VCARD_CONTACT_TABLE_ID_PHOTOS]) {
        if (!btapp_vcard_read_contact_table(pdb, VCARD_CONTACT_TABLE_ID_PHOTOS, 0)) {
            return;
        }
    }

    if (!vcard_sqlite_cb.db && pdb) {
        if (sqlite3_close(pdb) != SQLITE_OK) {
            error("%s: Error closing database - %s", __FUNCTION__,
                bte_appl_cfg.contacts_db);
            return;
        }
    }
#endif
}

/*******************************************************************************
**
** Function         btapp_vcard_read_contact_entry
**
** Description      Builds the vCard from the sqlite database in memory
**
**
** Returns          void
*******************************************************************************/
tBTA_OP_STATUS btapp_vcard_read_contact_entry (sqlite3 *db,
    tVCARD_SET_INDEX vcard_set_index, UINT32 key, UINT8 *p_card, UINT16 *p_len)
{
    tBTA_OP_STATUS status;

    vcard_sqlite_cb.vcard_set_index = vcard_set_index;
    memset(vcard_sqlite_cb.prop_array, 0, sizeof(tBTA_OP_PROP)*VCARD_MAX_PROP);
    vcard_sqlite_cb.num_prop = 0;

    btapp_vcard_read_contact_info(db, key);

    memset(p_card, 0, VCARD_MEM_LEN);
    *p_len = VCARD_MEM_LEN;
    status = BTA_OpBuildCard(p_card, p_len, BTA_OP_VCARD21_FMT,
        vcard_sqlite_cb.prop_array, vcard_sqlite_cb.num_prop);
    debug("Build Status:%d, Len:%d (%d properties)", status, *p_len,
        vcard_sqlite_cb.num_prop);

    return status;
}

/*******************************************************************************
**
** Function         btapp_vcard_parse_fdn_entry
**
** Description      parser for FDN entry. Format: obex://fdn/number;name
**
**
** Returns          void
*******************************************************************************/
static void btapp_vcard_parse_fdn_entry (char *contents, UINT8 *p_card, UINT16 *p_len)
{
    int name_len, number_len;
    char *number = contents;
    char *name = NULL;
    UINT32 parameters=0;
    tBTA_OP_STATUS status;

    memset(vcard_sqlite_cb.prop_array, 0, sizeof(tBTA_OP_PROP)*VCARD_MAX_PROP);
    vcard_sqlite_cb.num_prop = 0;

    if ((name = strchr(contents, ';')) == NULL) {
        return;
    }
    *name++ = '\0';

    if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_N, parameters, name)) { // name
        return;
    }
    if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_FN, parameters, name)) { // name
        return;
    }

    parameters = (BTA_OP_TEL_CELL | BTA_OP_TEL_HOME | BTA_OP_TEL_VOICE);
    if (!btapp_vcard_fill_prop_array(BTA_OP_VCARD_TEL, parameters, number)) { // number
        return;
    }

    memset(p_card, 0, VCARD_MEM_LEN);
    *p_len = VCARD_MEM_LEN;
    status = BTA_OpBuildCard(p_card, p_len, BTA_OP_VCARD21_FMT,
        vcard_sqlite_cb.prop_array, vcard_sqlite_cb.num_prop);
    debug("Build Status:%d, Len:%d (%d properties)", status, *p_len,
        vcard_sqlite_cb.num_prop);
}

/*******************************************************************************
**
** Function         btapp_vcard_build_vcard
**
** Description      Builds the vCard from the sqlite database
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_vcard_build_vcard (DTUN_PATH vcard_key, char *p_path)
{
    char *key = strrchr(vcard_key, '/'), *people_tbl;
    tVCARD_SET_INDEX vcard_set_index;
    int fd = BTA_FS_INVALID_FD;
    size_t file_size;
    UINT16 len=0;

    debug("Build vcard: %s", vcard_key);

    /* FDN format : "obex://fdn/number;name" */
    if (!strncmp(vcard_key, "obex://fdn/", 11)) {
        btapp_vcard_parse_fdn_entry((vcard_key+11), vcard_sqlite_cb.op_mem, &len);
    } else {
        if (key == NULL) {
            return FALSE;
        }

        *key = '\0';
        people_tbl = (strrchr(vcard_key, '/') + 1);
        if (!strcmp(people_tbl,
            vcard_tbl_set[VCARD_SET_INDEX_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE])) {
            vcard_set_index = VCARD_SET_INDEX_PEOPLE;
        } else if (!strcmp(people_tbl,
            vcard_tbl_set[VCARD_SET_INDEX_SIM_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE])) {
            vcard_set_index = VCARD_SET_INDEX_SIM_PEOPLE;
        } else if (!strcmp(people_tbl,
            vcard_tbl_set[VCARD_SET_INDEX_MYPROFILE_PEOPLE][VCARD_CONTACT_TABLE_ID_PEOPLE])) {
            vcard_set_index = VCARD_SET_INDEX_MYPROFILE_PEOPLE;
        } else {
            error("Bad 'people' table specified - %s", people_tbl);
            return FALSE;
        }

        if (btapp_vcard_read_contact_entry(NULL, vcard_set_index, atol(key+1),
            vcard_sqlite_cb.op_mem, &len) != BTA_OP_OK) {
            return FALSE;
        }
    }

    if ((fd = open(p_path, (O_WRONLY | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP|  S_IROTH | S_IWOTH) )) < 0) {
        error("Error creating %s (%d)", p_path, fd);
        return FALSE;
    }

    if ((file_size = write(fd, vcard_sqlite_cb.op_mem, len)) == 0) {
        error("Error writing %s (%d)", p_path, fd);
        close(fd);
        return FALSE;
    }

    close(fd);
    debug("Built vCard file:%s, size:%d", p_path, file_size);
    return TRUE;
}

#endif
