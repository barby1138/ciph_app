#!/bin/bash

#DEBHELPER#

echo "ciph_app.service: ## Starting ##" | systemd-cat -p info
cd /home/ciph_app
./dpdk-crypto-app cfg

#while :
#do
#TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
#echo "ciph_app.service: timestamp ${TIMESTAMP}" | systemd-cat -p info
#sleep 60
#done

