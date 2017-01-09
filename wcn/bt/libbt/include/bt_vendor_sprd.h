/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_vendor_brcm.h
 *
 *  Description:   A wrapper header file of bt_vendor_lib.h
 *
 *                 Contains definitions specific for interfacing with Broadcom
 *                 Bluetooth chipsets
 *
 ******************************************************************************/

#ifndef BT_VENDOR_SPRD_H
#define BT_VENDOR_SPRD_H

#include "bt_vendor_lib.h"
#include "vnd_buildcfg.h"
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <cutils/properties.h>

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

/* Run-time configuration file */
#ifndef VENDOR_LIB_CONF_FILE
#define VENDOR_LIB_CONF_FILE "/system/etc/connectivity_configure.ini"
#define VENDOR_BA_LIB_CONF_FILE  "/system/etc/marlinba/connectivity_configure.ini"
#endif

/* Device port name where Bluetooth controller attached */
#ifndef BLUETOOTH_UART_DEVICE_PORT
#define BLUETOOTH_UART_DEVICE_PORT      "/dev/sttybt0"    /* maguro */
#endif

/* Location of firmware patch files */
#ifndef FW_PATCHFILE_LOCATION
#define FW_PATCHFILE_LOCATION "/vendor/firmware/"  /* maguro */
#endif

#ifndef DATMISC_MAC_ADDR_PATH
#define DATMISC_MAC_ADDR_PATH "/data/misc/bluedroid/btmac.txt"
#endif

#ifndef UART_TARGET_BAUD_RATE
#define UART_TARGET_BAUD_RATE           3000000
#endif


/* sleep mode

    0: disable
    1: UART with Host wake/BT wake out of band signals
*/
#ifndef LPM_SLEEP_MODE
#define LPM_SLEEP_MODE                  1
#endif

/* Host Stack Idle Threshold in 300ms or 25ms 

  In sleep mode 1, this is the number of firmware loops executed with no
    activity before the Host wake line is deasserted. Activity includes HCI
    traffic excluding certain sleep mode commands and the presence of SCO
    connections if the "Allow Host Sleep During SCO" flag is not set to 1.
    Each count of this parameter is roughly equivalent to 300ms or 25ms.
*/
#ifndef LPM_IDLE_THRESHOLD
#define LPM_IDLE_THRESHOLD              1
#endif

/* Host Controller Idle Threshold in 300ms or 25ms

    This is the number of firmware loops executed with no activity before the
    HC is considered idle. Depending on the mode, HC may then attempt to sleep.
    Activity includes HCI traffic excluding certain sleep mode commands and
    the presence of ACL/SCO connections.
*/
#ifndef LPM_HC_IDLE_THRESHOLD
#define LPM_HC_IDLE_THRESHOLD           1
#endif

/* BT_WAKE Polarity - 0=Active Low, 1= Active High */
#ifndef LPM_BT_WAKE_POLARITY
#define LPM_BT_WAKE_POLARITY            1    /* maguro */
#endif

/* HOST_WAKE Polarity - 0=Active Low, 1= Active High */
#ifndef LPM_HOST_WAKE_POLARITY
#define LPM_HOST_WAKE_POLARITY          1    /* maguro */
#endif

/* LPM_ALLOW_HOST_SLEEP_DURING_SCO

    When this flag is set to 0, the host is not allowed to sleep while
    an SCO is active. In sleep mode 1, the device will keep the host
    wake line asserted while an SCO is active.
    When this flag is set to 1, the host can sleep while an SCO is active.
    This flag should only be set to 1 if SCO traffic is directed to the PCM
    interface.
*/
#ifndef LPM_ALLOW_HOST_SLEEP_DURING_SCO
#define LPM_ALLOW_HOST_SLEEP_DURING_SCO 1
#endif

/* LPM_COMBINE_SLEEP_MODE_AND_LPM

    In Mode 0, always set byte 7 to 0. In sleep mode 1, device always
    requires permission to sleep between scans / periodic inquiries regardless
    of the setting of this byte. In sleep mode 1, if byte is set, device must
    have "permission" to sleep during the low power modes of sniff, hold, and
    park. If byte is not set, device can sleep without permission during these
    modes. Permission to sleep in Mode 1 is obtained if the BT_WAKE signal is
    not asserted.
*/
#ifndef LPM_COMBINE_SLEEP_MODE_AND_LPM
#define LPM_COMBINE_SLEEP_MODE_AND_LPM  1
#endif

/* LPM_ENABLE_UART_TXD_TRI_STATE

    When set to 0, the device will not tristate its UART TX line before going
    to sleep.
    When set to 1, the device will tristate its UART TX line before going to
    sleep.
*/
#ifndef LPM_ENABLE_UART_TXD_TRI_STATE
#define LPM_ENABLE_UART_TXD_TRI_STATE   0
#endif

/* LPM_PULSED_HOST_WAKE
*/
#ifndef LPM_PULSED_HOST_WAKE
#define LPM_PULSED_HOST_WAKE            0
#endif

