#!/system/bin/sh

exec_cmd() {
    local cmd="$@"
    local emsg="$(eval $cmd 2>&1)"

    log -p d -t "tiny_firewall" "$cmd ecode=$? $emsg"
}

modify_dns_filter() {
    local action=$1

    for ipt in "iptables -w" "ip6tables -w"; do
        for chn in FORWARD INPUT OUTPUT; do
            exec_cmd "$ipt $action $chn -p udp --dport 53 -j DROP"
        done
    done
}

# Bug610743 - Drop MLD packets for PICLAB test
modify_mld_filter() {
    local action=$1

    exec_cmd "ip6tables -w $action OUTPUT -p icmpv6 --icmpv6-type 143 -j DROP"
}

# For PICLAB test, we want to drop MLD pakcets, so maybe it's better to rename
# "enable_dns" to "drop_miscpkts", "disable_dns" to "accpet_miscpkts"
case "$1" in
    enable_dns)
        modify_dns_filter "-A"
        modify_mld_filter "-A"
        ;;
    disable_dns)
        modify_dns_filter "-D"
        modify_mld_filter "-D"
        ;;
esac
