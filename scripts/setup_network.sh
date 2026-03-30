#!/bin/bash
set -e
BRIDGE="br0"
NUM_VMS=5

ip link add name $BRIDGE type bridge
ip link set $BRIDGE up

for i in $(seq 0 $((NUM_VMS - 1))); do
    ip tuntap add dev tap$i mode tap
    ip link set tap$i up
    ip link set tap$i master $BRIDGE
done
echo "rede pronta"