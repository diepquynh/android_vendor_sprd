/*****************************************************************************
**
**  Name:           btapp_pr.c
**
**  Description:   Contains application functions for printer
**
**  Copyright (c) 2003-2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bt_target.h"

#if( defined BTA_PR_INCLUDED ) && (BTA_PR_INCLUDED == TRUE)

#include <stdio.h>
// #include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "gki.h"
#include "bta_api.h"
#include "bta_pr_api.h"
#include "bta_op_api.h"
#include "bta_fs_api.h"
#include "bta_fs_co.h"
#include "bd.h"
#include "btapp_pr.h"
#include "btui.h"
#include "btui_int.h"
#include "btl_ifs.h"

#define LOG_TAG "BTAPP_PR:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

#define info(format, ...)  LOGI (format, ## __VA_ARGS__)
#define debug(format, ...) LOGD (format, ## __VA_ARGS__)
#define error(format, ...) LOGE (format, ## __VA_ARGS__)

#define stricmp(a,b) 0

/*****************************************************************************
**  Constants
*****************************************************************************/
#define BTA_FS_PATH_LEN     294
#define BTUI_PR_MAX_PROPS   32

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/

static void btapp_pr_on_rx_ctrl (tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
static void btui_pr_cback (tBTA_PR_EVT event, tBTA_PR *p_data);

/* BTUI PR main control block */
tBTUI_PR_CB btui_pr_cb;

/* BTL-IF Printer handle */
static tCTRL_HANDLE btl_if_pr_handle;

/* Temporary xhtml file for converting vObjects */
const char btapp_pr_xhtml_name[] = "xhtml_tmp.htm";

static const char * const bp_media_type_str[] =
{
    "reserved",
    "stationery",
    "stationery-coated",
    "stationery-inkjet",
    "stationery-preprinted",
    "stationery-letterhead",
    "stationery-prepunched",
    "stationery-fine",
    "stationery-heavyweight",
    "stationery-lightweight",
    "transparency",
    "envelope",
    "envelope-plain",
    "envelope-window",
    "continuous",
    "continuous-long",
    "continuous-short",
    "tab-stock",
    "pre-cut-tabs",
    "full-cut-tabs",
    "multi-part-form",
    "labels",
    "multi-layer",
    "screen",
    "screen-paged",
    "photographic",
    "photographic-glossy",
    "photographic-high-gloss",
    "photographic-semi-gloss",
    "photographic-satin",
    "photographic-matte",
    "photographic-film",
    "back-print-film",
    "cardstock",
    "roll",
    NULL
};

static const char * const bp_job_state[] =
{
    "reserved",
    "Printing",
    "Waiting",
    "Stopped",
    "Completed",
    "Aborted",
    "Cancelled",
    "Unknown",
    NULL
};

static const char * const bp_printer_state[] =
{
    "reserved",
    "Idle",
    "Processing",
    "Stopped",
    NULL
};

static const char * const bp_prst_reason[] =
{
    "reserved",
    "None",
    "Attention Required",
    "Media Jam",
    "Paused",
    "Door Open",
    "Media Low",
    "Media Empty",
    "Out Area Almost Full",
    "Out Area Full",
    "Marker Supply Low",
    "Marker Supply Empty",
    "Marker Failure",
    NULL
};

const char btapp_pr_type_xhtml_xml[] =   "application/vnd.pwg-xhtml-print+xml";
const char btapp_pr_type_pdf[] =         "application/pdf";
const char btapp_pr_type_pcl[] =         "application/vnd.hp-pcl";
const char btapp_pr_type_vcard[] =       "text/x-vcard";
const char btapp_pr_type_vcal[] =        "text/x-vcalendar";
const char btapp_pr_type_vnote[] =       "text/x-vnote";
const char btapp_pr_type_vmsg[] =        "text/x-vmessage";
const char btapp_pr_type_jpg[] =         "image/jpeg";
const char btapp_pr_type_gif[] =         "image/gif";
const char btapp_pr_type_plain_text[] =  "text/plain";
const char btapp_pr_type_multiplexed[] = "application/vnd.pwg-multiplexed";
const char btapp_pr_type_postscript[] =  "application/postscript";

/* Lookup table for obex type headers (order must match BTA_PR_FMT enumeration) */
const char* btapp_pr_type_tbl[] =
{
    btapp_pr_type_vcard,          /* BTA_PR_VCARD_FMT */
    btapp_pr_type_vcal,           /* BTA_PR_VCAL_FMT */
    btapp_pr_type_vcal,           /* BTA_PR_ICAL_FMT */
    btapp_pr_type_vnote,          /* BTA_PR_VNOTE_FMT */
    btapp_pr_type_vmsg,           /* BTA_PR_VMSG_FMT */
    btapp_pr_type_plain_text,     /* BTA_PR_PLAIN_TEXT_FMT */
    btapp_pr_type_pdf,            /* BTA_PR_PDF_FMT */
    btapp_pr_type_pcl,            /* BTA_PR_PCL_FMT */
    btapp_pr_type_multiplexed,    /* BTA_PR_XHTML_MULTIPLEXED_FMT */
    btapp_pr_type_postscript      /* BTA_PR_POSTSCRIPT_FMT */
};

#define BTAPP_PR_TYPE_TBL_SIZE 10

/*******************************************************************************
**
** Function         btapp_pr_init
**
** Description      Initializes Bluetooth Printer module.
**
** Returns          void
*******************************************************************************/
void btapp_pr_init(void)
{
    tBTL_IF_Result result;

    /* Initialize datapath server */
    result = BTL_IF_ServerInit();

    info("Initialized IFS (res = %d)", result);

    /* Register handler for subsystem PR */
    result = BTL_IF_RegisterSubSystem(&btl_if_pr_handle, SUB_PR, NULL, btapp_pr_on_rx_ctrl);

    info("Registered IFS (res = %d)", result);
}

/*******************************************************************************
**
** Function         btapp_pr_enable
**
** Description      Enables BTA Printer module
**
**
** Returns          void
*******************************************************************************/
void btapp_pr_enable(void)
{
    btui_pr_cb.progress_msg_handle = 0;
    btui_pr_cb.getcaps_active = FALSE;
    btui_pr_cb.is_enabled = TRUE;
    btui_cfg.pr_security = BTA_SEC_NONE;

    BTA_PrEnable(btui_cfg.pr_security, btui_pr_cback, 0);
}

/*******************************************************************************
**
** Function         btapp_pr_get_printer_caps
**
** Description      Call BTA_PrGetCaps for requested printer
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_pr_get_printer_caps (BD_ADDR bdaddr)
{
    /* If printer client has been disabled, then exit */
    if (!btui_pr_cb.is_enabled)
    {
        return FALSE;
    }

    btui_pr_cb.getcaps_active = TRUE;

    memset(&btui_pr_cb.getcaps_results, 0, sizeof(tBTA_PR_CAPS));
    BTA_PrGetCaps(bdaddr, btui_pr_cb.services_used);


    return TRUE;
}

