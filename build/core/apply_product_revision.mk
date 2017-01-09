# Spreadtrum Communication Inc.

# This make file must be included after defining BOARDDIR and PLATDIR

# Here is a common config dir now
PRODUCT_REVISION_COMMON_CONFIG_PATH ?= vendor/sprd/feature_configs

ifeq ($(strip $(PRODUCT_REVISION)),)
# Don't warning, only show nothing to include
# $(warning You should define the PRODUCT_REVISION before you call include APPLY_PRODUCT_REVISION)
endif

# If you want to point to another folder to control the features, declare
# the PRODUCT_FEAULTE_LIST_PATH
ifneq ($(strip $(PRODUCT_FEAULTE_LIST_PATH)),)

PRIVATE_FEATURE_LIST_PATH := $(PRODUCT_FEAULTE_LIST_PATH)

else

PRIVATE_FEATURE_LIST_PATH := \
        $(BOARDDIR)/features \
        $(PLATDIR)/features $(PLATDIR)/common/features \
        $(PRODUCT_REVISION_COMMON_CONFIG_PATH)

endif

# If not define the PRODUCT_DEFAULT_REVISION, give the base to default
ifeq (,$(strip $(PRODUCT_DEFAULT_REVISION)))
PRODUCT_DEFAULT_REVISION := base
endif

# If not define the PRODUCT_REVISION, use default one.
ifeq ($(strip $(PRODUCT_REVISION)),)
PRODUCT_REVISION := $(PRODUCT_DEFAULT_REVISION)
endif

# If hudson or the builder define a revision, override configs before.
ifdef HUDSON_CONFIG_REVISION
PRIVATE_FEATURE_REVISION := $(HUDSON_CONFIG_REVISION)
else
PRIVATE_FEATURE_REVISION := $(PRODUCT_REVISION)
endif

# If no including for base, add base features
ifeq (,$(strip $(filter base,$(PRIVATE_FEATURE_REVISION))))
PRIVATE_FEATURE_REVISION := base $(PRIVATE_FEATURE_REVISION)
endif

$(info ===============================================================)
$(info PRODUCT_REVISION : "$(PRIVATE_FEATURE_REVISION)")
$(info ===============================================================)

# Debug
#$(warning DEBUG PRIVATE_FEATURE_LIST_PATH = $(PRIVATE_FEATURE_LIST_PATH))
#$(warning DEBUG PRIVATE_FEATURE_REVISION = $(PRIVATE_FEATURE_REVISION))

PRIVATE_FEATURE_MODULES :=
# Scan PLATDIR and BOARDDIR features folder and check wether they exists.
PRIVATE_FEATURE_MODULES :=

$(foreach feature,$(PRIVATE_FEATURE_REVISION), \
  $(foreach path,$(PRIVATE_FEATURE_LIST_PATH), \
     $(eval PRIVATE_FEATURE_MODULES := $(wildcard $(path)/$(feature)/config.mk) $(PRIVATE_FEATURE_MODULES) \
     ) \
  ) \
)

ifneq ($(wildcard $(strip $(PRIVATE_FEATURE_MODULES))),)
# Debug
# $(warning including $(PRIVATE_FEATURE_MODULES))

# include exists config.mk files.
include $(PRIVATE_FEATURE_MODULES)

#$(info PRIVATE_FEATURE_MODULES := $(PRIVATE_FEATURE_MODULES))

else
$(warning Failed to apply any modules)
endif

##### After including the features, use the results to do something more.

# The addon packages is equal to PRODUCT_PACKAGES, combine them.
# TODO In future, they may be put into another partition
FEATURES.PRODUCT_PACKAGES += $(FEATURES.PRODUCT_ADDON_PACKAGES)

# Debug
#$(warning dump FEATURES.all $(foreach v,$(_product_var_list), \
#   FEATURES.$(v) -> $(FEATURES.$(v)) ))

# TODO, the OEM property should use a delegate to control them, that may be much different currently
ifdef BOARD_OEMIMAGE_PARTITION_SIZE
# TODO, should generate the delegate. And try to put into the oem.prop
property_overrides := PRODUCT_OEM_PROPERTIES
else
# TODO, append them into ADDITIONAL_BUILD_PROPERTIES
property_overrides := PRODUCT_PROPERTY_OVERRIDES
endif

raw_product := PRODUCTS.$(INTERNAL_PRODUCT)

# Foreach FEATURES.PRODUCT* and add them to PRODUCT* currently.
# TODO In future, they may be put into another parition
$(foreach v,$(_product_var_list), \
    $(eval $(v) := $(FEATURES.$(v)) $($(v))) \
    $(eval $(raw_product).$(v) := $(FEATURES.$(v)) $($(raw_product).$(v))) \
)

# To control the oem.prop
ifndef BOARD_OEMIMAGE_PARTITION_SIZE
# The properties has been set into the ADDITIONAL_BUILD_PROPERTIES, so append them.
ADDITIONAL_BUILD_PROPERTIES += $(FEATURES.PRODUCT_PROPERTY_OVERRIDES)
#$(info ADDITIONAL_BUILD_PROPERTIES := $(ADDITIONAL_BUILD_PROPERTIES))
endif

# The Locales will be overwrite completely
ifdef FEATURES.PRODUCT_LOCALES
PRODUCT_AAPT_CONFIG := $(strip $(filter-out $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_LOCALES),$(PRODUCT_AAPT_CONFIG)))
PRODUCT_LOCALES := $(strip $(FEATURES.PRODUCT_LOCALES))
PRODUCT_AAPT_CONFIG := $(strip $(PRODUCT_LOCALES) $(PRODUCT_AAPT_CONFIG))
#$(info PRODUCT_LOCALES := $(PRODUCT_LOCALES))
endif

#$(info )

# Debug
#$(warning PRODUCT_PACKAGES after including $(PRODUCT_PACKAGES) PRODUCT_BOOT_JARS -> $(PRODUCT_BOOT_JARS))

# clear vars
PRIVATE_FEATURE_MODULES :=
PRIVATE_FEATURE_LIST_PATH :=
PRIVATE_FEATURE_REVISION :=
