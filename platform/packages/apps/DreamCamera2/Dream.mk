
LOCAL_PATH:= $(call my-dir)

######################### dream folder #######################
LOCAL_DREAM_FILES := $(call all-java-files-under, src_dream/com/dream/camera)

LOCAL_DREAM_FILES_G += $(call all-java-files-under, src_dream/com/dream/camera/dreambasemodules/DreamGifModule.java)
LOCAL_DREAM_FILES_G += $(call all-java-files-under, src_dream/com/dream/camera/dreambasemodules/DreamGifUI.java)
LOCAL_DREAM_FILES_G += $(call all-java-files-under, src_dream/com/dream/camera/modules/gifphoto)
LOCAL_DREAM_FILES_G += $(call all-java-files-under, src_dream/com/dream/camera/modules/gifvideo)

LOCAL_DREAM_FILES_S += $(call all-java-files-under, src_dream/com/dream/camera/modules/scenerydream)

LOCAL_DREAM_FILES_S_F += $(call all-java-files-under, src_dream/com/dream/camera/dreambasemodules/DreamBasicUI.java)

LOCAL_DREAM_FILES_F += $(call all-java-files-under, src_dream/com/dream/camera/modules/filterdream)

LOCAL_DREAM_FILES_V += $(call all-java-files-under, src_dream/com/dream/camera/vgesture)

# filter out LOCAL_DREAM_FILES_G if MY_GIF_CAM is not support
ifeq ($(strip $(MY_GIF_CAM)),true)
else
$(foreach v,$(LOCAL_DREAM_FILES_G),\
$(eval LOCAL_DREAM_FILES := $(filter-out $(v), $(LOCAL_DREAM_FILES)))\
)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/dream/camera/modules/gifphoto)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/dream/camera/modules/gifvideo)
endif

ifeq ($(strip $(MY_FILTER_SCENERY_GIF)),true)
else
$(foreach v,$(LOCAL_DREAM_FILES_S_F),\
$(eval LOCAL_DREAM_FILES := $(filter-out $(v), $(LOCAL_DREAM_FILES)))\
)
endif

ifeq ($(strip $(MY_SCENERY_CAM)),true)
else
$(foreach v,$(LOCAL_DREAM_FILES_S),\
$(eval LOCAL_DREAM_FILES := $(filter-out $(v), $(LOCAL_DREAM_FILES)))\
)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/dream/camera/modules/scenerydream)
endif

ifeq ($(strip $(MY_FILTER_CAM)),true)
else
$(foreach v,$(LOCAL_DREAM_FILES_F),\
$(eval LOCAL_DREAM_FILES := $(filter-out $(v), $(LOCAL_DREAM_FILES)))\
)
LOCAL_DREAM_FILES += $(call all-java-files-under, src_fake/com/dream/camera/modules/filterdream)
endif

ifeq ($(strip $(MY_VGESTURE_CAM)),true)
else
$(foreach v,$(LOCAL_DREAM_FILES_V),\
$(eval LOCAL_DREAM_FILES := $(filter-out $(v), $(LOCAL_DREAM_FILES)))\
)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/dream/camera/vgesture)
endif

LOCAL_SRC_FILES += $(LOCAL_DREAM_FILES)

#$(info $(LOCAL_SRC_FILES))

########################### dreamoverlay folder #########################
LOCAL_DREAMOVERLAY_FILES := $(call all-java-files-under, src_dream/com/dreamoverlay/camera)

$(foreach v,$(LOCAL_DREAMOVERLAY_FILES),\
$(eval LOCAL_DREAMOVERLAY_ORG += $(shell (echo $(v) | sed -e "s?src_dream/com/dreamoverlay?src/com/android?g")))\
)

$(foreach v,$(LOCAL_DREAMOVERLAY_ORG),\
$(eval LOCAL_SRC_FILES := $(filter-out $(v), $(LOCAL_SRC_FILES)))\
)

$(foreach v,$(LOCAL_DREAMOVERLAY_FILES),\
$(eval LOCAL_SRC_FILES += $(v))\
)

########################### dream sprd overlay folder ####################
LOCAL_DREAMSPRDOVERLAY_FILES := $(call all-java-files-under, src_dream/com/dreamsprdoverlay/camera)

$(foreach v,$(LOCAL_DREAMSPRDOVERLAY_FILES),\
$(eval LOCAL_DREAMSPRDOVERLAY_ORG += $(shell (echo $(v) | sed -e "s?src_dream/com/dreamsprdoverlay?src/com/sprd?g")))\
)

