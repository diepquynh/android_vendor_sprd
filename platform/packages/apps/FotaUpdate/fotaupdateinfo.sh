#!/bin/bash

echo ""
echo "# begin fota properties"
echo "ro.fota.platform=Sprd9832_7.0"
#type info: phone, pad ,box, tv
echo "ro.fota.type=phone"
echo "ro.fota.app=5"
#oem info
echo "ro.fota.oem=Sprd9832_7.0"
#model info, Settings->About phone->Model number
FotaDevice=$(grep "ro.product.model=" "$1"|awk -F "=" '{print $NF}' )
echo "ro.fota.device=$FotaDevice" | sed 's/\$//' | sed 's/\///' | sed 's/\\//'
#version number, Settings->About phone->Build number
#echo "ro.fota.version=`date +%Y%m%d-%H%M`"
#echo "ro.fota.version=$(grep "ro.product.model=" "$1"|awk -F "=" '{print $NF}' )`date +%Y%m%d-%H%M`"
FotaVersion=$(grep "ro.build.display.id=" "$1"|awk -F "=" '{print $NF}' )
echo "ro.fota.version=$FotaVersion" | sed 's/\$//' | sed 's/\///' | sed 's/\\//'
echo "# end fota properties"
