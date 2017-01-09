TELE_RES_DIR := vendor/sprd/telephony-res

################# apn
APN_VERSION := $(shell cat frameworks/base/core/res/res/xml/apns.xml|grep "<apns version"|cut -d \" -f 2)
apn_src_file := $(TELE_RES_DIR)/apn/apns-conf_$(APN_VERSION).xml
ifneq (,$(wildcard $(apn_src_file)))
PRODUCT_COPY_FILES += \
    $(apn_src_file):system/etc/apns-conf.xml \
    $(apn_src_file):system/etc/old-apns-conf.xml
else
$(warning "APN config file: $(apn_src_file) not found, apn version: $(APN_VERSION)")
endif

################# spn
spn_src_file := $(TELE_RES_DIR)/spn/spn-conf.xml

ifneq (,$(wildcard $(spn_src_file)))
PRODUCT_COPY_FILES += \
    $(spn_src_file):system/etc/spn-conf.xml
else
$(warning "SPN config file: $(spn_src_file) not found")
endif


################# operator name
operator_name_overlay_res := $(TELE_RES_DIR)/operatorname_overlay

ifneq (,$(wildcard $(operator_name_overlay_res)))
PRODUCT_PACKAGE_OVERLAYS += $(operator_name_overlay_res)
else
$(warning "Operator name overlay: $(operator_name_overlay_res) not found")
endif


################# volte-conf
volte_conf_src_file := $(TELE_RES_DIR)/volte/volte-conf.xml

ifneq (,$(wildcard $(volte_conf_src_file)))
PRODUCT_COPY_FILES += \
    $(volte_conf_src_file):system/etc/volte-conf.xml
else
$(warning "Volte config file: $(volte_conf_src_file) not found")
endif
