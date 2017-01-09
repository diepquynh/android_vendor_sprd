#/bin/bash
curpath=$(pwd)
sansapath=$curpath/vendor/sprd/proprietories-source/packimage_scripts/signimage/sansa
cfgpath=$sansapath/config
outpath=$sansapath/output
pypath=$sansapath/python
certa=$cfgpath/certa.cfg
certb=$cfgpath/certb.cfg
certc=$cfgpath/certc.cfg
certcnt1_enc=$cfgpath/certcnt_1_enc.cfg
certcnt1=$cfgpath/certcnt_1.cfg
certcnt2=$cfgpath/certcnt_2.cfg
certcnt3=$cfgpath/certcnt_3.cfg
certp=$cfgpath/certp.cfg
certs=$cfgpath/certs.cfg
SW_TBL=$cfgpath/SW.tbl
IMAGE_NAME=$1

getImageName()
{
    local imagename=$1
    echo ${imagename##*/}
}

getRawImageName()
{
    local imagename=$1
    echo ${imagename/"-sign"/}
}

getLogName()
{
    local temp1=$(getImageName $IMAGE_NAME)
    local leftname=${temp1%.*}
    local temp2=$(getImageName $1)
    local rightname=${temp2%.*}
    local name=$outpath/${leftname}"_"${rightname}".log"
    echo $name
}

makeKeyCert()
{
    for loop in $@
    do
        if [ -f $loop ] ; then
            python3 $pypath/bin/cert_key_util.py $loop $(getLogName $loop)
        else
            echo "#### no $loop,please check ####"
        fi
    done
}

makeContentCert()
{
    for loop in $@
    do
        if [ -f $loop ] ; then
            python3 $pypath/bin/cert_sb_content_util.py $loop $(getLogName $loop)
        else
            echo "#### no $loop,please check ####"
        fi
    done
}

makeDebugCert()
{
    if [ -f $certp ] && [ -f $certs ]; then
        python3 $pypath/bin/cert_dbg_primary_util.py $certp $(getLogName $certp)
        python3 $pypath/bin/cert_dbg_secondary_util.py $certs $(getLogName $certs)
    else
        echo "#### no debug cert,please check ####"
    fi
}

doSansaSign()
{
    if [ ! -d "$outpath" ]; then
        mkdir -p "$outpath"
    fi
    case $(getImageName $1) in
        "u-boot-spl-16k-sign.bin")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xD000 0x200#" $SW_TBL
            makeKeyCert $certa $certb
	    if [ "DISABLE"x = $2x ]; then
            	makeContentCert $certcnt1
	    else
            	makeContentCert $certcnt1_enc
	    fi
            makeDebugCert
            ;;
        "fdl1-sign.bin")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xD000 0x200#" $SW_TBL
            makeKeyCert $certa $certb
	    if [ "DISABLE"x = $2x ]; then
            	makeContentCert $certcnt1
	    else
            	makeContentCert $certcnt1_enc
	    fi
            makeDebugCert
            ;;
        "sml-sign.bin")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
            makeContentCert $certcnt1
            ;;
        "tos-sign.bin")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
            makeContentCert $certcnt1
            ;;
        "u-boot-sign.bin")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
            makeKeyCert $certc
            makeContentCert $certcnt2
            ;;
        "fdl2-sign.bin")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
            makeKeyCert $certc
            makeContentCert $certcnt2
            ;;
        "boot-sign.img")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
            makeContentCert $certcnt3
            ;;
        "recovery-sign.img")
            sed -i "s#.* .* .*#$(getRawImageName $1) 0xFFFFFFFFFFFFFFFF 0x200#" $SW_TBL
            makeContentCert $certcnt3
            ;;
        *)
            ;;
    esac
}

doSansaSign $1 $2


