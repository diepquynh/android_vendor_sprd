/*****************************************************************************
**  Name:           bta_hh_co.c
**
**  Description:    HH (HID Host) call-out function implementation file.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "bt_target.h"

#if (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE))

#include "btui.h"
#include "btui_int.h"
#include "bta_hh_api.h"
#include "btl_ifs.h"
#include "btapp_hh.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>


#ifdef TAG
#undef TAG
#endif
#define TAG "BTA-CO_HH: "

#ifdef LOGI
#undef LOGI
#endif
#define LOGI(format, ...)  fprintf(stdout, TAG format"\n", ## __VA_ARGS__)

#ifdef LOGE
#undef LOGE
#endif
#define LOGE(format, ...)  fprintf(stderr, TAG format"\n", ## __VA_ARGS__)


#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))

#define BTHID_RPT_DSCP  1

#define BTHID_MAX_CTRL_BUF_LEN  1020

typedef struct BTHID_CONTROL
{
    int  size;
    char buf[BTHID_MAX_CTRL_BUF_LEN];
} tBTHID_CONTROL;

#else

static void  process_keyboard_rpt(tBTUI_HH_DEVICE *p_dev, tBTA_HH_KEYBD_RPT *p_rpt);
static void  process_mouse_rpt(tBTA_HH_MICE_RPT *p_rpt);

#endif  /* (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE)) */


/*******************************************************************************
**
** Function      bta_hh_co_open
**
** Description   When connection is opened, this call-out function is executed
**               by HH to do platform specific initialization.
**
** Returns       void.
*******************************************************************************/
void bta_hh_co_open(UINT8 dev_handle, UINT8 sub_class, tBTA_HH_ATTR_MASK attr_mask,
                    UINT8 app_id)
{
    tBTUI_HH_DEVICE *p_dev_cb;

    LOGI("%s: dev_handle = %d, subclass = 0x%02X, attr_mask = 0x%04X, app_id=%d",
         __FUNCTION__, dev_handle, sub_class, attr_mask, app_id);

    p_dev_cb = btapp_hh_find_unused_dev_entry();
    if (p_dev_cb == NULL)
    {
        LOGI("%s: Oops, too many HID devices are connected", __FUNCTION__);
        return;
    }

    p_dev_cb->dev_handle   = dev_handle;
    p_dev_cb->is_connected = TRUE;

#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))
    /* Before opening the device, need make sure the device node /dev/bthid
     * has been created. The device node location could be system dependent.
     */
    p_dev_cb->fd = open("/dev/bthid", O_RDWR);
    if (p_dev_cb->fd < 0)
    {
        LOGE("%s: Failed to open BTHID driver, fd = %d", __FUNCTION__, p_dev_cb->fd);
        return;
    }
#endif  /* (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE)) */
}


/*******************************************************************************
**
** Function      bta_hh_co_close
**
** Description   When connection is closed, this call-out function is executed
**               by HH to do platform specific finalization.
**
** Parameters    dev_handle  - device handle
**                  app_id      - application id
**
** Returns          void.
*******************************************************************************/
void bta_hh_co_close(UINT8 dev_handle, UINT8 app_id)
{
    tBTUI_HH_DEVICE *p_dev_cb;

    LOGI("%s: dev_handle = %d, app_id = %d", __FUNCTION__, dev_handle, app_id);

    p_dev_cb = btapp_hh_find_dev_by_handle(dev_handle);
    if (p_dev_cb == NULL)
    {
        LOGI("%s: Unknown HID device handle %d", __FUNCTION__, dev_handle);
        return;
    }

#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))
    if (p_dev_cb->fd >= 0)
    {
        close(p_dev_cb->fd);
        p_dev_cb->fd = -1;
    }
#endif  /* (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE)) */
}


