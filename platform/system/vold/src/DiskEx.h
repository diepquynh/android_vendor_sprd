/*
 * Created by Spreadst
 */

#ifndef DISK_EX_H
#define DISK_EX_H

/* SPRD: add for set link name @{ */
status_t setVolLinkName(int partIndex, const std::shared_ptr<VolumeBase> vol);
/* @} */

/* SPRD: add for usb otg @{ */
// disk check thread id
pthread_t mDiskCheckThreadID;
// check device node
bool mCtlStopCheckThread;

/* check thread run function */
static void *diskCheck(void *arg);
/* start check disk thread */
void startDiskCheckThread();
/* stop check disk thread */
void stopDiskCheckThread();
/* @} */


#endif
