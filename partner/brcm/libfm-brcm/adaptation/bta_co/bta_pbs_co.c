/*****************************************************************************
**
**  Name:           bta_pbs_co.c
**
**  Description:    This file contains the phone book access server call-out
**                  function implementation for Insight.
**
**  Copyright (c) 2003-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gki.h"
#include "bta_pbs_api.h"
#include "bta_pbs_co.h"
#include "bta_fs_co.h"
#include "bta_pbs_ci.h"
#include "btui.h"
#include "btapp_pbs.h"
#include "bta_op_api.h"
#include "bta_op_fmt.h"
#include "bte_appl.h"

#define LOG_TAG "PBS_CO"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

#ifndef LINUX_NATIVE
#define SQLITE_ENABLED
#endif

#ifdef SQLITE_ENABLED
#include "sqlite3.h"
#else
#error "SQLITE_ENABLED must be defined"
#endif

#ifndef BTA_PBS_CO_PHOTO_SUPPORTED
#define BTA_PBS_CO_PHOTO_SUPPORTED        TRUE
#endif

/**
 * Maximum SQL stmt length
 */
#define STMT_LENGTH 256

#define USE_BCMBTUI FALSE

#define BTUI_PBS_NUM_PROP           25

/* The CHARSET to use for N and FN field. Set to 0 to use default ASCII */
#define BTUI_PBS_CHARSET   BTA_OP_CHAR_UTF_8  //UTF-8

/* Max length of property in Vcard */
#define BTUI_PBS_PROP_SIZE           64

/* Max Len for individual Vcard */
#define BTUI_PBS_DEFAULT_CARD_MEM_LEN    8192

/* Extra memory over the vcard/phonebook file size
   needed to accomodate extra header for Vcard30 fmt*/
#define VCARD30_EXTRA_HDR_MEM       100

/* Default line length for base64 encoding. Min is 4. */
#define BTUI_PBS_DEFAULT_BASE64_LINE_LEN    76
#define BTUI_PBS_MAX_BASE64_MEM_LEN         (7*1024)

typedef struct
{
    int  count;
    UINT8 path;    /* main path of the operation */
} tBTUI_PBS_ENTRY_COUNT;

typedef struct
{
    char value[10][BTUI_PBS_PROP_SIZE];
    char p_name[BTUI_PBS_PROP_SIZE];
} tBTUI_PBS_PROP_PB;

typedef struct
{
    char value[BTUI_PBS_PROP_SIZE];
} tBTUI_PBS_NAME_NUM;

typedef struct
{
    int  num;
    char value[15][12][BTUI_PBS_PROP_SIZE];
} tBTUI_PBS_PROP_OTHER;

typedef struct
{
    int  num;
    int  col[BTUI_PBS_PROP_SIZE]; /* value used as index */
    char value[4][2][BTUI_PBS_PROP_SIZE];
} tBTUI_PBS_PROP_QUERY;

#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
typedef struct
{
    int   num;
    char  value[1][1][BTUI_PBS_PROP_SIZE];
    UINT8 src[BTUI_PBS_MAX_BASE64_MEM_LEN];
    UINT8 dst[BTUI_PBS_MAX_BASE64_MEM_LEN];
    int   srclen, dstlen;
} tBTUI_PBS_PROP_BLOB;
#endif

//row field now dynamically allocated
typedef struct
{
    UINT16   num;     /* number of database entries */
    UINT8    path;    /* main path of the operation */
    UINT16   *row;     /* the row entries. This is dynamically created */
    UINT16   max_count; /* number of database entries */
} tBTUI_PBS_RAW_PB;

#define BTUI_PBS_SQL_OPEN        1
#define BTUI_PBS_SQL_READ        2
#define BTUI_PBS_SQL_CLOSE       3
#define BTUI_PBS_SQL_GET_VLIST   4

typedef struct
{
    UINT8 event;
    char *p_path;
    tBTA_PBS_OPER operation;
    tBTA_PBS_PULLPB_APP_PARAMS *p_app_params;
} tBTUI_PBS_OPEN;

typedef struct
{
    UINT8 event;
    int fd;
    tBTA_PBS_OPER operation;
    UINT8 *p_buf;
    UINT16 nbytes;
} tBTUI_PBS_READ;

typedef struct
{
    UINT8 event;
    char *p_path;
    tBTA_PBS_VCARDLIST_APP_PARAMS *p_app_params;
    BOOLEAN first_item;
    tBTA_PBS_VCARDLIST *p_entry;
} tBTUI_PBS_GET_VLIST;

typedef struct
{
    UINT8 event;
    int fd;
} tBTUI_PBS_CLOSE;

typedef union
{
    UINT8 event;
    tBTUI_PBS_OPEN open;
    tBTUI_PBS_READ read;
    tBTUI_PBS_READ close;
    tBTUI_PBS_GET_VLIST getvlist;
} tBTUI_PBS_SQL_MSG;

static UINT8   *btui_bps_default_card_mem = NULL;
static BOOLEAN   btui_bps_is_SIM = FALSE;
static int bps_card_size = 0;
static int bps_card_read_offset =0;
/* pointer for parsing PB */
static int max_count;
/* format of vCard requested by Client */
static tBTA_OP_FMT fmt;

#ifdef SQLITE_ENABLED
/* android database */
static sqlite3* db = NULL;
#endif

static int bps_db_row_index = 0;
static int *vcard_list = NULL;

static tBTUI_PBS_RAW_PB  bps_db;
static tBTUI_PBS_PROP_PB *people_entry;
static tBTUI_PBS_PROP_PB *call_entry;
static tBTUI_PBS_PROP_OTHER *phone_entry;
static tBTUI_PBS_PROP_OTHER *contact_entry;
static tBTUI_PBS_PROP_OTHER *org_entry;
#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
static tBTUI_PBS_PROP_BLOB  *photo_entry;
#endif
static tBTUI_PBS_NAME_NUM *vlist_entry;

static const char bps_vcard_end[] = "END:VCARD\r\n";
static const char bps_vcard_begin[] = "BEGIN:VCARD\r\n";

static const char default_0_vcard_name[] = "Me";

static const char *op_str[] =
{
    "BTA_PBS_OPER_NONE",             // BTA_PBS_OPER_NONE
    "BTA_PBS_OPER_PULL_PB",          // BTA_PBS_OPER_PULL_PB
    "BTA_PBS_OPER_SET_PB",           // BTA_PBS_OPER_SET_PB
    "BTA_PBS_OPER_PULL_VCARD_LIST",  // BTA_PBS_OPER_PULL_VCARD_LIST
    "BTA_PBS_OPER_PULL_VCARD_ENTRY"  // BTA_PBS_OPER_PULL_VCARD_ENTRY
};

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

void bta_pbs_get_n_data (sqlite3 *db, char *n_data, char *fn_data,
    UINT32 data_size, UINT16 person);
void bta_pbs_get_addr_data (sqlite3 *db, char *data, UINT32 data_size,
    UINT16 person, UINT8 type);

/* forward declaration for base64 encode/decode functions */
static int bta_pbs_base64_encode(void *inbuf, void *outbuf, int datalen, int linesize);
static int bta_pbs_base64_decode(void *inbuf, void *outbuf, int inlen);

/*****************************************************************************
**  Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_pbs_sqlite3_exec
**
** Description      Execute a sqlite3 database query, and re-attempt if it's
**                  locked by other android app.
**
**
** Returns          The query succeed or not
**
*******************************************************************************/
int bta_pbs_sqlite3_exec(sqlite3 *db, const char *stmt, sqlite3_callback *xcb, void *pArg, char **errmsg)
{
    int count = 0;
    int err = SQLITE_OK;
    while (count++ < 5)
    {
        err = sqlite3_exec(db, stmt, xcb, pArg, errmsg);
        if (err == SQLITE_OK)
        {
            return SQLITE_OK;
        }
        else if (err == SQLITE_BUSY)
        {
            APPL_TRACE_DEBUG3("sqlite3_exec %s with db lock error %s at %dth try",
                              stmt, *errmsg, count);
            usleep(100000);
            continue;
        }
        else
        {
            APPL_TRACE_ERROR3("sqlite3_exec %s with err %d: %s", stmt, err, *errmsg);
            return err;
        }
    }
    return err;
}

/*******************************************************************************
**
** Function         bta_pbs_app_strcmp
**
** Description      Scan for a matching string
**
**
** Returns          Pointer to end of match or NULL if end of data reached.
**
*******************************************************************************/
static UINT8 *bta_pbs_app_strcmp(UINT8 *p, UINT8 *p_end, const char *p_str)
{
    int len = strlen(p_str);

    while (p != p_end)
    {
        /* check for match : TBD need to see if it compares case or not */
        //if (strnicmp((char *) p, p_str, len) == 0)
        if (strncmp((char *) p, p_str, len) == 0)
        {
            break;
        }
        else
        {
            p++;
        }
    }

    if (p == p_end)
        return NULL;
    else
        return p;
}

/*******************************************************************************
**
** Function         bta_pbs_is_valid_path
**
** Description      This function is used to check if the path is a valid path
**                  and decode the path if it's right.
**
** Parameters       p_path:     path at string format in the virtual folder
**                  d_path:     decoded path after analyzing the string format
**                              path p_path
**
** Returns          validity:   TRUE if the path is valid
**                              else FALSE
**
**
**
*******************************************************************************/
/* not differentiate Phone or sim card now */
#define BTA_PBS_PATH_PHONE_PB_STR    "pb"
#define BTA_PBS_PATH_PHONE_ICH_STR   "ich"
#define BTA_PBS_PATH_PHONE_OCH_STR   "och"
#define BTA_PBS_PATH_PHONE_MCH_STR   "mch"
#define BTA_PBS_PATH_PHONE_CCH_STR   "cch"
#define BTA_PBS_PATH_VCF_STR         ".vcf"
#define BTA_PBS_PATH_SIM_STR         "SIM1"

static BOOLEAN bta_pbs_is_valid_path(char *p_path, tBTA_PBS_OPER operation, tBTUI_PBS_PATH *d_path)
{
    BOOLEAN result = TRUE;

    memset(d_path, 0, sizeof(tBTUI_PBS_PATH));

    /* check out *.vcf string */
    if (operation != BTA_PBS_OPER_PULL_VCARD_LIST)
    {
        if (strstr((char *) p_path, BTA_PBS_PATH_VCF_STR) == NULL)
        {
            return FALSE;
        }
    }

    if (strstr((char *) p_path, BTA_PBS_PATH_PHONE_PB_STR) != NULL)
    {
        d_path->path = BTA_PBS_PATH_PHONE_PB;
    }
    else if (strstr((char *) p_path, BTA_PBS_PATH_PHONE_ICH_STR) != NULL)
    {
        d_path->path = BTA_PBS_PATH_PHONE_ICH;
    }
    else if (strstr((char *) p_path, BTA_PBS_PATH_PHONE_OCH_STR) != NULL)
    {
        d_path->path = BTA_PBS_PATH_PHONE_OCH;
    }
    else if (strstr((char *) p_path, BTA_PBS_PATH_PHONE_MCH_STR) != NULL)
    {
        d_path->path = BTA_PBS_PATH_PHONE_MCH;
    }
    else if (strstr((char *) p_path, BTA_PBS_PATH_PHONE_CCH_STR) != NULL)
    {
        d_path->path = BTA_PBS_PATH_PHONE_CCH;
    }
    else
    {
        return FALSE;
    }

    if (operation == BTA_PBS_OPER_PULL_VCARD_ENTRY)
    {
        sscanf(p_path, "%*[^0-9]%d.vcf", &(d_path->index));
    }
    else
    {
        d_path->index = -1;
    }

    return TRUE;
}

