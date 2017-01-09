#!/system/xbin/busybox sh
busybox="/system/xbin/busybox "
base_dir="/sdcard/logs4modem"
base_name="logs4modem--"
base_num_min=0
base_num_max=1

FIND="$busybox"find
SED="$busybox"sed
SORT="$busybox"sort
TAIL="$busybox"tail
EXPR="$busybox"expr
RM="$busybox"rm
MV="$busybox"mv
MKDIR="$busybox"mkdir
TOUCH="$busybox"touch
CAT="$busybox"cat
TEE="$busybox"tee
DATE="$busybox"date
STAT="$busybox"stat
TR="$busybox"tr
CP="$busybox"cp
KILLALL="$busybox"killall
AWK="$busybox"awk
LN="$busybox"ln

[ -x /system/bin/vhub ] || {
    echo "Kill logs4modem, no /system/bin/vhub" > /dev/kmsg
    stop logs4modem
    exit
    }

$LN /system/bin/vhub /system/bin/vhub1

if [ "$(cat /proc/mounts | grep vold)" ]; then
    echo "logs4modem just waits 5 seconds for /sdcard being pluged out, service logs4modem is restarted!"
    sleep 5
fi

while [ ! "$(cat /proc/mounts | grep vold)" ]; do
    echo "logs4modem is waiting for /sdcard being mounted" > /dev/kmsg
    sleep 5
done

$MKDIR -p $base_dir
cd $base_dir
num=$($FIND -maxdepth 1 -name "${base_name}*" |
      $SED -e "/[0-9]\{1,\}/!d;s/\.\/${base_name}//" |
      $SORT -n | $TAIL -n 1)
[ "${num}" ] || num=$($EXPR ${base_num_min} - 1)
num=$($EXPR ${num} + 1)

if [ $($EXPR ${num} '>' ${base_num_max}) -eq 1 ]; then
num=${base_num_max}
fi

while [ $($EXPR ${num} '>' ${base_num_min}) -eq 1 ]; do
    lmax_num=${num}
    num=$($EXPR ${num} - 1)
    $RM -rf ${base_name}${lmax_num}
    [ -e ${base_name}${num} ] && $MV ${base_name}${num} ${base_name}${lmax_num}
done


dest_file=${base_dir}/${base_name}${base_num_min}
$MKDIR -p ${dest_file}

cd ${dest_file}

rm -f /dev/vhub.modem
vhub1 -p modem -d /dev -o modem.logs. -s 30m -m 4 


