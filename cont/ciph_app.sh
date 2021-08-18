#!/bin/bash

#DEBHELPER#

echo "ciph_app.service: ## Starting ##" | systemd-cat -p info
cd /home/ciph_app
CPU_NUM=$(grep -c ^processor /proc/cpuinfo)
echo $CPU_NUM

if [[ $CPU_NUM -eq 48 ]]; then
    # CPU23 v6.1
    #taskset 0x800000 ./dpdk-crypto-app cfg
    # CPU22 && CPU46 v6.2
    taskset 0x400000400000 ./dpdk-crypto-app cfg
elif [[ $CPU_NUM -eq 28 ]]; then
    # CPU1 & CPU15
    taskset 0x8002 ./dpdk-crypto-app cfg
else
    echo "Error: unknown arch - just wait"
    while :
    do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    echo "ciph_app.service: timestamp ${TIMESTAMP}" | systemd-cat -p info
    sleep 600
    done
fi