static BOOLEAN bta_pbs_is_for_SIM(char *p_path)
{
    if (strstr((char *)p_path, BTA_PBS_PATH_SIM_STR) != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static int bta_pbs_build_N_field(char *src, char *buf, int buflen)
{
    /* Family Name;Given Name;Additional Names; Name Prefix;Name Suffix */
#define VCARD_MAX_N_FIELDS                  5

    char *ptr, *p_s, *p_e;
    char tokens[VCARD_MAX_N_FIELDS][BTUI_PBS_PROP_SIZE];
    int i, ntoken, len;

    if ((src == NULL) || (buf == NULL))
        return -1;

    memset(tokens, 0, sizeof(tokens));

    len = strlen(src);

    p_s = src;
    p_e = src + len;    /* '\0' */

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2("bta_pbs_build_N_field: \"%s\" , length: %d", src, len);
#endif

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

#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG2("bta_pbs_build_N_field: token \"%s\" , length: %d", tokens[i], strlen(tokens[i]));
#endif
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

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG1("bta_pbs_build_N_field: N: \"%s\"", buf);
#endif
    return ntoken;
}

static int count_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    UINT16 *p_rn = (UINT16 *)p_data;
    *p_rn = atoi(p_fields[0]);

    return 0;
}

static int index_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    tBTUI_PBS_RAW_PB *p_rn = (tBTUI_PBS_RAW_PB *)p_data;

    /* record the major index */
    p_rn->row[p_rn->num++] = atoi(p_fields[0]);

    return 0;
}

static int count_vlist_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    tBTUI_PBS_RAW_PB *p_rn = (tBTUI_PBS_RAW_PB *)p_data;

    /* filter out useless entried */
    if (p_rn->path != BTA_PBS_PATH_PHONE_PB)
    {
        int path = atoi(p_fields[2]);
        if (((p_rn->path == BTA_PBS_PATH_PHONE_ICH) && (path != 1)) ||
            ((p_rn->path == BTA_PBS_PATH_PHONE_OCH) && (path != 2)) ||
            ((p_rn->path == BTA_PBS_PATH_PHONE_MCH) && (path != 3)))
        {
            return 0;
        }
    }

    /* the 1st col is user name */
    if (p_fields[0])
    {
        /* special handling for N, need to add ; */
        if (strstr(p_fields[0], ";"))
            strncpy(vlist_entry[max_count].value, p_fields[0], BTUI_PBS_PROP_SIZE);
        else
            bta_pbs_build_N_field(p_fields[0], vlist_entry[max_count].value, BTUI_PBS_PROP_SIZE);
    }
    else
    {
        /* call log without user name */
        if (p_rn->path != BTA_PBS_PATH_PHONE_PB)
        {
            strncpy(vlist_entry[max_count].value, p_fields[1], BTUI_PBS_PROP_SIZE);
        }
        else
        {
            vlist_entry[max_count].value[0] = '\0';
        }
    }
    max_count++;

    return 0;
}

static int select_call_cb(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i;
    tBTUI_PBS_PROP_PB *p_db = (tBTUI_PBS_PROP_PB *)p_data;

    for (i=0; i < num_fields; i++)
    {
        if (p_fields[i] != NULL)
            strncpy(p_db->value[i], p_fields[i], BTUI_PBS_PROP_SIZE);
        else
            p_db->value[i][0] = '\0';
    }

    /* the 2nd col is user name */
    if (p_fields[1])
    {
        /* special handling for N, need to add ; */
        if (strstr(p_db->value[1], ";"))
            strncpy(p_db->p_name, p_db->value[1], BTUI_PBS_PROP_SIZE);
        else
            bta_pbs_build_N_field(p_db->value[1], p_db->p_name, BTUI_PBS_PROP_SIZE);
    }
    else
    {
        /* call log without user name */
        p_db->p_name[0] = '\0';
    }

    return 0;
}

static int select_people_cb(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i;
    tBTUI_PBS_PROP_PB *p_db = (tBTUI_PBS_PROP_PB *)p_data;

    for (i=0; i < num_fields; i++)
    {
        if (p_fields[i] != NULL)
        {
            strncpy(p_db->value[i], p_fields[i], BTUI_PBS_PROP_SIZE);
        }
        else
            p_db->value[i][0] = '\0';
    }

    /* the 1st col is user name */
    if (p_fields[0])
    {
        /* special handling for N, need to add ; */
        if (strstr(p_db->value[0], ";"))
            strncpy(p_db->p_name, p_db->value[0], BTUI_PBS_PROP_SIZE);
        else
            bta_pbs_build_N_field(p_db->value[0], p_db->p_name, BTUI_PBS_PROP_SIZE);
    }
    else
    {
        /* call log without user name */
        p_db->p_name[0] = '\0';
    }

    return 0;
}

static int select_callback2(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i;
    tBTUI_PBS_PROP_OTHER *p_db = (tBTUI_PBS_PROP_OTHER *)p_data;

    for (i=0; i < num_fields; i++)
    {
        if (p_fields[i] != NULL)
            strncpy(p_db->value[p_db->num][i], p_fields[i], BTUI_PBS_PROP_SIZE);
        else
            p_db->value[p_db->num][i][0] = '\0';
    }

    p_db->num++;
    return 0;
}

static int select_callback3(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i;
    tBTUI_PBS_PROP_QUERY *p_db = (tBTUI_PBS_PROP_QUERY *)p_data;

    /* assume the first colume is the value compared */
    if (p_fields[0] != NULL)
    {
        if (!strcmp(p_fields[0], p_db->col))
        {
            for (i=1; i < num_fields; i++)
            {
                if (p_fields[i] != NULL)
                    strncpy(p_db->value[p_db->num][i-1], p_fields[i], BTUI_PBS_PROP_SIZE);
                else
                    p_db->value[p_db->num][i-1][0] = '\0';
            }

            p_db->num++;
        }
    }
    return 0;
}

#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
static int select_callback4(void *p_data, int num_fields, char **p_fields, char **p_col_names)
{
    int i=0, dstlen;
    tBTUI_PBS_PROP_BLOB *p_db = (tBTUI_PBS_PROP_BLOB *)p_data;

    if (p_fields[0] != NULL)    /* get the length of data */
    {
        p_db->srclen = atoi(p_fields[0]);

        dstlen = ((p_db->srclen + 2) / 3) * 4;
        dstlen += ((dstlen + BTUI_PBS_DEFAULT_BASE64_LINE_LEN -1) / BTUI_PBS_DEFAULT_BASE64_LINE_LEN) * 2;

        if (dstlen < BTUI_PBS_MAX_BASE64_MEM_LEN)
        {
            memcpy(p_db->src, p_fields[1], p_db->srclen);
            p_db->dstlen = dstlen;
        }
        else
        {
            p_db->srclen = -2;
            p_db->dstlen = -2;
            return 0;
        }
    }
    else
    {
        p_db->value[p_db->num][0][0] = '\0';
        p_db->srclen = -1;
        p_db->dstlen = -1;
    }

    p_db->num++;
    return 0;
}
#endif

static int bta_pbs_app_parse_contact_entry(tBTA_OP_PROP *p_prop,
                                      UINT8 *p_num_prop, int index)
{
    char *errmsg;
    char stmt[STMT_LENGTH];
    int i;
    int max_prop = *p_num_prop ;

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG1("bta_pbs_app_parse_contact_entry: bps parse pb index %d ",index);
#endif

    if (bps_db.num < index)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_contact_entry: index:%d more than table size:%d", index, bps_db.num);
        return FALSE;
    }

    if (bps_db.row == NULL)
    {
      APPL_TRACE_ERROR1("bta_pbs_app_parse_contact_entry: no row data",1);
      return FALSE;
    }

    memset(stmt, 0, sizeof(stmt));
    memset(people_entry, 0, sizeof(tBTUI_PBS_PROP_PB));

    /* TBD index=0 for zero vcard ??? */
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select name, notes from view_v1_people where _id=%d", bps_db.row[index-1]);
#else
    snprintf(stmt, sizeof(stmt),
        "select name, notes from people where _id=%d", bps_db.row[index-1]);
#endif
#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG1(" bps parse person: %s", stmt);
#endif
#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_people_cb, people_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR1("bta_pbs_app_parse_contact_entry: Error in select statement %s ", errmsg);
        return FALSE;
    }
#endif

    *p_num_prop = 0;

    /* step 1: compose attributes from table People */
    /* property: N */
    p_prop[*p_num_prop].name = BTA_OP_VCARD_N;
    p_prop[*p_num_prop].parameters = BTUI_PBS_CHARSET;
    if (index == 1 && strlen(people_entry->p_name) == 0)
    {
        p_prop[*p_num_prop].p_data = default_0_vcard_name;
        p_prop[*p_num_prop].len = strlen(default_0_vcard_name);
    }
    else
    {
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
        bta_pbs_get_n_data(db, people_entry->p_name, people_entry->value[0],
            BTUI_PBS_PROP_SIZE, bps_db.row[index-1]);
#endif
        p_prop[*p_num_prop].p_data = people_entry->p_name;
        p_prop[*p_num_prop].len = strlen(people_entry->p_name);
    }

    /* property: FN */
    (*p_num_prop)++;
    p_prop[*p_num_prop].name = BTA_OP_VCARD_FN;
    p_prop[*p_num_prop].parameters = BTUI_PBS_CHARSET;
    p_prop[*p_num_prop].p_data = people_entry->value[0];
    p_prop[*p_num_prop].len = strlen(people_entry->value[0]);

    /* property: NOTE */
    if (strlen(people_entry->value[1]))
    {
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_NOTE;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = people_entry->value[1];
        p_prop[*p_num_prop].len = strlen(people_entry->value[1]);
    }

    /* step 2: compose attributes from table Phones */
    memset(phone_entry, 0, sizeof(tBTUI_PBS_PROP_OTHER));
    memset(stmt, 0, sizeof(stmt));
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select type, number from view_v1_phones where person=%d", bps_db.row[index-1]);
#else
    snprintf(stmt, sizeof(stmt),
        "select type, number from phones where person=%d", bps_db.row[index-1]);
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2(" bps parse pb index %d Phones select %s",bps_db.row[index-1], stmt);
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback2, phone_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_contact_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif

    i = 0;
    do {
        /* property: TEL */
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_TEL;
        p_prop[*p_num_prop].parameters = 0;

        /* entry empty */
        if (strlen(phone_entry->value[i][1]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }
        switch (atoi(phone_entry->value[i][0]))
        {
        case 1:
            /* home phone */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_HOME | BTA_OP_TEL_VOICE);
            break;
        case 2:
            /* mobile phone */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_CELL | BTA_OP_TEL_VOICE);
            break;
        case 3:
            /* work phone */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_WORK | BTA_OP_TEL_VOICE);
            break;
        case 4:
            /* work fax */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_WORK | BTA_OP_TEL_FAX);
            break;
        case 5:
            /* home fax */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_HOME | BTA_OP_TEL_FAX);
            break;
        case 6:
            /* pager */
            p_prop[*p_num_prop].parameters |= BTA_OP_TEL_PAGER;
            break;
        case 7:
            /* other phone no. */
            p_prop[*p_num_prop].parameters |= BTA_OP_TEL_MSG;
            break;
        }
        p_prop[*p_num_prop].p_data = phone_entry->value[i][1];
        p_prop[*p_num_prop].len = strlen(phone_entry->value[i][1]);
