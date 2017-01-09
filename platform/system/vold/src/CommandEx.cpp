/*
 * Created by Spreadst
 */

#include "CommandEx.h"

int sendGenericOkFailEx(SocketClient *cli, int cond) {
    if (!cond) {
        return cli->sendMsg(ResponseCode::CommandOkay, "Command succeeded", false);
    } else {
        return cli->sendMsg(ResponseCode::OperationFailed, "Command failed", false);
    }
}

int handleVolumeCmdEx(SocketClient *cli, VolumeManager *vm, std::string cmd, int argc, char **argv) {
    if (cmd == "pre_share" && argc > 2) {
        // pre_share [count]
        int count = atoi(argv[2]);
        return sendGenericOkFailEx(cli, vm->prepareShare(count));

    } else if (cmd == "share" && argc > 2) {
        // share [volId]
        std::string id(argv[2]);
        auto vol = vm->findVolume(id);
        if (vol == nullptr) {
            return cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown volume", false);
        }

        return sendGenericOkFailEx(cli, vm->shareVolume(vol));

    } else if (cmd == "unshare" && argc > 2) {
        // unshare [volId]
        std::string id(argv[2]);
        auto vol = vm->findVolume(id);
        if (vol == nullptr) {
            return cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown volume", false);
        }

        return sendGenericOkFailEx(cli, vm->unshareVolume(vol));

    } else if (cmd == "unshare_over") {
        // unshare_over
        return sendGenericOkFailEx(cli, vm->unshareOver());

    } else {
        return cli->sendMsg(ResponseCode::CommandSyntaxError, nullptr, false);
    }
}
