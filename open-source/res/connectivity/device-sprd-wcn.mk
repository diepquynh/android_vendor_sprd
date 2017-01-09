ifneq ($(strip $(CONNECTIVITY_HW_CHISET)),)
$(eval $(CONNECTIVITY_HW_CHISET))

ifeq ($(BOARD_SPRD_WCNBT_SR2351), true)
SPRD_WCNBT_CHISET := sr2351
endif
ifeq ($(BOARD_SPRD_WCNBT_MARLIN), true)
SPRD_WCNBT_CHISET := marlin
endif

endif


ifneq ($(strip $(BOARD_HAVE_SPRD_WCN_COMBO)),)
SPRD_WCNBT_CHISET := $(BOARD_HAVE_SPRD_WCN_COMBO)
endif


ifneq ($(strip $(SPRD_WCNBT_CHISET)),)
$(call inherit-product, vendor/sprd/open-source/res/connectivity/$(SPRD_WCNBT_CHISET)/connectivity.mk)
endif