/*******************************************************************************
**
** Function         bta_hh_co_data
**
** Description      This function is executed by BTA when HID host receive a data
**                  report.
**
** Parameters       dev_handle  - device handle
**                  *p_rpt      - pointer to the report data
**                  len         - length of report data
**                  mode        - Hid host Protocol Mode
**                  sub_clas    - Device Subclass
**                  app_id      - application id
**
** Returns          void
*******************************************************************************/
void bta_hh_co_data(UINT8 dev_handle, UINT8 *p_rpt, UINT16 len, tBTA_HH_PROTO_MODE mode,
                    UINT8 sub_class, UINT8 ctry_code, UINT8 app_id)
{
    tBTUI_HH_DEVICE   *p_dev_cb;
    tBTA_HH_BOOT_RPT  rpt;

    /*
    LOGI("%s: dev_handle = %d, subclass = 0x%02X, len = %d, mode = %d, "
         "ctry_code = %d, app_id = %d",
         __FUNCTION__, dev_handle, sub_class, len, mode, ctry_code, app_id);
    */

    p_dev_cb = btapp_hh_find_dev_by_handle(dev_handle);
    if (p_dev_cb == NULL)
    {
        LOGI("%s: Unknown HID device handle %d", __FUNCTION__, dev_handle);
        return;
    }

#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))
    /* Send the HID report to the kernel. */
    if (p_dev_cb->fd >= 0)
    {
        write(p_dev_cb->fd, p_rpt, len);
    }
#else
    switch (app_id) {
    case BTUI_HH_APP_ID_KB:
    case BTUI_HH_APP_ID_MI:
        if (mode == BTA_HH_PROTO_BOOT_MODE)
        {
            /* Call utility function to parse the boot mode report */
            BTA_HhParseBootRpt(&rpt, p_rpt, len);

            switch (rpt.dev_type) {
            case BTA_HH_KEYBD_RPT_ID:  /* HID keyboard */
                /* Process the keypress by looking up the key conversion table
                 * which could be different from language to language.
                 * Application can pass in different key conversion table depend on
                 * the keyboard language. For undefined country code or English
                 * keyboard, use the default key conversion table
                 *
                 * We only use default conversion table here, ignore the language
                 * and the country code.
                 */
                process_keyboard_rpt(p_dev_cb, &rpt.data_rpt.keybd_rpt);
                break;

            case BTA_HH_MOUSE_RPT_ID: /* HID mouse */
                process_mouse_rpt(&rpt.data_rpt.mice_rpt);
                break;

            default:
                LOGI("%s: Unknown boot-mode HID report(%d) \"%s\", dev_type = %d",
                     __FUNCTION__, len, p_rpt, rpt.dev_type);
                LOGI("%s: %x %x %x %x %x %x %x",
                     __FUNCTION__, p_rpt[0], p_rpt[1], p_rpt[2], p_rpt[3], p_rpt[4],
                     p_rpt[5], p_rpt[6]);
                break;
            }
        }
        else
        {
            LOGI("%s: Unsupported HID mode %d rpt(%d) \"%s\"",
                 __FUNCTION__, mode, len, p_rpt);
        }
        break;

    default:
        LOGI("%s: Unknown HID report app_id %d rpt(%d) \"%s\"",
             __FUNCTION__, app_id, len, p_rpt);
        break;
    }
#endif  /* (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE)) */
}


/*******************************************************************************
**
** Function         bta_hh_co_send_dscp
**
** Description      This function is called in btapp_hh.c to process DSCP received.
**
** Parameters       dev_handle  - device handle
**                  len         - report descriptor length
**                  *p_dscp     - report descriptor
**
** Returns          void
*******************************************************************************/
void bta_hh_co_send_dscp(tBTUI_HH_DEVICE *p_dev, int len, UINT8 *p_dscp)
{
#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))
    tBTHID_CONTROL  ctrl;
#endif

    /*
    int i;
    for (i = 0; i < len; i += 16)
    {
        LOGI("%02X %02X %02X %02X %02X %02X %02X %02X "
             "%02X %02X %02X %02X %02X %02X %02X %02X",
             p_dscp[i],    p_dscp[i+1],  p_dscp[i+2],  p_dscp[i+3],
             p_dscp[i+4],  p_dscp[i+5],  p_dscp[i+6],  p_dscp[i+7],
             p_dscp[i+8],  p_dscp[i+9],  p_dscp[i+10], p_dscp[i+11],
             p_dscp[i+12], p_dscp[i+13], p_dscp[i+14], p_dscp[i+15]);
    }
    */

#if (defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))
    if (len > BTHID_MAX_CTRL_BUF_LEN)
    {
        LOGI("Oops: HID report descriptor is too large. len = %d", len);
        return;
    }

    ctrl.size = len;
    memcpy(ctrl.buf, p_dscp, len);
    ioctl(p_dev->fd, BTHID_RPT_DSCP, &ctrl);
#endif
}