#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG6(" bps parse pb entry=%d row=%d c0=%s c1=%s len=%d p=%d", *p_num_prop, i, phone_entry->value[i][0],
                          p_prop[*p_num_prop].p_data, p_prop[*p_num_prop].len, p_prop[*p_num_prop].parameters);
#endif
    } while (++i < phone_entry->num);

    /* step 3: compose attributes from table Contact_methods */
    memset(contact_entry, 0, sizeof(tBTUI_PBS_PROP_OTHER));
    memset(stmt, 0, sizeof(stmt));
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select kind, data, type from view_v1_contact_methods where person=%d", bps_db.row[index-1]);
#else
    snprintf(stmt, sizeof(stmt),
        "select kind, data, type from contact_methods where person=%d", bps_db.row[index-1]);
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2(" bps parse pb index %d Contact_methods select %s",bps_db.row[index-1], stmt);
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback2, contact_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_contact_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif

    for (i = 0; i < contact_entry->num; i++)
    {
        /* entry empty */
        if (strlen(contact_entry->value[i][1]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }

        if (atoi(contact_entry->value[i][0]) == 1)
        {
            /* property: EMAIL */
            (*p_num_prop)++;
            p_prop[*p_num_prop].name = BTA_OP_VCARD_EMAIL;
            p_prop[*p_num_prop].parameters = 0;
            /* PREF */
            p_prop[*p_num_prop].parameters |= BTA_OP_EMAIL_PREF;
            /* INTERNET */
            p_prop[*p_num_prop].parameters |= BTA_OP_EMAIL_INTERNET;

            p_prop[*p_num_prop].p_data = contact_entry->value[i][1];
            p_prop[*p_num_prop].len = strlen(contact_entry->value[i][1]);

#if USE_BCMBTUI == TRUE
            APPL_TRACE_DEBUG6(" bps parse pb EMAIL entry=%d row=%d c0=%s c1=%s c2=%s p=%d",*p_num_prop, i, contact_entry->value[i][0],
                              contact_entry->value[i][1], contact_entry->value[i][2],
                              p_prop[*p_num_prop].parameters);
#endif
        }
        else if (atoi(contact_entry->value[i][0]) == 2)
        {
            /* property: ADR */
            (*p_num_prop)++;
            p_prop[*p_num_prop].name = BTA_OP_VCARD_ADR;
            p_prop[*p_num_prop].parameters = 0;

            if (atoi(contact_entry->value[i][2]) == 1)
            {
                /* HOME */
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_HOME;
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_POSTAL;
            }
            else if (atoi(contact_entry->value[i][2]) == 2)
            {
                /* WORK */
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_WORK;
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_POSTAL;
            }

#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
            bta_pbs_get_addr_data(db, contact_entry->value[i][1], BTUI_PBS_PROP_SIZE,
                bps_db.row[index-1], atoi(contact_entry->value[i][2]));
#endif
            p_prop[*p_num_prop].p_data = contact_entry->value[i][1];
            p_prop[*p_num_prop].len = strlen(contact_entry->value[i][1]);

#if USE_BCMBTUI == TRUE
            APPL_TRACE_DEBUG6(" bps parse pb ADR entry=%d row=%d c0=%s c1=%s c2=%s p=%d",
                *p_num_prop, i, contact_entry->value[i][0],
                p_prop[*p_num_prop].p_data, contact_entry->value[i][2],
                p_prop[*p_num_prop].parameters);
#endif
        }
        else if (atoi(contact_entry->value[i][0]) == 3)
        {
            APPL_TRACE_DEBUG0("PBAP currently doesn't support IM");
        }
    }

    /* step 4: compose attributes from table organizations */
    memset(org_entry, 0, sizeof(tBTUI_PBS_PROP_OTHER));
    memset(stmt, 0, sizeof(stmt));
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select company, title from view_v1_organizations where person=%d", bps_db.row[index-1]);
#else
    snprintf(stmt, sizeof(stmt),
        "select company, title from organizations where person=%d", bps_db.row[index-1]);
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2(" bps parse pb index %d Organization %s",bps_db.row[index-1], stmt);
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback2, org_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_contact_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif
    for (i = 0; i < org_entry->num; i++)
    {
        /* entry empty */
        if (strlen(org_entry->value[i][0]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }
        /* property: ORG */
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_ORG;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = org_entry->value[i][0];
        p_prop[*p_num_prop].len = strlen(org_entry->value[i][0]);

        /* entry empty */
        if (strlen(org_entry->value[i][1]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }
        /* property: TITLE */
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_TITLE;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = org_entry->value[i][1];
        p_prop[*p_num_prop].len = strlen(org_entry->value[i][1]);

#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG4(" bps parse ORG/Title entry=%d row=%d c0=%s c1=%s", *p_num_prop, i, org_entry->value[i][0],
                          org_entry->value[i][1]);
#endif
    }

#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
    /* step 5: compose attributes from table photos */
    memset(photo_entry, 0, sizeof(tBTUI_PBS_PROP_BLOB));
    memset(stmt, 0, sizeof(stmt));
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select length(data), data from view_v1_photos where person=%d", bps_db.row[index-1]);
#else
    snprintf(stmt, sizeof(stmt),
        "select length(data), data from photos where person=%d", bps_db.row[index-1]);
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2(" bps parse pb index %d Photos %s",index, stmt);
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback4, photo_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_contact_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif

    if (photo_entry->srclen > 0)
    {
        bta_pbs_base64_encode(photo_entry->src, photo_entry->dst, photo_entry->srclen, BTUI_PBS_DEFAULT_BASE64_LINE_LEN);

        /* property: PHOTO */
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_PHOTO;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = photo_entry->dst;
        p_prop[*p_num_prop].len = photo_entry->dstlen;

        p_prop[*p_num_prop].parameters |= BTA_OP_ENC_BASE64;
        p_prop[*p_num_prop].parameters |= BTA_OP_PHOTO_TYPE_JPEG;

#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG2(" bps parse PHOTO/data entry=%d data length=%d", *p_num_prop, photo_entry->dstlen);
#endif
    }
#endif

    (*p_num_prop)++;
    return TRUE;
}

static int bta_pbs_app_parse_call_entry(UINT8 path, tBTA_OP_PROP *p_prop,
                                      UINT8 *p_num_prop, int index)
{
    time_t mytime;
    struct tm *ts;
    char *errmsg;
    char tmp[10];
    char stmt[100];
    int i;
    int n_id;
    int max_prop = *p_num_prop ;
    tBTUI_PBS_PROP_QUERY *query;

    if (bps_db.row == NULL)
    {
      APPL_TRACE_ERROR1("bta_pbs_app_parse_call_entry: no row data",1);
      return FALSE;
    }


    memset(call_entry, 0, sizeof(tBTUI_PBS_PROP_PB));
    memset(stmt, 0, sizeof(stmt));
    snprintf(stmt, sizeof(stmt),
        "select number, name, type, date, duration, numbertype from calls where _id=%d",
           bps_db.row[index-1]);
#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_call_cb, call_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR1("bta_pbs_app_parse_call_entry: Error in select statement %s ", errmsg);
        return FALSE;
    }
#endif

    *p_num_prop = 0;

    /* step 1: compose attributes from table Calls */
    /* property: N */
    p_prop[*p_num_prop].name = BTA_OP_VCARD_N;
    p_prop[*p_num_prop].parameters = BTUI_PBS_CHARSET;
    if (strlen(call_entry->value[1])) /* name is shown in call log */
    {
        p_prop[*p_num_prop].p_data = call_entry->p_name;
        p_prop[*p_num_prop].len = strlen(call_entry->p_name);
    }
    else
    {
        p_prop[*p_num_prop].p_data = NULL;
        p_prop[*p_num_prop].len = 0;
    }

    /* property: FN */
    (*p_num_prop)++;
    p_prop[*p_num_prop].name = BTA_OP_VCARD_FN;
    p_prop[*p_num_prop].parameters = BTUI_PBS_CHARSET;
    if (strlen(call_entry->value[1])) /* name is shown in call log */
    {
        p_prop[*p_num_prop].p_data = call_entry->value[1];
        p_prop[*p_num_prop].len = strlen(call_entry->value[1]);
    }
    else
    {
        p_prop[*p_num_prop].p_data = NULL;
        p_prop[*p_num_prop].len = 0;
    }

    /* entry: TEL */
    (*p_num_prop)++;
    p_prop[*p_num_prop].name = BTA_OP_VCARD_TEL;
    p_prop[*p_num_prop].parameters = 0;

    if (strlen(call_entry->value[5]))
    {
        switch (atoi(call_entry->value[5]))
        {
        case 1:
            /* home phone */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_HOME | BTA_OP_TEL_VOICE);
            break;
        case 2:
            /* mobile phone */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_CELL | BTA_OP_TEL_VOICE);
            break;
        case 3:
            /* work phone */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_WORK | BTA_OP_TEL_VOICE);
            break;
        case 4:
            /* work fax */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_WORK | BTA_OP_TEL_FAX);
            break;
        case 5:
            /* home fax */
            p_prop[*p_num_prop].parameters |= (BTA_OP_TEL_HOME | BTA_OP_TEL_FAX);
            break;
        case 6:
            /* pager */
            p_prop[*p_num_prop].parameters |= BTA_OP_TEL_PAGER;
            break;
        case 7:
            /* other phone no. */
            p_prop[*p_num_prop].parameters |= BTA_OP_TEL_MSG;
            break;
        }
    }
    p_prop[*p_num_prop].p_data = call_entry->value[0];
    p_prop[*p_num_prop].len = strlen(call_entry->value[0]);

    /* entry: DATETIME */
    strncpy(tmp, call_entry->value[3], 10);
    mytime = atoi(tmp);
    ts = localtime(&mytime);
    switch (atoi(call_entry->value[2]))
    {
    case 1: /* incoming call */
        strftime(call_entry->value[3], sizeof(call_entry->value[3]),
                 "RECEIVED:%Y%m%dT%H%M%S", ts);
        break;
    case 2: /* outcoming call */
        strftime(call_entry->value[3], sizeof(call_entry->value[3]),
                 "DIALED:%Y%m%dT%H%M%S", ts);
        break;
    case 3: /* missed call */
        strftime(call_entry->value[3], sizeof(call_entry->value[3]),
                 "MISSED:%Y%m%dT%H%M%S", ts);
        break;
    }

    (*p_num_prop)++;
    p_prop[*p_num_prop].name = BTA_OP_VCARD_CALL;
    p_prop[*p_num_prop].parameters = 0;
    p_prop[*p_num_prop].p_data = call_entry->value[3];
    p_prop[*p_num_prop].len = strlen(call_entry->value[3]);

    if (!strlen(call_entry->value[1])) /* name is not shown in call log */
    {
        (*p_num_prop)++;
        return TRUE;
    }

    /* step 2: compose attributes from table People */
    memset(phone_entry, 0, sizeof(phone_entry));
    query = malloc(sizeof(tBTUI_PBS_PROP_QUERY));
    memset(query, 0, sizeof(query));
    query->num = 0;
    strcpy(query->col, call_entry->value[1]);
    query->num = 0;
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "SELECT name, _id, notes FROM view_v1_people");
#else
    snprintf(stmt, sizeof(stmt),
        "SELECT name, _id, notes FROM people");
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback3, query, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_call_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif

    //Check if query returned valid contact id. If not, free query and return error
    if (query->value[0][0]==NULL || strlen(query->value[0][0])<=0)
    {
      APPL_TRACE_ERROR1("bta_pbs_app_parse_call_entry: contact id not found for name %s",call_entry->value[1]);
      free(query);
      return FALSE;
    }


    strcpy(phone_entry->value[0][0], query->value[0][0]);
    strcpy(phone_entry->value[0][1], query->value[0][1]);
    phone_entry->num = 1;
    free(query);

    /* property: NOTE */
    if (strlen(phone_entry->value[0][1]))
    {
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_NOTE;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = phone_entry->value[0][1];
        p_prop[*p_num_prop].len = strlen(phone_entry->value[0][1]);
    }

    /* step 3: compose attributes from table Contact_methods */
    memset(contact_entry, 0, sizeof(contact_entry));
    memset(stmt, 0, sizeof(stmt));
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select kind, data, type from view_v1_contact_methods where person=%s", phone_entry->value[0][0]);
#else
    snprintf(stmt, sizeof(stmt),
        "select kind, data, type from contact_methods where person=%s", phone_entry->value[0][0]);
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2(" bps parse pb index %d Contact_methods %s",index, stmt);
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback2, contact_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_call_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif
    for (i = 0; i < contact_entry->num; i++)
    {
        /* entry empty */
        if (strlen(contact_entry->value[i][1]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }

        if (atoi(contact_entry->value[i][0]) == 1)
        {
            /* property: EMAIL */
            (*p_num_prop)++;
            p_prop[*p_num_prop].name = BTA_OP_VCARD_EMAIL;
            p_prop[*p_num_prop].parameters = 0;
            /* PREF */
            p_prop[*p_num_prop].parameters |= BTA_OP_EMAIL_PREF;
            /* INTERNET */
            p_prop[*p_num_prop].parameters |= BTA_OP_EMAIL_INTERNET;

            p_prop[*p_num_prop].p_data = contact_entry->value[i][1];
            p_prop[*p_num_prop].len = strlen(contact_entry->value[i][1]);

#if USE_BCMBTUI == TRUE
            APPL_TRACE_DEBUG5(" bps parse pb EMAIL row=%d c0=%s c1=%s c2=%s p=%d",i, contact_entry->value[i][0],
                              contact_entry->value[i][1], contact_entry->value[i][2],
                              p_prop[*p_num_prop].parameters);
#endif
        }
        else if (atoi(contact_entry->value[i][0]) == 2)
        {
            /* property: ADR */
            (*p_num_prop)++;
            p_prop[*p_num_prop].name = BTA_OP_VCARD_ADR;
            p_prop[*p_num_prop].parameters = 0;

            if (atoi(contact_entry->value[i][2]) == 1)
            {
                /* HOME */
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_HOME;
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_POSTAL;
            }
            else if (atoi(contact_entry->value[i][2]) == 2)
            {
                /* WORK */
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_WORK;
                p_prop[*p_num_prop].parameters |= BTA_OP_ADR_POSTAL;
            }

            p_prop[*p_num_prop].p_data = contact_entry->value[i][1];
            p_prop[*p_num_prop].len = strlen(contact_entry->value[i][1]);

#if USE_BCMBTUI == TRUE
            APPL_TRACE_DEBUG5(" bps parse pb ADR row=%d c0=%s c1=%s c2=%s p=%d",i, contact_entry->value[i][0],
                              contact_entry->value[i][1], contact_entry->value[i][2],
                              p_prop[*p_num_prop].parameters);
#endif
        }
        else if (atoi(contact_entry->value[i][0]) == 3)
        {
            APPL_TRACE_DEBUG0("PBAP currently doesn't support IM");
        }
    }

    /* step 4: compose attributes from table organizations */
    memset(org_entry, 0, sizeof(org_entry));
    memset(stmt, 0, sizeof(stmt));
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, sizeof(stmt),
        "select company, title from view_v1_organizations where person=%s", phone_entry->value[0][0]);
