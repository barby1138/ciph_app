#!/bin/bash

echo "htg.service: ## Starting ##" | systemd-cat -p info

while :
do
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
echo "htg.service: timestamp ${TIMESTAMP}" | systemd-cat -p info
sleep 60
done