$(foreach v,$(LOCAL_DREAMSPRDOVERLAY_ORG),\
$(eval LOCAL_SRC_FILES := $(filter-out $(v), $(LOCAL_SRC_FILES)))\
)

$(foreach v,$(LOCAL_DREAMSPRDOVERLAY_FILES),\
$(eval LOCAL_SRC_FILES += $(v))\
)

########################### dream ucam overlay folder ########################
ifeq ($(strip $(MY_FILTER_SCENERY_GIF)),true)
LOCAL_DREAMUCAMOVERLAY_FILES_FSG += $(call all-java-files-under, src_dream/com/dreamucamoverlay/ucam/modules/ui)
LOCAL_DREAMUCAMOVERLAY_FILES_FSG += $(call all-java-files-under, src_dream/com/dreamucamoverlay/ucam/modules/BasicModule.java)

$(foreach v,$(LOCAL_DREAMUCAMOVERLAY_FILES_FSG),\
$(eval LOCAL_DREAMUCAMOVERLAY_FSG_ORG += $(shell (echo $(v) | sed -e "s?src_dream/com/dreamucamoverlay/ucam?src_ucam/UCamera/src/com/ucamera/ucam?g")))\
)

$(foreach v,$(LOCAL_DREAMUCAMOVERLAY_FSG_ORG),\
$(eval LOCAL_SRC_FILES := $(filter-out $(v), $(LOCAL_SRC_FILES)))\
)

$(foreach v,$(LOCAL_DREAMUCAMOVERLAY_FILES_FSG),\
$(eval LOCAL_SRC_FILES += $(v))\
)
########################### dream ucam thundersoft folder ########################
LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG += $(call all-java-files-under, src_dream/com/dreamucamoverlay/thundersoft/advancedfilter)

$(foreach v,$(LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG),\
$(eval LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG_ORG += $(shell (echo $(v) | sed -e "s?src_dream/com/dreamucamoverlay/thundersoft?src_ucam/UCamera/src/com/thundersoft?g")))\
)

$(foreach v,$(LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG_ORG),\
$(eval LOCAL_SRC_FILES := $(filter-out $(v), $(LOCAL_SRC_FILES)))\
)

$(foreach v,$(LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG),\
$(eval LOCAL_SRC_FILES += $(v))\
)

############################ dream ucam thundersoft folder ########################
LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG += $(call all-java-files-under, src_dream/com/dreamucamoverlay/thundersoft/advancedfilter)

$(foreach v,$(LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG),\
$(eval LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG_ORG += $(shell (echo $(v) | sed -e "s?src_dream/com/dreamucamoverlay/thundersoft?src_ucam/UCamera/src/com/thundersoft?g")))\
)

$(foreach v,$(LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG_ORG),\
$(eval LOCAL_SRC_FILES := $(filter-out $(v), $(LOCAL_SRC_FILES)))\
)

$(foreach v,$(LOCAL_DREAMUCAMTHUNDERSOFTOVERLAY_FILES_FSG),\
$(eval LOCAL_SRC_FILES += $(v))\
)
else
endif

ifeq ($(strip $(MY_GIF_CAM)),true)
LOCAL_DREAMUCAMOVERLAY_FILES_G += $(call all-java-files-under, src_dream/com/dreamucamoverlay/ucam/modules/ugif)

$(foreach v,$(LOCAL_DREAMUCAMOVERLAY_FILES_G),\
$(eval LOCAL_DREAMUCAMOVERLAY_G_ORG += $(shell (echo $(v) | sed -e "s?src_dream/com/dreamucamoverlay/ucam?src_ucam/UCamera/src/com/ucamera/ucam?g")))\
)

$(foreach v,$(LOCAL_DREAMUCAMOVERLAY_G_ORG),\
$(eval LOCAL_SRC_FILES := $(filter-out $(v), $(LOCAL_SRC_FILES)))\
)

$(foreach v,$(LOCAL_DREAMUCAMOVERLAY_FILES_G),\
$(eval LOCAL_SRC_FILES += $(v))\
)
else
endif

############################# dream res folder #################################

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res_dream $(LOCAL_RESOURCE_DIR)

############################# print compile file and res on screen #############
#$(info $(LOCAL_SRC_FILES))
#$(info $(LOCAL_RESOURCE_DIR))

