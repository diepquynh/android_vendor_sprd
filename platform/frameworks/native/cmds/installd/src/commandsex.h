/*
** Copyright 2016,Spreadtrum
*/

#ifndef COMMANDS_EX_H_
#define COMMANDS_EX_H_

//SPRD: add for backup app @{
int backup_app(const char* pkgname, const char* dest_path, int uid, int gid);
int restore_app(const char* source_path, const char* pkgname, int uid, int gid);
// @}

#endif
