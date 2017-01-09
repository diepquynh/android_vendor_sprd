/*****************************************************************************
**
**  Name:           bta_db_co.c
**
**  Description:    This file contains the helper function implementation for
**                  database access
**
**  Copyright (c) 2002-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bt_target.h"
#include "utils/Log.h"
#include "bta_api.h"

#ifndef LINUX_NATIVE
#define SQLITE_ENABLED
#endif

#ifdef SQLITE_ENABLED
#include "sqlite3.h"
#else
#error "SQLITE_ENABLED must be defined"
#endif

#undef  LOG_TAG
#define LOG_TAG "DB_CO"

#define DB_CO_DBG 0
#define info(format, ...) LOGI (format, ## __VA_ARGS__)
#define debug(format, ...) if (DB_CO_DBG) LOGD (format, ## __VA_ARGS__)
#define error(format, ...) LOGE (format, ## __VA_ARGS__)

#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define STMT_LENGTH 256
#define DATA_LENGTH 64

enum {
    BTA_PBS_CO_ID_INVALID,
    BTA_PBS_CO_ID_N,
    BTA_PBS_CO_ID_ADDR,
};
typedef UINT8 tBTA_PBS_CO_ID;

enum {
    BTA_PBS_CO_N_DATA_DISPLAY_NAME,
    BTA_PBS_CO_N_DATA_PREFIX,
    BTA_PBS_CO_N_DATA_FIRST_NAME,
    BTA_PBS_CO_N_DATA_MIDDLE_NAME,
    BTA_PBS_CO_N_DATA_LAST_NAME,
    BTA_PBS_CO_N_DATA_SUFFIX,
    BTA_PBS_CO_N_DATA_MAX,
};

enum {
    BTA_PBS_CO_ADDR_DATA_PO_ADDR,
    BTA_PBS_CO_ADDR_DATA_STREET,
    BTA_PBS_CO_ADDR_DATA_LOCALITY,
    BTA_PBS_CO_ADDR_DATA_REGION,
    BTA_PBS_CO_ADDR_DATA_POSTAL_CODE,
    BTA_PBS_CO_ADDR_DATA_COUNTRY,
    BTA_PBS_CO_ADDR_DATA_MAX,
};

#define MAX_FIELDS (MAX(BTA_PBS_CO_N_DATA_MAX, BTA_PBS_CO_ADDR_DATA_MAX))

typedef struct {
    tBTA_PBS_CO_ID id;
    char           field[MAX_FIELDS][DATA_LENGTH];
} tBTA_PBS_CO_DATA;

int bta_pbs_sqlite3_exec(sqlite3 *db, const char *stmt, sqlite3_callback *xcb,
    void *pArg, char **errmsg);

static int select_data_cb (void *p_data, int num_fields,
    char **p_fields, char **p_col_names)
{
    tBTA_PBS_CO_DATA *co_data = (tBTA_PBS_CO_DATA *)p_data;
    int i, co_num_fields=0;

    if (co_data->id == BTA_PBS_CO_ID_N) {
        co_num_fields = BTA_PBS_CO_N_DATA_MAX;
    } else if (co_data->id == BTA_PBS_CO_ID_ADDR) {
        co_num_fields = BTA_PBS_CO_ADDR_DATA_MAX;
    } else {
        error("%s: Unknown select id %d", __FUNCTION__, co_data->id);
        return (-1);
    }

    if (num_fields != co_num_fields) {
        error("%s: Num of fields do not match for id %d",
            __FUNCTION__, co_data->id);
        return (-1);
    }

    for (i=0; i<num_fields; i++) {
        debug("%s[%d]:%s = %s", __FUNCTION__, i+1, p_col_names[i],
            p_fields[i] ? p_fields[i] : "(null)");

        if (p_fields[i]) {
            memcpy(co_data->field[i], p_fields[i], DATA_LENGTH);
        } else {
            co_data->field[i][0] = 0;
        }
    }
    return SQLITE_OK;
}

void bta_pbs_get_n_data (sqlite3 *db, char *n_data, char *fn_data,
    UINT32 data_size, UINT16 person)
{
    tBTA_PBS_CO_DATA co_data;
    char stmt[STMT_LENGTH], *errmsg;

    memset(&co_data, 0, sizeof(co_data));
    co_data.id = BTA_PBS_CO_ID_N;

    // display_name, prefix, first_name, middle_name, last_name, suffix
    snprintf(stmt, sizeof(stmt),
        "SELECT data1, data4, data2, data5, data3, data6 FROM data \
         WHERE mimetype_id = (SELECT _id FROM mimetypes WHERE mimetype='%s') \
         AND (raw_contact_id = %d)",
        "vnd.android.cursor.item/name", person);

    debug("%s: Execute - %s", __FUNCTION__, stmt);

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_data_cb, &co_data, &errmsg) != SQLITE_OK)
    {
        error("%s: select statement %s with error %s",
            __FUNCTION__, stmt, errmsg);
        *n_data = *fn_data = 0;
        return;
    }
#endif

    // FamilyName;GivenName;AdditionalName;Prefix;Suffix
    snprintf(n_data, data_size, "%s;%s;%s;%s;%s",
        co_data.field[BTA_PBS_CO_N_DATA_LAST_NAME],
        co_data.field[BTA_PBS_CO_N_DATA_FIRST_NAME],
        co_data.field[BTA_PBS_CO_N_DATA_MIDDLE_NAME],
        co_data.field[BTA_PBS_CO_N_DATA_PREFIX],
        co_data.field[BTA_PBS_CO_N_DATA_SUFFIX]);

    snprintf(fn_data, data_size, "%s",
        co_data.field[BTA_PBS_CO_N_DATA_DISPLAY_NAME]);
}

void bta_pbs_get_addr_data (sqlite3 *db, char *data, UINT32 data_size,
    UINT16 person, UINT8 type)
{
    tBTA_PBS_CO_DATA co_data;
    char stmt[STMT_LENGTH], *errmsg;

    memset(&co_data, 0, sizeof(co_data));
    co_data.id = BTA_PBS_CO_ID_ADDR;

    // pobox, street, locality (city), region (state), postcode, country
    snprintf(stmt, sizeof(stmt),
        "SELECT data5, data4, data7, data8, data9, data10 FROM data \
         WHERE mimetype_id = (SELECT _id FROM mimetypes WHERE mimetype='%s') \
         AND (raw_contact_id = %d) \
         AND (data2 = %d)",
        "vnd.android.cursor.item/postal-address_v2", person, type);

    debug("%s: Execute - %s", __FUNCTION__, stmt);

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_data_cb, &co_data, &errmsg) != SQLITE_OK)
    {
        error("%s: select statement %s with error %s",
            __FUNCTION__, stmt, errmsg);
        *data = 0;
        return;
    }
#endif

    // PostOfficeAddress;ExtendedAddress;Street;Locality;Region;PostalCode;Country
    snprintf(data, data_size, "%s;;%s;%s;%s;%s;%s",
        co_data.field[BTA_PBS_CO_ADDR_DATA_PO_ADDR],
        co_data.field[BTA_PBS_CO_ADDR_DATA_STREET],
        co_data.field[BTA_PBS_CO_ADDR_DATA_LOCALITY],
        co_data.field[BTA_PBS_CO_ADDR_DATA_REGION],
        co_data.field[BTA_PBS_CO_ADDR_DATA_POSTAL_CODE],
        co_data.field[BTA_PBS_CO_ADDR_DATA_COUNTRY]);
}