/* LPM_IDLE_TIMEOUT_MULTIPLE

    The multiple factor of host stack idle threshold in 300ms/25ms
*/
#ifndef LPM_IDLE_TIMEOUT_MULTIPLE
#define LPM_IDLE_TIMEOUT_MULTIPLE       10
#endif

#ifndef BT_VRITUAL_USERIAL_INTERFACE
#define BT_VRITUAL_USERIAL_INTERFACE FALSE
#endif

/* BT_WAKE_VIA_USERIAL_IOCTL

    Use userial ioctl function to control BT_WAKE signal
*/
#ifndef BT_WAKE_VIA_USERIAL_IOCTL
#define BT_WAKE_VIA_USERIAL_IOCTL       FALSE
#endif

/* BT_WAKE_USERIAL_LDISC

    Use line discipline if the BT_WAKE control is in line discipline
*/
#ifndef BT_WAKE_USERIAL_LDISC
#define BT_WAKE_USERIAL_LDISC           FALSE
#endif

/* BT_WAKE_VIA_PROC

    LPM & BT_WAKE control through PROC nodes
*/
#ifndef BT_WAKE_VIA_PROC
#define BT_WAKE_VIA_PROC       FALSE
#endif

#ifndef BT_WAKE_VIA_PROC_NOTIFY_DEASSERT
#define  BT_WAKE_VIA_PROC_NOTIFY_DEASSERT       FALSE
#endif

/* N_SPRD_HCI

    UART ioctl line discipline
*/
#ifndef N_SPRD_HCI
#define N_SPRD_HCI             25
#endif

/* SCO_CFG_INCLUDED

    Do SCO configuration by default. If the firmware patch had been embedded
    with desired SCO configuration, set this FALSE to bypass configuration
    from host software.
*/
#ifndef SCO_CFG_INCLUDED
#define SCO_CFG_INCLUDED                FALSE
#endif

/* HW_END_WITH_HCI_RESET

    Sample code implementation of sending a HCI_RESET command during the epilog
    process. It calls back to the callers after command complete of HCI_RESET
    is received.
*/
#ifndef HW_END_WITH_HCI_RESET
#define HW_END_WITH_HCI_RESET    TRUE
#endif

#ifndef BT_VND_STACK_PRELOAD
#define BT_VND_STACK_PRELOAD TRUE
#endif

#ifndef BT_WCND_POWER_CTRL
#define BT_WCND_POWER_CTRL TRUE
#endif

#ifndef BT_SITM_SERVICE
#define BT_SITM_SERVICE FALSE
#endif

#ifndef BT_RFKILL_CTRL
#define BT_RFKILL_CTRL FALSE
#endif


/******************************************************************************
**  Extern variables and functions
******************************************************************************/

extern bt_vendor_callbacks_t *bt_vendor_cbacks;

/******************************************************************************
**  SPRD DATA
******************************************************************************/

#define HCI_CMD_MAX_LEN 258

#define HCI_RESET 0x0C03
#define HCI_CMD_PREAMBLE_SIZE 3
#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE 5
#define HCI_EVT_CMD_CMPL_OPCODE 3

typedef unsigned char BOOLEAN;

#define UNUSED(x) (void)(x)
#define CONF_ITEM_TABLE(ITEM, ACTION, BUF, LEN) \
  { #ITEM, ACTION, &(BUF.ITEM), LEN, (sizeof(BUF.ITEM) / LEN) }

#define STREAM_TO_UINT8(u8, p) \
  {                            \
    u8 = (uint8_t)(*(p));      \
    (p) += 1;                  \
  }
#define STREAM_TO_UINT16(u16, p)                                \
  {                                                             \
    u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
    (p) += 2;                                                   \
  }
#define UINT8_TO_STREAM(p, u8) \
  { *(p)++ = (uint8_t)(u8); }
#define UINT16_TO_STREAM(p, u16)    \
  {                                 \
    *(p)++ = (uint8_t)(u16);        \
    *(p)++ = (uint8_t)((u16) >> 8); \
  }
#define UINT32_TO_STREAM(p, u32)     \
  {                                  \
    *(p)++ = (uint8_t)(u32);         \
    *(p)++ = (uint8_t)((u32) >> 8);  \
    *(p)++ = (uint8_t)((u32) >> 16); \
    *(p)++ = (uint8_t)((u32) >> 24); \
  }

typedef int(conf_action_t)(char *p_conf_name, char *p_conf_value, void *buf,
                           int len, int size);

typedef int(pskey_preload_t)(void *arg);
typedef void(epilog_process_t)(void);
typedef void(pskey_dump_t)(void *arg);
typedef char *(get_conf_file_t)(void);

typedef struct {
    const char *conf_entry;
    conf_action_t *p_action;
    void *buf;
    int len;
    int size;
} conf_entry_t;

typedef struct {
    char *name;
    void *pskey;
    conf_entry_t *tab;
    pskey_preload_t *pskey_preload;
    epilog_process_t *epilog_process;
    pskey_dump_t *pskey_dump;
    get_conf_file_t *get_conf_file;
} bt_adapter_module_t;

#endif /* BT_VENDOR_SPRD_H */

