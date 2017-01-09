#!/bin/bash

ROOTDIR=$(pwd)
CERTPATH=$ROOTDIR/../bin

if [ ! -f $CERTPATH/primary_debug.cert ] ;
	then echo -e "\033[31m primary_debug.cert is not exist,pls execute the script in mkprimarycert document  first! \033[0m "
fi

if [ ! -f $CERTPATH/u-boot-spl-16k-sign.bin ] ;
	then echo -e "\033[31m u-boot-spl-16k-sign.bin is not exist,pls execute make chipram  first! \033[0m "
fi

if [ ! -f $CERTPATH/fdl1-sign.bin ] ;
	then echo -e "\033[31m fdl1-sign.bin is not exist, pls execute make chipram  first! \033[0m "
fi

read -p "enter your device serial number type [ 1:soc_id 2:uid ]    " type

if  [[ $type -eq 1 ]];then
    echo "your device id is soc_id"
	echo -n " pls input parameter like:  0xfacd...de  0xffff eg. "
    read socid mask
if [ "$socid" = "" -o "$mask" = "" ];then
	 echo -e "\033[31m pls input command: mkdbimg 0xface...de 0xffff eg.\033[0m "
     exit
 fi
elif [[ $type -eq 2 ]]; then
    echo "your device is is uid"
	echo -n  " pls input parameter like:  0x1234 0x5678 0xffff eg. "
    read uid0 uid1 mask
if [ "$uid0" = "" -o "$uid1" = "" -o "$mask" = "" ] ;
	then echo -e "\033[31m pls input command: mkdbimg 0x1234 0x5678 0xffff eg.\033[0m "
	    exit
fi
else
    echo -e "\033[31m pls re-run this script and make a choose: soc_id or uid \033[0m"
    exit
fi


IMAGE_FDL1=$CERTPATH/fdl1-sign.bin
IMAGE_SPL=$CERTPATH/u-boot-spl-16k-sign.bin
KEY_PATH=$ROOTDIR/../config
CERT=$CERTPATH/primary_debug.cert
UID0=$uid0
UID1=$uid1
SOCID=$socid
MASK=$mask

dosprdmkdbimg()
{

if [ -f $IMAGE_FDL1 ] ; then
if [[ $type -eq 1 ]]; then
	$ROOTDIR/../bin/mkdbimg $IMAGE_FDL1 $CERT $KEY_PATH $SOCID $MASK
elif [[ $type -eq 2 ]]; then
	$ROOTDIR/../bin/mkdbimg $IMAGE_FDL1 $CERT $KEY_PATH $UID0 $UID1 $MASK
fi
	echo -e "\033[33m mkdbimg fdl1-sign.bin ok!\033[0m "
fi
if [ -f $IMAGE_SPL ] ;then
if [[ $type -eq 1 ]];then
	 $ROOTDIR/../bin/mkdbimg $IMAGE_SPL $CERT $KEY_PATH $SOCID $MASK
elif [[ $type -eq 2 ]];then
	 $ROOTDIR/../bin/mkdbimg $IMAGE_SPL $CERT $KEY_PATH $UID0 $UID1 $MASK
 fi
		echo -e "\033[33m mkdbimg spl-sign.bin ok! \033[0m "
fi

}

dosprdmkdbimg
