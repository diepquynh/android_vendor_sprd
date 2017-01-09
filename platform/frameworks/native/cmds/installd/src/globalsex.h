/*
** Copyright 2016,Spreadtrum
*/

#ifndef GLOBALS_EX_H_
#define GLOBALS_EX_H_

//SPRD: add for backup app @{
typedef struct {
    int mode;
    int uid;
    int gid;
} arg_chown;
// @}

// SPRD: add feature for scan the preload directory
extern dir_rec_t android_app_preload_dir;
// SPRD: add feature for scan the vital directory
extern dir_rec_t android_app_vital_dir;

#endif
