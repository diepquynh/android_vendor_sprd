LOCAL_PATH := $(call my-dir)

LOCAL_NVMERGE_INTERMEDIATES_FILE := $(call intermediates-dir-for,EXECUTABLES,nvmerge,,,$(TARGET_PREFER_32_BIT))/nvmerge
LOCAL_SPLMERGE_INTERMEDIATES_FILE := $(call intermediates-dir-for,EXECUTABLES,splmerge,,,$(TARGET_PREFER_32_BIT))/splmerge
LOCAL_REPART_INTERMEDIATES_FILE := $(call intermediates-dir-for,EXECUTABLES,repart,,,$(TARGET_PREFER_32_BIT))/repart
LOCAL_PACK_INTERMEDIATES_FILE := $(call intermediates-dir-for,EXECUTABLES,pack,,,$(TARGET_PREFER_32_BIT))/pack

ifeq ($(strip $(BOARD_KERNEL_SEPARATED_DT)),true)
$(BUILT_TARGET_FILES_PACKAGE): $(INSTALLED_DTIMAGE_TARGET)
DTIMAGE_TARGET_FILE_NAME := $(shell basename $(INSTALLED_DTIMAGE_TARGET))
endif

OTA_TOOLS := $(LOCAL_NVMERGE_INTERMEDIATES_FILE) \
	$(LOCAL_SPLMERGE_INTERMEDIATES_FILE) \
	$(LOCAL_REPART_INTERMEDIATES_FILE) \
	$(LOCAL_PACK_INTERMEDIATES_FILE) \
	$(TARGET_RECOVERY_NVMERGE_CONFIG) \
	$(MODEM_UPDATE_CONFIG_FILE)

$(BUILT_TARGET_FILES_PACKAGE) : $(OTA_TOOLS)
$(BUILT_TARGET_FILES_PACKAGE) : PRIVATE_OTA_TOOLS += $(OTA_TOOLS)

#-------------------------------------------------------------------------------------------------

SPRD_BUILT_TARGET_FILES_PACKAGE := $(basename $(BUILT_TARGET_FILES_PACKAGE))

$(SPRD_BUILT_TARGET_FILES_PACKAGE): $(BUILT_TARGET_FILES_PACKAGE)
	$(hide) mkdir -p $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES
ifeq ($(strip $(BOARD_KERNEL_SEPARATED_DT)),true)
	$(hide) echo "with_dt_img=$(DTIMAGE_TARGET_FILE_NAME)" >> $(SPRD_BUILT_TARGET_FILES_PACKAGE)/META/misc_info.txt
	$(hide) echo "adapt_block_device=true" >> $(SPRD_BUILT_TARGET_FILES_PACKAGE)/META/misc_info.txt
	$(hide) $(ACP) $(INSTALLED_DTIMAGE_TARGET) $(SPRD_BUILT_TARGET_FILES_PACKAGE)/$(DTIMAGE_TARGET_FILE_NAME)
endif
ifeq ($(strip $(PRODUCT_SECURE_BOOT)), NONE)
	$(hide) $(ACP) $(PRODUCT_OUT)/boot.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/boot.img
	$(hide) $(ACP) $(PRODUCT_OUT)/recovery.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/recovery.img
ifeq ($(wildcard $(PRODUCT_OUT)/u-boot-sign.bin),)
	$(hide) $(HOST_OUT)/bin/imgheaderinsert  $(PRODUCT_OUT)/u-boot.bin 1
endif
ifeq ($(wildcard $(PRODUCT_OUT)/u-boot-spl-16k-sign.bin),)
	$(hide) $(HOST_OUT)/bin/imgheaderinsert $(PRODUCT_OUT)/u-boot-spl-16k.bin 1
endif
ifeq ($(wildcard $(PRODUCT_OUT)/sml-sign.bin),)
	$(hide) $(HOST_OUT)/bin/imgheaderinsert $(PRODUCT_OUT)/sml.bin 1
endif
ifeq ($(wildcard $(PRODUCT_OUT)/tos-sign.bin),)
	$(hide) $(HOST_OUT)/bin/imgheaderinsert $(PRODUCT_OUT)/tos.bin 1
