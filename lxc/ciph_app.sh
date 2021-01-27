#!/bin/bash

echo "ciph_app.service: ## Starting ##" | systemd-cat -p info
./dpdk-crypto-app cfg

#while :
#do
#TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
#echo "ciph_app.service: timestamp ${TIMESTAMP}" | systemd-cat -p info
#sleep 60
#done