#else
    snprintf(stmt, sizeof(stmt),
        "select company, title from organizations where person=%s", phone_entry->value[0][0]);
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2(" bps parse pb index %d Organization %s",index, stmt);
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(db, stmt, select_callback2, org_entry, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_parse_call_entry: select statement %s with error %s", stmt, errmsg);
        return FALSE;
    }
#endif
    for (i = 0; i < org_entry->num; i++)
    {
        /* entry empty */
        if (strlen(org_entry->value[i][0]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }
        /* property: ORG */
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_ORG;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = org_entry->value[i][0];
        p_prop[*p_num_prop].len = strlen(org_entry->value[i][0]);

        /* entry empty */
        if (strlen(org_entry->value[i][1]) == 0)
        {
            continue;
        }
        if ((*p_num_prop) == (max_prop-1))
        {
            APPL_TRACE_DEBUG1(" bps parse pb entry exceeds the maximum %d", max_prop);
            return TRUE;
        }
        /* property: TITLE */
        (*p_num_prop)++;
        p_prop[*p_num_prop].name = BTA_OP_VCARD_TITLE;
        p_prop[*p_num_prop].parameters = 0;
        p_prop[*p_num_prop].p_data = org_entry->value[i][1];
        p_prop[*p_num_prop].len = strlen(org_entry->value[i][1]);

#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG3(" bps parse ORG/Title row=%d c0=%s c1=%s",i, org_entry->value[i][0],
                          org_entry->value[i][1]);
#endif
    }

    (*p_num_prop)++;
    return TRUE;
}

static UINT16 bta_pbs_app_count_tb_size( sqlite3 *p_db, tBTUI_PBS_PATH d_path )
{
    char *errmsg;
    char stmt[100];

    UINT16 tsize = 0;
    bps_db.path = d_path.path;

    memset(stmt, 0, sizeof(stmt));
    if (d_path.path == BTA_PBS_PATH_PHONE_PB)
    {
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
        strcpy(stmt, "select count(_id) from view_v1_people");
#else
        strcpy(stmt, "select count(_id) from people");
#endif
    }
    else
    {
        if (d_path.path == BTA_PBS_PATH_PHONE_ICH ||
            d_path.path == BTA_PBS_PATH_PHONE_OCH ||
            d_path.path == BTA_PBS_PATH_PHONE_MCH)
        {
            snprintf(stmt, sizeof(stmt),
                "select count(_id) from calls where type=%d", d_path.path-1);
        }
        else if (d_path.path == BTA_PBS_PATH_PHONE_CCH)
        {
            snprintf(stmt, sizeof(stmt), "select count(_id) from calls");
        }
        else
        {
            LOGE("%s: call type %d Error", __FUNCTION__, d_path.path);
            return 0;
        }
    }

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(p_db, stmt, count_callback, &tsize, &errmsg) != SQLITE_OK)
    {
        LOGE("%s: Failed to execute SQL statement [%s], errmsg = [%s]", __FUNCTION__, stmt, errmsg);
        return 0;
    }
#endif
    return tsize;
}

/* process the phone book object main entry for given db */
static BOOLEAN bta_pbs_app_process_pb( sqlite3 *p_db, tBTUI_PBS_PATH d_path )
{
    char *errmsg;
    char stmt[100];
    UINT16 t_size;

    bps_db.num = 0;
    bps_db.path = d_path.path;
    t_size = bta_pbs_app_count_tb_size(p_db, d_path);
    if (t_size == 0)
    {
        return TRUE;
    }

    bps_db.row = malloc(t_size*sizeof(UINT16));
    if (bps_db.row == NULL)
    {
        APPL_TRACE_ERROR1("bta_pbs_app_process_pb: memory allocation failure for %d entries", bps_db.num);
        return FALSE;
    }
#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG1("bta_pbs_app_process_pb: total table had %d entries", t_size);
#endif

    memset(stmt, 0, sizeof(stmt));
    if (d_path.path == BTA_PBS_PATH_PHONE_PB)
    {
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
        strcpy(stmt, "select _id from view_v1_people");
#else
        strcpy(stmt, "select _id from people");
#endif
    }
    else
    {
        if (d_path.path == BTA_PBS_PATH_PHONE_ICH ||
            d_path.path == BTA_PBS_PATH_PHONE_OCH ||
            d_path.path == BTA_PBS_PATH_PHONE_MCH)
        {
            snprintf(stmt, sizeof(stmt),
                "select _id from calls where type=%d", d_path.path-1);
        }
        else if (d_path.path == BTA_PBS_PATH_PHONE_CCH)
        {
            snprintf(stmt, sizeof(stmt), "select _id from calls");
        }
    }

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec(p_db, stmt, index_callback, &bps_db, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR2("bta_pbs_app_process_pb: Error in select statement %s to count person %s ", stmt, errmsg);
        return FALSE;
    }
#endif

    return TRUE;
}


/* process the phone book object vcard list */
static int bta_pbs_app_process_pb_vlist( sqlite3 *p_db, tBTUI_PBS_PATH d_path)
{
    char *errmsg;
    char stmt[100];
    UINT16 t_size;

    bps_db.num = bta_pbs_app_count_tb_size(p_db, d_path);
    if (bps_db.num == 0)
    {
        APPL_TRACE_DEBUG0("bta_pbs_app_process_pb_vlist: the phone book size is 0");
        return FALSE;
    }

    if ((vlist_entry = malloc(bps_db.num * sizeof(tBTUI_PBS_NAME_NUM))) == NULL)
    {
        APPL_TRACE_ERROR0("bta_pbs_app_process_pb_vlist: Memory Alloc Error for raw database");
        return FALSE;
    }

    memset(vlist_entry, 0, sizeof(vlist_entry));
    memset(stmt, 0, sizeof(stmt));
    max_count = 0;
    if (d_path.path == BTA_PBS_PATH_PHONE_PB)
    {
#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
        strcpy(stmt, "select name from view_v1_people");
#else
        strcpy(stmt, "select name from people");
#endif
    }
    else
    {
        strcpy(stmt, "select name, number, type from calls ORDER BY date DESC");
    }
#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec( p_db, stmt, count_vlist_callback, &bps_db, &errmsg) != SQLITE_OK)
    {
        APPL_TRACE_ERROR1("bta_pbs_app_process_pb_vlist: Error in select statement to count person %s ", errmsg);
        return FALSE;
    }
#endif

    return TRUE;
}

