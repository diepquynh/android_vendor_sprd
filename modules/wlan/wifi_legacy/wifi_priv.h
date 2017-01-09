
#ifndef _WIFI_PRIV_H
#define _WIFI_PRIV_H

#if __cplusplus
extern "C" {
#endif

void sprd_wifi_preload_driver();
void sprd_wifi_after_unload_driver();

//NOTE: Bug#474462 Add for SoftAp Advance Feature BEG-->
int wifi_connect_to_hostapd(const char *ifname);
void wifi_close_hostapd_connection(const char *ifname);
void wifi_stop_connect_to_hostapd(const char *ifname);
int wifi_hostapd_command(const char *cmd, char *reply, size_t *reply_len);
int wifi_hostapd_wait_for_event(char *buf, size_t buflen);
//<-- Bug#474462 Add for SoftAp Advance Feature END

#if __cplusplus
};  // extern "C"
#endif

#endif  // _WIFI_PRIV_H
