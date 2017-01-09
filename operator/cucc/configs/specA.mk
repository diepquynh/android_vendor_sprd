# CUCC SpecA
# $(call inherit-product, vendor/sprd/operator/cucc/specA.mk)
# CUCC Prebuild packages
# Attention the packages name is treat as LOCAL_MODULE when building,
# the items should be unique.
PRODUCT_PACKAGES += \
	SohuNewsClient_CUCC \
	Weibo_CUCC \
	BaiduMap_CUCC \
	SogouInput_CUCC \
	Wo116114 \
	UnicomClient \
	WoStore \
	WPSOffice_CUCC \
    WoMenHu \
    GoogleInputMethod_CUCC \
	SystemHelper \
    GuoWuYuan



PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator=cucc \
	ro.operator.version=specA


