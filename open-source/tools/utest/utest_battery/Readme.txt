
Usage:
    utest_battery  status
    utest_battery  health
    utest_battery  voltage
    utest_battery  set_battery_0
    utest_battery  set_battery_1
    utest_battery  set_hw_switch_point
    utest_battery  stop_charge


Test for battery module: (Here's the example in SP8810EA)

/* read the current status of battery:charging, discharging or full*/

shell@android:/ $ utest_battery status
utest battery -- status
Current status of the battery  Charging

/* read the current health condition of battery:cold, dead or good*/
shell@android:/ $ utest_battery health
utest battery -- health
Current condition of the battery   Good

/* read the current charge voltage of battery*/
shell@android:/ $ utest_battery voltage
utest battery -- voltage
Current voltage of the battery   4090000

/*set the adb voltage "adc_voltage_table[0][] =  {928, 4200} " of battery_0*/
utest battery -- set_battery_0
write /sys/class/power_supply/battery/battery_0 success

/*set the adb voltage "adc_voltage_table[1][] =  {796, 3600} " of battery_1*/
root@android:/ # utest_battery set_battery_1
utest battery -- set_battery_1
write /sys/class/power_supply/battery/battery_1 success

/*set the hw_switch_point*/
root@android:/ # utest_battery set_hw_switch_point
utest battery -- set_hw_switch_point
write /sys/class/power_supply/battery/hw_switch_point success

/*let the battery stop charge*/
root@android:/ # utest_battery
write /sys/class/power_supply/battery/stop_charge success