#if !(defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE))

/* Virtual key values */
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_LMENU     0xA4

#define KEYEVENTF_KEYUP  0x2

/* Keyboard scan code translation.
 *
 * Virtual Keys, Standard Set mapping from windows.h
 */
#define BTUI_HH_VK_LBUTTON        0x01           /* left mouse */
#define BTUI_HH_VK_RBUTTON        0x02           /* right mouse */
#define BTUI_HH_VK_MBUTTON        0x04           /* Middle mouse button */

#define BTUI_HH_VK_BACK           0x08           /* same as ASCII */
#define BTUI_HH_VK_TAB            0x09
#define BTUI_HH_VK_RETURN         0x0D
#define BTUI_HH_VK_ESCAPE         0x1B
#define BTUI_HH_VK_SPACE          0x20

/* Windows Virtual Keys Standard Set */
#define BTUI_HH_VK_PRIOR          0x21
#define BTUI_HH_VK_NEXT           0x22
#define BTUI_HH_VK_END            0x23
#define BTUI_HH_VK_HOME           0x24
#define BTUI_HH_VK_LEFT           0x25
#define BTUI_HH_VK_UP             0x26
#define BTUI_HH_VK_RIGHT          0x27
#define BTUI_HH_VK_DOWN           0x28
#define BTUI_HH_VK_SELECT         0x29
#define BTUI_HH_VK_INSERT         0x2D
#define BTUI_HH_VK_DELETE         0x2E
#define BTUI_HH_VK_HELP           0x2F

#define BTUI_HH_VK_QUOTE          0x62           /* " */
#define BTUI_HH_VK_MULTIPLY       0x6A           /* * */
#define BTUI_HH_VK_ADD            0x6B           /* + */
#define BTUI_HH_VK_SUBTRACT       0x6D           /* - */
#define BTUI_HH_VK_DECIMAL        0x6E           /* . */
#define BTUI_HH_VK_DIVIDE         0x6F           /* / */

#define BTUI_HH_VK_SEMICOLON      0xBA           /* ; */
#define BTUI_HH_VK_EQUAL          0xBB           /* = */
#define BTUI_HH_VK_COMMA          0xBC           /* , */
#define BTUI_HH_VK_MINUS          0xBD           /* - */
#define BTUI_HH_VK_PERIOD         0xBE           /* . */
#define BTUI_HH_VK_SLASH          0xBF           /* / */
#define BTUI_HH_VK_BACKQUOTE      0xC0           /* ` */

#define BTUI_HH_VK_LBRACKET       0xDB           /* < */
#define BTUI_HH_VK_BACKSLASH      0xDC           /* \ */
#define BTUI_HH_VK_RBRACKET       0xDD           /* > */
#define BTUI_HH_VK_APOSTROPHE     0xDE           /* ' */

#define BTUI_HH_VK_F1             0x70           /* VK_F1 */
#define BTUI_HH_VK_PRINT          0x2a


/* BTUI_HH_VK_0 thru BTUI_HH_VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39).
 * BTUI_HH_VK_A thru BTUI_HH_VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A).
 *
 * Platform dependant key code translation.
 * Default keycode conversion table : windows virtual key code is used here.
 */
