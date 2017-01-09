/*
 * Created by Spreadst
 */

status_t PublicVolume::doMount() {
    // TODO: expand to support mounting other filesystems
    readMetadata();

    if (mFsType == "exfat" && exfat::IsSupported()) {
        LOG(VERBOSE) << getId() << " detects filesystem " << mFsType;
    } else if (mFsType != "vfat") {
        LOG(ERROR) << getId() << " unsupported filesystem " << mFsType;
        return -EIO;
    }

    if (mFsType == "vfat") {
        if (vfat::Check(mDevPath)) {
            LOG(ERROR) << getId() << " failed filesystem check";
            return -EIO;
        }
    } else if (mFsType == "exfat") {
        if (exfat::Check(mDevPath)) {
            LOG(ERROR) << getId() << " failed filesystem check";
            return -EIO;
        }
    }

    // Use UUID as stable name, if available
    std::string stableName = getId();
    if (!mFsUuid.empty()) {
        stableName = mFsUuid;
    }

    mRawPath = StringPrintf("/mnt/media_rw/%s", stableName.c_str());

    mFuseDefault = StringPrintf("/mnt/runtime/default/%s", stableName.c_str());
    mFuseRead = StringPrintf("/mnt/runtime/read/%s", stableName.c_str());
    mFuseWrite = StringPrintf("/mnt/runtime/write/%s", stableName.c_str());

    setInternalPath(mRawPath);
    if (getMountFlags() & MountFlags::kVisible) {
        setPath(StringPrintf("/storage/%s", stableName.c_str()));
    } else {
        setPath(mRawPath);
    }

    if (fs_prepare_dir(mRawPath.c_str(), 0700, AID_ROOT, AID_ROOT)) {
        PLOG(ERROR) << getId() << " failed to create mount points";
        return -errno;
    }


    if (mFsType == "vfat") {
        if (vfat::Mount(mDevPath, mRawPath, false, false, false,
                AID_MEDIA_RW, AID_MEDIA_RW, 0007, true)) {
            PLOG(ERROR) << getId() << " failed to mount " << mDevPath;
            return -EIO;
        }
    } else if (mFsType == "exfat") {
        if (exfat::Mount(mDevPath, mRawPath, false, false, false,
                AID_MEDIA_RW, AID_MEDIA_RW, 0007, true)) {
            PLOG(ERROR) << getId() << " failed to mount " << mDevPath;
            return -EIO;
        }
    }

   /* SPRD: Add support for install apk to internal sdcard @{
    * @orig if (getMountFlags() & MountFlags::kPrimary) {
    *         initAsecStage();
    * }
  */
    initAsecStage();
    /* @} */
    if (!(getMountFlags() & MountFlags::kVisible)) {
        // Not visible to apps, so no need to spin up FUSE
        return OK;
    }

    if (fs_prepare_dir(mFuseDefault.c_str(), 0700, AID_ROOT, AID_ROOT) ||
            fs_prepare_dir(mFuseRead.c_str(), 0700, AID_ROOT, AID_ROOT) ||
            fs_prepare_dir(mFuseWrite.c_str(), 0700, AID_ROOT, AID_ROOT)) {
        PLOG(ERROR) << getId() << " failed to create FUSE mount points";
        return -errno;
    }

    createSymlink(stableName);


    dev_t before = GetDevice(mFuseWrite);

    if (!(mFusePid = fork())) {
        if (getMountFlags() & MountFlags::kPrimary) {
            if (execl(kFusePath, kFusePath,
                    "-u", "1023", // AID_MEDIA_RW
                    "-g", "1023", // AID_MEDIA_RW
                    "-U", std::to_string(getMountUserId()).c_str(),
                    "-w",
                    mRawPath.c_str(),
                    stableName.c_str(),
                    NULL)) {
                PLOG(ERROR) << "Failed to exec";
            }
        } else {
            if (execl(kFusePath, kFusePath,
                    "-u", "1023", // AID_MEDIA_RW
                    "-g", "1023", // AID_MEDIA_RW
                    "-U", std::to_string(getMountUserId()).c_str(),
                    // SPRD: add for not primary volume writable
                    "-w",
                    mRawPath.c_str(),
                    stableName.c_str(),
                    NULL)) {
                PLOG(ERROR) << "Failed to exec";
            }
        }

        LOG(ERROR) << "FUSE exiting";
        _exit(1);
    }

    if (mFusePid == -1) {
        PLOG(ERROR) << getId() << " failed to fork";
        return -errno;
    }

    while (before == GetDevice(mFuseWrite)) {
        LOG(VERBOSE) << "Waiting for FUSE to spin up...";
        usleep(50000); // 50ms
    }

    return OK;
}

