# The Spreadtrum Communication Inc. 2016

merge_c_include := $(addprefix $(VENDOR_PLATFORM)/,$(LOCAL_C_INCLUDES) $(LOCAL_PATH) $(LOCAL_PATH)/src $(LOCAL_PATH)/$(LOCAL_MODULE)_src)
exist_paths :=

$(foreach v,$(merge_c_include), \
  $(if $(wildcard $(v)), \
    $(eval exist_paths += $(v)), \
  ) \
)

ifdef exist_paths
LOCAL_C_INCLUDES += $(exist_paths)
endif

ifdef localDebug
$(info LOCAL_C_INCLUDES := $(LOCAL_C_INCLUDES))
endif

