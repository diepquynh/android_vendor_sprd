#!/system/bin/sh

ip_m_log() {
	log -p d -t "ip_monitor-$$" "$@"
}

_dump_snapshot() {
	local command="$@"
	ip_m_log "Dump ip $command info:"

	ip $command | while read line; do
		ip_m_log "$line"
	done
}

#Dump initial ip address & rule & route's snapshot
dump_snapshot() {
	_dump_snapshot address ls
	_dump_snapshot rule ls
	_dump_snapshot route ls
	_dump_snapshot route ls table all
}

dump_ifindex() {
	for iface in /sys/class/net/*/ifindex; do
		local idx=$(cat "$iface")
		ip_m_log "$iface:$idx"
	done
}

watch_iprt_changes() {
	local is_dumped=0

	ip monitor address rule route | while read line; do
		if [ $is_dumped -eq 0 ]; then
			is_dumped=1
			dump_ifindex
			dump_snapshot
		fi

		ip_m_log "$line"
	done
}

#Now only dump ip infos in userdebug version. If 'busybox' is available, current version is userdebug :).
#I know this is ugly, but now it is a valid method.
if [ -f /system/xbin/busybox ]; then
	watch_iprt_changes
fi

