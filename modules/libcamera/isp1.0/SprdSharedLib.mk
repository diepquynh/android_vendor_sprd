# add shared libs
#

ifeq ($(strip $(ISP_HW_VER)),1.0)

LOCAL_SHARED_LIBRARIES += libae libawb libaf liblsc libev

endif
