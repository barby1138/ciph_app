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
#GZ_DATA=${UBRANCH}:${DATE}-${HASH}:${DATE}-${HASH}
export PATH=$PATH:/opt/swtools/bin/depot_tools/jfrog_CLI/

echo "Checking current folder and content"
pwd
ls -la

if [ -d dist ]; then
    cd dist

    for tgzfile in *.tar.gz; do
        echo "Publish file: $tgzfile"
        tfname="${tgzfile%.*}"
        tfext="${tgzfile##*.}"
        GZ_DATA=${UBRANCH}:${DATE}-${HASH}:$tfname.${DATE}-${HASH}
        tfile=${GZ_DATA}.$tfext
        FSTRING=$(echo $tfile | sed 's/:/\//g')
        MYDIR=$(dirname ${FSTRING})/
        MYFILE=$(basename ${FSTRING})
        mv $tgzfile "$MYFILE"
        echo "To publish '$MYFILE' into dir '$MYDIR'"
        #if [[ $vmname == "docker-pipeline"*  || $vmname == "c7-nhbld"* ]]; then
            echo "please the find the tgz files in dist dir !"
            jfrog rt use pwartifactory
            jfrog rt u --props "commitID=${HASH}" $MYFILE pw-products/ciph_app/"$MYDIR"
        #else
        #    jfrog rt use artifactory
        #    jfrog rt u  $MYFILE Developer-Sandbox/ciph_app/"$MYDIR"
        #fi
    done
fi

echo "Done ci-publish.sh"
