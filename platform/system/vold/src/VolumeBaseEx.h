/*
 * Created by Spreadst
 */

#ifndef VOLUME_BASE_EX_H
#define VOLUME_BASE_EX_H

/* SPRD: link name for mount path */
std::string mLinkname;

/* SPRD: add for UMS @{ */
status_t share(const std::string& massStorageFilePath);
status_t unshare();
std::string findState(State state);
/* @} */
/* SPRD: add for UMS @{ */
virtual status_t doShare(const std::string& massStorageFilePath);
virtual status_t doUnshare();
virtual status_t doSetState(State state);
/* @} */
/* SPRD: get link name for mount path */
const std::string& getLinkName() { return mLinkname; }
/* SPRD: set link name for mount path */
status_t setLinkName(const std::string& linkName);


#endif
