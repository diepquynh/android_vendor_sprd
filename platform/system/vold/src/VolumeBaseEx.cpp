/*
 * Created by Spreadst
 */

/* SPRD: add for UMS @{ */
status_t VolumeBase::share(const std::string& massStorageFilePath) {
    if ((mState != State::kUnmounted) && (mState != State::kUnmountable)) {
        LOG(WARNING) << getId() << " share requires state unmounted or unmountable";
        return -EBUSY;
    }

    if (mType != Type::kPublic) {
        LOG(WARNING) << getId() << " just public volume can be shared";
        return -EBUSY;
    }

    status_t res = doShare(massStorageFilePath);
    if (res == OK) {
        setState(State::kShared);
    }

    return res;
}

status_t VolumeBase::doShare(const std::string& massStorageFilePath) {
    return OK;
}

status_t VolumeBase::doSetState(State state) {
    return OK;
}

status_t VolumeBase::unshare() {
    if (mState != State::kShared) {
        LOG(WARNING) << getId() << " unshare requires state shared";
        return -EBUSY;
    }

    if (mType != Type::kPublic) {
        LOG(WARNING) << getId() << " just public volume can be unshared";
        return -EBUSY;
    }

    status_t res = doUnshare();
    if (res == OK) {
        setState(State::kUnmounted);
    }

    return res;
}

status_t VolumeBase::doUnshare() {
    return OK;
}

std::string VolumeBase::findState(State state) {
    std::string stateStr;
    switch (state) {
    case State::kUnmounted:
        stateStr = "unmounted";
        break;
    case State::kChecking:
        stateStr = "checking";
        break;
    case State::kMounted:
        stateStr = "mounted";
        break;
    case State::kMountedReadOnly:
        stateStr = "mounted_ro";
        break;
    case State::kFormatting:
        stateStr = "formatting";
        break;
    case State::kEjecting:
        stateStr = "ejecting";
        break;
    case State::kUnmountable:
        stateStr = "unmountable";
        break;
    case State::kRemoved:
        stateStr = "removed";
        break;
    case State::kBadRemoval:
        stateStr = "bad_removal";
        break;
    case State::kShared:
        stateStr = "shared";
        break;
    default:
        stateStr = "unknown";
    }
    return stateStr;
}
/* @} */

/* SPRD: set link name for mount path @{ */
status_t VolumeBase::setLinkName(const std::string& linkName) {
    if (mCreated) {
        LOG(WARNING) << getId() << " linkName change requires destroyed";
        return -EBUSY;
    }

    mLinkname = linkName;
    return OK;
}
/* @} */

