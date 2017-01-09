#ifndef ANDROID_HARDWARE_SPRD_AUTOTEST_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_SPRD_AUTOTEST_CAMERA_HARDWARE_H

int32_t autotest_set_testmode(int camerinterface,int maincmd ,int subcmd,int cameraid,int width,int height);
int autotest_cam_from_buf(void**pp_image_addr,int size,int *out_size);
int autotest_close_testmode(void);

#endif
