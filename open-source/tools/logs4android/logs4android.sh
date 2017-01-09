#!/system/xbin/busybox sh
busybox="/system/xbin/busybox "
base_dir="/sdcard/logs4android"
base_name="logs4android--"
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
GREP="$busybox"grep

mount -o remount rootfs /
ln -s $busybox /sbin/env
ln -s $busybox /sbin/reset
ln -s $busybox /sbin/find
ln -s $busybox /sbin/grep
ln -s $busybox /sbin/cp
ln -s $busybox /sbin/tail
ln -s $busybox /sbin/less
ln -s $busybox /sbin/more
ln -s $busybox /sbin/killall
ln -s $busybox /sbin/sed
ln -s $busybox /sbin/vi
ln -s $busybox /sbin/sort
ln -s $busybox /sbin/md5sum
ln -s $busybox /sbin/du
ln -s $busybox /sbin/diff
ln -s $busybox /sbin/gzip
ln -s $busybox /sbin/gunzip
ln -s $busybox /sbin/gzip
ln -s $busybox /sbin/tar
ln -s $busybox /sbin/unzip
ln -s $busybox /sbin/bunzip2
ln -s $busybox /sbin/bzip2
ln -s $busybox /sbin/cal
ln -s $busybox /sbin/uniq
ln -s $busybox /sbin/which
ln -s $busybox /sbin/xargs
ln -s $busybox /sbin/free

$KILLALL vhub

if [ "$(cat /proc/mounts | ${GREP} vold)" ]; then
    echo "logs4android just waits 5 seconds for /sdcard being pluged out, service logs4android is restarted!"
    sleep 5
fi

while [ ! "$(cat /proc/mounts | ${GREP} vold)" ]; do
    echo "logs4android is waiting for /sdcard being mounted" > /dev/kmsg
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

base_date=$($DATE +%Y%m%d%H%M%S)
# build_date="$($DATE -d \"1970-01-01 UTC $(getprop ro.build.date.utc) seconds\" +%Y%m%d)"
# build_date="$(getprop ro.build.date.utc)"
# build_date="$AWK 'BEGIN{print strftime("%Y%m%d","'"${build_date}"'");}'"
build_date="$($AWK 'BEGIN{print strftime("%Y%m%d","'"$(getprop ro.build.date.utc)"'");}')"

dest_file=${base_dir}/${base_name}${base_num_min}

$MKDIR -p ${dest_file}
cd ${dest_file}
[ -f /system/build.prop ] && cp -f /system/build.prop build.prop_${build_date}.${base_date}
[ -f /data/dontpanic/apanic_console -o -f /data/dontpanic/apanic_threads ] && {
# useing busybox time releated shell, need /etc/localtime, we can copy it from x86
    apanic_dir_name=dontpanic.$(${STAT} -c %y /data/dontpanic/apanic_console | ${TR} ' ' _ | ${TR} ':' +)
    ${CP} -a /data/dontpanic ${apanic_dir_name}
}
# $TOUCH $($DATE +%Y%m%d%H%M%S)

# $CAT /proc/kmsg 2>&1                | $TEE kernel.${base_date}.log >/dev/null &
# logcat -v time -b radio 2>&1        | $TEE ril.${base_date}.log >/dev/null &
# logcat -v time 2>&1                 | $TEE android.${base_date}.log >/dev/null

$KILLALL vhub
[ -x /system/bin/vhub ] || {
    echo "Kill logs4android, no /system/bin/vhub"
    stop logs4android
    exit
}
rm -f /dev/vhub.bt
vhub -p bt -d /dev -o bt.logs. -s 2m -m 2 &

[ -e /data/anr/traces.txt ] || {
${MKDIR} -p /data/anr/
>/data/anr/traces.txt
}

rm -f /dev/vhub.android /dev/vhub.rild /dev/vhub.kernel
vhub -p android -p rild -p kernel \
     -F :/data/anr/traces.txt -Z 60 -E /system/xbin/gsnap.save_anr_tombstones.sh -Y 1 -M 0x00000008 \
     -f  /data/anr/traces.txt -T 60 -e /system/xbin/gsnap.save_anr_tombstones.sh \
     -f  'Starting SDP server' -T 2 -e /system/xbin/logs4android.bt.sh \
     -d  /dev -o android.rild.kernel.logs. -s 20m -m 4 &
#    -f "Updating external media status from mounted to unmounted" -T 10 -e /system/xbin/logs4android.sdcard.umount.sh \

# while [ ! -L /dev/vhub.kernel ]; do
#     echo "waiting vhub up ..."
#     sleep 0.1
# done

rm -f /dev/vhub.port*
vhub -p port0 -p port1 -p port2 -p port3 -p port4 -p port5 -p port6 -p port7 \
     -d /dev -o vhub.ports.logs. -s 1m -m 2 &

while [ ! -L /dev/vhub.kernel -o \
        ! -L /dev/vhub.port0 ]; do
    echo "waiting vhub up ..."
    sleep 0.1
done

$CAT /proc/kmsg 2>&1            > /dev/vhub.kernel  &
logcat -v threadtime -b radio 2>&1    > /dev/vhub.rild    &
logcat -v threadtime 2>&1             > /dev/vhub.android

/system/xbin/logs4android.bt.sh stop