/*******************************************************************************
**
** Function         btui_act_get_printer_caps_cback
**
** Description      Handle callback for BTA_PrGetCaps
**
**
** Returns          void
*******************************************************************************/
static void btui_act_get_printer_caps_cback (tBTA_PR_CAPS *p_prcaps)
{
    tBIP_IMAGING_CAPS *p_caps, *p_capd;
    tBTUI_BTA_MSG *p_msg;
    int i, num_fmt_types = 0;
    UINT32 ml, mask;
    char   *p_mime = NULL;

    /* Make sure getcaps wasn't cancelled */
    if (btui_pr_cb.getcaps_active)
    {
        /* Delete GetCaps menu */
        btui_pr_cb.getcaps_active = FALSE;

        /* Store printer capabilities */
        if (p_prcaps)
        {
            memcpy(&btui_pr_cb.getcaps_results, p_prcaps, sizeof(tBTA_PR_CAPS));
            mask = btui_pr_cb.getcaps_results.bp_caps_mask;
            /* convert format supported in BPP SDP record into a list of mime type headers */
            /* Save the document format types */
            if (!btui_cb.pr_doc_fmt_types)
            {
                btui_cb.pr_doc_fmt_types =
                    (char *)GKI_getbuf((UINT16)(sizeof(tBTA_MIME_HDR) * BTUI_MAX_FMT_TYPES));
            }
            memset(btui_cb.pr_doc_fmt_types, 0, sizeof(tBTA_MIME_HDR) * BTUI_MAX_FMT_TYPES);
            p_mime = btui_cb.pr_doc_fmt_types;
            /* convert BPP SDP file format into MIME type header list */
            for (ml = 0; ml < BTAPP_PR_TYPE_TBL_SIZE && mask != 0; ml ++, mask >>= 1)
            {
                if ((mask & 1) &&  num_fmt_types < BTUI_MAX_FMT_TYPES)
                {
                    strncpy(p_mime, btapp_pr_type_tbl[ml], BTA_PA_MIME_TYPE_LEN_MAX);
                    num_fmt_types++;
                    p_mime += sizeof(tBTA_MIME_HDR);
                }
            }


            /* copy the BIP imaging capabilities */
            if(p_prcaps->p_bi_caps)
            {
                info("btui_act_get_printer_caps_cback copying imaging capabilities");
                p_capd  = &btui_pr_cb.bi_caps;
                p_caps  = p_prcaps->p_bi_caps;
                btui_pr_cb.getcaps_results.p_bi_caps    = p_capd;
                memcpy(p_capd, p_caps, sizeof(tBIP_IMAGING_CAPS));
                if(p_capd->num_fmts && p_capd->fmts)
                {
                    if(p_capd->num_fmts > BTA_PR_MAX_BI_FORMATS)
                        p_capd->num_fmts = BTA_PR_MAX_BI_FORMATS;
                    p_capd->fmts    = btui_pr_cb.bi_fmts;
                    memcpy(p_capd->fmts, p_caps->fmts, sizeof(tBIP_IMAGE_FORMAT)*p_capd->num_fmts);
                }
                if(p_capd->num_attfmt && p_capd->attfmt)
                {
                    if(p_capd->num_attfmt > BTA_PR_MAX_BI_FORMATS)
                        p_capd->num_attfmt = BTA_PR_MAX_BI_FORMATS;
                    p_capd->attfmt    = btui_pr_cb.bi_attfmt;
                    memcpy(p_capd->attfmt, p_caps->attfmt, sizeof(tBIP_ATTACH_FORMAT)*p_capd->num_attfmt);
                }
            }
            else
                btui_pr_cb.getcaps_results.services &= ~BTA_BIP_SERVICE_MASK;

            if (p_prcaps->p_bp_pr_attrs)
            {
                tBTA_BP_PR_CAPS *p_attr = p_prcaps->p_bp_pr_attrs;

                /* Display attributes */
                if (p_prcaps->p_bp_pr_attrs->bta_prstat == BTA_PR_OK)
                {
                    info("BPP Capabilities:");
                    info("    Name [%s]", p_attr->p_name);
                    info("    Location [%s]", p_attr->p_loc);
                    info("    BTP [height, width] -> [%d, %d]",
                        (int)p_attr->btp_height, (int)p_attr->btp_width);
                    info("    Max [copies, number up] -> [%d, %d]",
                        (int)p_attr->max_copies, (int)p_attr->max_number_up);
                    info("    Printer [state, reason] -> [%d, %d]",
                        (int)p_attr->state, (int)p_attr->state_reason);
                    info("    Queued jobs -> [%d]", (int)p_attr->queued_jobs);
                    info("    Orientation Mask -> [0x%04x]", p_attr->orient_mask);
                    info("    Sides Mask -> [0x%04x]", p_attr->sides_mask);
                    info("    Quality Mask -> [0x%04x]", p_attr->quality_mask);

                    if (p_attr->color_supported)
                    {
                        info("    Color Supported");
                    }
                    else
                    {
                        info("    Color Not Supported");
                    }

                    if (p_attr->enhanced_supported)
                    {
                        info("    Enhanced Layout Supported");
                    }
                    else
                    {
                        info("    Enhanced Layout Not Supported");
                    }

                    info("    Character Repertoire -> [%02x%02x%02x%02x]",
                        p_attr->char_rep[0], p_attr->char_rep[1],
                        p_attr->char_rep[2], p_attr->char_rep[3]);
                    info("                            [%02x%02x%02x%02x]",
                        p_attr->char_rep[4], p_attr->char_rep[5],
                        p_attr->char_rep[6], p_attr->char_rep[7]);
                    info("                            [%02x%02x%02x%02x]",
                        p_attr->char_rep[8], p_attr->char_rep[9],
                        p_attr->char_rep[10], p_attr->char_rep[11]);
                    info("                            [%02x%02x%02x%02x]",
                        p_attr->char_rep[12], p_attr->char_rep[13],
                        p_attr->char_rep[14], p_attr->char_rep[15]);

                    {
                        tBTA_ATTR_STR_LIST *p_list;
                        BOOLEAN new_fmt;

                        info("    Media types mask (63..0) -> [0x%08x%08x]",
                            (unsigned int)p_attr->mtypes_mask[1],
                            (unsigned int)p_attr->mtypes_mask[0]);
                        for (ml = 1; ml < (UINT32)BTA_MAX_MEDIA_TYPES; ml++)
                        {
                            if (BTA_IS_MTYPE_SUPPORTED(p_attr->mtypes_mask, ml))
                            {
                                info("        [%s]", bp_media_type_str[ml]);
                            }
                        }

                        info("\n");
                        info("    Media Sizes Supported");
                        for (p_list = p_attr->p_media_sizes_list; p_list; p_list = p_list->p_next)
                        {
                            info("        [%s]", p_list->p_str);
                        }

                        info("\n");
                        info("    Image Formats Supported");
                        for (p_list = p_attr->p_img_fmt_list; p_list; p_list = p_list->p_next)
                        {
                            info("        [%s]", p_list->p_str);
                        }

                        info("\n");
                        info("    Document Formats Supported");
                        for (p_list = p_attr->p_doc_fmt_list; p_list; p_list = p_list->p_next)
                        {
                            new_fmt = TRUE;
                            /* parsing a lit of document format attribute, can be expanded in application */
                            for (ml = 0; ml < BTAPP_PR_TYPE_TBL_SIZE; ml ++)
                            {
                                int type_len = 0;
                                type_len = strlen(btapp_pr_type_tbl[ml]);
                                if (strncmp(p_list->p_str, btapp_pr_type_tbl[ml], type_len) == 0)
                                {
                                    if (btui_pr_cb.getcaps_results.bp_caps_mask & BTA_PR_FMT_MASK(ml))
                                        new_fmt = FALSE;
                                    else
                                        btui_pr_cb.getcaps_results.bp_caps_mask|= BTA_PR_FMT_MASK(ml);
                                }

                            }
                            /* Add BPP printer capability supported file format into MIME type header list */
                            if (num_fmt_types < BTUI_MAX_FMT_TYPES && new_fmt)
                            {
                                strncpy(p_mime, p_list->p_str, BTA_PA_MIME_TYPE_LEN_MAX);
                                num_fmt_types++;
                                p_mime += sizeof(tBTA_MIME_HDR);
                            }
                            info("        [%s]", p_list->p_str);
                        }

                        info("\n");
                        info("    Medium Loaded [Type, Size] (%d)", (int)p_attr->num_mloaded);
                        for (ml = 0; ml < p_attr->num_mloaded; ml++)
                        {
                            info("        [%d, %s]",
                                p_attr->mloaded[ml].mtype, p_attr->mloaded[ml].p_msize);
                        }
                    }
                }
                else
                {
                    info("BPP Capabilities FAILED: bpp code 0x%04x",
                        p_prcaps->p_bp_pr_attrs->bpp_prstat);
                }

                /* Load up some default parameters based on printer capabilities */
                btui_pr_cb.bp_parms.copies = 1;
                btui_pr_cb.bp_parms.number_up = 1;
                btui_pr_cb.bp_parms.orient = BTA_BP_PORTRAIT;
                btui_pr_cb.bp_parms.quality = BTA_BP_QUALITY_NORMAL;
                btui_pr_cb.bp_parms.sides = BTA_BP_ONE_SIDED;
                btui_pr_cb.bp_parms.use_precise_job = FALSE;
                btui_pr_cb.bp_parms.use_ref_channel = FALSE;
                btui_pr_cb.bp_parms.media_size[0] = '\0';

                /* Find the first common paper type */
                for (i = BTA_STATIONERY+1; i < BTA_MAX_MEDIA_TYPES; i++)
                {
                    if (BTA_IS_MTYPE_SUPPORTED(p_attr->mtypes_mask, i))
                        btui_pr_cb.bp_parms.media_type = i;
                }

                /* load a default paper size */
                if (p_attr->p_media_sizes_list)
                {
#if 0
                    strncpy(btui_pr_cb.bp_parms.media_size, p_attr->p_media_sizes_list->p_str,
                            BTA_MAX_MEDIA_SIZE_STRING);
#else   /* Look for the 4x6 photo media size first */
                    {
                        tBTA_ATTR_STR_LIST *p_list;
                        for (p_list = p_attr->p_media_sizes_list;
                             p_list; p_list = p_list->p_next)
                        {
                            if (strstr(p_list->p_str, "na_index") &&
                                strstr(p_list->p_str, "4x6"))
                                break;
                        }

                        if (p_list)
                        {
                            strncpy(btui_pr_cb.bp_parms.media_size, p_list->p_str,
                                    BTA_MAX_MEDIA_SIZE_STRING);
                        }
                        else    /* Use the first size found */
                        {
                            strncpy(btui_pr_cb.bp_parms.media_size, p_attr->p_media_sizes_list->p_str,
                                    BTA_MAX_MEDIA_SIZE_STRING);
                        }
                    }
#endif
                    btui_pr_cb.bp_parms.media_size[BTA_MAX_MEDIA_SIZE_STRING] = '\0';
                }

            }
        }
#if 0
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->hdr.event = BTUI_MMI_PR_GETCAPS;
            p_msg->get_caps.services = btui_pr_cb.getcaps_results.services;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);

        }
