CONN_PRODUCT_PATH := $(shell find $(PLATDIR) -name $(TARGET_PRODUCT).mk)
CONN_PRODUCT_PATH_DIR := $(dir $(CONN_PRODUCT_PATH))
CONN_BOARD_FOLDER := $(shell var=$(CONN_PRODUCT_PATH_DIR) ; echo $${var%/*})
CONN_BOARD_NAME := $(notdir $(CONN_BOARD_FOLDER))
CONN_BOARD_CONFIG :=$(CONN_PRODUCT_PATH_DIR)BoardConfig.mk

CONN_KEYWORD := BOARD_SPRD_WCNBT
CONN_USE_SPRD_BT := $(shell grep $(CONN_KEYWORD) $(CONN_BOARD_CONFIG))
CONNECTIVITY_HW_CONFIG := $(CONN_BOARD_NAME)
$(eval $(CONN_USE_SPRD_BT))

ifeq ($(BOARD_SPRD_WCNBT_SR2351), true)
SPRD_WCNBT_CHISET := sr2351
endif

ifeq ($(BOARD_SPRD_WCNBT_MARLIN), true)
SPRD_WCNBT_CHISET := marlin
endif

ifneq ($(strip $(BOARD_HAVE_SPRD_WCN_COMBO)),)
SPRD_WCNBT_CHISET := $(BOARD_HAVE_SPRD_WCN_COMBO)
endif


ifneq ($(strip $(SPRD_WCNBT_CHISET)),)
$(call inherit-product, vendor/sprd/modules/wcn/connconfig/$(SPRD_WCNBT_CHISET)/connectivity.mk)
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := vendor/sprd/modules/wcn/bt/libbt/conf/sprd/$(SPRD_WCNBT_CHISET)/include \
                                               vendor/sprd/modules/wcn/bt/libbt/include
endif

ifneq ($(strip $(SPRD_WCNBT_CHISET)),)
PRODUCT_PACKAGES += hcidump \
                   libbqbbt
endif
