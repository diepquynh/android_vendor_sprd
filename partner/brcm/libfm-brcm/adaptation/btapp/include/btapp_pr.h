/*****************************************************************************
**
**  Name:             btapp_pr.h
**
**  Description:     This file contains btui internal interface
**				     definition
**
**  Copyright (c) 2000-2005, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_PR_H
#define BTAPP_PR_H


#if (defined BTA_OP_INCLUDED) && (BTA_OP_INCLUDED == TRUE)
#include "bta_op_api.h"
#endif

#if (defined BTA_PR_INCLUDED) && (BTA_PR_INCLUDED == TRUE)
#include "bta_pr_api.h"
#endif

/* BTUI pr control block */
#define BTA_PR_MAX_BI_FORMATS         4
#define BTA_PR_MAX_BP_DOC_FMTS_LIST   10                /* Maximum number of document formats lists to save */
#define BTA_PR_MAX_BP_DOC_FMTS_LEN    31                /* Maximum number of each document format string */
#define BTA_PR_MAX_BP_IMG_FMTS_LIST   5                 /* Maximum number of image formats lists to save */
#define BTA_PR_MAX_BP_IMG_FMTS_LEN    31                /* Maximum number of each image format string */
#define BTA_PR_MAX_BP_MEDIA_SIZE_LIST 30                /* Maximum number of media size strings to save */
#define BTA_PR_MAX_BP_MEDIA_SIZE_LEN  31                /* Maximum length of each media size string */

typedef struct
{
     UINT32         services_used;                      /* Mask of services to use when printing */
     UINT32         bytes_transferred;
     BOOLEAN        dev_stored;
     BOOLEAN        aborting;
     BOOLEAN        is_enabled;
     UINT8          num_files;
     UINT8          *p_otherfmt;
     UINT32         progress_msg_handle;
     BOOLEAN        getcaps_active;
     UINT32         getcaps_menu_handle;
     tBTA_PR_CAPS   getcaps_results;
     BOOLEAN        auth_already_failed_once;
     tBIP_IMAGING_CAPS    bi_caps;            /* BIP imaging capabilities */
     tBIP_IMAGE_FORMAT    bi_fmts[BTA_PR_MAX_BI_FORMATS];  /* image format can be retrieved by other devices */
     tBIP_ATTACH_FORMAT  bi_attfmt[BTA_PR_MAX_BI_FORMATS]; /* supported attachment formats */
     tBTA_PR_BP_PARAMS  bp_parms;                          /* bpp printer parameters */
} tBTUI_PR_CB;

extern tBTUI_PR_CB btui_pr_cb;

extern void btapp_pr_init(void);
extern BOOLEAN btapp_pr_get_printer_caps (BD_ADDR bdaddr);
extern void btapp_pr_abort(void);
extern BOOLEAN btapp_act_print_file (char *p_name);




#endif