static const UINT8 english_key_tbl[][2] =
{
    {0x00, 92},   /* total number of entry */

    /* {keycode, conv_code} */
    {0x04, 'A'},  /* a- z*/
    {0x05, 'B'},
    {0x06, 'C'},
    {0x07, 'D'},
    {0x08, 'E'},
    {0x09, 'F'},
    {0x0a, 'G'},
    {0x0b, 'H'},
    {0x0c, 'I'},
    {0x0d, 'J'},
    {0x0e, 'K'},
    {0x0f, 'L'},
    {0x10, 'M'},
    {0x11, 'N'},
    {0x12, 'O'},
    {0x13, 'P'},
    {0x14, 'Q'},
    {0x15, 'R'},
    {0x16, 'S'},
    {0x17, 'T'},
    {0x18, 'U'},
    {0x19, 'V'},
    {0x1a, 'W'},
    {0x1b, 'X'},
    {0x1c, 'Y'},
    {0x1d, 'Z'},
    {0x1e, '1'},  /* 1 - 9 */
    {0x1f, '2'},
    {0x20, '3'},
    {0x21, '4'},
    {0x22, '5'},
    {0x23, '6'},
    {0x24, '7'},
    {0x25, '8'},
    {0x26, '9'},
    {0x27, '0'},
    {0x28, BTUI_HH_VK_RETURN},
    {0x29, BTUI_HH_VK_ESCAPE},
    {0x2a, BTUI_HH_VK_BACK  },
    {0x2b, BTUI_HH_VK_TAB   },
    {0x2c, BTUI_HH_VK_SPACE },
    {0x2d, BTUI_HH_VK_MINUS },  /* BTUI_HH_VK_SUBTRACT*/
    {0x2e, BTUI_HH_VK_EQUAL },
    {0x2f, BTUI_HH_VK_LBRACKET  },
    {0x30, BTUI_HH_VK_RBRACKET  },
    {0x31, BTUI_HH_VK_BACKSLASH },
    {0x32, BTUI_HH_VK_BACKSLASH },
    {0x33, BTUI_HH_VK_SEMICOLON },
    {0x34, BTUI_HH_VK_APOSTROPHE},
    {0x35, BTUI_HH_VK_BACKQUOTE },
    {0x36, BTUI_HH_VK_COMMA },
    {0x37, BTUI_HH_VK_PERIOD},
    {0x38, BTUI_HH_VK_SLASH },
    {0x39, 0x39},              /* Caps lock */
    {0x3a, BTUI_HH_VK_F1},     /* F1 */
    {0x3b, BTUI_HH_VK_F1+1},
    {0x3c, BTUI_HH_VK_F1+2},
    {0x3d, BTUI_HH_VK_F1+3},
    {0x3e, BTUI_HH_VK_F1+4},
    {0x3f, BTUI_HH_VK_F1+5},
    {0x40, BTUI_HH_VK_F1+6},
    {0x41, BTUI_HH_VK_F1+7},
    {0x42, BTUI_HH_VK_F1+8},
    {0x43, BTUI_HH_VK_F1+9},
    {0x44, BTUI_HH_VK_F1+10},
    {0x45, BTUI_HH_VK_F1+11},  /* F12 */
    {0x46, BTUI_HH_VK_PRINT},
    {0x49, BTUI_HH_VK_INSERT},
    {0x4a, BTUI_HH_VK_HOME  },
    {0x4b, BTUI_HH_VK_PRIOR },
    {0x4c, BTUI_HH_VK_DELETE},
    {0x4d, BTUI_HH_VK_END   },
    {0x4e, BTUI_HH_VK_NEXT  },
    {0x4f, BTUI_HH_VK_RIGHT },
    {0x50, BTUI_HH_VK_LEFT  },
    {0x51, BTUI_HH_VK_DOWN  },
    {0x52, BTUI_HH_VK_UP    },
    {0x53, 0x53             },  /* num lock */
    {0x54, BTUI_HH_VK_SLASH },  /* BTUI_HH_VK_DIVIDE; */
    {0x55, BTUI_HH_VK_MULTIPLY},
    {0x56, BTUI_HH_VK_SUBTRACT},
    {0x57, BTUI_HH_VK_ADD   },
    {0x58, BTUI_HH_VK_RETURN},  /*  0x58 */
    {0x59, BTUI_HH_VK_END   },  /*  0x59 */
    {0x5a, BTUI_HH_VK_DOWN  },  /* 0x5a: */
    {0x5b, BTUI_HH_VK_NEXT  },  /* 0x5b: */
    {0x5c, BTUI_HH_VK_LEFT  },  /* 0x5c: BTUI_HH_VK_COMMA ?? */
    {0x5e, BTUI_HH_VK_RIGHT },  /* 0x5e: */
    {0x5f, BTUI_HH_VK_HOME  },  /* 0x5f: */
    {0x60, BTUI_HH_VK_UP    },  /* 0x60: */
    {0x61, BTUI_HH_VK_PRIOR },  /* 0x61: */
    {0x62, BTUI_HH_VK_INSERT},  /* 0x62: */
    {0x63, BTUI_HH_VK_DELETE}   /* 0x63: */
};


typedef const UINT8 (*tHH_KEYCODE_TBL)[2];


