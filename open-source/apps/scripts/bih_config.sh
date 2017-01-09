#!/system/bin/sh
if [ "$1" = "-d" ]; then
v4addr=192.168.3.34
#v6addr=8800::1
#v6gw=8800::2300
#nameserver=2001:da8:bf:1010::1
pool=192.168.3.40-192.168.3.200
#map="192.168.3.50 8800::2300"
#map="192.168.1.35 2001:da8:bf:1010::c0a8:123"
#map="192.168.1.200 2001:da8:bf:1010::c0a8:1c8"
#map="192.168.1.40 2001:da8:bf:1010::c0a8:128"
#################################################
dev=`busybox ifconfig|busybox grep ^veth_|busybox awk '{print $1}'|busybox tail -1`
if [ "X$v6addr" != "X" ] ; then
        busybox ifconfig $dev add $v6addr/64
fi
if [ "X$v4addr" != "X" ] ; then
        busybox ifconfig $dev $v4addr
fi
if [ "X$v6gw" != "X" ] ; then
        busybox        route -A inet6 add default gw $v6gw
fi
if [ "X$nameserver" != "X" ] ; then
#        echo "nameserver $nameserver" > /system/etc/resolv.conf
        setprop net.$dev.ipv6_dns1 $nameserver
fi
        rmmod bih > /dev/null 2>&1
#        insmod `pwd`/system/lib/modules/bih.ko
        insmod /system/lib/modules/bih.ko
if [ "X$pool" != "X" ] ; then
        echo ADD $pool > /proc/bih/pool
fi
if [ "X$map" != "X" ] ; then
        echo ADD $map > /proc/bih/map
fi
        busybox route add default dev $dev
        echo "1" > /proc/sys/net/ipv6/conf/all/forwarding
        echo "BIS" > /proc/bih/mode

elif [ "$1" = "-u" ]; then
v4addr=192.168.3.34
#v6addr=8800::1
#v6gw=8800::2300
#nameserver=2001:da8:bf:1010::1
#################################################
dev=`busybox ifconfig|busybox grep ^veth_|busybox awk '{print $1}'|busybox tail -1`

if [ "X$v6addr" != "X" ] ; then
        ip -f inet6 addr flush dev $dev
fi
if [ "X$v4addr" != "X" ] ; then
        ip -f inet addr flush dev $dev
fi
if [ "X$v6gw" != "X" ] ; then
        busybox        route -A inet6 del default gw $v6gw
fi
if [ "X$nameserver" != "X" ] ; then
#        echo "nameserver $nameserver" > /system/etc/resolv.conf
        setprop net.$dev.ipv6_dns1
fi

        rmmod bih > /dev/null 2>&1
        echo "0" > /proc/sys/net/ipv6/conf/all/forwarding
fi
