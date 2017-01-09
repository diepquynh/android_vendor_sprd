#!/bin/bash
IMG_DIR=$PWD
MODEM_DIR=$1
PROJ=$2
MODEM_LTE=ltemodem.bin
#ltemodem.bin
DSP_LTE_TG=ltetgdsp.bin
DSP_LTE_LDSP=ltedsp.bin
DSP_LTE_AG=lteagdsp.bin
#pmsys.bin
DFS=pmsys.bin

STOOL_PATH=$IMG_DIR/out/host/linux-x86/bin
SPRD_CONFIG_FILE=$IMG_DIR/vendor/sprd/proprietories-source/packimage_scripts/signimage/sprd/config
SANSA_CONFIG_PATH=$IMG_DIR/vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa/config
SANSA_OUTPUT_PATH=$IMG_DIR/vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa/output
SANSA_PYPATH=$IMG_DIR/vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa/python

getImageName()
{
    local imagename=$1
    echo ${imagename##*/}
}

getSignImageName()
{
    local temp1=$(getImageName $1)
    local left=${temp1%.*}
    local temp2=$(getImageName $1)
    local right=${temp2##*.}
    local signname=${left}"-sign."${right}
    echo $signname
}

getRawSignImageName()
{
    local temp1=$1
    local left=${temp1%.*}
    local temp2=$1
    local right=${temp2##*.}
    local signname=${left}"-sign."${right}
    echo $signname
}

getLogName()
{
    local temp=$(getImageName $1)
    local left=${temp%.*}
    local name=$SANSA_OUTPUT_PATH/${left}".log"
    echo $name
}

doModemInsertHeader()
{
    echo "doModemInsertHeader enter"
    for loop in $@
    do
        if [ -f $loop ] ; then
            $STOOL_PATH/imgheaderinsert $loop 0
        else
            echo "#### no $loop,please check ####"
        fi
    done

    echo "doModemInsertHeader leave. now list"
	  ls ${IMG_DIR}/${PROJ}
    echo "list end"
}

makeModemCntCert()
{
    local CNTCFG=${SANSA_CONFIG_PATH}/certcnt_modem.cfg
    #local CNTCFG=${SANSA_CONFIG_PATH}/certcnt_3.cfg
    local SW_TBL=${SANSA_CONFIG_PATH}/SW.tbl

    sed -i "s#.* .* .*#$1 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
    if [ -f $1 ] ; then
        python3 $SANSA_PYPATH/bin/cert_sb_content_util.py $CNTCFG $(getLogName $1)
    else
        echo "#### no $1,please check ####"
    fi
}

genModemCntCfgFile()
{
    local CNTCFG=${SANSA_CONFIG_PATH}/certcnt_modem.cfg
    local KEY=${SANSA_CONFIG_PATH}/key_3.pem
    local KEYPWD=${SANSA_CONFIG_PATH}/pass_3.txt
    local SWTBL=${SANSA_CONFIG_PATH}/SW.tbl
    local NVID=2
    local NVVAL=1
    local CERTPKG=${SANSA_OUTPUT_PATH}/certcnt.bin

    echo "enter genModemCntCfgFile "

    if [ -f "$CNTCFG" ]; then
        rm -rf $CNTCFG
    fi

    touch ${CNTCFG}
    echo "[CNT-CFG]" >> ${CNTCFG}
    echo "cert-keypair = ${KEY}" >> ${CNTCFG}
    echo "cert-keypair-pwd = ${KEYPWD}" >> ${CNTCFG}
    echo "images-table = ${SWTBL}" >> ${CNTCFG}
    echo "nvcounter-id = ${NVID}" >> ${CNTCFG}
    echo "nvcounter-val = ${NVVAL}" >> ${CNTCFG}
    echo "cert-pkg = ${CERTPKG}" >> ${CNTCFG}
}

doSansaModemSignImage()
{
    echo "doSansaModemSignImage: $@ "
    genModemCntCfgFile

    for loop in $@
    do
        if [ -f $loop ] ; then
            makeModemCntCert $loop
            $STOOL_PATH/signimage $SANSA_OUTPUT_PATH $(getRawSignImageName $loop) 1 0
        else
            echo "#### no $loop,please check ####"
        fi
    done
    echo "doSansaModemSignImage leave. now list"
#	  ls ${IMG_DIR}/${PROJ}
    echo "list end"
}

doSprdModemSignImage()
{
    for loop in $@
    do
        if [ -f $loop ] ; then
	    $STOOL_PATH/sprd_sign $(getRawSignImageName $loop) $SPRD_CONFIG_FILE
        else
            echo "#### no $loop,please check ####"
        fi
    done
}

doModemSignImage()
{
    echo "doModemSignImage  proj = $PROJ"
    if (echo $PROJ | grep "spsec") 1>/dev/null 2>&1 ; then
        doSprdModemSignImage $@
    else
        if (echo $PROJ | grep "dxsec") 1>/dev/null 2>&1 ; then
            doSansaModemSignImage $@
        fi
    fi
}

doModemPackImage()
{
    doModemInsertHeader $@
    doModemSignImage $@
}

doModemPackImage $MODEM_DIR/$MODEM_LTE $MODEM_DIR/$DSP_LTE_TG $MODEM_DIR/$DSP_LTE_LDSP $MODEM_DIR/$DSP_LTE_AG $MODEM_DIR/$DFS
mv $MODEM_DIR/$(getSignImageName $MODEM_LTE) $MODEM_DIR/$MODEM_LTE
mv $MODEM_DIR/$(getSignImageName $DSP_LTE_TG) $MODEM_DIR/$DSP_LTE_TG
mv $MODEM_DIR/$(getSignImageName $DSP_LTE_LDSP) $MODEM_DIR/$DSP_LTE_LDSP
mv $MODEM_DIR/$(getSignImageName $DSP_LTE_AG) $MODEM_DIR/$DSP_LTE_AG
mv $MODEM_DIR/$(getSignImageName $DFS) $MODEM_DIR/$DFS
