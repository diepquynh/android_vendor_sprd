$(info *** We will build pac locally and replace specific images when u "make sprdpac" ***)

#Now is a hardcode for basepac name! We need to change it when we have multi projects need local pack function
stamp := ${ANDROID_PRODUCT_OUT}/pac/${TARGET_PRODUCT}-${TARGET_BUILD_VARIANT}-native.pac

$(stamp): $(stamp)
	$(hide) bash vendor/sprd/build/buildpac/pack.sh

.PHONY: sprdpac
sprdpac: $(stamp)