#endif

    }
}

/*******************************************************************************
**
** Function         btapp_pr_get_file_size
**
** Description      find the file size
**
**
** Returns          UINT32
*******************************************************************************/
UINT32 btapp_pr_get_file_size(char *p_name)
{
#if 0
   struct _stat buf;
   UINT32   size = 0;

   if( _stat( p_name, &buf ) == 0)
   {
       size = buf.st_size;
   }
   debug("btapp_pr_get_file_size: %d", (int)size);

   return size;
#endif
return 0;
}


/*******************************************************************************
**
** Function         btapp_pr_abort
**
** Description      Cancels print job
**
**
** Returns          void
*******************************************************************************/
void btapp_pr_abort(void)
{
    /* Abort the current print job */
    btui_pr_cb.aborting = TRUE;
    BTA_PrAbort();

}

/*******************************************************************************
**
** Function         btapp_pr_check_fmt_support
**
** Description      Returns TRUE if printer supports requested format
**
*******************************************************************************/
static BOOLEAN btapp_pr_check_fmt_support(tBTA_PR_FMT_MASK format_mask)
{
    if ((btui_pr_cb.getcaps_results.bp_caps_mask & format_mask) ||
        (btui_pr_cb.getcaps_results.op_caps_mask & format_mask))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*******************************************************************************
**
** Function         btapp_pr_check_bip_img_desc
**
** Description      Returns TRUE if printer supports requested format
**
*******************************************************************************/
static BOOLEAN btapp_pr_check_bip_img_desc(tBIP_IMAGE_DESC *p_desc)
{
    BOOLEAN supported = FALSE;

    info("btui_pr_check_bip_img_desc");
    if(btui_pr_cb.getcaps_results.p_bi_caps)
        supported = BTA_BicCheckImageFormat(btui_pr_cb.getcaps_results.p_bi_caps, p_desc);

    return supported;
}

/*******************************************************************************
**
** Function         btapp_pr_xhtml_convert
**
** Description      Returns TRUE if vObject successfully converted to xhtml
**
*******************************************************************************/
static BOOLEAN btapp_pr_xhtml_convert(tBTA_PR_FMT_MASK format_mask,
    char p_source_name[], char p_xhtml_file[])
{
#if 0
    tBTA_OP_PROP props[BTUI_PR_MAX_PROPS];
    UINT8 num_props = BTUI_PR_MAX_PROPS;
    UINT8 *p_vobj_buf = NULL, *p_xhtml_buf = NULL;
    UINT16 vobj_len = -1;
    UINT32 xhtml_len;
    long   file_size = 0, pos ;
    tBTA_OP_VCAL vobj_type;
    BOOLEAN retval = FALSE;
    int fd_r, fd_w;
    struct  _stat file_stat;
    int bytes_written;
    tBTA_PR_STATUS status = BTA_PR_OK;

    memset(props, 0, BTUI_PR_MAX_PROPS*sizeof(tBTA_OP_PROP));

    /* Read the source file */
    if (format_mask & (BTA_PR_VCAL_MASK | BTA_PR_VCARD_MASK | BTA_PR_VNOTE_MASK | BTA_PR_PLAIN_TEXT_MASK))
    {
        /* read the length of the file to be converted */
        if ((fd_r = _open (p_source_name, _O_BINARY, 0)) >= 0 &&
            (fd_w = _open (p_xhtml_file, _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY,S_IWRITE)) >= 0)
        {
            _fstat(fd_r, &file_stat);
            file_size = file_stat.st_size;

            while ( file_size > 0)
            {
                /* get the length to be converted each time */
                if (file_size < vobj_len)
                    vobj_len = (UINT16)file_size;

                /* Read file into buffer */
                pos = _lseek(fd_r, 0L, SEEK_CUR);

                /* Allocate a buffer to hold the file */
                if (pos != -1L && (p_vobj_buf = malloc(vobj_len)) != NULL)
                {
                    /* Read the file */
                    _read (fd_r, p_vobj_buf, vobj_len);
                }
                else
                    break;

                /* Finished reading source file into p_vobj_buf. */
                /* Convert to xhtml */
                if (p_vobj_buf)
                {
                    if (format_mask & BTA_PR_VCAL_MASK)
                    {
                        /* Parse the vobject into table of tBTA_OP_PROP properties */
                        if (BTA_OpParseCal(props, &num_props, p_vobj_buf, vobj_len, &vobj_type) == BTA_OP_OK)
                        {
                            /* Get size of xhtml buffer required */
                            BTA_PrCal2Xhtml(NULL, &xhtml_len, props, num_props);

                            /* Allocate memory for xhtml buffer, and perform the conversion */
                            if ((p_xhtml_buf = malloc(xhtml_len)) != NULL)
                            {
                                /* Convert vobj into xhtml buf */
                                status = BTA_PrCal2Xhtml(p_xhtml_buf, &xhtml_len, props, num_props);
                            }
                        }
                    }
                    else if (format_mask & BTA_PR_VCARD_MASK)
                    {
                        /* Parse the vobject into table of tBTA_OP_PROP properties */
                        if (BTA_OpParseCard(props, &num_props, p_vobj_buf, vobj_len) == BTA_OP_OK)
                        {
                            /* Get size of xhtml buffer required */
                            BTA_PrCard2Xhtml(NULL, &xhtml_len, props, num_props);

                            /* Allocate memory for xhtml buffer, and perform the conversion */
                            if ((p_xhtml_buf = malloc(xhtml_len)) != NULL)
                            {
                                /* Convert vobj into xhtml buf */
                                status = BTA_PrCard2Xhtml(p_xhtml_buf, &xhtml_len, props, num_props);
                            }
                        }
                    }
                    else if (format_mask & BTA_PR_VNOTE_MASK)
                    {
                        /* Parse the vobject into table of tBTA_OP_PROP properties */
                        if (BTA_OpParseNote(props, &num_props, p_vobj_buf, vobj_len) == BTA_OP_OK)
                        {
                            /* Get size of xhtml buffer required */
                            BTA_PrNote2Xhtml(NULL, &xhtml_len, props, num_props);

                            /* Allocate memory for xhtml buffer, and perform the conversion */
                            if ((p_xhtml_buf = malloc(xhtml_len)) != NULL)
                            {
                                /* Convert vobj into xhtml buf */
                                status = BTA_PrNote2Xhtml(p_xhtml_buf, &xhtml_len, props, num_props);
                            }
                        }
                    }
                    else if (format_mask & BTA_PR_PLAIN_TEXT_MASK)
                    {
                        /* Get size of xhtml buffer required */
                        BTA_PrText2Xhtml(NULL, &xhtml_len, p_vobj_buf, vobj_len);

                        /* Allocate memory for xhtml buffer, and perform the conversion */
                        if ((p_xhtml_buf = malloc(xhtml_len)) != NULL)
                        {
                            /* Convert vobj into xhtml buf */
                            status = BTA_PrText2Xhtml(p_xhtml_buf, &xhtml_len, p_vobj_buf, vobj_len);
                        }
                    }

                    free(p_vobj_buf);
                } /* Convert p_vobj_buf to xhtml */

                /* Finished with conversion */
                /* Save the xhtml buffer to a file */
                if (p_xhtml_buf)
                {
                    if (status == BTA_PR_OK)
                    {
                        pos = _lseek(fd_w, 0L, SEEK_CUR);
                        if ( pos != -1L )               /* Read file into buffer */
                        {
                            bytes_written = _write (fd_w, p_xhtml_buf, xhtml_len);
                        }
                    }
                    free(p_xhtml_buf);
                }

                /* adjust converted lenth */
                file_size -= vobj_len;
                vobj_len = -1;
            }/* end while */

            /* close read/write files after convertion done*/
            _close(fd_r);
            _close(fd_w);

            if (file_size == 0)
                retval = TRUE;
        }

    }/* mask check */

    return (retval);
#endif
return 0;
}

/*******************************************************************************
**
** Function         btapp_act_print_file
**
** Description      Sends the file
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_act_print_file (char *p_name)
{
#if 0
    tBTA_SERVICE_MASK services = (BTA_OPP_SERVICE_MASK | BTA_BPP_SERVICE_MASK);
    tBTA_PR_FMT format;
    tBTA_MIME_HDR   mime_type;
    char  *p_mime;
    BOOLEAN use_xhtml = FALSE;
    BOOLEAN fmt_supported = TRUE;
    char xhtml_filename[BTA_FS_PATH_LEN];
    char *p_char, *p_enc = "JPEG";
    tBTA_PR_BP_PARAMS *p_bp_param = NULL;
    tBIP_IMAGE_DESC  image;
    tBIP_IMAGE_DESC *p_desc = NULL;//&image;
    UINT16      h, w;
    int fmts;

    mime_type[0] = '\0';
    if (btui_pr_cb.getcaps_results.p_bp_pr_attrs != NULL)
        p_bp_param = &btui_pr_cb.bp_parms;

    /* Get temporary xhtml-print filename (used for converting vObjects) */
    sprintf(xhtml_filename, "%s\\%s", btui_cfg.root_path, btapp_pr_xhtml_name);

    p_char = strrchr(p_name, '.');

    p_char++;                               /* point past the . */

    if(!stricmp(p_char, "vcf"))
    {
        format = BTA_PR_VCARD_FMT;

        /* If selected printer does not support VCARD, then convert to xhtml-print */
        if (!btapp_pr_check_fmt_support(BTA_PR_VCARD_MASK))
        {
            /* Printer does not support requested format */
            if (!btapp_pr_check_fmt_support(BTA_PR_XHTML_PRINT_MASK) ||
                !(use_xhtml = btapp_pr_xhtml_convert(BTA_PR_VCARD_MASK, p_name, xhtml_filename)))
               fmt_supported = FALSE;
        }

        if (fmt_supported && !use_xhtml)    /* If part of document type supported list (BPP) retrieve MIME Type */
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "vcard") || strstr(p_mime, "vCard") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "vcd"))
    {
        format = BTA_PR_VCARD_FMT;

        /* Check if selected printer supports VCARD */
        if (!btapp_pr_check_fmt_support(BTA_PR_VCARD_MASK))
        {
            /* Printer does not support requested format */
            fmt_supported = FALSE;
        }
        else    /* If part of document type supported list (BPP) retrieve MIME Type */
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "vcard") || strstr(p_mime, "vCard") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "vcs"))
    {
        format = BTA_PR_VCAL_FMT;

        /* If selected printer does not support VCAL, then convert to xhtml-print */
        if (!btapp_pr_check_fmt_support(BTA_PR_VCAL_MASK))
        {
            /* Printer does not support requested format */
            if (!btapp_pr_check_fmt_support(BTA_PR_XHTML_PRINT_MASK) ||
                !(use_xhtml = btapp_pr_xhtml_convert(BTA_PR_VCAL_MASK, p_name, xhtml_filename)))
               fmt_supported = FALSE;
        }

        if (fmt_supported  && !use_xhtml)    /* If part of document type supported list (BPP) retrieve MIME Type */
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "vcal") || strstr(p_mime, "vCal") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "ics"))
    {
        format = BTA_PR_ICAL_FMT;

        /* Check if selected printer supports ICAL */
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_ICAL_MASK)) == TRUE)
        {
            /* If part of document type supported list (BPP) retrieve MIME Type */
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "icalendar") || strstr(p_mime, "iCalendar") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "vnt"))
    {
        format = BTA_PR_VNOTE_FMT;

        /* If selected printer does not support VNOTE, then convert to xhtml-print */
        if (!btapp_pr_check_fmt_support(BTA_PR_VNOTE_MASK))
        {
            /* Printer does not support requested format */
            if (!btapp_pr_check_fmt_support(BTA_PR_XHTML_PRINT_MASK) ||
                !(use_xhtml = btapp_pr_xhtml_convert(BTA_PR_VNOTE_MASK, p_name, xhtml_filename)))
               fmt_supported = FALSE;
        }

        if (fmt_supported && !use_xhtml)    /* If part of document type supported list (BPP) retrieve MIME Type */
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "vnote") || strstr(p_mime, "vNote") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "vmg"))
    {
        format = BTA_PR_VMSG_FMT;

        /* Check if selected printer supports VMSG */
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_VMSG_MASK)) == TRUE)
        {
            /* If part of document type supported list (BPP) retrieve MIME Type */
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "vmessage") || strstr(p_mime, "vMessage") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if (!stricmp(p_char, "gif"))
    {
        if(btui_gif_scan_header (p_name, &h, &w) == BTUI_IMG_ERR_NONE)
        {
            p_desc  = &image;
            p_enc = "GIF";
        }
        format = BTA_PR_IMAGE_FMT;
        fmt_supported = btapp_pr_check_fmt_support(BTA_PR_GIF_MASK);
    }
    else if (!stricmp(p_char, "bmp"))
    {
        if(btui_bmp_scan_header(p_name, &h, &w ) == BTUI_IMG_ERR_NONE)
        {
            p_desc  = &image;
            p_enc = "BMP";
        }
        format = BTA_PR_IMAGE_FMT;

        /* bmp is not predefined type, check to see if supt other type */
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_OTHER_MASK)) == TRUE)
        {
            /* BPP supported document format */
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                     fmts < BTUI_MAX_FMT_TYPES && p_mime;
                     fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr((const char *)p_mime, "bmp") && strstr(p_mime, "image") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if (!stricmp(p_char, "jpg") || !stricmp(p_char, "jpeg"))
    {
        format = BTA_PR_IMAGE_FMT;

        fmt_supported = btapp_pr_check_fmt_support(BTA_PR_JPEG_MASK);

        /* set up the image descriptor */
        if(btui_jpg_scan_header(p_name, &h, &w ) == BTUI_IMG_ERR_NONE)
        {
            p_desc  = &image;
        }

        for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
        {
            if (strstr((const char *)p_mime, "jpeg") && strstr(p_mime, "image") )
            {
                strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                break;
            }
        }
    }
    else if(!stricmp(p_char, "dpof"))
    {
        format = BTA_PR_DPOF_FMT;
    }
    else if(!stricmp(p_char, "mx"))
    {
        format = BTA_PR_XHTML_MULTIPLEXED_FMT;
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_XHTML_MULTIPLEXED_MASK))== TRUE)
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                     fmts < BTUI_MAX_FMT_TYPES && p_mime;
                     fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "multiplexed"))
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if (!stricmp(p_char, "htm") || !stricmp(p_char, "html") ||
             !stricmp(p_char, "xhtml"))
    {
        format = BTA_PR_XHTML_PRINT_FMT;

        /* Check if selected printer supports XHTML */
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_XHTML_PRINT_MASK)) == TRUE)
        /* If part of document type supported list (BPP) retrieve MIME Type */
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "xhtml-print+xml"))
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "txt"))
    {
        format = BTA_PR_PLAIN_TEXT_FMT;

        /* If selected printer does not support plain text, then convert to xhtml-print */
        if (!btapp_pr_check_fmt_support(BTA_PR_PLAIN_TEXT_MASK))
        {
            /* Printer does not support requested format */
            if (!btapp_pr_check_fmt_support(BTA_PR_XHTML_PRINT_MASK) ||
                !(use_xhtml = btapp_pr_xhtml_convert(BTA_PR_PLAIN_TEXT_MASK, p_name, xhtml_filename)))
               fmt_supported = FALSE;
        }

        if (fmt_supported && !use_xhtml)    /* If part of document type supported list (BPP) retrieve MIME Type */
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                 fmts < BTUI_MAX_FMT_TYPES && p_mime;
                 fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "plain") && strstr(p_mime, "text"))
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "pdf"))
    {
        format = BTA_PR_PDF_FMT;
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_PDF_MASK)) == TRUE)
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                     fmts < BTUI_MAX_FMT_TYPES && p_mime;
                     fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "pdf") || strstr(p_mime, "PDF") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else if(!stricmp(p_char, "ps"))
    {
        format = BTA_PR_POSTSCRIPT_FMT;
        fmt_supported = btapp_pr_check_fmt_support(BTA_PR_POSTSCRIPT_MASK);
    }
    else if(!stricmp(p_char, "pcl"))
    {
        format = BTA_PR_PCL_FMT;
        if ((fmt_supported = btapp_pr_check_fmt_support(BTA_PR_PCL_MASK)) == TRUE)
        {
            for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
                     fmts < BTUI_MAX_FMT_TYPES && p_mime;
                     fmts++, p_mime += sizeof(tBTA_MIME_HDR))
            {
                if (strstr(p_mime, "PCL") || strstr(p_mime, "pcl") )
                {
                    strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                    break;
                }
            }
        }
    }
    else
    {
        /* If file extenstion is not recognized. */
        /* If OPP supports 'all formats' BTA_PR will send file to printer. */
        /* If printer does not support file extension, it will discard the */
        /* file and send a notification.                                   */
        format = BTA_PR_OTHER_FMT;
        fmt_supported = TRUE;
    }

    /* If vObject has been converted to xhtml, then print xhtml file instead */
    if (use_xhtml)
    {
        /* replace file name and format */
        p_name = xhtml_filename;
        format = BTA_PR_XHTML_PRINT_FMT;
        /* retreive MIME type */
        for (fmts = 0, p_mime = btui_cb.pr_doc_fmt_types;
             fmts < BTUI_MAX_FMT_TYPES && p_mime;
             fmts++, p_mime += sizeof(tBTA_MIME_HDR))
        {
            if (strstr(p_mime, "xhtml-print+xml"))
            {
                strncpy(mime_type, p_mime, BTA_PA_MIME_TYPE_LEN_MAX);
                break;
            }
        }
    }
    /* use OPP for printing service */
    if (fmt_supported && strlen((const char *)mime_type) == 0 &&
        format != BTA_PR_IMAGE_FMT)
    {
        if (btui_pr_cb.getcaps_results.services & BTA_OPP_SERVICE_MASK)
            services = BTA_OPP_SERVICE_MASK;
        else
            services = 0;
    }

    if( format == BTA_PR_DPOF_FMT)
    {
        if(btui_pr_cb.getcaps_results.bi_features & BIP_FT_ADV_IMG_PRINT)
            services = BTA_BIP_SERVICE_MASK;
        else
            fmt_supported = FALSE;
    }
    else if( format == BTA_PR_IMAGE_FMT && p_desc != NULL)
    {
        /* set up the image descriptor */
        memset(p_desc, 0, sizeof(tBIP_IMAGE_DESC));
        strcpy((char *)p_desc->encoding, p_enc);
        p_desc->pixel.h = h;
        p_desc->pixel.w = w;
        p_desc->size = btapp_pr_get_file_size(p_name);
        if(btapp_pr_check_bip_img_desc(p_desc) == TRUE)
        {
            fmt_supported = TRUE;
            services |= BTA_BIP_SERVICE_MASK;
        }
        else
        {
            /* the image format is not supported by BIP */
            p_desc = NULL;
        }
    }


    /* Verify that requested service(s) have not been disabled by btui_cfg */
    services &= (btui_pr_cb.services_used);

    /* Print the object */
    if ((fmt_supported) && (services))
    {
        /* Reset the aborting flag */
        btui_pr_cb.aborting = FALSE;

/*** DAHDEBUG ***/
/* File used for testing referenced object */
        if (p_bp_param && strstr(p_name, "12refobj_test.htm"))
        {
            p_bp_param->use_ref_channel = TRUE;
            debug("BPP Referenced Object File");
        }
/* File used for testing referenced object */
        if (p_bp_param && strstr(p_name, "unspt_attr.htm"))
        {
            debug("BPP Unsupported Job Attribute testing: 2 copies, 3 number up");
            p_bp_param->copies = 2;
            p_bp_param->number_up = 3;
        }
/*** DAHDEBUG ***/

        /* Print the object */
        BTA_PrPrint(btui_cb.p_selected_rem_device->bd_addr,
                    services,
                    format,
                    mime_type,
                    p_name,
                    p_desc,
                    p_bp_param);

        btui_cb.ui_state = UI_SENDING_OP_FILE;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif
    return 0;
}

