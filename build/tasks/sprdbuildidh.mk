$(info *** we will build IDH while u make sprdidh ***)

#IDH Release Solution One, for Git release.
#proprietories source whitelist, need to be remove in the furture.
PROP_SOURCE_ZIP=vendor/sprd/release/IDH/proprietories_open-source.zip
ifneq ($(IDH_PROP_ZIP),)
_idh_proprietories_source_list := \
	$(shell unzip -o $(PROP_SOURCE_ZIP) 2>/dev/null)
_idh_proprietories_file_list := \
	$(shell unzip -o $(IDH_PROP_ZIP) 2>/dev/null)
$(info *** unzip IDH proprietorie zip file:$(IDH_PROP_ZIP) done ***)
endif


#IDH Release Solution Two, for IDH packet release.
IDH_BUILD_SYSTEM=vendor/sprd/build/buildidh
ifneq ($(SPRD_IDH_PROP),)
  ifeq '$(SPRD_IDH_PROP)' '$(wildcard $(SPRD_IDH_PROP))'
    unzip_prop_idh := \
	$(shell cp -r $(SPRD_IDH_PROP)/target/product/*/* out/target/product/${TARGET_DEVICE}/)
    $(info import IDH proprietorie file:$(SPRD_IDH_PROP) done)
  else
    $(warning No such proprietorie file: SPRD_IDH_PROP = $(SPRD_IDH_PROP))
  endif
else
  unzip_prop_idh := $(shell bash $(IDH_BUILD_SYSTEM)/unzip_prop.sh ${TARGET_DEVICE})
endif

OUT_IDH_DIR=$(OUT_DIR)/IDH
idh.stamp := ${OUT_IDH_DIR}/proprietories-${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}.zip

$(idh.stamp): IDH_REPO := $(strip $(shell command repo forall --group=idh -c printenv REPO_PATH))
$(idh.stamp): _ignore := $(strip $(foreach m,$(ALL_MODULES), \
    $(foreach n,$(IDH_REPO), \
        $(if $(filter $(n)%,$(ALL_MODULES.$(m).PATH)), \
            $(if $(filter $(shell dirname $(n)),$(shell dirname $(ALL_MODULES.$(m).PATH))), \
                $(if $(filter $(shell basename $(n)),$(shell basename $(ALL_MODULES.$(m).PATH))), \
                    $(eval IDH_BUILTS := $(IDH_BUILTS) $(ALL_MODULES.$(m).INSTALLED) \
                        $(ALL_MODULES.$(m).BUILT) $(ALL_MODULES.$(m).INTERMEDIATES) $(ALL_MODULES.$(m).INTERMEDIATES_COMMON)), \
                $(info not in same dir, match failed)) \
        , $(eval IDH_BUILTS := $(IDH_BUILTS) $(ALL_MODULES.$(m).INSTALLED) \
            $(ALL_MODULES.$(m).BUILT) $(ALL_MODULES.$(m).INTERMEDIATES) $(ALL_MODULES.$(m).INTERMEDIATES_COMMON)),), \
         ) \
     )\
))

# IDH release no trusty sources.
# RELEASE_DIR:=
# ifeq ($(strip $(BOARD_TEE_CONFIG)), trusty)
RELEASE_DIR :=vendor/sprd/proprietories-source/sprdtrusty/vendor/sprd/modules/common
IDH_BUILTS +=${PRODUCT_OUT}/tos.bin ${TARGET_OUT_SHARED_LIBRARIES}/hw/keystore.$(TARGET_BOARD_PLATFORM).so
# endif

ifeq ($(strip $(BOARD_VMM_CONFIG)), true)
IDH_BUILTS +=${PRODUCT_OUT}/vmm.bin
endif

$(idh.stamp):
	mkdir -p $(OUT_IDH_DIR);zip -ry $(idh.stamp) $(IDH_BUILTS);bash $(IDH_BUILD_SYSTEM)/makeidh.sh ${RELEASE_DIR}


.PHONY: sprdidh
sprdidh : $(idh.stamp)
