#!/system/bin/sh
EXT_PID=$$

ext_log() {
	log -p d -t "data_off.sh-$EXT_PID" "$@"
}

ext_log "Now start exec script, arg is: $@"

	ifname=`getprop sys.ifconfig.down`
	ip $ifname
	ext_log "If down: ip $ifname"
	ifname=`getprop sys.data.clearip`
	ip $ifname
	setprop sys.ifconfig.down done

	ethdown=`getprop ril.gsps.eth.down`
	if [ "$ethdown" = "1" ]; then
                iptables -X
		setprop ril.gsps.eth.down 0
		setprop sys.gsps.eth.ifname ""
		setprop sys.gsps.eth.localip ""
		setprop sys.gsps.eth.peerip ""
		#for ipv6 test@20150902@junjie.wang@6704#
		ndc tether radvd remove_upstream seth_lte0
		ip -6 rule del iif rndis0 lookup 1002 pref 18500
	fi