/*******************************************************************************
**
** Function         btapp_pr_service_str
**
** Description
**
**
** Returns          void
*******************************************************************************/
static char *btapp_pr_service_str (UINT8 service_id)
{
    char * p_str;
    switch(service_id)
    {
    case BTA_OPP_SERVICE_ID:
        p_str = "OPP";
        break;
    case BTA_BPP_SERVICE_ID:
        p_str = "BPP";
        break;
    case BTA_BIP_SERVICE_ID:
        p_str = "BIP";
        break;
    default:
        p_str = "unknown";
    }
    return p_str;
}

/*******************************************************************************
**
** Function         ExtractBulkInt16
**
** Description      A helper function for deserializing integers into a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkInt16 (UINT16* extracted_int, char* bulk_array, int current_index)
{
    UINT32 temp_int;

    memcpy (&temp_int, bulk_array+current_index, sizeof(UINT32));
    current_index += sizeof(UINT32);
    *extracted_int = (UINT16)(temp_int & 0x0000FFFF);
    return current_index;
}

/*******************************************************************************
**
** Function         ExtractBulkInt32
**
** Description      A helper function for deserializing integers from a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkInt32 (UINT32 *extracted_int, char *bulk_array, int current_index)
{
    memcpy (extracted_int, bulk_array+current_index, sizeof(UINT32));
    current_index += sizeof(UINT32);
    return current_index;
}

/*******************************************************************************
**
** Function         ExtractBulkByte
**
** Description      A helper function for deserializing bytes from a bulk array
**
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkByte (UINT8 *extracted_byte, char *bulk_array, int current_index)
{
    memcpy (extracted_byte, bulk_array+current_index, sizeof(UINT8));
    current_index += sizeof(UINT8);
    return current_index;
}

/*******************************************************************************
**
** Function         ExtractBulkFixedByteArray
**
** Description      A helper function for deserializing strings from a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkFixedByteArray (UINT8 *extracted_array, char *bulk_array,
                               int current_index, int fixed_length)
{
    UINT32 stored_length;

    /* Find length of encoded array. */
    current_index = ExtractBulkInt32(&stored_length, bulk_array, current_index);

    /* Sanity check. */
    if ((UINT32)fixed_length != stored_length)
    {
        debug("ExtractBulkFixedByteArray: Required %d, Found %d",
            fixed_length, (int)stored_length);
    }

    /* Extract encoded array to existing buffer. */
    memcpy(&(extracted_array[0]), bulk_array + current_index, stored_length);
    current_index += stored_length;
    return current_index;
}