/*******************************************************************************
**
** Function     lookup_keyconv
**
** Description  This utility function looks up the conversion key from the
**              keycode table for the specified keycode.
**
** Returns      void
*******************************************************************************/
static BOOLEAN lookup_keyconv(UINT8 keycode, tHH_KEYCODE_TBL key_tbl, UINT8 * p_char)
{
    UINT8   num_entry = key_tbl[0][1];
    BOOLEAN found = FALSE;

    for (; num_entry > 0; num_entry--)
    {
        if (key_tbl[num_entry][0] == keycode)
        {
            *p_char = key_tbl[num_entry][1];
            found = TRUE;
            break;
        }
    }

    return found;
}


/*******************************************************************************
**
** Function     keybd_event
**
** Description  Handles a keyboard event.
**
**              This is a Windows API.
**
** Parameters   bVk         - Virtual key code in the range 1 to 254.
**              bScan       - Hardware scan code for the key.
**              dwFlags     - Specifies various aspects of function operation.
**                            This parameter can be one or more of the following values.
**                                KEYEVENTF_EXTENDEDKEY:
**                                    If specified, the scan code is preceded by a prefix
**                                    byte having the value 0xE0 (224).
**                                KEYEVENTF_KEYUP
**                                    If specified, the key is being released.
**                                    If not specified, the key is being depressed.
**              dwExtraInfo - An additional value associated with the key stroke.
**
** Returns      void
*******************************************************************************/
static void keybd_event(UINT8 bVk, UINT8 bScan, UINT32 dwFlags, void* dwExtraInfo)
{
}


/*******************************************************************************
**
** Function     MapVirtualKey
**
** Description  Maps a virtual-key code or scan code to a key.
**
**              This is a Windows API. How this value is interpreted depends on the
**              value of the uMapType parameter.
**
** Parameters   uCode    - Specifies the virtual-key code or scan code for a key.
**                         How this value is interpreted depends on the value of
**                         the uMapType parameter.
**              uMapType - Specifies the translation to perform. The value of this
**                         parameter depends on the value of the uCode parameter.
**
**   MAPVK_VK_TO_VSC
**     uCode is a virtual-key code and is translated into a scan code. If it is a
**     virtual-key code that does not distinguish between left- and right-hand keys,
**     the left-hand scan code is returned. If there is no translation, the function
**     returns 0.
**   MAPVK_VSC_TO_VK
**     uCode is a scan code and is translated into a virtual-key code that does not
**     distinguish between left- and right-hand keys. If there is no translation,
**     the function returns 0.
**   MAPVK_VK_TO_CHAR
**     uCode is a virtual-key code and is translated into an unshifted character
**     value in the low-order word of the return value. Dead keys (diacritics) are
*      indicated by setting the top bit of the return value. If there is no translation,
**     the function returns 0.
**   MAPVK_VSC_TO_VK_EX
**     Windows NT/2000/XP: uCode is a scan code and is translated into a virtual-key
**     code that distinguishes between left- and right-hand keys. If there is no
**     translation, the function returns 0.
**   MAPVK_VK_TO_VSC_EX
**
** Returns      Either a scan code, a virtual-key code, or a character value,
**              depending on the value of uCode and uMapType. If there is no
**              translation, returns zero.
*******************************************************************************/
static UINT32 MapVirtualKey(UINT32 uCode, UINT32 uMapType)
{
    return 0;
}