/* cleanup state and memory */
static void bta_pbs_app_clean_up( void )
{
    if (people_entry)
    {
        free(people_entry);
        people_entry = NULL;
    }
    if (call_entry)
    {
        free(call_entry);
        call_entry = NULL;
    }
    if (phone_entry)
    {
        free(phone_entry);
        phone_entry = NULL;
    }
    if (contact_entry)
    {
        free(contact_entry);
        contact_entry = NULL;
    }
    if (org_entry)
    {
        free(org_entry);
        org_entry = NULL;
    }

#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
    if (photo_entry)
    {
        free(photo_entry);
        photo_entry = NULL;
    }
#endif

    //Deallocate bps_db.row that was dynamically created
    if (bps_db.row != NULL)
    {
      free(bps_db.row);
      bps_db.row=NULL;
    }

    memset(&bps_db, 0, sizeof(tBTUI_PBS_RAW_PB));

    if (btui_bps_default_card_mem)
    {
        free(btui_bps_default_card_mem);
        btui_bps_default_card_mem = NULL;
    }

    max_count = 0;
    bps_card_size = 0;
    bps_card_read_offset = 0;
    bps_db_row_index = 0;

    btui_pbs_cb.fd = BTA_FS_INVALID_FD;
    btui_bps_is_SIM = FALSE;

}


/*******************************************************************************
**
** Function         bta_pbs_co_open
**
** Description      This function is executed by BTA when a pb file is requested to be opened.
**                  The phone book access profile server uses this function to open
**                  a file for reading on two phone book access operations
**                  (pull pb or pull pb entry)
**
** Parameters       p_path  - path and file name.
**                  operation - BTA_PB_OPER_PULL_PB or BTA_PB_OPER_PULL_VCARD_ENTRY
**                  p_app_params - obex application params
**
**
** Returns          void
**
**                  Note: Upon completion of the request
**                        if successful, and an error code (tBTA_PBS_CO_STATUS)
**                        are returned in the call-in function, bta_pbs_ci_open().
**
*******************************************************************************/
void bta_pbs_co_open(const char *p_path, tBTA_PBS_OPER operation, tBTA_PBS_PULLPB_APP_PARAMS *p_app_params)
{
    tBTUI_PBS_SQL_MSG *p_event_msg;

    if ((p_event_msg = (tBTUI_PBS_SQL_MSG *)GKI_getbuf(sizeof(tBTUI_PBS_SQL_MSG))) != NULL)
    {
        p_event_msg->open.event = BTUI_PBS_SQL_OPEN;
        p_event_msg->open.p_path = p_path;
        p_event_msg->open.operation = operation;
        p_event_msg->open.p_app_params = p_app_params;
        GKI_send_msg(PBS_SQL_TASK, TASK_MBOX_0, p_event_msg);
    }
    else
    {
        LOGE("%s: Failed to allocate message memory", __FUNCTION__);
    }
}

static void pbs_sql_co_open(tBTUI_PBS_OPEN *data)
{
    tBTA_PBS_CO_STATUS  status = BTA_PBS_CO_OK;
    tBTUI_PBS_PATH  d_path;
    char *errmsg;

    UINT16  file_size = 0;
    UINT32  filter = 0;
    tBTA_OP_STATUS bps_status;
    int fd = BTA_FS_INVALID_FD;
    tBTA_OP_PROP prop_array[BTUI_PBS_NUM_PROP];
    UINT8 num_prop = BTUI_PBS_NUM_PROP;
    UINT8 *next_start_p;

    LOGI("%s: p_path = [%s], operation = [%s]",
         __FUNCTION__, data->p_path, op_str[data->operation]);

    /* check if the path is valid */
    if (bta_pbs_is_valid_path(data->p_path, data->operation, &d_path) == FALSE)
    {
        LOGE("%s: the path [%s] is invalid", __FUNCTION__, data->p_path);
        btui_bps_is_SIM = TRUE;
        status = BTA_PBS_CO_EODIR;
        goto done;
    }

    /* current PBAP version doesn't support contacts in SIM card */
    if (bta_pbs_is_for_SIM(data->p_path) == TRUE)
    {
        LOGE("%s: p_path = [%s], not support SIM", __FUNCTION__, data->p_path);
        btui_bps_is_SIM = TRUE;
        status = BTA_PBS_CO_FAIL;
        goto done;
    }

    /* initialize the state */
    bta_pbs_app_clean_up();

    /*vCard format expected by the Client */
    fmt = (data->p_app_params->format == BTA_PBS_VCF_FMT_30)?BTA_OP_VCARD30_FMT:BTA_OP_VCARD21_FMT;

    people_entry = malloc(sizeof(tBTUI_PBS_PROP_PB));
    call_entry = malloc(sizeof(tBTUI_PBS_PROP_PB));
    phone_entry = malloc(sizeof(tBTUI_PBS_PROP_OTHER));
    contact_entry = malloc(sizeof(tBTUI_PBS_PROP_OTHER));
    org_entry = malloc(sizeof(tBTUI_PBS_PROP_OTHER));
#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
    photo_entry = malloc(sizeof(tBTUI_PBS_PROP_BLOB));
#endif

    if ((people_entry == NULL)  || (call_entry == NULL) || (phone_entry == NULL) ||
        (contact_entry == NULL) || (org_entry == NULL)
#if defined(BTA_PBS_CO_PHOTO_SUPPORTED) && (BTA_PBS_CO_PHOTO_SUPPORTED == TRUE)
        || (photo_entry == NULL)
#endif
       )
    {
        status = BTA_PBS_CO_FAIL;
        LOGI("%s: Failed to allocate contact entry memory", __FUNCTION__);
        goto done;
    }

    /* fake path */
    fd = d_path.path;

#ifdef SQLITE_ENABLED
    LOGI("%s: Open Android database [%s]", __FUNCTION__, bte_appl_cfg.contacts_db);
    sqlite3_open_v2(bte_appl_cfg.contacts_db, &db, SQLITE_OPEN_READONLY, NULL);
    if (db == NULL) {
        LOGE("%s: Failed to open Android database [%s]", __FUNCTION__, bte_appl_cfg.contacts_db);
        fd = BTA_FS_INVALID_FD;
        status = BTA_PBS_CO_EACCES;
        goto done;
    }
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG4( "[CO] bta_pbs_co_open: handle:%d path:%s index:%d, open(db ptr: x%x)",
                       fd, data->p_path, d_path.index, db );
    APPL_TRACE_DEBUG4("PBS app params filter=0x%x format=%d max_count=%d start_offset=%d",
                       data->p_app_params->filter, data->p_app_params->format, data->p_app_params->max_count, data->p_app_params->start_offset);
#endif

    bps_db_row_index = 0;

    /* only parse vcard for vcard entry only */
    if ((fd > 0) && db)
    {
        btui_pbs_cb.fd = fd;
        filter = data->p_app_params->filter;
        if (data->p_app_params->format == BTA_PBS_VCF_FMT_21 && filter !=0) {
            /* Removed BTA_OP_FILTER_PHOTO because photo is not mandatory item for BTA_PBS_VCF_FMT_21
             * Even though phone supports this item we don't need to add it here.
             * When peer requests photo, we can send the data */
            filter |= (BTA_OP_FILTER_N | BTA_OP_FILTER_TEL);
        } else if (data->p_app_params->format == BTA_PBS_VCF_FMT_30 && filter !=0) {
            /* Removed BTA_OP_FILTER_PHOTO because photo is not mandatory item for BTA_PBS_VCF_FMT_30
             * Even though phone supports this item we don't need to add it here.
             * When peer requests photo, we can send the data */
            filter |= (BTA_OP_FILTER_FN | BTA_OP_FILTER_N | BTA_OP_FILTER_TEL);
        }

        /* if it is PULLPB, need to find the first VCARD Object */
        if ((data->operation != BTA_PBS_OPER_PULL_PB) && (data->operation != BTA_PBS_OPER_PULL_VCARD_ENTRY))
        {
            LOGE("%s: Unsupported operation [%s]", __FUNCTION__, op_str[data->operation]);
            fd = BTA_FS_INVALID_FD;
            status = BTA_PBS_CO_FAIL;
            goto done;
        }

        if (bta_pbs_app_process_pb( db, d_path ) == FALSE)
        {
            LOGE("%s: Failed to retrieve PB entry IDs", __FUNCTION__);
            fd = BTA_FS_INVALID_FD;
            status = BTA_PBS_CO_FAIL;
            goto done;
        }

        LOGI("%s: PBS main table parsed size = %d", __FUNCTION__, bps_db.num);

        if (bps_db.num == 0)
        {
            /* empty data base */
            fd = BTA_FS_INVALID_FD;
            bta_pbs_app_clean_up();
#ifdef SQLITE_ENABLED
            sqlite3_close(db);
            APPL_TRACE_DEBUG1( "bta_pbs_co_open():sqlite3_close( db ptr: x%x )", db );
#endif
            goto done;
        }

        bps_db_row_index = data->p_app_params->start_offset;
        if (data->operation == BTA_PBS_OPER_PULL_VCARD_ENTRY)
        {
            memset(prop_array, 0, BTUI_PBS_NUM_PROP*sizeof(tBTA_OP_PROP));
            if (d_path.path == BTA_PBS_PATH_PHONE_PB)
            {
#if defined(PBAP_ZERO_VCARD_IN_DB) && (PBAP_ZERO_VCARD_IN_DB == TRUE)
                if (bta_pbs_app_parse_contact_entry(prop_array, &num_prop,
                                                   d_path.index+1) == FALSE)
#else
                if (bta_pbs_app_parse_contact_entry(prop_array, &num_prop,
                                                   d_path.index) == FALSE)
#endif
                {
                    LOGE("%s: bta_pbs_app_parse_contact_entry failed", __FUNCTION__);
                    fd = BTA_FS_INVALID_FD;
                    status = BTA_PBS_CO_FAIL;
                    goto done;
                }
            }
            else
            {
                if (bta_pbs_app_parse_call_entry(d_path.path, prop_array, &num_prop,
                                                   d_path.index) == FALSE)
                {
                    LOGE("%s: bta_pbs_app_parse_call_entry failed", __FUNCTION__);
                    fd = BTA_FS_INVALID_FD;
                    status = BTA_PBS_CO_FAIL;
                    goto done;
                }
            }

            LOGI("%s: PBS parsed PB entry, index = %d, size = %d", __FUNCTION__, bps_db_row_index, num_prop);
        }
        else
        {
            if (data->p_app_params->start_offset > bps_db.num)
            {
                APPL_TRACE_ERROR2("bta_pbs_co_open: the offset %d is invalid since table size is %d",
                                      data->p_app_params->start_offset, bps_db.num);
                status = BTA_PBS_CO_FAIL;
                goto done;
            }
            max_count = min(data->p_app_params->max_count, bps_db.num - data->p_app_params->start_offset);

            memset(prop_array, 0, BTUI_PBS_NUM_PROP*sizeof(tBTA_OP_PROP));
            if (d_path.path == BTA_PBS_PATH_PHONE_PB)
            {
                if (bta_pbs_app_parse_contact_entry(prop_array, &num_prop,
                                                   bps_db_row_index+1) == FALSE)
                {
                    LOGE("%s: bta_pbs_app_parse_contact_entry failed", __FUNCTION__);
                    fd = BTA_FS_INVALID_FD;
                    status = BTA_PBS_CO_FAIL;
                    goto done;
                }
            }
            else
            {
                if (bta_pbs_app_parse_call_entry(d_path.path, prop_array, &num_prop,
                                                   bps_db_row_index+1) == FALSE)
                {
                    LOGE("%s: bta_pbs_app_parse_call_entry failed", __FUNCTION__);
                    fd = BTA_FS_INVALID_FD;
                    status = BTA_PBS_CO_FAIL;
                    goto done;
                }
            }
#if USE_BCMBTUI == TRUE
            APPL_TRACE_DEBUG2(" bps parse pb entry index %d size %d",bps_db_row_index, num_prop);
#endif

            /* decrease count */
            max_count--;
            bps_db_row_index++;
        }

        BTA_OpSetCardPropFilterMask(filter);

        if ((btui_bps_default_card_mem = malloc(BTUI_PBS_DEFAULT_CARD_MEM_LEN)) == NULL)
        {
            APPL_TRACE_ERROR0("bta_pbs_co_open: memory allocation failure for temp vcard storage");
            status = BTA_PBS_CO_FAIL;
            goto done;
        }

        file_size = BTUI_PBS_DEFAULT_CARD_MEM_LEN;
        bps_status = BTA_OpBuildCard(btui_bps_default_card_mem, &file_size, fmt,
                                         (tBTA_OP_PROP*)prop_array, num_prop);
        APPL_TRACE_DEBUG3(" bps build status %d size %d entries %d",bps_status, file_size, num_prop);

#if USE_BCMBTUI == TRUE
        {
            int i;
            char *c = btui_bps_default_card_mem;
            fprintf(stderr, "\n");
            for (i = 0; i < file_size; i++)
            {
                fprintf(stderr, "%c", *(c+i));
            }
            fprintf(stderr, "\n");
        }
#endif
        bps_card_size = file_size;
        bps_card_read_offset = 0;
    }

done:
    if ((status != BTA_PBS_CO_OK) &&
        (btui_bps_is_SIM != TRUE))
    {
        APPL_TRACE_ERROR1(" bps open done called. Status = %d ", status);
        bta_pbs_app_clean_up();
#ifdef SQLITE_ENABLED
        if (db)
        {
            sqlite3_close(db);
            APPL_TRACE_DEBUG1( "bta_pbs_co_open():sqlite3_close( db ptr: x%x )", db );
        }
#endif
    }
    /* need to set up the size, rough estimate */
    bta_pbs_ci_open(fd, status, (UINT32)(file_size * bps_db.num));
}

