/*
 * Created by Spreadst
 */

#ifndef COMMAND_EX_H
#define COMMAND_EX_H

#include "VolumeManager.h"
#include "ResponseCode.h"
#include <sysutils/SocketClient.h>

int sendGenericOkFailEx(SocketClient *cli, int cond);
int handleVolumeCmdEx(SocketClient *cli, VolumeManager *vm, std::string cmd, int argc, char **argv);

#endif
