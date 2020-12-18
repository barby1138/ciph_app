#!/bin/bash

#
#lopyright (c) 2014-2020 Parallel Wireless, Inc. All rights reserved.
#

GIT_COMMIT_SHORT=`git rev-parse --short HEAD`
uname=$(whoami)
branch=`git rev-parse --abbrev-ref HEAD`
repo=`basename $(git rev-parse --show-toplevel)`
hostname=$(hostname)
homedir=$(getent passwd $uname | cut -d: -f6)
buildNumber=$BUILD_NUMBER
dockerName=$uname-$GIT_COMMIT_SHORT-$buildNumber
dockerName=$uname-$GIT_COMMIT_SHORT

trap 'handle_error' ERR

handle_error () {
  echo "docker build failed"
  docker stop $uname-$GIT_COMMIT_SHORT-$buildNumber > /dev/null 2>&1
  exit 1
}

# BUILD THE DOCKER #
docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) --build-arg UNAME=$(whoami) -f Dockerfile -t ${dockerName} .
#sudo docker build -f Dockerfile -t ${dockerName} .

echo 3

echo "===================="
echo "Docker image created"
echo "===================="

# RUN THE DOCKER mi#
echo $hostname
cont_id=$( docker run --rm -d -it --hostname ${hostname} -e COMMIT_ID=$GIT_COMMIT_SHORT \
            --mount type=bind,source="$(dirname "$(pwd)")",target=/opt/ciph_app \
	          --mount type=bind,source="$(dirname "$(pwd)")"/..,target=/opt/ciph_app_old \
            --mount type=bind,source=$homedir,target=/home/$uname/ \
            ${dockerName} )
            #--mount type=bind,source=/stage,target=/stage ${dockerName}

echo ${cont_id}

echo "========================================"
echo "Docker run successful- Container created"
echo "========================================"

# BUILD THE NetCon ROOTFS - mapping local workspace to container #
docker exec -it ${cont_id} /bin/bash
#docker exec -it ${cont_id} cd /opt/ciph_app/ciph_app && /bin/bash build.sh

#
##################### END OF DEVOPS CONTAINER INFORMTION
# STOP AND REMOVE THE CONTAINER #
#docker stop ${dockerName} > /dev/null 2>&1
echo "========================="
echo "Container stop successful"
echo "========================="
