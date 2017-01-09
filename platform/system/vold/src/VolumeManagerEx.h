/*
 * Created by Spreadst
 */

#ifndef VOLUME_MANAGER_EX_H
#define VOLUME_MANAGER_EX_H

/* SPRD: add for UMS @{ */
int                    mUmsSharePrepareCount;
int                    mUmsSharedCount;
int                    mUmsShareIndex;
/* @} */
/* SPRD: add for UMS @{ */
std::vector<std::string> mUMSFilePaths;
std::string            mSupportLunsFilePath;
/* @} */
/* SPRD: add for UMS @{ */
int prepareShare(int count);
int shareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol);
int unshareVolume(const std::shared_ptr<android::vold::VolumeBase>& vol);
int unshareOver();
/* @} */

// SPRD: add for emulated storage
std::shared_ptr<android::vold::VolumeBase> mEmulated;

/* SPRD: add for emulated storage @{ */
int setEmulated(const std::shared_ptr<android::vold::VolumeBase>& vol);
int clearEmulated();
/* @} */
/* SPRD: add for emulated storage */
int linkEmulated(userid_t userId);

int linkInternalPrimary(userid_t userId);


#endif
