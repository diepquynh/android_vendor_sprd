/*
 * Created by Spreadst
 */

#ifndef PUBLIC_VOLUME_EX_H
#define PUBLIC_VOLUME_EX_H

/* SPRD: add for UMS @{ */
std::string mMassStorageFilePath;
/* @} */

/* SPRD: add for UMS @{ */
status_t doShare(const std::string& massStorageFilePath) override;
status_t doUnshare() override;
status_t doSetState(State state) override;
/* @} */
void createSymlink(const std::string & stableName);
void deleteSymlink(void);


#endif
