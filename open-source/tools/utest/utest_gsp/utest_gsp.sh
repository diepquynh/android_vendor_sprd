#!/bin/sh


sudo adb shell "mkdir /data/gsp"
sudo adb shell "rm /data/gsp/* -rf"
sudo adb shell "mkdir /data/gsp/out -p"

sudo adb push ./utest_gsp /data/gsp/
sudo adb push ./640x480_YUV420SP.raw /data/gsp/

adb shell rm /data/gsp/out/*


#phy buffer: YUV420 2P 640x480-> YUV4202P 320x480 
sudo adb shell /data/gsp/utest_gsp \
-f0 /data/gsp/640x480_YUV420SP.raw -cf0 4 -bt0 1 -pw0 640 -ph0 480 -ix0 0 -iy0 0 -iw0 640 -ih0 480 -rot0 1 -ox0 0 -oy0 0 -ow0 320 -oh0 480 \
-fd /data/gsp/out/320x480_YUV420SP.raw -cfd 4 -btd 1 -pwd 320 -phd 480

#virt buffer: YUV420 2P 640x480-> YUV4202P 320x480 
sudo adb shell /data/gsp/utest_gsp \
-f0 /data/gsp/640x480_YUV420SP.raw -cf0 4 -bt0 1 -pw0 640 -ph0 480 -ix0 0 -iy0 0 -iw0 640 -ih0 480 -rot0 1 -ox0 0 -oy0 0 -ow0 320 -oh0 480 \
-fd /data/gsp/out/320x480_YUV420SP.raw -cfd 4 -btd 1 -pwd 320 -phd 480

# virt YUV420 2P 640x480 --cpy--> phy YUV420 2P 640x480 --GSP--> phy YUV4202P 320x480 
sudo adb shell /data/gsp/utest_gsp \
-f0 /data/gsp/640x480_YUV420SP.raw -cf0 4 -bt0 1 -pw0 640 -ph0 480 -ix0 0 -iy0 0 -iw0 640 -ih0 480 -rot0 1 -ox0 0 -oy0 0 -ow0 320 -oh0 480 -cpy0 1 -cbt0 0 \
-fd /data/gsp/out/320x480_YUV420SP.raw -cfd 4 -btd 0 -pwd 320 -phd 480

#virt buffer: (ARGB888 480x854)x2 -> ARGB888 480x854 
#generate src
sudo adb shell /data/gsp/utest_gsp \
-f0 /data/gsp/640x480_YUV420SP.raw -cf0 4 -bt0 1 -pw0 640 -ph0 480 -ix0 0 -iy0 0 -iw0 640 -ih0 480 -rot0 0 -ox0 0 -oy0 0 -ow0 854 -oh0 480 \
-fd /data/gsp/out/854x480_ARGB888.raw -cfd 0 -btd 1 -pwd 854 -phd 480

#1layer
sudo adb shell /data/gsp/utest_gsp \
-f0 /data/gsp/out/854x480_ARGB888.raw -cf0 0 -bt0 1 -pw0 854 -ph0 480 -ix0 0 -iy0 0 -iw0 854 -ih0 480 -rot0 1 -ox0 0 -oy0 0 -ow0 480 -oh0 854 \
-fd /data/gsp/out/480x854_ARGB888.raw -cfd 0 -btd 1 -pwd 480 -phd 854 -perf 1

#2layer
sudo adb shell /data/gsp/utest_gsp \
-f0 /data/gsp/out/854x480_ARGB888.raw -cf0 0 -bt0 1 -pw0 854 -ph0 480 -ix0 0 -iy0 0 -iw0 854 -ih0 480 -rot0 1 -ox0 0 -oy0 0 -ow0 480 -oh0 854 \
-f1 /data/gsp/out/854x480_ARGB888.raw -cf1 0 -bt1 1 -pw1 854 -ph1 480 -ix1 0 -iy1 0 -iw1 854 -ih1 480 -rot1 1 -ox1 0 -oy1 0 -ow1 480 -oh1 854 \
-fd /data/gsp/out/480x854_ARGB888.raw -cfd 0 -btd 1 -pwd 480 -phd 854 -pwr 1 





