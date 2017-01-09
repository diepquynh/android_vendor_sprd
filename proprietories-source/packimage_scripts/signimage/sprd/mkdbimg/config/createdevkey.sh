#!/bin/bash

ROOTDIR=$(pwd)
    echo -e "\033[33m create developer cert key! \033[0m "
    openssl genrsa -out rsa2048_devkey.pem 2048
    echo -e "\033[32m create developer cert key ok! \033[0m "
    openssl rsa -in rsa2048_devkey.pem -pubout -out rsa2048_devkey_pub.pem
    echo -e "\033[32m create developer cert pubkey ok! \033[0m " 
if [ -f rsa2048_devkey_pub.pem ] ;
	then cp rsa2048_devkey_pub.pem $ROOTDIR/../../config
fi
