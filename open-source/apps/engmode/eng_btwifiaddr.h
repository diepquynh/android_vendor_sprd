#ifndef _ENG_BTWIFIMAC_H_
#define _ENG_BTWIFIMAC_H_

typedef enum
{
    ENG_BT_MAC,
    ENG_WIFI_MAC
} MacType;

int eng_btwifimac_write(char* bt_mac, char* wifi_mac);

int eng_btwifimac_read(char* mac, MacType type);

#endif