/*******************************************************************************
**
** Function         ExtractBulkString
**
** Description      A helper function for deserializing strings from a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkString (char **extracted_string, char *bulk_array, int current_index)
{
    int i;
    UINT32 string_length;

    current_index = ExtractBulkInt32(&string_length, bulk_array, current_index);

    if (string_length > 0) {
        *extracted_string = (char*)malloc(string_length);
        memcpy(*extracted_string, bulk_array + current_index, string_length);
        current_index += string_length;
    } else {
        *extracted_string = NULL;
    }

    return current_index;
}

/*******************************************************************************
**
** Function         ExtractBulkBipParameters
**
** Description      A helper function for deserializing BipParameters object
**                  from a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkBipParameters (tBTA_PR_BI_DESC **extracted_struct,
                              char *bulk_array, int current_index)
{
    UINT32 exists = 0;

    /* Does struct exist? */
    current_index = ExtractBulkInt32(&exists, bulk_array, current_index);

    if (0 != exists) {
        /* Populate existing structure with stored data. */
        memcpy(*extracted_struct, bulk_array+current_index, sizeof(tBTA_PR_BI_DESC));
        current_index += sizeof(tBTA_PR_BI_DESC);
    } else {
        *extracted_struct = NULL;
    }

    return current_index;
}

/*******************************************************************************
**
** Function         ExtractBulkBppParameters
**
** Description      A helper function for deserializing BppParameters object
**                  from a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int ExtractBulkBppParameters (tBTA_PR_BP_PARAMS** extracted_struct,
    char* bulk_array, int current_index)
{
    UINT32 exists = 0;

    /* Does struct exist? */
    current_index = ExtractBulkInt32(&exists, bulk_array, current_index);

    if (0 != exists) {
        /* Populate existing structure with stored data. */
        memcpy(*extracted_struct, bulk_array+current_index, sizeof(tBTA_PR_BP_PARAMS));
        current_index += sizeof(tBTA_PR_BP_PARAMS);
    } else {
        *extracted_struct = NULL;
    }

    return current_index;
}

