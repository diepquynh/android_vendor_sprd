#!/system/bin/sh
opcontrol --setup;
opcontrol --vmlinux=/storage/sdcard0/vmlinux --event=CPU_CYCLES --kernel-range=0xc0000000,0xffffffff;
opcontrol --start;
sleeptime=$1;
month=`date | busybox awk '{print $2}'`;
day=`date | busybox awk '{print $3}'`;
time=`date | busybox grep -o "[0-9]*:[0-9]*:[0-9]*" | busybox sed -e 's/://g'`;
logdir=`ls /storage/sdcard0/slog/ | busybox grep -o -E "[0-9]+-[0-9]+-[0-9]+-[0-9]+-[0-9]+-[0-9]+"`;
sleep $sleeptime;
opcontrol --stop;
opcontrol --dump;
busybox cp /data/oprofile/ -r /storage/sdcard0/slog/$logdir/misc/oprofile_"$month"-"$day"-"$time"_"sleep$sleeptime"/;
busybox mv /storage/sdcard0/slog/$logdir/misc/oprofile_"$month"-"$day"-"$time"_"sleep$sleeptime"/samples /storage/sdcard0/slog/$logdir/misc/oprofile_"$month"-"$day"-"$time"_"sleep$sleeptime"/raw_samples;
opcontrol --shutdown;
rm -r /data/oprofile/*;
