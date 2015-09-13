#!/bin/bash
# An example script for running the flow-disruptor using Linux veth interfaces.
# 
# Copyright 2015 Teclo Networks AG

set -e

DEV=eth0

function die {
    echo "$@";
    exit 1;
}

# Determine current network settings
IP=$(ip addr show dev $DEV | perl -nle '/inet ([\d.]+)/ && print $1')
[ -n "$IP" ] || die "Can't determine ip of $DEV"
GW=$(ip route show default dev $DEV | perl -nle '/^default via (\S+)/ && print $1')
[ -n "$GW" ] || die "Can't determine default route"

# Restore original settings once we quit
function cleanup {
    echo Restoring original setup
    sleep 1

    ifconfig $DEV $IP
    ifconfig veth1 0.0.0.0
    route add default gw $GW dev $DEV

    ifconfig veth0 down
    ifconfig veth1 down
}
trap cleanup EXIT

echo Setting up interfaces ip $IP gw $GW

# Create the veth0/veth1 pair if it doesn't exist.
ip link ls | grep veth0 || ip link add type veth
ifconfig veth0 down
ifconfig veth1 down

# Turn off tx segment offload on the veths
ethtool -K veth0 tx off
ethtool -K veth1 tx off

# Turn off all known kinds of offload on the actual interface
ethtool -K $DEV tso off gso off gro off
# Make sure we can route all kinds of packets through the interface.
echo 1 | tee /proc/sys/net/ipv4/conf/$DEV/rp_filter

# Bind the IP to veth1 instead of the real interface
echo Switching to veth interface
ifconfig veth0 up promisc
ifconfig veth1 $IP up promisc
ifconfig $DEV 0.0.0.0

route add default gw $GW dev veth1
# ip route change $TARGET via $GW dev veth1 proto static initrwnd 100

# Start the flow-disruptor between veth0 and the real interface
./flow-disruptor --downlink_iface veth0 --uplink_iface $DEV --config my.conf
