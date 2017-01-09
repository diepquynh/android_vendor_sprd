#!/bin/bash
# $1 - project name : e.g.

ROOTPATH="otaPackage"
mkdir -p $ROOTPATH

#build
mkdir -p  $ROOTPATH/build/target/product/
cp -a build/target/product/security/  $ROOTPATH/build/target/product/
mkdir -p $ROOTPATH/build/tools/
cp -ur build/tools/releasetools/  $ROOTPATH/build/tools/
#device
mkdir -p  $ROOTPATH/device/sprd/scx35l/common/
cp -u device/sprd/scx35l/common/modem.cfg device/sprd/scx35l/common/nvmerge.cfg $ROOTPATH/device/sprd/scx35l/common/
#out
mkdir -p $ROOTPATH/out/host/linux-x86/bin/
cp -u out/host/linux-x86/bin/minigzip  out/host/linux-x86/bin/mkbootfs  out/host/linux-x86/bin/mkbootimg  out/host/linux-x86/bin/fs_config out/host/linux-x86/bin/zipalign  out/host/linux-x86/bin/aapt  out/host/linux-x86/bin/bsdiff  out/host/linux-x86/bin/imgdiff  out/host/linux-x86/bin/mkuserimg.sh  out/host/linux-x86/bin/make_ext4fs  out/host/linux-x86/bin/simg2img  out/host/linux-x86/bin/e2fsck  $ROOTPATH/out/host/linux-x86/bin/
cp -u out/host/linux-x86/bin/build_verity_tree  out/host/linux-x86/bin/verity_signer  out/host/linux-x86/bin/append2simg  out/host/linux-x86/bin/boot_signer $ROOTPATH/out/host/linux-x86/bin/
mkdir -p $ROOTPATH/out/host/linux-x86/framework
cp -u out/host/linux-x86/framework/signapk.jar out/host/linux-x86/framework/dumpkey.jar out/host/linux-x86/framework/BootSignature.jar $ROOTPATH/out/host/linux-x86/framework/
mkdir -p $ROOTPATH/$4/
cp -u $4/libc++$5 $4/liblog$5 $4/libcutils$5 $4/libselinux$5 $4/libcrypto-host$5 $4/libdivsufsort$5 $4/libdivsufsort64$5 $4/libext2fs-host$5 $4/libext2_blkid-host$5 $4/libext2_com_err-host$5 $4/libext2_e2p-host$5 $4/libext2_profile-host$5 $4/libext2_quota-host$5 $4/libext2_uuid-host$5 $4/libconscrypt_openjdk_jni$5 $4/libbrillo$5 $4/libbrillo-stream$5 $4/libbrillo-http$5 $4/libchrome$5 $4/libcurl-host$5 $4/libevent-host$5 $4/libprotobuf-cpp-lite$5 $4/libssl-host$5 $4/libz-host$5 $4/libbase$5 $ROOTPATH/$4/
mkdir -p $ROOTPATH/system/extras/verity/
cp -u system/extras/verity/build_verity_metadata.py $ROOTPATH/system/extras/verity/
#mkdir -p $ROOTPATH/out/host/linux-x86/lib64/
#vendor
mkdir -p $ROOTPATH/vendor/sprd/tools/ota
cp -u vendor/sprd/tools/ota/releasetools.py $ROOTPATH/vendor/sprd/tools/ota/
#system_image_info
mkdir -p $ROOTPATH/out/target/product/$3/root/
cp -u $1/obj/PACKAGING/systemimage_intermediates/system_image_info.txt $ROOTPATH/
cp -u $1/root/file_contexts.bin  $ROOTPATH/out/target/product/$3/root/
#target_files
if [ $# -ge 6 ] && [ $6 = IMG ] ; then
cp -u $1/img-target-files.zip $ROOTPATH/ota_target_files.zip
mkdir -p IMAGES/
cp -u $1/obj/PACKAGING/systemimage_intermediates/system.map IMAGES/system.map
zip -q $ROOTPATH/ota_target_files.zip IMAGES/system.map
rm -rf IMAGES
cp $1/obj/PACKAGING/systemimage_intermediates/system.map $1/system.map
else
echo `ls -lrt $1/obj/PACKAGING/target_files_intermediates/*target_files*.zip|tail -n 1|awk '{print $NF}'`
cp -u `ls -lrt $1/obj/PACKAGING/target_files_intermediates/*target_files*.zip|tail -n 1|awk '{print $NF}'`  $ROOTPATH/ota_target_files.zip
fi

#build.prop
cp -u $1/system/build.prop $ROOTPATH/build.prop

#configure.xml
echo "">$ROOTPATH/configure.xml
echo "<root>">>$ROOTPATH/configure.xml

#version
var=$(grep  "ro.fota.version=" "$1/system/build.prop" )
buildnumber=${var##"ro.fota.version="}
echo "<buildnumber>$buildnumber</buildnumber>">>$ROOTPATH/configure.xml

#language
if [ -n "`grep "ro.fota.language=" $1/system/build.prop`" ] ; then
    var=$(grep  "ro.fota.language=" "$1/system/build.prop" )
    echo "<language>${var##"ro.fota.language="}</language>">>$ROOTPATH/configure.xml
elif [ -n "`grep "ro.product.locale=" $1/system/build.prop`" ] ; then
    var=$(grep  "ro.product.locale=" "$1/system/build.prop" )
    echo "<language>${var##"ro.product.locale="}</language>">>$ROOTPATH/configure.xml
elif [ -n "`grep "ro.product.locale.language=" $1/system/build.prop`" ] ; then
    var=$(grep  "ro.product.locale.language=" "$1/system/build.prop" )
    echo "<language>${var##"ro.product.locale.language="}</language>">>$ROOTPATH/configure.xml
else
    echo "<language>en</language>">>$ROOTPATH/configure.xml
fi

#oem
var=$(grep  "ro.fota.oem=" "$1/system/build.prop" )
echo "<oem>${var##"ro.fota.oem="}</oem>">>$ROOTPATH/configure.xml

#operator
var=$(grep  "ro.operator.optr=" "$1/system/build.prop")
if [ "$var" = "" ] ; then
  var=other
else
var=$(echo $var|tr A-Z a-z)
if [ ${var##"ro.operator.optr="} = op01 ] ; then
var=CMCC
elif [ ${var##"ro.operator.optr="} = op02 ] ; then
var=CU
else
var=other
fi
fi
echo "<operator>${var##"ro.operator.optr="}</operator>">>$ROOTPATH/configure.xml

#device
var=$(grep  "ro.fota.device=" "$1/system/build.prop" )
product=${var##"ro.fota.device="}
echo "<product>$product</product>">>$ROOTPATH/configure.xml

#publishtime
echo "<publishtime>$(date +20%y%m%d%H%M%S)</publishtime>">>$ROOTPATH/configure.xml

#提取versionname
echo "<versionname>$buildnumber</versionname>">>$ROOTPATH/configure.xml
#key
if [ $# -ge 2 ] ; then
echo "<key>$2</key>">>$ROOTPATH/configure.xml
fi
echo "</root>">>$ROOTPATH/configure.xml

#productName
var=$(grep  "ro.product.name=" "$1/system/build.prop" )
productName=${var##"ro.product.name="}
productName=${productName/ /}

#buildType
var=$(grep  "ro.build.type=" "$1/system/build.prop" )
buildType=${var##"ro.build.type="}
buildType=${buildType/ /}

if [ -f $1/${productName}-fota-Package.zip ]; then
echo "delete exist file:$1/${productName}-fota-Package.zip"
rm -f $1/${productName}-fota-Package.zip
fi

if [ -f $1/img-target-files.zip ]; then
echo "delete exist file:$1/img-target-files.zip"
rm -f $1/img-target-files.zip
fi

#zip
cd otaPackage
zip -rq otaPackage.zip build device out system vendor configure.xml build.prop ota_target_files.zip
cd ..
mv otaPackage/otaPackage.zip $1/${productName}-fota-Package.zip
rm -rf otaPackage