/*******************************************************************************
**
** Function         bta_pbs_co_close
**
** Description      This function is called by BTA when a connection to a
**                  client is closed.
**
** Parameters       fd      - file descriptor of file to close.
**
** Returns          (tBTA_PBS_CO_STATUS) status of the call.
**                      [BTA_PBS_CO_OK if successful],
**                      [BTA_PBS_CO_FAIL if failed  ]
**
*******************************************************************************/
tBTA_PBS_CO_STATUS bta_pbs_co_close(int fd)
{
    tBTUI_PBS_SQL_MSG *p_event_msg;

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG1("bta_pbs_co_close with fd %d", fd);
#endif
    if ((p_event_msg = (tBTUI_PBS_SQL_MSG *)GKI_getbuf(sizeof(tBTUI_PBS_SQL_MSG))) != NULL)
    {
        p_event_msg->close.event = BTUI_PBS_SQL_CLOSE;
        p_event_msg->close.fd = fd;
        GKI_send_msg(PBS_SQL_TASK, TASK_MBOX_0, p_event_msg);
        return BTA_PBS_CO_OK;
    }

    LOGE("%s: Failed to allocate message memory", __FUNCTION__);
    return BTA_PBS_CO_FAIL;
}

static tBTA_PBS_CO_STATUS pbs_sql_co_close(tBTUI_PBS_CLOSE *data)
{
#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2("[CO] pbs_sql_co_close: handle:%d, sqlite3_close(db ptr: x%x)", data->fd, db);
#endif
#ifdef SQLITE_ENABLED
    if (db)
    {
        sqlite3_close(db);
    }
#endif

    bta_pbs_app_clean_up();

    return (BTA_PBS_CO_OK);
}

/*******************************************************************************
**
** Function         bta_pbs_co_read
**
** Description      This function is called by BTA to read in data from the
**                  previously opened pb file on the phone.
**                  the application callin should fill in the PB object needed to be
**                  send to the client
**
** Parameters       fd      - file descriptor of file to read from.
**                  operation - BTA_PBS_OPER_PULL_PB or BTA_PBS_OPER_PULL_VCARD_ENTRY
**                  p_buf   - buffer to read the data into.
**                  nbytes  - number of bytes to read into the buffer.
**
**
** Returns          void
**
**                  Note: Upon completion of the request, bta_pbs_ci_read() is
**                        called with the buffer of data, along with the number
**                        of bytes read into the buffer, and a status.
**
*******************************************************************************/
void bta_pbs_co_read(int fd, tBTA_PBS_OPER operation, UINT8 *p_buf, UINT16 nbytes)
{
    tBTUI_PBS_SQL_MSG *p_event_msg;

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG1("bta_pbs_co_read with operation %d", operation);
#endif
    if ((p_event_msg = (tBTUI_PBS_SQL_MSG *)GKI_getbuf(sizeof(tBTUI_PBS_SQL_MSG))) != NULL)
    {
        p_event_msg->read.event = BTUI_PBS_SQL_READ;
        p_event_msg->read.fd = fd;
        p_event_msg->read.operation = operation;
        p_event_msg->read.p_buf = p_buf;
        p_event_msg->read.nbytes = nbytes;
        GKI_send_msg(PBS_SQL_TASK, TASK_MBOX_0, p_event_msg);
    }
    else
        APPL_TRACE_ERROR0(" bta_pbs_co_read: memory allocation failure");
}

static void pbs_sql_co_read(tBTUI_PBS_READ *data)
{
    tBTA_PBS_CO_STATUS  status = BTA_PBS_CO_OK;
    INT32   num_read = 0;
    BOOLEAN final = FALSE;
    tBTA_OP_STATUS bps_status;
    tBTA_OP_PROP prop_array[BTUI_PBS_NUM_PROP];
    UINT8 num_prop = BTUI_PBS_NUM_PROP;
    UINT16  file_size = 0;

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG3("[CO] pbs_sql_co_read: handle:%d operation:%d bytes:%d", data->fd,data->operation, data->nbytes);
#endif

    if (data->operation == BTA_PBS_OPER_PULL_VCARD_ENTRY ||
        data->operation == BTA_PBS_OPER_PULL_PB)
    {
        num_read = min(data->nbytes, bps_card_size - bps_card_read_offset);

        memcpy(data->p_buf, btui_bps_default_card_mem + bps_card_read_offset, num_read);

        bps_card_read_offset += num_read;
        data->nbytes -= num_read;
        data->p_buf += num_read;

        if (bps_card_read_offset >= bps_card_size)
        {
            final = TRUE;
        }
        if (final && data->operation == BTA_PBS_OPER_PULL_PB)
        {
            UINT8 i = 0;
            while (max_count && (bps_db_row_index < bps_db.num))
            {
                memset(prop_array, 0, BTUI_PBS_NUM_PROP*sizeof(tBTA_OP_PROP));
                num_prop = 0;
                if (data->fd == BTA_PBS_PATH_PHONE_PB)
                {
                    if (bta_pbs_app_parse_contact_entry(prop_array, &num_prop,
                                                   bps_db_row_index+1) == FALSE)
                    {
                        status = BTA_PBS_CO_FAIL;
                        goto done;
                    }
                }
                else
                {
                    if (bta_pbs_app_parse_call_entry(data->fd, prop_array, &num_prop,
                                                   bps_db_row_index+1) == FALSE)
                    {
                        status = BTA_PBS_CO_FAIL;
                        goto done;
                    }
                }

                file_size = BTUI_PBS_DEFAULT_CARD_MEM_LEN;
                memset(btui_bps_default_card_mem, 0, BTUI_PBS_DEFAULT_CARD_MEM_LEN);
                bps_status = BTA_OpBuildCard(btui_bps_default_card_mem, &file_size, fmt,
                                             (tBTA_OP_PROP*)prop_array, num_prop);
                APPL_TRACE_DEBUG3(" bps build status %d size %d entries %d",bps_status, file_size, num_prop);
#if USE_BCMBTUI == TRUE
                {
                    int i;
                    char *c = btui_bps_default_card_mem;
                    fprintf(stderr, "\n");
                    for (i = 0; i < file_size; i++)
                    {
                        fprintf(stderr, "%c", *(c+i));
                    }
                    fprintf(stderr, "\n");
                }
#endif

                bps_card_size = file_size;
                bps_card_read_offset = 0;

                /* decrease count */
                max_count--;
                bps_db_row_index++;

                /* maximum 6 vcards each packet */
                if (i++ > 5)
                {
                    final = FALSE;
                    goto done;
                }
                /* remaining buffer big enough for the vcard */
                if (data->nbytes >= bps_card_size)
                {
                    memcpy(data->p_buf, btui_bps_default_card_mem, bps_card_size);
                    data->nbytes -= bps_card_size;
                    data->p_buf += bps_card_size;
                    num_read += bps_card_size;

                    continue;
                }
                /* remaining buffer NOT big enough for the vcard */
                else
                {
                    if (data->nbytes)
                    {
                        memcpy(data->p_buf, btui_bps_default_card_mem, data->nbytes);
                        num_read += data->nbytes;
                    }
                    bps_card_read_offset = data->nbytes;
                }
                final = FALSE;
                goto done;
            }
            /* no more entries in the db */
            bps_card_read_offset = 0;
        }
    }

done:
    bta_pbs_ci_read(data->fd, (UINT16)num_read, status, final);
}

#define DEFAULT_PBS_MISSED_CALL 0
/*******************************************************************************
**
** Function         bta_pbs_co_get_pbinfo
**
** Description      This function is called by BTA to inquire about pb size and new missed calls.
**
** Parameters       p_name: which type of phone book object Eg. telecom/pb.vcf, telecom/ich.vcf
**                  operation: phone book operation type Eg. BTA_PBS_OPER_PULL_PB
**                  obj_type: phone book repository type Eg. BTA_PBS_MCH_OBJ
**
** Returns          pb_size - phone book size
**                  new_missed_call - new missed calls
*                       (tBTA_PBS_CO_STATUS) status of the call.
**                      [BTA_PBS_CO_OK if successful],
**                      [BTA_PBS_CO_FAIL if failed  ]
**
*******************************************************************************/
tBTA_PBS_CO_STATUS bta_pbs_co_getpbinfo(tBTA_PBS_OPER operation, tBTA_PBS_OBJ_TYPE obj_type, UINT16 *pb_size, UINT16 *new_missed_call)
{
    tBTUI_PBS_PATH  d_path;
    sqlite3         *p_sql3;        /* local data base structure for getbpinfo as this is an atomic
                                       operation! */
    if (bps_db.num != 0 && bps_db.row != NULL)
    {
        *pb_size = bps_db.num;
        APPL_TRACE_DEBUG3("pbs_sql_co_getpbinfo operation %d, object type %d, pb size %d",
                       operation, obj_type, *pb_size);

        return BTA_PBS_CO_OK;
    }

    *pb_size = 0;
    *new_missed_call = DEFAULT_PBS_MISSED_CALL;

    if (btui_bps_is_SIM == TRUE)
    {
        APPL_TRACE_DEBUG0("bta_pbs_co_getpbinfo: SIM card read is not supported ");
        return BTA_PBS_CO_OK;
    }

    /* initialize the state */
    bta_pbs_app_clean_up();

    d_path.path = obj_type;

#ifdef SQLITE_ENABLED
    sqlite3_open_v2( bte_appl_cfg.contacts_db,  &p_sql3, SQLITE_OPEN_READONLY, NULL );
    if ( p_sql3 == NULL )
    {
        APPL_TRACE_ERROR0("bta_pbs_co_getpbinfo: cannot open Android database");
        return BTA_PBS_CO_FAIL;
    }
#endif

    *pb_size = bta_pbs_app_count_tb_size(p_sql3, d_path);

    /* empty data base */
    bta_pbs_app_clean_up();
#ifdef SQLITE_ENABLED
    sqlite3_close( p_sql3 );
#endif

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG4( "bta_pbs_co_getpbinfo operation %d, object type %d, pb size %d, close(p_sql3: x%x)",
                       operation, obj_type, *pb_size, p_sql3);
#endif

    return BTA_PBS_CO_OK;
}

