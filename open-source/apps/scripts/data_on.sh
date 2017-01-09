#!/system/bin/sh
EXT_PID=$$

ext_log() {
	log -p d -t "data_on.sh-$EXT_PID" "$@"
}

ext_log "Now start exec script, arg is: $@"

	temp=`getprop sys.data.IPV6.disable`
	ifname=`getprop sys.data.net.addr`
	#if pdp is ipv4, disable_ipv6 to avoid v6 packets
	echo ${temp} > /proc/sys/net/ipv6/conf/$ifname/disable_ipv6
	echo -1 > /proc/sys/net/ipv6/conf/$ifname/accept_dad
	#for ZTE lab test, do not drop RA from local IP
	echo 1 > /proc/sys/net/ipv6/conf/$ifname/accept_ra_from_local

	# Reduce the time cost of getting the ipv6's global address
	echo 1 > /proc/sys/net/ipv6/conf/$ifname/router_solicitation_interval
	echo 12 > /proc/sys/net/ipv6/conf/$ifname/router_solicitations

	ifname=`getprop sys.data.setip`
	ip $ifname
	ext_log "Conf dev: ip $ifname"

	ifname=`getprop sys.data.setmtu`
	ip $ifname
	ext_log "Conf dev: ip $ifname"

	ifname=`getprop sys.ifconfig.up`
	ip $ifname
	ext_log "Conf dev: ip $ifname"

	ifname=`getprop sys.data.noarp.ipv6`
	ip $ifname
	ext_log "Conf dev: ip $ifname"

	ifname=`getprop sys.data.noarp`
	ip $ifname
	ext_log "Conf dev: ip $ifname"

	ifname=`ip addr show up`
	ext_log "$ifname"

##For Auto Test
	ethup=`getprop ril.gsps.eth.up`
	if [ "$ethup" = "1" ]; then

		ifname=`getprop sys.data.net.addr`
		localip=`getprop sys.gsps.eth.localip`

		iptype=`getprop sys.data.activating.type`

		if [ "$iptype" = "IPV4" ]; then

			pcv4addr=`getprop sys.gsps.eth.peerip`

			setprop ril.gsps.eth.up 0

			iptables -t nat --flush
			iptables -t mangle --flush
			iptables -t filter --flush
			ip rule del table 66
			ip route del table 66
			ip route add default via $localip dev $ifname
			ip route add default via $localip dev $ifname table 66
			ip route add local $localip dev $ifname proto kernel scope host src $localip
			ip rule add from all iif rndis0 lookup 66
			iptables -D FORWARD -j natctrl_FORWARD
			iptables -D natctrl_FORWARD -j DROP
			iptables -t nat -A PREROUTING -i $ifname -j DNAT --to-destination $pcv4addr
			iptables -I FORWARD 1 -i $ifname -d $pcv4addr -j ACCEPT
			iptables -A FORWARD -i rndis0 -o $ifname -j ACCEPT
			iptables -t nat -A POSTROUTING -s $pcv4addr -j SNAT --to-source $localip
			iptables -I FORWARD -o $ifname -p all ! -d $localip/16 -j DROP
			iptables -I OUTPUT -s $localip -p udp --dport 53 -j DROP
			iptables -I OUTPUT -s $localip -p udp --dport 123 -j DROP
		  	temp=`getprop net.$ifname.ip_type`
			if [ "$temp" = "1" ]; then #only v4
				echo 1 > proc/sys/net/ipv6/conf/$ifname/disable_ipv6
			fi
		elif [ "$iptype" = "IPV6" ]; then
			#start radvd and dhcp6s  for lan. @20150902@junjie.wang@6704#
			ndc tether radvd remove_upstream $ifname
			ip -6 rule del iif rndis0 lookup 1002 pref 18500
			sleep 5

			ndc tether radvd add_upstream $ifname
			#add rule for ipv6 route
			ip -6 rule add iif rndis0 lookup 1002 pref 18500
			#add default route to ifname
			ip -6 route add default dev $ifname
			temp=`getprop net.$ifname.ip_type`
			if [ "$temp" = "2" ]; then #only v6
				setprop ril.gsps.eth.up 0
			fi
		fi
	fi

	setprop sys.ifconfig.up done
	setprop sys.data.noarp done
