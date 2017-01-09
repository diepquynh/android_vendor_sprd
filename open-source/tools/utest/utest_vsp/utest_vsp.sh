#!/bin/sh

echo "start test H.263 codec"
adb shell /system/xbin/utest_vsp_m4vh263enc -i /data/test.yuv -o /data/test.h263 -w 352 -h 288 -bitrate 32 -format 0
adb shell /system/xbin/utest_vsp_m4vh263dec -i /data/test.h263 -o /dev/null -format 0
echo "stop test H.263 codec"

echo "start test MPEG-4 codec"
adb shell /system/xbin/utest_vsp_m4vh263enc -i /data/test.yuv -o /data/test.m4v -w 352 -h 288 -bitrate 32 -format 1
adb shell /system/xbin/utest_vsp_m4vh263dec -i /data/test.m4v -o /dev/null -format 1
echo "stop test MPEG-4 codec"

echo "start test H.264 codec"
adb shell /system/xbin/utest_vsp_avch264enc -i /data/test.yuv -o /data/test.m4v -w 352 -h 288 -bitrate 32
adb shell /system/xbin/utest_vsp_avch264dec -i /data/test.m4v -o /dev/null
echo "stop test H.264 codec"

echo "start test VP8 codec"
adb shell /system/xbin/utest_vsp_vpxdec -i /data/vp80-00-comprehensive-001.ivf -o /dev/null
echo "stop test VP8 codec"


