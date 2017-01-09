# perload patches after run make sprdpreloadpatch

# $(info *** We will merge preloadPatches before u make source-code when u "make sprdpreloadpatch" ***)

LOCAL_PATH := $(call my-dir)

current-time:=[$$(date "+%Y-%m-%d %H:%M:%S")]
log:=@echo $(current-time)

SCRIPT_PATH := $(LOCAL_PATH)/mergepatch/tools
PATCHES := $(LOCAL_PATH)/mergepatch/patches/
stamp := ${ANDROID_PRODUCT_OUT}/sucesspreload.csv

$(stamp): $(stamp)
	$(log) "*** starting to preload patches... ***"
	$(hide) $(SCRIPT_PATH)/merge.py -i $(PATCHES) -o $(stamp)
	$(log) "*** preload patches done ***"

.PHONY: sprdpreloadpatch
sprdpreloadpatch: $(stamp)
