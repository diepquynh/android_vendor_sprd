#!/system/bin/sh
EXT_PID=$$

ext_log() {
	log -p d -t "ext_data.sh-$EXT_PID" "$@"
}

ext_log "Now start exec script, arg is: $@"

if [ "$1" = "-u" ]; then
	temp=`getprop sys.data.IPV6.disable`
        ifname=`getprop sys.data.net.addr`
        `echo ${temp} > /proc/sys/net/ipv6/conf/$ifname/disable_ipv6`
        `echo -1 > /proc/sys/net/ipv6/conf/$ifname/accept_dad`
        ifname=`getprop sys.data.setip`
	ip $ifname
	ext_log "Conf ip: ip $ifname"
	ifname=`getprop sys.data.setmtu`
	ip $ifname
	ifname=`getprop sys.ifconfig.up`
	ip $ifname
	ext_log "If up: ip $ifname"
	ifname=`getprop sys.data.noarp`
	ip $ifname
    ifname=`getprop sys.data.noarp.ipv6`
    ip $ifname

##For Auto Test
	ethup=`getprop ril.gsps.eth.up`
	if [ "$ethup" = "1" ]; then
		ifname=`getprop sys.gsps.eth.ifname`
		localip=`getprop sys.gsps.eth.localip`
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
                iptables -I FORWARD -o $ifname -p all ! -d $localip/24 -j DROP
                iptables -I OUTPUT -s $localip -p udp --dport 53 -j DROP
                iptables -I OUTPUT -s $localip -p udp --dport 123 -j DROP

        echo 1 > proc/sys/net/ipv6/conf/$ifname/disable_ipv6
	fi

	setprop sys.ifconfig.up done
	setprop sys.data.noarp done
elif [ "$1" = "-d" ]; then
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
	fi

elif [ "$1" = "-e" ]; then
        iptables -A FORWARD -p udp --dport 53 -j DROP
        iptables -A INPUT -p udp --dport 53 -j DROP
        iptables -A OUTPUT -p udp --dport 53 -j DROP
        ip6tables -A FORWARD -p udp --dport 53 -j DROP
        ip6tables -A INPUT -p udp --dport 53 -j DROP
        ip6tables -A OUTPUT -p udp --dport 53 -j DROP

elif [ "$1" = "-c" ]; then
        iptables -D FORWARD -p udp --dport 53 -j DROP
        iptables -D INPUT -p udp --dport 53 -j DROP
        iptables -D OUTPUT -p udp --dport 53 -j DROP
        ip6tables -D FORWARD -p udp --dport 53 -j DROP
        ip6tables -D INPUT -p udp --dport 53 -j DROP
        ip6tables -D OUTPUT -p udp --dport 53 -j DROP

fi