/*******************************************************************************
**
** Function         process_keyboard_rpt
**
** Description      Process a keyboard HID report and post the key stroke.
**                  Keyboard report carries the keycode sent from HID device,
**                  looking up the key conversion table for different language,
**                  map the keycode into displayable character or keyevent.
**
** Returns          void
*******************************************************************************/
static void process_keyboard_rpt(tBTUI_HH_DEVICE *p_dev, tBTA_HH_KEYBD_RPT *p_rpt)
{
    UINT8    i = 0;
    UINT8    scan_code;
    UINT8    this_char;
    BOOLEAN  is_upper     = p_rpt->mod_key[BTA_HH_MOD_SHFT_KEY];
    BOOLEAN  is_ctrl      = p_rpt->mod_key[BTA_HH_MOD_CTRL_KEY];
    BOOLEAN  is_alt       = p_rpt->mod_key[BTA_HH_MOD_ALT_KEY];
    BOOLEAN  caps_lock    = p_rpt->caps_lock;
    BOOLEAN  *p_pre_ctrl  = &p_dev->pre_mod_key[BTA_HH_MOD_CTRL_KEY];
    BOOLEAN  *p_pre_alt   = &p_dev->pre_mod_key[BTA_HH_MOD_ALT_KEY];
    BOOLEAN  *p_pre_upper = &p_dev->pre_mod_key[BTA_HH_MOD_SHFT_KEY];

    /* Process up to 6 keycode in one report. */
    do {
        /* At least modifier key is processed with the first key code */
        scan_code = p_rpt->this_char[i++];
        this_char = 0;

        /* Convert scan code into keypress by looking up the key conversion table. */
        if (lookup_keyconv(scan_code, english_key_tbl, &this_char))
        {
            /*
            LOGI("%s: map keycode [%02x] -> char [%02x]", __FUNCTION__, scan_code, this_char);
            */
        }
        else if (scan_code != 0)
        {
            LOGI("%s: Cannot interpret scan code 0x%02x", __FUNCTION__, scan_code);
            return;
        }

        // LOGI("%s: i = %d, scan_mode = %02x, this_char = %02x", __FUNCTION__, i - 1, scan_code, this_char);

        if (is_upper)
        {
            if (!*p_pre_upper)
            {
                keybd_event(VK_SHIFT, 0, 0, NULL);
                // LOGI("%s: [Shift] (down)", __FUNCTION__);
            }
        }
        else
        {
            keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, NULL);
            // LOGI("%s: [Shift] (up)", __FUNCTION__);
        }

        if (is_ctrl)
        {
            if (!*p_pre_ctrl)
            {
                keybd_event(VK_CONTROL, 0, 0, NULL);
                // LOGI("%s: [Ctrl] (down)", __FUNCTION__);
            }
        }
        else
        {
            keybd_event(VK_CONTROL, 0,KEYEVENTF_KEYUP, NULL);
            // LOGI("%s: [Ctrl] (up)", __FUNCTION__);
        }

        if (is_alt)
        {
            if (!*p_pre_alt)
            {
                keybd_event(VK_LMENU, MapVirtualKey(VK_LMENU, 0), 0, NULL);
            }
        }
        else
        {
            keybd_event(VK_LMENU, 0,KEYEVENTF_KEYUP, NULL);
            // LOGI("%s: [Alt] (up)", __FUNCTION__);
        }

        if (caps_lock && isalpha(this_char))
        {
            if (is_upper)
            {
                keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP , NULL);
            }
            else
            {
                keybd_event(VK_SHIFT, 0, 0, NULL);
            }

            keybd_event(this_char, 0, 0, NULL);
            keybd_event(this_char, 0, KEYEVENTF_KEYUP , NULL);

            LOGI("%s: char = '%c'", __FUNCTION__, this_char);

            if (is_upper)
            {
                keybd_event(VK_SHIFT, 0, 0, NULL);
            }
            else
            {
                keybd_event(VK_SHIFT, 0,KEYEVENTF_KEYUP , NULL);
            }
        }
        else if (this_char != 0)
        {
            keybd_event(this_char, 0, 0, NULL);
            keybd_event(this_char, 0, KEYEVENTF_KEYUP , NULL);

            if (this_char >= '0' && this_char <= '9')
            {
                LOGI("%s: char = '%c'", __FUNCTION__, this_char);
            }
            else if (this_char >= 'A' && this_char <= 'Z')
            {
                LOGI("%s: char = '%c'", __FUNCTION__, this_char + 'a' - 'A');
            }
            else
            {
                LOGI("%s: char = 0x%02x", __FUNCTION__, this_char);
            }
        }
    } while (i < 6 && p_rpt->this_char[i] != 0);

    *p_pre_ctrl  = is_ctrl;
    *p_pre_upper = is_upper;
    *p_pre_alt   = is_alt;
}


/*******************************************************************************
**
** Function         process_mouse_rpt
**
** Description      Process a mouse HID report.
**
** Returns          void
*******************************************************************************/
static void process_mouse_rpt(tBTA_HH_MICE_RPT *p_rpt)
{
    LOGI("Mouse event: button = %d, delta_x = %d, delta_y = %d",
         p_rpt->mouse_button, p_rpt->delta_x, p_rpt->delta_y);
}

#endif  /* !(defined(HH_USE_BTHID) && (HH_USE_BTHID == TRUE)) */

#endif  /* (defined(BTA_HH_INCLUDED) && (BTA_HH_INCLUDED == TRUE)) */