/*******************************************************************************
**
** Function         AppendBulkInt
**
** Description      A helper function for serializing integers into a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendBulkInt (int new_int, char* bulk_array, int index)
{
    memcpy (bulk_array + index, &new_int, sizeof(new_int));
    index += sizeof(new_int);
    return index;
}

/* A helper function for serializing bytes into a bulk array.
    Returns the index of the next item to append. */
int AppendBulkByte(UINT8 new_byte, char* bulk_array, int index) {

    bulk_array[index++] = new_byte;

    return index;
}

/*******************************************************************************
**
** Function         AppendBipImageFormatStruct
**
** Description      A helper function for serializing BIP Image Format Structs
**                  into a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendBipImageFormatStruct(tBIP_IMAGE_FORMAT *st, char* bulk_data, int index)
{
    if (st != NULL) {
        memcpy(bulk_data + index, st, sizeof(tBIP_IMAGE_FORMAT));
        index += sizeof(tBIP_IMAGE_FORMAT);
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendBipAttachFormatStruct
**
** Description      A helper function for serializing BIP Attachment Format
**                  Structs into a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendBipAttachFormatStruct(tBIP_ATTACH_FORMAT *st, char* bulk_data, int index)
{
    if (st != NULL) {
        memcpy(bulk_data + index, st, sizeof(tBIP_ATTACH_FORMAT));
        index += sizeof(tBIP_ATTACH_FORMAT);
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendBipCapsStruct
**
** Description      A helper function for serializing BIP Capabilities Struct
**                  into a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendBipCapsStruct(tBIP_IMAGING_CAPS *st, char* bulk_data, int index)
{
    int i = 0;

    /* Check for presence of BIP capabilities struct. */
    if (st != NULL) {
        memcpy(bulk_data + index, st, sizeof(tBIP_IMAGING_CAPS));
        index += sizeof(tBIP_IMAGING_CAPS);

        /* Check for presence of BIP subfields. */
        for (i = 0; i < (st->num_fmts); i++) {
            index = AppendBipImageFormatStruct(&(st->fmts[i]), bulk_data, index);
        }
        for (i = 0; i < (st->num_attfmt); i++) {
            index = AppendBipAttachFormatStruct(&(st->attfmt[i]), bulk_data, index);
        }
    } else {
        LOGI("No BIP Attributes detected");
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendStructString
**
** Description      A helper function for serializing structure based strings
**                  into a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendStructString (char* st, char* bulk_data, int index, int max_length)
{
    int i = 0;

    if (st != NULL) {
        for (i = 0; i<max_length; i++) {
            if (0 == st[i])
                break;
        }
        if (i != max_length) {
            i++;
        }
        /* Store length of string including NULL terminator. */
        index = AppendBulkInt(i ,bulk_data, index);
        /* Store string. */
        memcpy(bulk_data+index, st, i);
        /* Track next free block address. */
        index += i;
        /* Null terminate. (In case of overflow) */
        bulk_data[index-1] = 0;
    } else {
        index = AppendBulkInt(0 ,bulk_data, index);
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendAttrListNode
**
** Description      A helper function for serializing string attribute list
**                  nodes into a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendAttrListNode (tBTA_ATTR_STR_LIST* st, char* bulk_data, int index,
    int max_string_length)
{
    /* Check for presence of struct. */
    if (st != NULL) {
        /* Store actual Node structure. */
        memcpy(bulk_data + index, st, sizeof(tBTA_ATTR_STR_LIST));
        index += sizeof(tBTA_ATTR_STR_LIST);
        /* Store string from this node. */
        index = AppendStructString(st->p_str, bulk_data, index, max_string_length);
        /* Check for presence of BIP subfields. (Recursive linked listed parsing!)*/
        index = AppendAttrListNode((tBTA_ATTR_STR_LIST*)st->p_next, bulk_data, index, max_string_length);
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendBpAttrStruct
**
** Description      A helper function for serializing BPP attribute structure
**                  into a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendBpAttrStruct (tBTA_BP_PR_CAPS *st, char* bulk_data, int index)
{
    int i;

    /* Check for presence of BPP capabilities struct. */
    if (st != NULL) {

        memcpy(bulk_data + index, st, sizeof(tBTA_BP_PR_CAPS));
        index += sizeof(tBTA_BP_PR_CAPS);

        /* Check for presence of BPP LoadedMedia strings. */
        for (i = 0; i<BTA_MAX_MEDIA_LOADED; i++) {
            index = AppendStructString(st->mloaded[i].p_msize, bulk_data, index, 128);
        }

        /* Check for presence of BPP subfields. */
        index = AppendStructString(st->p_name, bulk_data, index, 128);
        index = AppendStructString(st->p_loc, bulk_data, index, 128);

        /* Append all the formats, media sizes etc. */
        index = AppendAttrListNode(st->p_doc_fmt_list, bulk_data, index, 128);
        index = AppendAttrListNode(st->p_media_sizes_list, bulk_data, index, 128);
        index = AppendAttrListNode(st->p_img_fmt_list, bulk_data, index, 128);

        /* This section is currently disabled since the BTLIF does not have the necessary bandwidth.*/
    } else {
        LOGI("No BPP Attributes detected");
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendCapsStruct
**
** Description      A helper function for serializing GetCaps data to a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendCapsStruct(tBTA_PR *pr_params, char* bulk_data, int index)
{
    LOGI("Serializing printer capabilities.");

    /* Store main struct. */
    memcpy(bulk_data + index, pr_params, sizeof(tBTA_PR));
    index += sizeof(tBTA_PR);

    index = AppendBipCapsStruct(pr_params->caps.p_bi_caps, bulk_data, index);
    index = AppendBpAttrStruct(pr_params->caps.p_bp_pr_attrs, bulk_data, index);

    return index;
}

/*******************************************************************************
**
** Function         AppendStructByteArray
**
** Description      A helper function for serializing structure based byte array
**                  to a bulk array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendStructByteArray (UINT8* st, char* bulk_data, int index, int array_length)
{
    if (st != NULL) {
        /* Store array. */
        memcpy(bulk_data+index, st, array_length);
        /* Track next free block address. */
        index += array_length;
    }

    return index;
}

/*******************************************************************************
**
** Function         AppendAuthStruct
**
** Description      A helper function for serializing Auth data to a bulk array.
**
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendAuthStruct (tBTA_PR *pr_params, char* bulk_data, int index)
{
    /* Store main struct. */
    memcpy(bulk_data + index, pr_params, sizeof(tBTA_PR));
    index += sizeof(tBTA_PR);

    /* Append realm array. */
    index = AppendStructByteArray(pr_params->auth.p_realm, bulk_data, index, (int)pr_params->auth.realm_len);

    return index;
}

/*******************************************************************************
**
** Function         AppendGetObjStruct
**
** Description      A helper function for serializing Object structure to a bulk
**                  array.
**
** Returns          Returns the index of the next item to extract.
*******************************************************************************/
int AppendGetObjStruct (tBTA_PR *pr_params, char* bulk_data, int index)
{
    /* Store main struct. */
    memcpy(bulk_data + index, pr_params, sizeof(tBTA_PR));
    index += sizeof(tBTA_PR);

   /* Append name string. */
    index = AppendStructString(pr_params->p_name, bulk_data, index, 128);

    return index;
}

/*******************************************************************************
**
** Function         btapp_pr_on_rx_ctrl
**
** Description      Process received BTL-IF control messages
**
**
** Returns          void
*******************************************************************************/
static void btapp_pr_on_rx_ctrl (tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id,
    tBTL_PARAMS *params)
{
    /* Update handle to app using returned handle. */
    btl_if_pr_handle = fd;

    switch (id)
    {
        case BTLIF_PR_ENABLE:
            btapp_pr_enable();
            break;

        case BTLIF_PR_DISABLE:
            BTA_PrDisable();
            break;

        case BTLIF_PR_GET_CAPS:
        {
            int index = 0;
            BD_ADDR address;
            UINT32 services;

            index = ExtractBulkFixedByteArray(address, params->pr_bulk_data, index, BD_ADDR_LEN);
            index = ExtractBulkInt32(&services, params->pr_bulk_data, index);
            BTA_PrGetCaps(address, services);
            break;
        }

        case BTLIF_PR_AUTH_RSP:
        {
            int index = 0;
            char *p_password;
            char *p_userid;

            /* Deserialize params. */
            index = ExtractBulkString(&p_password, params->pr_bulk_data, index);
            index = ExtractBulkString(&p_userid, params->pr_bulk_data, index);

            BTA_PrAuthRsp(p_password, p_userid);
            if (p_password) free(p_password);
            if (p_userid) free(p_userid);
            break;
        }

        case BTLIF_PR_PRINT:
        {
            int index = 0;
            UINT8 format;
            BD_ADDR address;
            UINT32 services, handle;
            char *p_header, *p_name;
            tBTA_PR_BI_DESC bipParameters, *p_BipParameters = &bipParameters;
            tBTA_PR_BP_PARAMS bppParameters, *p_BppParameters = &bppParameters;

            index = ExtractBulkFixedByteArray(address, params->pr_bulk_data, index, BD_ADDR_LEN);
            index = ExtractBulkInt32(&services, params->pr_bulk_data, index);
            index = ExtractBulkByte(&format, params->pr_bulk_data, index);
            index = ExtractBulkString(&p_header, params->pr_bulk_data, index);
            index = ExtractBulkString(&p_name, params->pr_bulk_data, index);
            index = ExtractBulkInt32(&handle, params->pr_bulk_data, index);
            index = ExtractBulkBipParameters(&p_BipParameters, params->pr_bulk_data, index);
            index = ExtractBulkBppParameters(&p_BppParameters, params->pr_bulk_data, index);

            info("Address:  %02x-%02x-%02x-%02x-%02x-%02x",
                address[0], address[1], address[2], address[3], address[4], address[5]);
            info("Services: 0x%04x", (unsigned int)services);
            info("Format:   %d", format);
            info("MimeType: %s", p_header);
            info("Name:     %s", p_name);

            if (NULL != p_BipParameters) {
                info("*** BIP Parameters ***");
                info("Size:      %d", (int)p_BipParameters->size);
                info("Pixel-X:   %d", p_BipParameters->pixel.w);
                info("Pixel-Y:   %d", p_BipParameters->pixel.h);
                info("Pixel-X2:  %d", p_BipParameters->pixel.w2);
                info("Pixel-Y2:  %d", p_BipParameters->pixel.h2);
                info("Transform: %d", p_BipParameters->transform);
            }

            if (NULL != p_BppParameters) {
                info("*** BPP Parameters ***");
                info("Copies:      %d", (int)p_BppParameters->copies);
                info("Num-up:      %d", (int)p_BppParameters->number_up);
                info("MediaSize:   %s", p_BppParameters->media_size);
                info("MediaType:   %d", (int)p_BppParameters->media_type);
                info("Orientation: %d", (int)p_BppParameters->orient);
                info("Sides:       %d", (int)p_BppParameters->sides);
                info("Quality:     %d", (int)p_BppParameters->quality);
                info("Ref Channel: %s", (p_BppParameters->use_ref_channel?"Yes":"No"));
                info("Precise Job: %s", (p_BppParameters->use_precise_job?"Yes":"No"));
            }

            BTA_PrPrint(address, (tBTA_SERVICE_MASK)services, (tBTA_PR_FMT)format,
                p_header, p_name, p_BipParameters, p_BppParameters);
            if (p_header) free(p_header);
            if (p_name) free(p_name);
            break;
        }

        case BTLIF_PR_PARTIAL_IMAGE_RSP:
        {
            int index = 0;
            char *p_name;

            /* Deserialize params. */
            index = ExtractBulkString(&p_name, params->pr_bulk_data, index);

            BTA_PrPartialImageRsp(p_name);
            if (p_name) free(p_name);
            break;
        }

        case BTLIF_PR_REF_OBJ_RSP:
        {
            int index = 0;
            char *p_name;

            /* Deserialize params. */
            index = ExtractBulkString(&p_name, params->pr_bulk_data, index);

            BTA_PrRefObjRsp(p_name);
            if (p_name) free(p_name);
            break;
        }

        case BTLIF_PR_ABORT:
            btapp_pr_abort();
            break;

        case BTLIF_PR_CANCEL_BP_STATUS:
            BTA_PrCancelBpStatus();
            break;

        case BTLIF_PR_CARD_TO_XHTML:
            break;

        case BTLIF_PR_CAL_TO_XHTML:
            break;

        case BTLIF_PR_NOTE_TO_XHTML:
            break;

        default:
            debug("%s: Command not supported (%d)",__FUNCTION__, id);
            break;
    }
}

/*******************************************************************************
**
** Function         btui_pr_cback
**
** Description      Decodes events from Bluetooth Printer stack and re-encodes
**                  for transmission over BTL-IF back to application.
**
** Returns          void
*******************************************************************************/
static void btui_pr_cback(tBTA_PR_EVT event, tBTA_PR *p_data)
{
    char message[BTA_FS_PATH_LEN];
    char *p_file = NULL;
    int  n, length;
    tBTA_PR_CAPS    *p_pr_caps;
//    struct _stat buf;
    tBTUI_BTA_MSG *p_msg;
    tBTL_PARAMS btl_if_params;
    tBTLIF_PR_PROGRESS_EVT_PARAM prog_param;

    switch (event)
    {
    case BTA_PR_ENABLE_EVT:
        btui_pr_cb.is_enabled = TRUE;
        info("Printer Client ENABLED");

        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_ENABLE_EVT, NULL, 0);
        break;

    case BTA_PR_OPEN_EVT:
        info("Printer Client CONNECTED: %s",
            btapp_pr_service_str(p_data->service));
        btui_pr_cb.bytes_transferred = 0;
        btui_pr_cb.auth_already_failed_once = FALSE;
#if 0
        if (p_data->service == BTA_BPP_SERVICE_ID)
        {
            BTA_PrCancelBpStatus(); /* Don't wait for status after spooling */
        }
#endif
        memcpy(btl_if_params.pr_bulk_data, p_data, sizeof(tBTA_SERVICE_ID));
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_OPEN_EVT, &btl_if_params, sizeof(tBTA_SERVICE_ID));
        break;

    case BTA_PR_AUTH_EVT:
#if 0
        if (!btui_pr_cb.auth_already_failed_once)
        {
            btui_pr_cb.auth_already_failed_once = TRUE;
            BTA_PrAuthRsp(btui_cfg.pr_password, btui_cfg.pr_userid);
        }
        else
        {
            error("Printer Client AUTH Failure (pin %s)...",
                              btui_cfg.pr_password);
            btui_pr_cb.auth_already_failed_once = FALSE;
            BTA_PrAbort(); /* Close the connection because pin code didn't match */
        }
#endif
        length = AppendAuthStruct(p_data, btl_if_params.pr_bulk_data, 0);
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_AUTH_EVT, &btl_if_params, length);
        break;

    case BTA_PR_PROGRESS_EVT:
        btui_pr_cb.bytes_transferred += p_data->prog.bytes;
        if (p_data->prog.obj_size != BTA_FS_LEN_UNKNOWN)
        {
            info("Printer Client PROGRESS (%d of %d total)...",
                (int)btui_pr_cb.bytes_transferred, (int)p_data->prog.obj_size);
        }
        else
        {
            info("Printer Client PROGRESS (%d bytes total)...",
                (int)btui_pr_cb.bytes_transferred);
        }
#if 0
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->hdr.event = BTUI_MMI_PR_PROGRESS;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);

        }
#endif
        prog_param.bytes = btui_pr_cb.bytes_transferred;
        prog_param.obj_size = p_data->prog.obj_size;
        memcpy(btl_if_params.pr_bulk_data, &prog_param, sizeof(tBTLIF_PR_PROGRESS_EVT_PARAM));
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_PROGRESS_EVT, &btl_if_params, sizeof(tBTLIF_PR_PROGRESS_EVT_PARAM));
        break;

    case BTA_PR_GETCAPS_EVT:
        info("Printer Client Caps services: 0x%X, bpp doc support: 0x%X, opp doc support: 0x%X, bip features: 0x%X",
                        (unsigned int)p_data->caps.services, (unsigned int)p_data->caps.bp_caps_mask,
                        (unsigned int)p_data->caps.op_caps_mask, (unsigned int)p_data->caps.bi_features);
        p_pr_caps = &p_data->caps;
        btui_pr_cb.auth_already_failed_once = FALSE;
        n = 0;
        message[0] = 0;
        if(p_pr_caps->bi_features & (BIP_FT_IMG_PUSH|BIP_FT_IMG_PUSH_PRINT))
            n = sprintf(message, "Image Push");
        if(p_pr_caps->bi_features & (BIP_FT_ADV_IMG_PRINT))
        {
            if(n)
                n = sprintf(&message[n], "and ");
            sprintf(&message[n], "Advanced Image Print");
        }

        if (message[0] != 0)
        {
            info("Supported BIP feature(s): %s.", message);
        }
