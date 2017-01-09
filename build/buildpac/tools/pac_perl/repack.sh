#! /bin/bash

PACK_DIR="vendor/sprd/build/buildpac"

BASE_DIR="$PACK_DIR/tools/pac_perl"
##Need to be modify as multi project##
PAC_SCRIPT="pac_sharkl5mode.pl"
IMG_DIR="$ANDROID_PRODUCT_OUT/pac"
AP_BASE="$PACK_DIR/tmpUnpackPac"
CP_BASE="$AP_BASE"

PROJ="$TARGET_PRODUCT-$TARGET_BUILD_VARIANT-native"
##Need to be modify as multi project##
MODEM_TYPE="SHARKL_GLOBAL_5MOD"
##Need to be modify as multi project##
PAC_PROJ="SharkL5ModeMarlinAndroid5.1"
##Need to be modify as multi project##
PAC_VER="MocorDroid6.0_trunk_SharkL"
##Need to be modify as multi project##
PAC_CONFILE="$AP_BASE/SharkL5ModeMarlinAndroid5.1.xml"

. $PACK_DIR/tools/pac_perl/modem_config

PAC_NAME="${PROJ}.pac"

case "$PROJ" in
  "sp9830a_5h10_native-userdebug-native") echo "Run pac_perl [$PROJ]"
  /usr/bin/perl $BASE_DIR/$PAC_SCRIPT $IMG_DIR/$PAC_NAME $PAC_PROJ $PAC_VER $PAC_CONFILE $AP_BASE/fdl1.bin $AP_BASE/fdl2.bin $AP_BASE/nvitem.bin $AP_BASE/prodnv.img $AP_BASE/u-boot-spl-16k.bin $AP_BASE/SC9600_sharkl_5modcsfb_volte.bin $AP_BASE/LTE_DSP.bin $AP_BASE/SHARKL_DM_DSP.bin $AP_BASE/SC9600_sharkl_wphy_5mod_volte_zc.bin $AP_BASE/PM_sharkl_arm7.bin $AP_BASE/fdl1.bin\(1\) $AP_BASE/EXEC_KERNEL_IMAGE0.bin $AP_BASE/boot.img $AP_BASE/dt.img $AP_BASE/recovery.img $AP_BASE/system.img $AP_BASE/userdata.img $AP_BASE/sprd_480854.bmp $AP_BASE/sprd_480854.bmp\(1\) $AP_BASE/cache.img $AP_BASE/u-boot.bin
  ;;
  "sp9830a_5h10_native_stg-userdebug-native") echo "Run pac_perl [$PROJ]"
  /usr/bin/perl $BASE_DIR/$PAC_SCRIPT $IMG_DIR/$PAC_NAME $PAC_PROJ $PAC_VER $PAC_CONFILE $AP_BASE/fdl1.bin $AP_BASE/fdl2.bin $AP_BASE/nvitem.bin $AP_BASE/prodnv.img $AP_BASE/u-boot-spl-16k.bin $AP_BASE/SC9600_sharkl_5modcsfb_volte.bin $AP_BASE/LTE_DSP.bin $AP_BASE/SHARKL_DM_DSP.bin $AP_BASE/SC9600_sharkl_wphy_5mod_volte_zc.bin $AP_BASE/PM_sharkl_arm7.bin $AP_BASE/fdl1.bin\(1\) $AP_BASE/EXEC_KERNEL_IMAGE0.bin $AP_BASE/boot.img $AP_BASE/dt.img $AP_BASE/recovery.img $AP_BASE/system.img $AP_BASE/userdata.img $AP_BASE/sprd_480854.bmp $AP_BASE/sprd_480854.bmp\(1\) $AP_BASE/cache.img $AP_BASE/u-boot.bin
  ;;
  "sp9830a_7h10_native-userdebug-native") echo "Run pac_perl [$PROJ]"
  /usr/bin/perl $BASE_DIR/$PAC_SCRIPT $IMG_DIR/$PAC_NAME $PAC_PROJ $PAC_VER $PAC_CONFILE $AP_BASE/fdl1.bin $AP_BASE/fdl2.bin $AP_BASE/nvitem.bin $AP_BASE/prodnv.img $AP_BASE/u-boot-spl-16k.bin $AP_BASE/SC9600_sharkl_5modcsfb_volte.bin $AP_BASE/LTE_DSP.bin $AP_BASE/SHARKL_DM_DSP.bin $AP_BASE/SC9600_sharkl_wphy_5mod_volte_zc.bin $AP_BASE/PM_sharkl_arm7.bin $AP_BASE/fdl1.bin\(1\) $AP_BASE/EXEC_KERNEL_IMAGE0.bin $AP_BASE/boot.img $AP_BASE/dt.img $AP_BASE/recovery.img $AP_BASE/system.img $AP_BASE/userdata.img $AP_BASE/sprd_480854.bmp $AP_BASE/sprd_480854.bmp\(1\) $AP_BASE/cache.img $AP_BASE/u-boot.bin
  ;;
  "sp9830a_7h10_native_stg-userdebug-native") echo "Run pac_perl [$PROJ]"
  /usr/bin/perl $BASE_DIR/$PAC_SCRIPT $IMG_DIR/$PAC_NAME $PAC_PROJ $PAC_VER $PAC_CONFILE $AP_BASE/fdl1.bin $AP_BASE/fdl2.bin $AP_BASE/nvitem.bin $AP_BASE/prodnv.img $AP_BASE/u-boot-spl-16k.bin $AP_BASE/SC9600_sharkl_5modcsfb_volte.bin $AP_BASE/LTE_DSP.bin $AP_BASE/SHARKL_DM_DSP.bin $AP_BASE/SC9600_sharkl_wphy_5mod_volte_zc.bin $AP_BASE/PM_sharkl_arm7.bin $AP_BASE/fdl1.bin\(1\) $AP_BASE/EXEC_KERNEL_IMAGE0.bin $AP_BASE/boot.img $AP_BASE/dt.img $AP_BASE/recovery.img $AP_BASE/system.img $AP_BASE/userdata.img $AP_BASE/sprd_480854.bmp $AP_BASE/sprd_480854.bmp\(1\) $AP_BASE/cache.img $AP_BASE/u-boot.bin
  ;;
  *) echo "Run pac_perl [$PROJ]"
  /usr/bin/perl $BASE_DIR/$PAC_SCRIPT $IMG_DIR/$PAC_NAME $PAC_PROJ $PAC_VER $PAC_CONFILE $AP_BASE/fdl1.bin $AP_BASE/fdl2.bin $AP_BASE/nvitem.bin $AP_BASE/prodnv.img $AP_BASE/u-boot-spl-16k.bin $AP_BASE/SC9600_sharkl_5modcsfb.bin $AP_BASE/LTE_DSP.bin $AP_BASE/SHARKL_DM_DSP.bin $AP_BASE/SC9600_sharkl_wphy_5mod.bin $AP_BASE/PM_sharkl_arm7.bin $AP_BASE/fdl1.bin\(1\) $AP_BASE/EXEC_KERNEL_IMAGE0.bin $AP_BASE/boot.img $AP_BASE/dt.img $AP_BASE/recovery.img $AP_BASE/system.img $AP_BASE/userdata.img $AP_BASE/sprd_480854.bmp $AP_BASE/sprd_480854.bmp\(1\) $AP_BASE/cache.img $AP_BASE/u-boot.bin
  ;;
esac
