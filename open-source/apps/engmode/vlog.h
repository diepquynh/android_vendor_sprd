#ifndef _VLOG_H
#define _VLOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    int get_ser_fd(void);
    int restart_gser(int* fd, char* dev_path);
    int restart_logfile(int* fd, char* log_path);
    int eng_write_data_to_file(char *diag_data, int r_cnt,int test_fd);
    int create_log_dir();
    int open_log_path();
    int eng_diag_write2pc(char* diag_data, int r_cnt, int ser_fd);
    void eng_filter_calibration_log_diag(char* diag_data,int r_cnt,int ser_fd);

#ifdef __cplusplus
}
#endif

#endif