#if 0
        if (p_pr_caps->p_bi_caps) {
            info("*** Printer BIP Caps ***");
            info("\tPreference - Size: %d Pixel(%d, %d, %d, %d) Encoding: %s Transform: %d",
                (int)p_pr_caps->p_bi_caps->prefer.size,
                p_pr_caps->p_bi_caps->prefer.pixel.w, p_pr_caps->p_bi_caps->prefer.pixel.h,
                p_pr_caps->p_bi_caps->prefer.pixel.w2, p_pr_caps->p_bi_caps->prefer.pixel.h2,
                p_pr_caps->p_bi_caps->prefer.encoding, p_pr_caps->p_bi_caps->prefer.transform);
            info("\tFormat - Size: %d Pixel(%d, %d, %d, %d) Encoding: %s",
                (int)p_pr_caps->p_bi_caps->fmts->size,
                p_pr_caps->p_bi_caps->fmts->pixel.w, p_pr_caps->p_bi_caps->fmts->pixel.h,
                p_pr_caps->p_bi_caps->fmts->pixel.w2, p_pr_caps->p_bi_caps->fmts->pixel.h2,
                p_pr_caps->p_bi_caps->fmts->encoding);
            info("\tAttFormat - Type: %s Charset: %s",
                p_pr_caps->p_bi_caps->attfmt->type, p_pr_caps->p_bi_caps->attfmt->charset);
            info("\tFilter - Created: %d Modified: %d Encoding: %d Pixel: %d",
                p_pr_caps->p_bi_caps->filter.created, p_pr_caps->p_bi_caps->filter.modified,
                p_pr_caps->p_bi_caps->filter.encoding, p_pr_caps->p_bi_caps->filter.pixel);
            info("\tPres: %d NumFmts: %d NumAttFmts: %d",
                p_pr_caps->p_bi_caps->pres, p_pr_caps->p_bi_caps->num_fmts, p_pr_caps->p_bi_caps->num_attfmt);
            info("\tDPOF - Std: %d Index: %d Multiple: %d SpSize: %d NumSets: %d ChStamp: %d Trim: %d",
                p_pr_caps->p_bi_caps->dpof.std, p_pr_caps->p_bi_caps->dpof.idx,
                p_pr_caps->p_bi_caps->dpof.mul, p_pr_caps->p_bi_caps->dpof.siz,
                p_pr_caps->p_bi_caps->dpof.qty, p_pr_caps->p_bi_caps->dpof.dsc,
                p_pr_caps->p_bi_caps->dpof.trm);
        }
