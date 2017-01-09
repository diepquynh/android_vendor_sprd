#ifndef __ENG_AP_MODEM__H__
#define __ENG_DIAG_H__

#define FDS_SIZE 2

enum CPEVENT_E {
  CE_ALIVE,
  CE_ASSERT,
  CE_RESET,
  CE_BLOCKED,
  CE_NONE
};

enum SUBSYS_E {
  SS_ALL, // all system, or more sub system
  SS_MODEM, // 3G/4G MOOEM
  SS_WCN, // WCN
  SS_GNSS, // GNSS
  SS_PM_SH // PM and Sensor Hub
};

struct SocketConnection {
  int fd;
  int index; // Index in the fds array
  int max_retry; // Max number of connection trials, 0 for no limit
  int try_num; // Number of unsuccessful connections
};

int get_timezone(void);
void current_ap_time_stamp_handle(TIME_SYNC_T *time_sync,
                                  MODEM_TIMESTAMP_T *time_stamp);
#endif
