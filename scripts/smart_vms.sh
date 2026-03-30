#!/bin/bash
set -e
NUM_VMS=5
BINARY="${1:-./build/vehicle}"

for i in $(seq 1 $NUM_VMS); do
    IDX=$((i - 1))
    MAC=$(printf "52:54:00:00:00:%02x" $i)

    qemu-system-x86_64 \
        -m 256M \
        -nographic \
        -drive file=vm${i}.qcow2,format=qcow2 \
        -netdev tap,id=net0,ifname=tap${IDX},script=no,downscript=no \
        -device virtio-net-pci,netdev=net0,mac=$MAC \
        &

    echo "VM $i iniciada (tap=tap${IDX} mac=$MAC)"
done

wait