#endif
        if (p_pr_caps->p_bp_pr_attrs) {
            info("*** Printer BPP Caps ***");
            info("\tName: %s", p_pr_caps->p_bp_pr_attrs->p_name);
            info("\tBTAStatus: %d BPPStatus: %d MaxCopies: %d MaxNumUp: %d",
                p_pr_caps->p_bp_pr_attrs->bta_prstat, p_pr_caps->p_bp_pr_attrs->bpp_prstat,
                (int)p_pr_caps->p_bp_pr_attrs->max_copies, (int)p_pr_caps->p_bp_pr_attrs->max_number_up);
            info("\tQueuedJobs: %d BTPWidth: %d BTPHeight: %d NumMedia: %d",
                (int)p_pr_caps->p_bp_pr_attrs->queued_jobs, (int)p_pr_caps->p_bp_pr_attrs->btp_width,
                (int)p_pr_caps->p_bp_pr_attrs->btp_height, (int)p_pr_caps->p_bp_pr_attrs->num_mloaded);
            info("\tPrState: %d StateReason: %d Sides: %d Quality: %d Orientation: %d Color: %d Enhanced: %d",
                p_pr_caps->p_bp_pr_attrs->state, p_pr_caps->p_bp_pr_attrs->state_reason,
                p_pr_caps->p_bp_pr_attrs->sides_mask, p_pr_caps->p_bp_pr_attrs->quality_mask,
                p_pr_caps->p_bp_pr_attrs->orient_mask, p_pr_caps->p_bp_pr_attrs->color_supported,
                p_pr_caps->p_bp_pr_attrs->enhanced_supported);
            // UINT32          mtypes_mask[BTA_MTYPE_ARRAY_INDEX]; /* List of media types supported (bitmask). */
            // tBTA_UINT128    char_rep;                /* Char repertoires supported. See BPP spec. sect 12.2.3 */
            // tBTA_1284_INFO ID1284_info;     /* 1284 device ID string parsed information */
            // tBTA_PR_MLOADED mloaded[BTA_MAX_MEDIA_LOADED];  /* Array of loaded media */
            // tBTA_ATTR_STR_LIST  *p_doc_fmt_list;     /* List of document formats supported */
            // tBTA_ATTR_STR_LIST  *p_media_sizes_list; /* List of media sizes supported. NULL if none */
            // tBTA_ATTR_STR_LIST  *p_img_fmt_list;     /* List of image formats supported. NULL if none */
        }

        btui_act_get_printer_caps_cback((tBTA_PR_CAPS *)p_data);

        length = AppendCapsStruct(p_data, btl_if_params.pr_bulk_data, 0);
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_GETCAPS_EVT, &btl_if_params, length);
        break;

    case BTA_PR_THUMBNAIL_EVT:
        btui_pr_cb.bytes_transferred = 0;
#if 0
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->hdr.event = BTUI_MMI_PR_THUMBNAIL;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);

        }
#endif
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_THUMBNAIL_EVT, NULL, 0);
        break;

    case BTA_PR_PARTIAL_IMAGE_EVT:
        debug("ui iname: %s", p_data->part.iname);
#if 0
        sprintf(message, "%s\\%s\\%s", btui_cfg.root_path, BTUI_PR_FILEPATH, p_data->part.iname);
        if( _stat( message, &buf ) == 0)
            p_file = message;
        BTA_PrPartialImageRsp(p_file);
#endif
        memcpy(btl_if_params.pr_bulk_data, p_data, sizeof(tBIP_GET_PART_REQ_EVT));
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_PARTIAL_IMAGE_EVT, &btl_if_params,
            sizeof(tBIP_GET_PART_REQ_EVT));
        break;

    case BTA_PR_CLOSE_EVT:
        btui_pr_cb.auth_already_failed_once = FALSE;
#if 0
        if ((p_msg = (tBTUI_BTA_MSG *)GKI_getbuf(sizeof(tBTUI_BTA_MSG))) != NULL)
        {
            p_msg->hdr.event = BTUI_MMI_PR_CLOSE;
            p_msg->close.status = p_data->status;
            GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, p_msg);

        }
#endif
        info("Printer Client DISCONNECTED (status %d)", (int)p_data->status);
        memcpy(btl_if_params.pr_bulk_data, p_data, sizeof(tBTA_PR_STATUS));
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_CLOSE_EVT, &btl_if_params, sizeof(tBTA_PR_STATUS));
        break;

    case BTA_PR_JOB_STATUS_EVT:
        {
            tBTA_PR_JOB_STATUS *p_js = &p_data->bp_job_status;

            info("\n");
            info("Printer Client BP JOB STATUS:");
            info("    Job State: (%d) [%s]", p_js->job_state,
                bp_job_state[p_js->job_state]);
            info("    Printer State: (%d) [%s]", p_js->printer_state,
                bp_printer_state[p_js->printer_state]);
            info("    Printer State Reason: (%d) [%s]", p_js->printer_state_reason,
                bp_prst_reason[p_js->printer_state_reason]);
            memcpy(btl_if_params.pr_bulk_data, p_js, sizeof(tBTA_PR_JOB_STATUS));
            BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
                BTLIF_PR_JOB_STATUS_EVT, &btl_if_params, sizeof(tBTA_PR_JOB_STATUS));
            break;
        }

    case BTA_PR_BP_DOC_CMPL_EVT:
        info("\n");
        if (p_data->status != BTA_PR_OK)
        {
            error("Printer Client BP Document Not Sent (Error: %d):",
                p_data->status);
        }
        else
        {
            info("Printer Client BP Document Sent (Spooled):");
        }
        memcpy(btl_if_params.pr_bulk_data, p_data, sizeof(tBTA_PR_STATUS));
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_BP_DOC_CMPL_EVT, &btl_if_params,
            sizeof(tBTA_PR_STATUS));
        break;

    case BTA_PR_GET_OBJ_EVT:
        info("Ref Obj Name: %s", p_data->p_name);
#if 0
        sprintf(message, "%s\\%s\\%s", btui_cfg.root_path, BTUI_PR_FILEPATH,
                p_data->p_name);
        BTA_PrRefObjRsp(message);
#endif
        length = AppendGetObjStruct(p_data, btl_if_params.pr_bulk_data, 0);
        BTL_IF_CtrlSend(btl_if_pr_handle, SUB_PR,
            BTLIF_PR_GET_OBJ_EVT, &btl_if_params, length);
        break;

    default:
        info("Unexpected event received (%d)", event);
        break;
    }
}

#endif
