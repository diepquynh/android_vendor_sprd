#!/bin/bash

IMG_PATH=

# functions
gen_key()
{
    echo "Generating $BSCKEY1 ......"
    ./RSAKeyGen -pw $PASSW1 -pn $BSCKEY1 >/dev/null
    #echo "RSAKeyGen key1 result =" $?
    if [ $? -ne 0 ]; then
        echo "Generate $BSCKEY1 failed!"
        return 1;
    else
        echo "Generate $BSCKEY1 succeed!"
    fi

    echo "Generating $BSCKEY2 ......"
    ./RSAKeyGen -pw $PASSW2 -pn $BSCKEY2 >/dev/null
    if [ $? -ne 0 ]; then
        echo "Generate $BSCKEY2 failed!"
        return 1;
    else
        echo "Generate $BSCKEY2 succeed!"
    fi

    echo "Generating $VLRKEY ......"
    ./RSAKeyGen -pw $PASSW3 -pn $VLRKEY >/dev/null
    if [ $? -ne 0 ]; then
        echo "Generate $VLRKEY failed!"
        return 1;
    else
        echo "Generate $VLRKEY succeed!"
    fi
    return 0;
}

bsc_sign()
{
    local index=0

    for i in ${BSC_BIN[@]}
    do
        echo "will sign $i to ${BSC_SIGN[index]}"
        if [ ! -e $IMG_PATH/$i ]; then
            echo "$IMG_PATH/$i not found, skip!"
            continue
        fi
        ./BscGen -img $IMG_PATH/$i -out $IMG_PATH/${BSC_SIGN[index]} -pw $PASSW1 -pn $BSCKEY1 -ha sha1 -pw2 $PASSW2 -pn2 $BSCKEY2 >/dev/null
        if [ $? -ne 0 ]; then
            echo "Sign $i failed!"
            #return 1;
        else
            echo -e "\033[32mSign $i succeed!\033[0m"
        fi
        index=$(($index+1))
    done

    return 0;
}

vlr_sign()
{
    local index=0

    for i in ${VLR_BIN[@]}
    do
        echo "will sign $i to ${VLR_SIGN[index]}"
        if [ ! -e $IMG_PATH/$i ]; then
            echo "$IMG_PATH/$i not found, skip!"
            continue
        fi
        ./VLRSign -img $IMG_PATH/$i -out $IMG_PATH/${VLR_SIGN[index]} -pw $PASSW2 -pn $BSCKEY2 -ipbk true -ha sha1 -pw2 $PASSW3 -pn2 $VLRKEY >/dev/null
        if [ $? -ne 0 ]; then
            echo "Sign $i failed!"
            #return 1;
        else
            echo -e "\033[32mSign $i succeed!\033[0m"
        fi
        index=$(($index+1))
    done

    return 0;
}

vlr_sign_other()
{
    local index=0

    for i in ${VLR_OTHER_BIN[@]}
    do
        echo "will sign $i to ${VLR_OTHER_SIGN[index]}"
        if [ ! -e $IMG_PATH/$i ]; then
            echo "$IMG_PATH/$i not found, skip!"
            continue
        fi
        ./VLRSign -img $IMG_PATH/$i -out $IMG_PATH/${VLR_OTHER_SIGN[index]} -pw $PASSW3 -pn $VLRKEY >/dev/null
        if [ $? -ne 0 ]; then
            echo "Sign $i failed!"
            #return 1;
        else
            echo -e "\033[32mSign $i succeed!\033[0m"
        fi
        index=$(($index+1))
    done

    return 0;
}

help_info()
{
    echo "missing parameter:"
    echo "Usage: ./sig_script.sh IMAGE_PATH"
}

# Main function
# read input parameter
if [ $# -le 0 ]; then
    help_info
    exit
fi
IMG_PATH=$1

# Read config
while read line;do
    eval "$line"
done < sig_key.ini

BSC_BIN=(`awk -F'BSC_BIN=' '{print $2}' sig_bin.ini`)
BSC_SIGN=(`awk -F'BSC_SIGN=' '{print $2}' sig_bin.ini`)
VLR_BIN=(`awk -F'VLR_BIN=' '{print $2}' sig_bin.ini`)
VLR_SIGN=(`awk -F'VLR_SIGN=' '{print $2}' sig_bin.ini`)
VLR_OTHER_BIN=(`awk -F'VLR_OTHER_BIN=' '{print $2}' sig_bin.ini`)
VLR_OTHER_SIGN=(`awk -F'VLR_OTHER_SIGN=' '{print $2}' sig_bin.ini`)

#echo "arr bsc_bin: ${BSC_BIN[@]}"
#echo "arr bsc_sign: ${BSC_SIGN[@]}"
#echo "arr vlr_bin: ${VLR_BIN[@]}"
#echo "arr vlr_sign: ${VLR_SIGN[@]}"
#echo "arr vlr_other_bin: ${VLR_OTHER_BIN[@]}"
#echo "arr vlr_other_sign: ${VLR_OTHER_SIGN[@]}"

#echo $PASSW1
#echo $PASSW2
#echo $PASSW3
#echo $BSCKEY1
#echo $BSCKEY2
#echo $VLRKEY

# Check tool files
if [ ! -e BscGen ]; then
    echo -e "\033[31mBscGen does not exist, please check! \033[0m"
    exit
fi
if [ ! -e RSAKeyGen ]; then
    echo -e "\033[31mRSAKeyGen does not exist, please check! \033[0m"
    exit
fi
if [ ! -e VLRSign ]; then
    echo -e "VLRSign does not exist, please check! \033[0m"
    exit
fi

echo -e "\033[34m=========\033[0m"
echo -e "\033[34m* START *\033[0m"
echo -e "\033[34m=========\033[0m"
if [ -e key.db ]; then
# comments Start
if false; then
# comments Start
    while true
    do
        read -p "key.db already exists, do you want to regenerate? [Y/N]?" answer
        case $answer in
        Y | y)
            echo "Deleting key.db"
            rm key.db
            #echo "del key.db result is:" $?
            echo "run function gen_key()"
            gen_key
            ret=$?
            echo "gen_key ret = "$ret
            break;;
        N | n)
            echo "Using existing keys"
            break;;
        *)
            if [ -z "$answer" ]; then
                echo "Using existing keys"
                break;
            else
                echo "Please enter Y or N"
            fi
        esac
    done
# comments End
fi
# comments End
else
    echo "Generating keys......"
    gen_key
    ret=$?
    echo "ret = "$ret
fi

echo "Start BSC sign......"
bsc_sign
ret=$?
echo "BSC ret = "$ret

echo "Start VLR sign......"
vlr_sign
ret=$?
echo "VLR ret = "$ret

echo "Start VLR sign other......"
vlr_sign_other
ret=$?
echo "VLR other ret = "$ret

echo -e "\033[34m========\033[0m"
echo -e "\033[34m* EXIT *\033[0m"
echo -e "\033[34m========\033[0m"


