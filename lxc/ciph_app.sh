#!/bin/bash

#DEBHELPER#

echo "ciph_app.service: ## Starting ##" | systemd-cat -p info
cd /home/ciph_app
CPU_MASK=$(cat CPU_MASK)
echo $CPU_MASK
taskset $CPU_MASK ./dpdk-crypto-app cfg

#while :
#do
#TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
#echo "ciph_app.service: timestamp ${TIMESTAMP}" | systemd-cat -p info
#sleep 600
#done

