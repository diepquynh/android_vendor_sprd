#!/system/bin/sh
device1="9830"
device2="9630"
device_name=`cat /system/build.prop | grep "ro.product.board"`

value1=`echo "$device_name" | grep "$device1"`
value2=`echo "$device_name" | grep "$device2"`

echo disabled > /sys/class/thermal/thermal_zone1/mode
if [[ "$value1" != "" || "$value2" != "" ]]
then
	echo 1 > /sys/class/thermal/cooling_device0/cur_state
else
	echo 0 > /sys/class/thermal/cooling_device0/cur_state
fi
