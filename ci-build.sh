#!/bin/bash

#
#lopyright (c) 2014-2020 Parallel Wireless, Inc. All rights reserved.
#

GIT_COMMIT_SHORT=`git rev-parse --short HEAD`
uname=$(whoami)
branch=${push_changes_0_new_name}
repo=`basename $(git rev-parse --show-toplevel)`
hostname=$(hostname)
homedir=$(getent passwd $uname | cut -d: -f6)
buildNumber=$BUILD_NUMBER
#dockerName=$uname-$GIT_COMMIT_SHORT-$buildNumber
dockerName=$uname-ciph-app

trap 'handle_error' ERR

handle_error () {
  echo "docker build failed"
  docker stop $uname-$GIT_COMMIT_SHORT-$buildNumber > /dev/null 2>&1
  exit 1
}

# BUILD THE DOCKER #
docker build --build-arg BUILD_NUMBER=${BUILD_NUMBER} --build-arg BRANCH=${branch} --build-arg UID=$(id -u) --build-arg GID=$(id -g) --build-arg UNAME=$(whoami) --build-arg DOCKER_GID=$(cut -d: -f3 < <(getent group docker)) -f Dockerfile -t ${dockerName} .

echo "===================="
echo "Docker image created"
echo "===================="

# RUN THE DOCKER #
cont_id=$( docker run --rm -d -it --hostname ${dockerName} -e COMMIT_ID=$GIT_COMMIT_SHORT \
            --mount type=bind,source="$(pwd)",target=/opt/ciph_app \
            --mount type=bind,source=$homedir,target=/home/$uname/ \
            -v /var/run/docker.sock:/var/run/docker.sock \
            ${dockerName} )

echo ${cont_id}

echo "========================================"
echo "Docker run successful- Container created"
echo "========================================"

# BUILD THE NetCon ROOTFS - mapping local workspace to container #
#docker exec -it ${cont_id} /bin/bash
docker exec ${cont_id} /bin/bash -c 'cd /opt/ciph_app && chmod +x build.sh && ./build.sh'

#
##################### END OF DEVOPS CONTAINER INFORMTION
# STOP AND REMOVE THE CONTAINER #
#docker stop ${dockerName} > /dev/null 2>&1
echo "========================="
echo "Container stop successful"
echo "========================="

