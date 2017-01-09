#ifndef __WCND_UTIL_H__
#define __WCND_UTIL_H__

#include <sys/types.h>
#include <signal.h>


int wcnd_kill_process(pid_t pid, int signal);
int wcnd_kill_process_by_name(const char *proc_name, int signal);

int wcnd_find_process_by_name(const char *proc_name);

int wcnd_down_network_interface(const char *ifname);
int wcnd_up_network_interface(const char *ifname);

void wcnd_wait_for_supplicant_stopped();
void wcnd_wait_for_driver_unloaded(void);

int wcnd_notify_wifi_driver_cp2_state(int state_ok);

int wcnd_check_process_exist(const char *proc_name, int proc_pid);

int wcnd_find(const char *path, const char* target);

int wcnd_stop_process(const char *proc_name, int time_out);

#endif