int sort_cmp(const void *a, const void *b)
{
    return strcmp(vlist_entry[*(int*)a].value, vlist_entry[*(int*)b].value);
}

void bta_pbs_sort_vcard_list(tBTA_PBS_ORDER order)
{
    int i;

    /* sort the vcard list according to the order */
    /* default is by indexing */
    for (i = 0; i < max_count; i++)
    {
        vcard_list[i] = i;
    }
    if (order != BTA_PBS_ORDER_ALPHA_NUM)
    {
        APPL_TRACE_DEBUG0("bta_pbs_sort_vcard_list not sort Alpha");
        return;
    }

    /* sort by alpha order: Customer could optimize this */
    qsort(vcard_list, max_count, sizeof(vcard_list[0]), sort_cmp);
}



static BOOLEAN bta_pbs_app_vcard_from_number( sqlite3 *p_db, tBTUI_PBS_PATH d_path, char *phnumber)
{
    char *errmsg;
    char stmt[150];
    max_count=0; //Fill the first element in the vlist_entry with the result

	if(vlist_entry != NULL)
		free(vlist_entry);

    if ((vlist_entry = malloc(1 * sizeof(tBTUI_PBS_NAME_NUM))) == NULL)
    {
        LOGE("%s: Failed to alloc memory for raw database", __FUNCTION__);
        return FALSE;
    }
    memset(vlist_entry, 0, sizeof(vlist_entry));
    memset(stmt, 0, sizeof(stmt));

#if defined(BTLA_REL_2_X) && (BTLA_REL_2_X == TRUE)
    snprintf(stmt, 150, "%s%s%s", "select * from view_v1_people c inner join view_v1_phones p on c._id= p.person where p.number='", phnumber,"'");
#else
    snprintf(stmt, 150, "%s%s%s", "select * from people c inner join phones p on c._id= p.person where p.number='", phnumber,"'");
#endif

#ifdef SQLITE_ENABLED
    if (bta_pbs_sqlite3_exec( p_db, stmt, count_vlist_callback, &bps_db, &errmsg) != SQLITE_OK)
    {
        LOGE("%s: Failed to execute SQL statement [%s], errmsg = [%s]", __FUNCTION__, stmt, errmsg);
        return FALSE;
    }
#endif

    return TRUE;
}


static void bta_pbs_convert_to_valid_search_string(char *tostr, UINT8* fromstr, UINT16 len)
{
	memcpy(tostr, fromstr,3);
	tostr[3]= '-';
	memcpy(&tostr[4], &fromstr[3], 3);
	tostr[7]= '-';
	memcpy(&tostr[8], &fromstr[6], (len-6)>0 ? (len-6):0);
}


void bta_pbs_gen_vlist(const char *p_path, tBTA_PBS_VCARDLIST_APP_PARAMS *p_app_params,
                         BOOLEAN first_item, tBTA_PBS_VCARDLIST *p_entry)
{
    tBTUI_PBS_PATH  d_path;
    tBTA_PBS_CO_STATUS status = BTA_PBS_CO_FAIL;
    static UINT8 search_attr = 0;
    static char search_value[64] = "\0";
    BOOLEAN final = FALSE;

    /* check if the path is valid */
    if (bta_pbs_is_valid_path(p_path, BTA_PBS_OPER_PULL_VCARD_LIST, &d_path) == FALSE)
    {
        APPL_TRACE_ERROR1("pbs_sql_co_getvlist: the path %s is invalid", p_path);
        status = BTA_PBS_CO_EODIR;
        goto Done;
    }

    if (first_item)
    {
        /* Store the app_params */
        memset(search_value, 0, sizeof(search_value));
        /* remember search att and value */
        if (p_app_params->value_len != 0)
        {
            search_attr = p_app_params->attribute;
            strncpy(search_value, p_app_params->p_value, p_app_params->value_len);
        } else
            search_value[0] = '\0';
    }


#if defined(PBAP_ZERO_VCARD_IN_DB) && (PBAP_ZERO_VCARD_IN_DB == TRUE)
    if (d_path.path == BTA_PBS_PATH_PHONE_PB)
    {
        snprintf(p_entry->handle, sizeof(p_entry->handle),
            "%d.vcf", vcard_list[bps_db_row_index]);
        if (vcard_list[bps_db_row_index] == 0 && strlen(vlist_entry[0].value) == 0)
            strcpy(p_entry->name, default_0_vcard_name);
        else
            strcpy(p_entry->name, vlist_entry[vcard_list[bps_db_row_index]].value);
    }
    else
#endif
    {
        snprintf(p_entry->handle, sizeof(p_entry->handle),
            "%d.vcf", vcard_list[bps_db_row_index]+1);
        strcpy(p_entry->name, vlist_entry[vcard_list[bps_db_row_index]].value);
    }

    /* Perform search if we find name and need to do search */
    if (search_attr == BTA_PBS_ATTR_NAME && search_value[0] != '\0')
    {
        if (!strlen(vlist_entry[vcard_list[bps_db_row_index]].value))
            p_entry->handle[0] = '\0';
        else if (bta_pbs_app_strcmp(p_entry->name, p_entry->name + strlen(p_entry->name),
                                    search_value) == NULL)
            p_entry->handle[0] = '\0';
    }
    else if (search_attr == BTA_PBS_ATTR_NUMBER && search_value[0] != '\0')
    {
        if (bta_pbs_app_strcmp(vlist_entry[vcard_list[bps_db_row_index]].value[0],
                               vlist_entry[vcard_list[bps_db_row_index]].value[0] +
                               strlen(vlist_entry[vcard_list[bps_db_row_index]].value),
                               search_value) == NULL)
            p_entry->handle[0] = '\0';
    }

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG3("PBS Vcard list entry index as %d handle %s name %s", bps_db_row_index,
                       p_entry->handle, p_entry->name);
#endif

    if ((++bps_db_row_index == bps_db.num) || (--max_count == 0))
    {
#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG1("PBS Vcard list the last entry %d", bps_db.num);
#endif

        final = TRUE;
        memset(&bps_db, 0, sizeof(tBTUI_PBS_RAW_PB));
        if (vcard_list)
        {
            free(vcard_list);
            vcard_list = NULL;
        }
        if (vlist_entry)
        {
            free(vlist_entry);
            vlist_entry = NULL;
        }
        max_count = 0;
        bps_db_row_index = 0;
    }

    status = BTA_PBS_CO_OK;

Done:
    if (status != BTA_PBS_CO_OK)
    {
        memset(&bps_db, 0, sizeof(tBTUI_PBS_RAW_PB));
        if (vcard_list)
        {
            free(vcard_list);
            vcard_list = NULL;
        }
        if (vlist_entry)
        {
            free(vlist_entry);
            vlist_entry = NULL;
        }
        max_count = 0;
        bps_db_row_index = 0;
    }

    /* call the callin function */
    bta_pbs_ci_getvlist(status, final);
}

/*******************************************************************************
**
** Function         bta_pbs_co_getvlist
**
** Description      This function is called to retrieve a vcard list entry for the
**                  specified path.
**                  The first/next directory should be filled by application
**                  into the location specified by p_entry.
**
** Parameters       p_path      - directory to search
**                  p_app_params - Obex application params, NULL if first_item is FALSE
**                  first_item  - TRUE if first get, FALSE if next getvlist
**                                      (p_entry contains previous)
**                  p_entry(input/output)  - Points to prev entry data, the callout application need
**                                           to fill in with the next entry
**
**
** Returns
**                  final - whether it is the final item in the vlist
**                  (tBTA_PBS_CO_STATUS) status of the call.
**                      [BTA_PBS_CO_OK if successful],
**                      [BTA_PBS_CO_FAIL if failed  ]
**
** NOTE             The routines to return the vlist in specific order (indexed or alphanumeric)
**                  is mips intensive and is of O(N-square). If trying to port, please customize
**                  or implement own routines suitable to the platform.
**
**
**
*******************************************************************************/
void bta_pbs_co_getvlist(const char *p_path, tBTA_PBS_VCARDLIST_APP_PARAMS *p_app_params,
                         BOOLEAN first_item, tBTA_PBS_VCARDLIST *p_entry)
{
    tBTUI_PBS_SQL_MSG *p_event_msg;
#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2("bta_pbs_co_getvlist: path %s, first callout %d",
                       p_path, first_item);
#endif

    if (first_item)
    {
        if ((p_event_msg = (tBTUI_PBS_SQL_MSG *)GKI_getbuf(sizeof(tBTUI_PBS_SQL_MSG))) != NULL)
        {
            p_event_msg->getvlist.event = BTUI_PBS_SQL_GET_VLIST;
            p_event_msg->getvlist.p_path = p_path;
            p_event_msg->getvlist.p_app_params = p_app_params;
            p_event_msg->getvlist.first_item = first_item;
            p_event_msg->getvlist.p_entry = p_entry;
            GKI_send_msg(PBS_SQL_TASK, TASK_MBOX_0, p_event_msg);
        }
        else
        {
            LOGE("%s: Failed to allocate message memory", __FUNCTION__);
        }

        return;
    }
    else
    {
        bta_pbs_gen_vlist(p_path, p_app_params, first_item, p_entry);
    }
}

