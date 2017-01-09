#!/system/xbin/busybox sh
busybox="/system/xbin/busybox "
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
CP="$busybox"cp
STAT="$busybox"stat
TR="$busybox"tr
DATE=date #"$busybox"date

BASE_DIR=/sdcard/logs4android/logs4android--0/$($DATE +%Y%m%d%H%M%S)

${MKDIR} -p ${BASE_DIR}
cd ${BASE_DIR}

/system/bin/gsnap $($DATE +%Y%m%d%H%M%S).jpg /dev/graphics/fb0
sleep 1
/system/bin/gsnap $($DATE +%Y%m%d%H%M%S).jpg /dev/graphics/fb0
top -t -n 1 > top
/system/bin/gsnap $($DATE +%Y%m%d%H%M%S).jpg /dev/graphics/fb0
[ -f /data/anr/traces.txt ] && {
# useing busybox time releated shell, need /etc/localtime, we can copy it from x86
anr_dir_name=anr.$(${STAT} -c %y /data/anr/traces.txt | ${TR} ' ' _ | ${TR} ':' +)
${CP} -a /data/anr ${anr_dir_name}
# ${CP} -r /data/anr .
#${RM} -rf /data/anr
}
[ -d /data/tombstones ] && {
${CP} -r /data/tombstones .
#${RM} -rf /data/tombstones
}
[ -d /data/watchdog ] && {
${CP} -r /data/watchdog .
#${RM} -rf /data/watchdog
}
[ -d /data/crush ] && {
${CP} -r /data/crush .
#${RM} -rf /data/crush
}
# bugreport > bugreport.log
sleep 1
/system/bin/gsnap $($DATE +%Y%m%d%H%M%S).jpg /dev/graphics/fb0
