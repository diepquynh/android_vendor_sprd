#!/system/bin/sh

term="/dev/pts/* "

modem_type=`getprop ro.radio.modemtype`

setprop ril.$modem_type.assert 1
phone_count=`getprop ro.modem.$modem_type.count`
setprop ril.$modem_type.sim.power 0

if [ "$phone_count" = "2" ]; then
        setprop ril.$modem_type.sim.power1 0
elif [ "$phone_count" = "3" ]; then
        setprop ril.$modem_type.sim.power1 0
        setprop ril.$modem_type.sim.power2 0
elif [ "$phone_count" = "1" ]; then
        ssda_mode=`getprop persist.radio.ssda.mode`
        if [ "$ssda_mode" = "svlte" ]; then
                setprop ril.l.sim.power 0
                setprop ril.service.l.enable -1
                setprop ril.lte.cereg.state -1
        fi
fi
#phone=`getprop sys.phone.app`
#kill $phone
