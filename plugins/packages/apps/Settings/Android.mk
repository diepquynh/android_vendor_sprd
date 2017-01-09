LOCAL_PATH := $(call my-dir)

# TODO:SPRD_CHECK because of Google change the telephony structure, the Settings addon about SubscriptionInfo build failed.
 include $(call all-makefiles-under,$(LOCAL_PATH))
