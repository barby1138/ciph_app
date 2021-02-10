#!/bin/bash

echo "Executing ci-publish.sh ..."

vmname=$(hostname)
uname=$(whoami)
echo "Current git commit: ${GIT_COMMIT}"
GIT_COMMIT=$(git rev-parse HEAD)
echo "Actual git commit: ${GIT_COMMIT}"
HASH=${GIT_COMMIT:0:8}
echo $HASH
#BRANCH=${BUILD_DISPLAY_NAME}
echo "Current branch name: ${BRANCH_NAME}; ${BUILD_DISPLAY_NAME}"
BRANCH=${BRANCH_NAME}
UBRANCH=$(echo $BRANCH | sed 's/\//:/g')
DATE=$(git log --format=format:%cd --date=format:%Y%m%d.%H%M -n 1)

export PATH=$PATH:/opt/swtools/bin/depot_tools/jfrog_CLI/

upload_to_artifactory()
{
    local image_name="${1}"

    echo "Publish file: $image_name"
    tfname="${image_name%.*}"
    tfext="${image_name##*.}"
    GZ_DATA=${UBRANCH}:${DATE}-${HASH}:$tfname.${DATE}-${HASH}
    tfile=${GZ_DATA}.$tfext
    FSTRING=$(echo $tfile | sed 's/:/\//g')
    MYDIR=$(dirname ${FSTRING})/
    MYFILE=$(basename ${FSTRING})
    mv $image_name "$MYFILE"
    echo "To publish '$MYFILE' into dir '$MYDIR'"
    echo "please the find the tgz files in dist dir !"
    jfrog rt use pwartifactory
    jfrog rt u --props "commitID=${HASH}" $MYFILE pw-products/ciph-app/"$MYDIR"
}

echo "Checking current folder and content"
pwd
ls -la

if [ -d dist ]; then
    cd dist

    cont_image=$(ls -a |grep -ie ciph_app*.rpm)
    upload_to_artifactory $cont_image

    devel_image=$(ls -a |grep -ie ciph_app_devel*.tar.gz)
    upload_to_artifactory $devel_image
fi

echo "Done ci-publish.sh"
