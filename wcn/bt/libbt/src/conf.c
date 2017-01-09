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
 *  Filename:      conf.c
 *
 *  Description:   Contains functions to conduct run-time module configuration
 *                 based on entries present in the .conf file
 *
 ******************************************************************************/

#define LOG_TAG "bt_vnd_conf"

#include <utils/Log.h>
#include <string.h>
#include "bt_vendor_sprd.h"
#include "comm.h"


/******************************************************************************
**  Externs
******************************************************************************/
int userial_set_port(char *p_conf_name, char *p_conf_value, int param);
/******************************************************************************
**  Local type definitions
******************************************************************************/

#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t#"
#define CONF_VALUES_PARTITION " ,=\n\r\t#"
#define CONF_MAX_LINE_LEN 255

#define MAC_ADDR_FILE_LEN 25
#define MAC_ADDR_LEN 6
#define MAC_ADDR_BUF_LEN (strlen("FF:FF:FF:FF:FF:FF"))

/******************************************************************************
**  Static variables
******************************************************************************/

static const bt_adapter_module_t *adapter_module;

static void parse_number(char *p_conf_name, char *p_conf_value, void *buf,
                         int len, int size)
{
    uint8_t *dest = (uint8_t *)buf;
    char *sub_value, *p;
    uint32_t value;
    UNUSED(p_conf_name);
    sub_value = strtok_r(p_conf_value, CONF_VALUES_PARTITION, &p);
    do {
        if (sub_value == NULL) {
            break;
        }

        if (sub_value[0] == '0' && (sub_value[1] == 'x' || sub_value[1] == 'X')) {
            value = strtoul(sub_value, 0, 16) & 0xFFFFFFFF;
        } else {
            value = strtoul(sub_value, 0, 10) & 0xFFFFFFFF;
        }

        switch (size) {
        case sizeof(uint8_t):
            *dest = value & 0xFF;
            dest += size;
            break;

        case sizeof(uint16_t):
            *((uint16_t *)dest) = value & 0xFFFF;
            dest += size;
            break;

        case sizeof(uint32_t):
            *((uint32_t *)dest) = value & 0xFFFFFFFF;
            dest += size;
            break;

        default:
            break;
        }
        sub_value = strtok_r(NULL, CONF_VALUES_PARTITION, &p);
    } while (--len);
}

static int read_mac_address(char *file_name, uint8_t *addr)
{
    char buf[MAC_ADDR_FILE_LEN] = {0};
    uint32_t addr_t[MAC_ADDR_LEN] = {0};
    int i = 0;
    int fd = open(file_name, O_RDONLY);
    ALOGI("%s read file: %s", __func__, file_name);
    if (fd < 0) {
        ALOGI("%s open %s error reason: %s", __func__, file_name, strerror(errno));
        return -1;
    }
    if (read(fd, buf, MAC_ADDR_BUF_LEN) < 0) {
        ALOGI("%s read %s error reason: %s", __func__, file_name, strerror(errno));
        goto done;
    }
    if (sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", &addr_t[0], &addr_t[1],
               &addr_t[2], &addr_t[3], &addr_t[4], &addr_t[5]) < 0) {
        ALOGI("%s sscanf %s error reason: %s", __func__, file_name,
              strerror(errno));
        goto done;
    }
    for (i = 0; i < MAC_ADDR_LEN; i++) {
        addr[i] = addr_t[i] & 0xFF;
    }
    ALOGI("%s %s addr: [%02X:%02X:%02X:%02X:%02X:%02X]", __func__, file_name,
          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

done:
    close(fd);
    return 0;
}

static void mac_address_stream_compose(uint8_t *addr)
{
    uint8_t tmp, i, j;
    for (i = 0, j = MAC_ADDR_LEN - 1; (i < MAC_ADDR_LEN / 2) && (i != j);
         i++, j--) {
        tmp = addr[i];
        addr[i] = addr[j];
        addr[j] = tmp;
    }
}

static void set_mac_address(uint8_t *addr)
{
    int ret = -1;
    uint8_t addr_t[6] = {0};

    ALOGI("%s", __func__);
    /* check misc mac file exist */
    ret = access(DATMISC_MAC_ADDR_PATH, F_OK);
    if (ret != 0) {
        ALOGI("%s %s miss", __func__, DATMISC_MAC_ADDR_PATH);
        return;
    }

    /* read mac file */
    read_mac_address(DATMISC_MAC_ADDR_PATH, addr_t);

    /* compose mac stream */
    mac_address_stream_compose(addr_t);

    memcpy(addr, addr_t, MAC_ADDR_LEN);
}

static void sprd_vnd_reload(void)
{
    int ret;
    char ssp_property[128] = {0};
    pskey_config_t *pskey = (pskey_config_t *)adapter_module->pskey;

    set_mac_address(pskey->device_addr);

    ret = property_get("persist.sys.bt.non.ssp", ssp_property, "close");
    if (ret >= 0 && !strcmp(ssp_property, "open")) {
        ALOGI("### disable BT SSP function due to property setting ###");
        ALOGI("SSP setting pskey from : 0x%02x to: 0x%02x", pskey->feature_set[6],
              (pskey->feature_set[6] & 0xF7));
        pskey->feature_set[6] &= 0xF7;
    }
    adapter_module->pskey_dump(NULL);
}

/*****************************************************************************
**   CONF INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        vnd_load_conf
**
** Description     Read conf entry from p_path file one by one and call
**                 the corresponding config function
**
** Returns         None
**
*******************************************************************************/
void vnd_load_conf(const char *p_path)
{
    FILE *p_file;
    char *p_name, *p_value, *p;
    conf_entry_t *p_entry;
    char line[CONF_MAX_LINE_LEN + 1]; /* add 1 for \0 char */

    ALOGI("Attempt to load conf from %s", p_path);

    adapter_module = get_adapter_module();

    if ((p_file = fopen(p_path, "r")) != NULL) {
        /* read line by line */
        while (fgets(line, CONF_MAX_LINE_LEN + 1, p_file) != NULL) {
            if (line[0] == CONF_COMMENT) continue;

            p_name = strtok_r(line, CONF_DELIMITERS, &p);

            if (NULL == p_name) {
                continue;
            }

            p_value = strtok_r(NULL, CONF_VALUES_DELIMITERS, &p);

            if (NULL == p_value) {
                ALOGW("vnd_load_conf: missing value for name: %s", p_name);
                continue;
            }

            p_entry = adapter_module->tab;

            while (p_entry->conf_entry != NULL) {
                if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0) {
                    if (p_entry->p_action) {
                        p_entry->p_action(p_name, p_value, p_entry->buf, p_entry->len,
                                          p_entry->size);
                    } else {
                        parse_number(p_name, p_value, p_entry->buf, p_entry->len,
                                     p_entry->size);
                    }
                    break;
                }

                p_entry++;
            }
        }

        fclose(p_file);
    } else {
        ALOGI("vnd_load_conf file >%s< not found", p_path);
    }
    sprd_vnd_reload();
}
