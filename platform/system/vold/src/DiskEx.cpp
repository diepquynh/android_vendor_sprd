/*
 * Created by Spreadst
 */

/* SPRD: add for set link name @{ */
status_t Disk::setVolLinkName(int partIndex, const std::shared_ptr<VolumeBase> volume) {
    auto vol = volume;
    std::string linkName;

    if (partIndex == 0 || partIndex == 1) {
        linkName = mNickname;
    } else {
        linkName = StringPrintf("%s%d", mNickname.c_str(), partIndex);
    }
    LOG(DEBUG) << StringPrintf("createVolume partIndex is %d, linkName is %s",
                               partIndex, linkName.c_str());
    vol->setLinkName(linkName);
    return OK;
}
/* @} */

/* SPRD: add for usb otg @{ */
/* check thread run function */
void *Disk::diskCheck(void *arg) {
    Disk *disk = (Disk *)arg;
    const char *devPath = disk->mDevPath.c_str();
    int fd;

    while (1) {
        LOG(DEBUG) << "open device " << devPath;
        fd = open(devPath, O_RDONLY, 0);
        if (fd < 0) {
            PLOG(ERROR) << StringPrintf("wait for USB OTG device %s ready", devPath);
        } else {
            LOG(DEBUG) << StringPrintf("USB OTG device %s is ready.", devPath);
            close(fd);
        }
        if (disk->mCtlStopCheckThread) {
            LOG(DEBUG) << "mCtlStopCheckThread is true, stop disk check thread.";
            break;
        }
        sleep(1);
    }

    disk->mCtlStopCheckThread = false;
    disk->mDiskCheckThreadID = 0;
    return NULL;
}

/* start check disk thread */
void Disk::startDiskCheckThread() {
    if (mDiskCheckThreadID != 0) {
        LOG(DEBUG) << "disk check thread is already running.";
        return;
    }

    mCtlStopCheckThread = false;
    if (pthread_create(&mDiskCheckThreadID, NULL, Disk::diskCheck, (void *)this)) {
        PLOG(ERROR) << "disk check thread create error";
        mDiskCheckThreadID = 0;
    }
}

/* stop check disk thread */
void Disk::stopDiskCheckThread() {
    int err = 0;

    if (mDiskCheckThreadID == 0) {
        LOG(DEBUG) << "disk check thread have not start.";
        return;
    }

    mCtlStopCheckThread = true;
    LOG(DEBUG) << "wait cancel disk check thread";
    err = pthread_join(mDiskCheckThreadID, NULL);
    LOG(DEBUG) << StringPrintf("stop disk check thread over: %d", err);
}
/* @} */