status_t PublicVolume::doFormat(const std::string& fsType) {
    int  fsVfat = 0, fsExfat = 0;
    unsigned long long size64 = 0ull;

    if (getBlkDeviceSize(mDevPath, size64) != OK) {
        LOG(ERROR) << getId() << " failed to get size of block device.";
    }

    if ((fsType == "exfat" || (fsType == "auto" && size64 > CARD_SIZE_32G)) && exfat::IsSupported()) {
        fsExfat = 1;
        LOG(VERBOSE) << "Format to " << fsType << " for device block size " << size64;
    } else if (fsType == "vfat"  || fsType == "auto") {
        fsVfat = 1;
    } else {
        LOG(ERROR) << "Unsupported filesystem " << fsType;
        return -EINVAL;
    }

    if (size64 > CARD_SIZE_32G) {
        //skip it, avoid waiting for a long time.
    } else {
        if (WipeBlockDevice(mDevPath) != OK) {
            LOG(WARNING) << getId() << " failed to wipe";
        }
    }

    if (fsVfat) {
        if (vfat::Format(mDevPath, 0)) {
            LOG(ERROR) << getId() << " failed to format(vfat)";
            return -errno;
        }
    } else {
        if (exfat::Format(mDevPath, 0)) {
            LOG(ERROR) << getId() << " failed to format(exfat)";
            return -errno;
        }
    }

    return OK;
}

/* SPRD: add for UMS @{ */
status_t PublicVolume::doShare(const std::string& massStorageFilePath) {
    mMassStorageFilePath = massStorageFilePath;

    std::string shareDevPath;
    VolumeManager *vm = VolumeManager::Instance();
    auto disk = vm->findDisk(getDiskId());

    // external physical SD card, share whole disk
    shareDevPath = disk->getDevPath();

    return android::vold::WriteToFile(getId(), mMassStorageFilePath, shareDevPath, 0);
}

status_t PublicVolume::doUnshare() {
    if (mMassStorageFilePath.empty()) {
        LOG(WARNING) << "mass storage file path is empty";
        return -1;
    }
    return android::vold::WriteToFile(getId(), mMassStorageFilePath, std::string(), 0);
}

status_t PublicVolume::doSetState(State state) {
    if (getLinkName().empty()) {
        LOG(WARNING) << "LinkName is empty, this is not a physical storage.";
    } else {
        property_set(StringPrintf("vold.%s.state", getLinkName().c_str()).c_str(), findState(state).c_str());
    }
    return OK;
}
/* @} */

void PublicVolume::createSymlink(const std::string & stableName) {
    /* SPRD: add for storage, create link for fuse path @{ */
    if (!getLinkName().empty()) {
        LOG(VERBOSE) << "create link for fuse path, linkName=" << getLinkName();
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/default/%s", getLinkName().c_str()));
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/read/%s", getLinkName().c_str()));
        CreateSymlink(stableName, StringPrintf("/mnt/runtime/write/%s", getLinkName().c_str()));
        property_set(StringPrintf("vold.%s.path", getLinkName().c_str()).c_str(),
                StringPrintf("/storage/%s", stableName.c_str()).c_str());
    }
    /* @} */
}

void PublicVolume::deleteSymlink() {
    /* SPRD: add for storage, delete link for fuse path @{
     *  */
    if (!getLinkName().empty()) {
        LOG(VERBOSE) << "delete link for fuse path, linkName=" << getLinkName();
        DeleteSymlink(StringPrintf("/mnt/runtime/default/%s", getLinkName().c_str()));
        DeleteSymlink(StringPrintf("/mnt/runtime/read/%s", getLinkName().c_str()));
        DeleteSymlink(StringPrintf("/mnt/runtime/write/%s", getLinkName().c_str()));
    }
    /* @* } */
}