endif
endif
ifeq ($(strip $(PRODUCT_SECURE_BOOT)), SANSA)
#	$(hide) source build/envsetup.sh; source $(HOST_OUT)/bin/packimage.sh
	$(hide) echo "secure_boot=$(PRODUCT_SECURE_BOOT)" >> $(SPRD_BUILT_TARGET_FILES_PACKAGE)/META/misc_info.txt
	$(hide) $(ACP) $(PRODUCT_OUT)/boot-sign.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/boot.img
	$(hide) $(ACP) $(PRODUCT_OUT)/recovery-sign.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/recovery.img
	$(hide) rm -f $(PRODUCT_OUT)/boot.img
	$(hide) rm -f $(PRODUCT_OUT)/recovery.img
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH \
            ./vendor/sprd/build/tasks/otascripts/sign_modem_image.sh $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO $(TARGET_PRODUCT)
endif

ifndef PRODUCT_SECURE_BOOT
	$(hide) $(ACP) $(PRODUCT_OUT)/boot.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/boot.img
	$(hide) $(ACP) $(PRODUCT_OUT)/recovery.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/recovery.img
	$(hide) $(ACP) $(PRODUCT_OUT)/u-boot.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/u-boot.bin
	$(hide) $(ACP) $(PRODUCT_OUT)/u-boot-spl-16k.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/u-boot-spl-16k.bin
	$(hide) -$(ACP) $(PRODUCT_OUT)/tos.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/tos.bin
	$(hide) -$(ACP) $(PRODUCT_OUT)/vmm.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/vmm.bin
ifeq ($(strip $(BOARD_SECURE_BOOT_ENABLE)), true)
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH \
            ./vendor/sprd/build/tasks/otascripts/sign_target_files_secureboot.py $(PRODUCT_OUT) $(SPRD_BUILT_TARGET_FILES_PACKAGE) $(SPRD_BUILT_TARGET_FILES_PACKAGE)
endif
else
	$(hide) $(ACP) $(PRODUCT_OUT)/u-boot-sign.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/u-boot-sign.bin
	$(hide) $(ACP) $(PRODUCT_OUT)/u-boot-spl-16k-sign.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/u-boot-spl-16k-sign.bin
	$(hide) $(ACP) $(PRODUCT_OUT)/sml-sign.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/sml-sign.bin
	$(hide) $(ACP) $(PRODUCT_OUT)/tos-sign.bin $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/tos-sign.bin
endif

ifdef BOARD_SYSINFOIMAGE_FILE_SYSTEM_TYPE
	$(hide) $(ACP) $(PRODUCT_OUT)/sysinfo.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/sysinfo.img
endif
ifdef BOARD_PERSISTIMAGE_PARTITION_SIZE
	$(hide) -$(ACP) $(PRODUCT_OUT)/persist.img $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/persist.img
endif

