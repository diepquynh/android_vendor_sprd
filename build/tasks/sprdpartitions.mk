define generate-sprd-userimage-prop-dictionary
$(if $(BOARD_PERSISTIMAGE_PARTITION_SIZE),$(hide) echo "persist_size=$(BOARD_PERSISTIMAGE_PARTITION_SIZE)" >> $(1))
$(if $(BOARD_SYSINFOIMAGE_FILE_SYSTEM_TYPE),$(hide) echo "sysinfo_fs_type=$(BOARD_SYSINFOIMAGE_FILE_SYSTEM_TYPE)" >> $(1))
$(if $(BOARD_SYSINFOIMAGE_PARTITION_SIZE),$(hide) echo "sysinfo_size=$(BOARD_SYSINFOIMAGE_PARTITION_SIZE)" >> $(1))
endef

# -----------------------------------------------------------------
#systeminfo partition image
ifdef BOARD_SYSINFOIMAGE_FILE_SYSTEM_TYPE
TARGET_OUT_SYSINFO := $(PRODUCT_OUT)/sysinfo
INTERNAL_SYSINFOIMAGE_FILES := \
    $(filter $(TARGET_OUT_SYSINFO)/%,$(ALL_DEFAULT_INSTALLED_MODULES))

sysinfoimage_intermediates := \
    $(call intermediates-dir-for,PACKAGING,sysinfo)
BUILT_SYSINFOIMAGE_TARGET := $(PRODUCT_OUT)/sysinfo.img

define build-sysinfoimage-target
  @mkdir -p $(TARGET_OUT_SYSINFO)
  ./vendor/sprd/build/tasks/otascripts/out_check_bin.py \
  $(TARGET_OUT) $(TARGET_OUT_SYSINFO)
  $(call pretty,"Target sysinfo fs image: $(INSTALLED_SYSINFOIMAGE_TARGET)")
  @mkdir -p $(sysinfoimage_intermediates) && rm -rf $(sysinfoimage_intermediates)/sysinfo_image_info.txt
  $(call generate-sprd-userimage-prop-dictionary, $(sysinfoimage_intermediates)/sysinfo_image_info.txt)
  $(info cunstom info $(TARGET_OUT_SYSINFO) $(sysinfoimage_intermediates) $(INSTALLED_SYSINFOIMAGE_TARGET) $(TARGET_OUT_SYSINFO))
  $(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH \
      ./vendor/sprd/build/tasks/otascripts/build_sprd_image.py \
      $(TARGET_OUT_SYSINFO) $(sysinfoimage_intermediates)/sysinfo_image_info.txt $(INSTALLED_SYSINFOIMAGE_TARGET) $(TARGET_OUT_SYSINFO)
  $(hide) $(call assert-max-image-size,$(INSTALLED_SYSINFOIMAGE_TARGET),$(BOARD_SYSINFOIMAGE_PARTITION_SIZE),yaffs)
endef

INSTALLED_SYSINFOIMAGE_TARGET := $(BUILT_SYSINFOIMAGE_TARGET)
$(INSTALLED_SYSINFOIMAGE_TARGET): $(INTERNAL_USERIMAGES_DEPS) $(INTERNAL_SYSINFOIMAGE_FILES) $(INSTALLED_SYSTEMIMAGE)
	$(build-sysinfoimage-target)

.PHONY: sysinfoimage-nodeps
systeminfoimage-nodeps: | $(INTERNAL_USERIMAGES_DEPS)
	$(build-sysinfoimage-target)
.PHONY: sysinfoimage
sysinfoimage: $(INSTALLED_SYSINFOIMAGE_TARGET)

droidcore: $(INSTALLED_SYSINFOIMAGE_TARGET)

# we need to build sysinfoimage when make target-files. Add the bin file to ..../target-files/RADIO/
$(BUILT_TARGET_FILES_PACKAGE): $(INSTALLED_SYSINFOIMAGE_TARGET)
INSTALLED_RADIOIMAGE_TARGET +=	$(TARGET_OUT_SYSINFO)/check.bin
endif # BOARD_SYSINFOIMAGE_FILE_SYSTEM_TYPE


# -----------------------------------------------------------------
#persist partition image
ifdef BOARD_PERSISTIMAGE_PARTITION_SIZE
INSTALLED_PERSISTIMAGE_TARGET := $(PRODUCT_OUT)/persist.img
define build-persistimage-target
  $(info Target persist fs image: $(INSTALLED_PERSISTIMAGE_TARGET))
  @dd if=/dev/zero of=$(INSTALLED_PERSISTIMAGE_TARGET) bs=1024 count=2048
  $(hide) $(call assert-max-image-size,$(INSTALLED_PERSISTIMAGE_TARGET),$(BOARD_PERSISTIMAGE_PARTITION_SIZE))
endef
$(INSTALLED_PERSISTIMAGE_TARGET): $(INTERNAL_USERIMAGES_DEPS)
	$(build-persistimage-target)

.PHONY: persistimage-nodeps
persistimage-nodeps: | $(INTERNAL_USERIMAGES_DEPS)
	$(build-persistimage-target)

droidcore: $(INSTALLED_PERSISTIMAGE_TARGET)

$(BUILT_TARGET_FILES_PACKAGE): $(INSTALLED_PERSISTIMAGE_TARGET)
INSTALLED_RADIOIMAGE_TARGET += $(INSTALLED_PERSISTIMAGE_TARGET)
endif # BOARD_PERSISTIMAGE_PARTITION_SIZE
