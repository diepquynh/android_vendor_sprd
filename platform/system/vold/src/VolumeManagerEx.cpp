/*
 * Created by Spreadst
 */

/* SPRD: add for emulated storage @{ */
int VolumeManager::linkEmulated(userid_t userId) {
    if (mEmulated == nullptr
        || mEmulated->getType() != android::vold::VolumeBase::Type::kEmulated) {
        return -1;
    }
    std::string source(StringPrintf("%s/%d", mEmulated->getPath().c_str(), userId));
    fs_prepare_dir(source.c_str(), 0755, AID_ROOT, AID_ROOT);

    std::string target(StringPrintf("/mnt/user/%d/emulated", userId));
    if (TEMP_FAILURE_RETRY(unlink(target.c_str()))) {
        if (errno != ENOENT) {
            SLOGW("Failed to unlink %s: %s", target.c_str(), strerror(errno));
        }
    }
    LOG(DEBUG) << "Linking " << source << " to " << target;
    if (TEMP_FAILURE_RETRY(symlink(source.c_str(), target.c_str()))) {
        SLOGW("Failed to link %s to %s: %s", source.c_str(), target.c_str(),
                strerror(errno));
        return -errno;
    }
    return 0;
}
/* @} */

int VolumeManager::setEmulated(const std::shared_ptr<android::vold::VolumeBase>& vol) {
    mEmulated = vol;
    for (userid_t userId : mStartedUsers) {
        linkEmulated(userId);
    }
    return 0;
}
int VolumeManager::clearEmulated() {
    mEmulated.reset();
    return 0;
}
/* @} */

/* SPRD: add for external primary storage @{ */
int VolumeManager::linkInternalPrimary(userid_t userId) {
    if (mEmulated == nullptr
            || mEmulated->getType() != android::vold::VolumeBase::Type::kEmulated) {
        return -1;
    }
    std::string source(StringPrintf("%s/%d", mEmulated->getPath().c_str(), userId));
    fs_prepare_dir(source.c_str(), 0755, AID_ROOT, AID_ROOT);

    std::string target(StringPrintf("/mnt/user/%d/emulated", userId));
    if (TEMP_FAILURE_RETRY(unlink(target.c_str()))) {
        if (errno != ENOENT) {
            SLOGW("Failed to unlink %s: %s", target.c_str(), strerror(errno));
        }
    }
    LOG(DEBUG)<< "Linking " << source << " to " << target;
    if (TEMP_FAILURE_RETRY(symlink(source.c_str(), target.c_str()))) {
        SLOGW("Failed to link %s to %s: %s", source.c_str(), target.c_str(), strerror(errno));
        return -errno;
    }

    std::string targetPrimary(StringPrintf("/mnt/user/%d/primary", userId));
    if (TEMP_FAILURE_RETRY(unlink(targetPrimary.c_str()))) {
        if (errno != ENOENT) {
            SLOGW("Failed to unlink %s: %s", targetPrimary.c_str(), strerror(errno));
        }
    }
    LOG(DEBUG) << "Linking Primary " << target << " to " << targetPrimary;
    if (TEMP_FAILURE_RETRY(symlink(target.c_str(), targetPrimary.c_str()))) {
        SLOGW("Failed to link %s to %s: %s", target.c_str(), targetPrimary.c_str(),
                strerror(errno));
        return -errno;
    }
    return 0;
}
/* @} */


/* SPRD: add for UMS @{ */
int VolumeManager::prepareShare(int count) {
    if (mUmsShareIndex >= 0 || mUmsSharePrepareCount > 0) {
        SLOGE("prepare share done, can not do it again");
        return -EINVAL;
    }

    int maxUmsCount = mUMSFilePaths.size();
    if (count > maxUmsCount) {
        SLOGW("prepare to share %d volumes, but max ums paths count is %d", count, maxUmsCount);
        count = maxUmsCount;
    }

    int res = android::OK;

#ifndef UMS_K44
    res = android::vold::WriteToFile(
              std::string("prepare share"),
              mSupportLunsFilePath,
              StringPrintf("%d", count),
              0);
#endif

    if (res == android::OK) {
        mUmsShareIndex = 0;
        mUmsSharePrepareCount = count;
        mUmsSharedCount = 0;
    }
    return res;
}

int VolumeManager::shareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol) {
    if (mUmsShareIndex < 0 || mUmsSharePrepareCount == 0) {
        SLOGE("share volume no prapare, please prepare share first");
        return -EINVAL;
    }

    if (mUmsShareIndex >= mUmsSharePrepareCount) {
        SLOGE("shared too more volumes, total:%d, index:%d", mUmsSharePrepareCount, mUmsShareIndex);
        return -EINVAL;
    }

    auto massStorageFilePath = mUMSFilePaths.at(mUmsShareIndex);
    LOG(INFO) << "massStorageFilePath = " << massStorageFilePath;
    int res = vol->share(massStorageFilePath);
    if (res == android::OK) {
        mUmsShareIndex ++;
        mUmsSharedCount ++;
    }

    return res;
}

int VolumeManager::unshareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol) {
    if (mUmsShareIndex < 0 || mUmsSharePrepareCount == 0) {
        SLOGE("unshare volume no prapare, please prepare share first");
        return -EINVAL;
    }

    int res = vol->unshare();
    if (res == android::OK) {
        mUmsSharedCount --;
    }

    return res;
}

int VolumeManager::unshareOver() {
    if (mUmsShareIndex < 0 || mUmsSharePrepareCount == 0) {
        SLOGE("unshare over no prapare, please prepare share first");
        return -EINVAL;
    }

    if (mUmsSharedCount > 0) {
        SLOGE("not all shared volume unshared, can not do this");
        return -EINVAL;
    }

    int res = android::OK;

#ifndef UMS_K44
    res = android::vold::WriteToFile(
              std::string("prepare share"),
              mSupportLunsFilePath,
              std::string("0"),
              0);
#endif

    if (res == android::OK) {
        mUmsShareIndex = -1;
        mUmsSharePrepareCount = 0;
    }

    return res;
}
/* @} */