static void pbs_sql_co_getvlist(tBTUI_PBS_GET_VLIST *data)
{
    tBTUI_PBS_PATH  d_path;
    tBTA_PBS_CO_STATUS status = BTA_PBS_CO_FAIL;
    UINT16 list_offset = 0;
    BOOLEAN final = FALSE;
    static UINT8 search_attr = 0;
    static char search_value[64] = "\0";
    tBTA_PBS_VCARDLIST_APP_PARAMS *p_app_params= data->p_app_params;
    tBTA_PBS_VCARDLIST *p_entry= data->p_entry;

#if USE_BCMBTUI == TRUE
    APPL_TRACE_DEBUG2("pbs_sql_co_getvlist: path %s, first callout %d",
                       data->p_path, data->first_item);
#endif
    /* check if the path is valid */
    if (bta_pbs_is_valid_path(data->p_path, BTA_PBS_OPER_PULL_VCARD_LIST, &d_path) == FALSE)
    {
        APPL_TRACE_ERROR1("pbs_sql_co_getvlist: the path %s is invalid", data->p_path);
        status = BTA_PBS_CO_EODIR;
        goto Done;
    }

    if (data->first_item)
    {
#if USE_BCMBTUI == TRUE
        APPL_TRACE_DEBUG6("PBS get vlist max list =%d offset = %d search "
                          "attribute =%d value_len = %d value =%s order=%d",
                          data->p_app_params->max_count, data->p_app_params->start_offset,
                          data->p_app_params->attribute, data->p_app_params->value_len,
                          data->p_app_params->p_value, data->p_app_params->order);
#endif

        memset(&bps_db, 0, sizeof(tBTUI_PBS_RAW_PB));

        if (vcard_list)
        {
            free(vcard_list);
            vcard_list = NULL;
        }
        if (vlist_entry)
        {
            free(vlist_entry);
            vlist_entry = NULL;
        }

        /* remember search att and value */
        if (p_app_params->value_len != 0)
        {
            search_attr = p_app_params->attribute;
            memset(search_value, 0, sizeof(search_value));
            strncpy(search_value, p_app_params->p_value, p_app_params->value_len);
        }
        else
            search_value[0] = '\0';

        if (search_attr == BTA_PBS_ATTR_NUMBER && search_value[0] != '\0')
        {
            if (d_path.path == BTA_PBS_PATH_PHONE_PB)
            {
#ifdef SQLITE_ENABLED
                p_entry->handle[0]= '\0';
                sqlite3_open_v2(bte_appl_cfg.contacts_db, &db, SQLITE_OPEN_READONLY, NULL);

                if (db == NULL)
                {
                    LOGE("%s: Failed to open Android database [%s]", __FUNCTION__, bte_appl_cfg.contacts_db);
                    status = BTA_PBS_CO_FAIL;
                    goto Done;
                }
                bta_pbs_convert_to_valid_search_string(search_value, p_app_params->p_value, p_app_params->value_len);

                if (bta_pbs_app_vcard_from_number(db, d_path, search_value) == FALSE)
                {
                    LOGE("%s: Search using phone number failed", __FUNCTION__);
                    status = BTA_PBS_CO_FAIL;
                    goto Done;
                }
                else
                {
                    if(max_count)
                    {
                        snprintf(p_entry->handle, sizeof(p_entry->handle),
                            "%d.vcf", 0);
                        strcpy(p_entry->name, vlist_entry[0].value);
                        APPL_TRACE_DEBUG0("bta_pbs_co_getvlist: Search successfully returned 1 entry");
                    }
                    else
                    {
                        APPL_TRACE_DEBUG0("bta_pbs_co_getvlist: Search Returned no entries");
                    }
                }
                sqlite3_close(db);
#endif
                final= TRUE;
                status = BTA_PBS_CO_OK;
                goto Done;
            }
        }

        /* current PBAP version doesn't support contacts in SIM card */
        if (bta_pbs_is_for_SIM(data->p_path) == TRUE)
        {
            final = TRUE;
            data->p_entry->handle[0] = '\0';
            status = BTA_PBS_CO_OK;
            goto Done;
        }

#ifdef SQLITE_ENABLED
        sqlite3_open_v2(bte_appl_cfg.contacts_db, &db, SQLITE_OPEN_READONLY, NULL);
#endif
        if (db == NULL) {
            APPL_TRACE_ERROR0("pbs_sql_co_getvlist: cannot open Android database");
            status = BTA_PBS_CO_FAIL;
            goto Done;
        }

        /* only parse vcard for vcard entry only */
        if (d_path.path > 0)
        {
            btui_pbs_cb.fd = d_path.path;

            if (bta_pbs_app_process_pb_vlist( db, d_path ) == FALSE)
            {
#ifdef SQLITE_ENABLED
                sqlite3_close(db);
                APPL_TRACE_DEBUG1( "pbs_sql_co_getvlist():sqlite3_close( db ptr: x%x )", db );
#endif
                status = BTA_PBS_CO_FAIL;
                goto Done;
            }

#ifdef SQLITE_ENABLED
            sqlite3_close(db);
            APPL_TRACE_DEBUG1( "pbs_sql_co_getvlist():2nd:sqlite3_close( db ptr: x%x )", db );
#endif

            bps_db.max_count = max_count;

#if USE_BCMBTUI == TRUE
            APPL_TRACE_DEBUG1("PBS Vcard list has totally %d entries", max_count);
#endif

            if (max_count == 0)
            {
                /* vlist emtry */
                final = TRUE;
                data->p_entry->handle[0] = '\0';
                status = BTA_PBS_CO_OK;
                goto Done;
            }
        }

        if ((vcard_list = (int *)malloc(sizeof(int) * max_count)) == NULL)
        {
            LOGE("%s: Failed to allocate memory for vcard_list!", __FUNCTION__);
            status = BTA_PBS_CO_FAIL;
            goto Done;
        }
        memset(vcard_list, -1, sizeof(vcard_list));

        bta_pbs_sort_vcard_list(data->p_app_params->order);
        /* if this is for PB, then leave 0 as subscriber's phone no. */
        if (d_path.path == BTA_PBS_PATH_PHONE_PB)
        {
#if defined(PBAP_ZERO_VCARD_IN_DB) && (PBAP_ZERO_VCARD_IN_DB == TRUE)
            list_offset = data->p_app_params->start_offset;
#else
            if (data->p_app_params->start_offset > 0)
            {
                list_offset = data->p_app_params->start_offset - 1;
            }
#endif
        }
        else
        {
            list_offset = data->p_app_params->start_offset;
        }

        bps_db_row_index = list_offset;
        max_count = data->p_app_params->max_count;
    }

    return bta_pbs_gen_vlist(data->p_path, data->p_app_params, data->first_item,
                      data->p_entry);
Done:
    memset(&bps_db, 0, sizeof(tBTUI_PBS_RAW_PB));
    if (vcard_list)
    {
        free(vcard_list);
        vcard_list = NULL;
    }
    if (vlist_entry)
    {
        free(vlist_entry);
        vlist_entry = NULL;
    }
    max_count = 0;
    bps_db_row_index = 0;

    /* call the callin function */
    bta_pbs_ci_getvlist(status, final);
}

void pbs_sql_task( void *p )
{
    UINT16  event;
    //UINT8 *p_msg;
    tBTUI_PBS_SQL_MSG *p_msg;

    LOGI("%s started.", __FUNCTION__);
    for (;;)
    {
        event = GKI_wait(0xFFFF, 3000);

        if(event & TASK_MBOX_0_EVT_MASK)
        {
            while ((p_msg = (tBTUI_PBS_SQL_MSG *)GKI_read_mbox(TASK_MBOX_0)) != NULL)
            {
                switch (p_msg->event)
                {
                    case BTUI_PBS_SQL_OPEN:
                        pbs_sql_co_open((tBTUI_PBS_OPEN *)p_msg);
                        break;
                    case BTUI_PBS_SQL_READ:
                        pbs_sql_co_read((tBTUI_PBS_READ *)p_msg);
                        break;
                    case BTUI_PBS_SQL_CLOSE:
                        pbs_sql_co_close((tBTUI_PBS_CLOSE *)p_msg);
                        break;
                    case BTUI_PBS_SQL_GET_VLIST:
                        pbs_sql_co_getvlist((tBTUI_PBS_GET_VLIST *)p_msg);
                        break;
                }
                GKI_freebuf((tBTUI_PBS_SQL_MSG *)p_msg);
            }
        }

        if (event & EVENT_MASK(GKI_SHUTDOWN_EVT))
            break;

    }
    LOGI("%s is exiting.", __FUNCTION__);
}

/*
NOTE:           The four functions below have been copied and modified:
                base64_encode_block, bta_pbs_base64_encode,
                base64_decode_block, bta_pbs_base64_decode.

                This source code may be used as you wish, subject to
                the MIT license.  See the LICENCE section below.

LICENCE:        Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.

                Permission is hereby granted, free of charge, to any person
                obtaining a copy of this software and associated
                documentation files (the "Software"), to deal in the
                Software without restriction, including without limitation
                the rights to use, copy, modify, merge, publish, distribute,
                sublicense, and/or sell copies of the Software, and to
                permit persons to whom the Software is furnished to do so,
                subject to the following conditions:

                The above copyright notice and this permission notice shall
                be included in all copies or substantial portions of the
                Software.

                THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
                KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
                WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
                PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
                OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
                OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
                OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
                SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* Translation Table as described in RFC1113 */
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Translation Table to decode (created by author) */
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/* encode 3 8-bit binary bytes as 4 '6-bit' characters */
static void base64_encode_block( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/*
NOTE    base64 encode a stream adding padding and line breaks as per spec.
        The size of outbuf should be multiple of 4.
*/
static int bta_pbs_base64_encode(void *inbuf, void *outbuf, int datalen, int linesize )
{
    unsigned char *in  = inbuf;
    unsigned char *out = outbuf;
    int n2enc, len, blocksout = 0;

    for (n2enc = datalen; n2enc > 0; n2enc -= len)
    {
        len = n2enc < 3 ? n2enc : 3;
        if (len == 1)
        {
            *(out+1) = 0;   /* padding */
            *(out+2) = 0;
        }
        else if (len == 2)
        {
            *(out+2) = 0;
        }

        base64_encode_block(in, out, len);

        in  += 3;
        out += 4;

        blocksout++;
        if (blocksout >= (linesize/4) || len < 3)
        {
            *out++ = '\r';  /* wrap line */
            *out++ = '\n';
            blocksout = 0;
        }
    }

    if (len == 3)
    {
        *out++ = '\r';  /* wrap line */
        *out++ = '\n';
    }

    return 0;   /* FIXME: make it meaningful */
}

/* decode 4 '6-bit' characters into 3 8-bit binary bytes */
static void base64_decode_block( unsigned char in[4], unsigned char out[3] )
{
    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/* decode a base64 encoded stream discarding padding, line breaks and noise */
static int bta_pbs_base64_decode(void *inbuf, void *outbuf, int inlen)
{
    unsigned char *in  = inbuf;
    unsigned char *out = outbuf;
    unsigned char tmp[4];
    unsigned char v;
    int i, len, n2dec, outlen = 0;

    n2dec = inlen;
    while (n2dec > 0)
    {
        for (len = 0, i = 0; i < 4 && n2dec > 0; i++)
        {
            v = 0;
            while (n2dec > 0 && v == 0)
            {
                v = (unsigned char) *in++;
                v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
                if (v)
                    v = (unsigned char) ((v == '$') ? 0 : v - 61);

                n2dec--;
            }
            if (n2dec > 0)
            {
                len++;
                if (v)
                    tmp[i] = (unsigned char) (v - 1);
            }
            else
                tmp[i] = 0;
        }

        if (len)
        {
            base64_decode_block(tmp, out);

            out += (len - 1);
            outlen += (len -1);
        }
    }
    return outlen;
}