# SPRD: add for add partititon.xml put into target-files-package @{
	$(hide) $(foreach xmlname, $(wildcard $(PRODUCT_OUT)/*.xml), \
		if grep -q ProductList $(xmlname) && grep -q Partitions $(xmlname); \
		then $(ACP) $(xmlname)  $(SPRD_BUILT_TARGET_FILES_PACKAGE)/RADIO/partition.xml; \
		fi;)
# @ }

	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH MKBOOTIMG=$(MKBOOTIMG) \
	    ./build/tools/releasetools/make_recovery_patch $(SPRD_BUILT_TARGET_FILES_PACKAGE) $(SPRD_BUILT_TARGET_FILES_PACKAGE)
	$(hide) $(ACP) -rd $(dir $(BUILT_SYSTEMIMAGE))/* $(SPRD_BUILT_TARGET_FILES_PACKAGE)/IMAGES/
# zip qry override the content. system.img in target.zip cannot be replaced
	$(hide) rm -rf $<
	$(hide) (cd $(SPRD_BUILT_TARGET_FILES_PACKAGE) && zip -qry ../$(notdir $<) .)
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH MKBOOTIMG=$(MKBOOTIMG) \
	    ./build/tools/releasetools/add_img_to_target_files -a -v -p $(HOST_OUT) $<

.PHONY: sprd_built_packages
sprd_built_packages: $(SPRD_BUILT_TARGET_FILES_PACKAGE)

RECOVERY-FROM-BOOT-PATCH := recovery-from-boot.p
$(RECOVERY-FROM-BOOT-PATCH): $(INSTALLED_BOOTIMAGE_TARGET) $(INSTALLED_RECOVERYIMAGE_TARGET) $(RECOVERY_RESOURCE_ZIP) $(HOST_OUT_EXECUTABLES)/imgdiff $(HOST_OUT_EXECUTABLES)/bsdiff
	$(hide) mkdir -p $(PRODUCT_OUT)/IMAGES
	$(hide) $(ACP) $(PRODUCT_OUT)/boot.img $(PRODUCT_OUT)/IMAGES/boot.img
	$(hide) $(ACP) $(PRODUCT_OUT)/recovery.img $(PRODUCT_OUT)/IMAGES/recovery.img
#SPRD: add for old secureboot OTA @ {
	$(hide) -rm $(PRODUCT_OUT)/sprd_misc_info.txt
ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
	$(hide) echo "secure_boot=$(BOARD_SECURE_BOOT_ENABLE)" > $(PRODUCT_OUT)/sprd_misc_info.txt
ifdef SECURE_BOOT_SIGN_TOOL
	$(hide) echo "secure_boot_tool=$(SECURE_BOOT_SIGN_TOOL)" >> $(PRODUCT_OUT)/sprd_misc_info.txt
endif # ifdef SECURE_BOOT_SIGN_TOOL
ifdef SECURE_BOOT_SIGNAL_KEY
	$(hide) echo "single_key=$(SECURE_BOOT_SIGNAL_KEY)" >> $(PRODUCT_OUT)/sprd_misc_info.txt
endif # ifdef SECURE_BOOT_SIGNAL_KEY
endif # ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
# @ }
ifeq ($(strip $(PRODUCT_SECURE_BOOT)), SANSA)
	$(hide) source build/envsetup.sh; source $(HOST_OUT)/bin/packimage.sh
	$(hide) $(ACP) $(PRODUCT_OUT)/IMAGES/boot.img $(PRODUCT_OUT)/boot.img
	$(hide) $(ACP) $(PRODUCT_OUT)/IMAGES/recovery.img $(PRODUCT_OUT)/recovery.img
	$(hide) $(ACP) $(PRODUCT_OUT)/boot-sign.img $(PRODUCT_OUT)/IMAGES/boot.img
	$(hide) $(ACP) $(PRODUCT_OUT)/recovery-sign.img $(PRODUCT_OUT)/IMAGES/recovery.img
endif
	$(hide) mkdir -p $(PRODUCT_OUT)/SYSTEM/etc
	$(hide) cp $(PRODUCT_OUT)/system/etc/recovery-resource.dat $(PRODUCT_OUT)/SYSTEM/etc/
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH \
	  ./vendor/sprd/build/tasks/otascripts/make_recovery_from_boot_patch.py $(PRODUCT_OUT) $(PRODUCT_OUT)
	$(hide) rm -rf $(PRODUCT_OUT)/SYSTEM
	$(hide) rm -rf $(PRODUCT_OUT)/IMAGES

./PHONY: recovery-from-bootp
recovery-from-bootp : $(RECOVERY-FROM-BOOT-PATCH)


$(BUILT_SYSTEMIMAGE): $(RECOVERY-FROM-BOOT-PATCH)

target-files-package: $(SPRD_BUILT_TARGET_FILES_PACKAGE)

ifeq (true, $(strip $(FOTA_UPDATE_SUPPORT)))
GETOTAPACKAGE_SH := ./vendor/sprd/platform/packages/apps/FotaUpdate/getOtaPackage.sh
endif

ifneq (true,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_SUPPORTS_VERITY))
$(INTERNAL_OTA_PACKAGE_TARGET): $(SPRD_BUILT_TARGET_FILES_PACKAGE) $(DISTTOOLS)
	@echo "Package OTA: $@"
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH MKBOOTIMG=$(MKBOOTIMG) \
	   ./build/tools/releasetools/ota_from_target_files -v \
	   -p $(HOST_OUT) \
	   -k $(KEY_CERT_PAIR) \
	   $(if $(OEM_OTA_CONFIG), -o $(OEM_OTA_CONFIG)) \
	   $(BUILT_TARGET_FILES_PACKAGE) $@
else
$(INTERNAL_OTA_PACKAGE_TARGET): $(SPRD_BUILT_TARGET_FILES_PACKAGE) $(DISTTOOLS)
	@echo "Package OTA: $@"
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH MKBOOTIMG=$(MKBOOTIMG) \
	   ./build/tools/releasetools/ota_from_target_files -v \
	   --block \
	   -p $(HOST_OUT) \
	   -k $(KEY_CERT_PAIR) \
	   $(if $(OEM_OTA_CONFIG), -o $(OEM_OTA_CONFIG)) \
	   $(BUILT_TARGET_FILES_PACKAGE) $@
endif

ifeq (true, $(strip $(FOTA_UPDATE_SUPPORT)))
	@echo "Package fota-Package"
	-$(hide) bash $(GETOTAPACKAGE_SH) $(PRODUCT_OUT) $(KEY_CERT_PAIR) $(TARGET_DEVICE) $(HOST_LIBRARY_PATH) $(HOST_SHLIB_SUFFIX)
endif

