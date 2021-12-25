#!/bin/bash

echo "Executing ci-publish.sh ..."

vmname=$(hostname)
uname=$(whoami)
commit_hash_date=$(git log --format=format:%cd-%h --date=format:%Y%m%d.%H%M -n 1)
shortHash=$(git rev-parse --short HEAD)
buildNumber=$BUILD_NUMBER

branch=${push_changes_0_new_name}

export PATH=$PATH:/opt/swtools/bin/depot_tools/jfrog_CLI/

upload_to_artifactory()
{
    local p_image_name="${1}"
    local p_folder_to_upload="pw-products/ciph-app/${branch}/${commit_hash_date}/"
    echo "Upload ${p_image_name} to folder ${p_folder_to_upload} with the tag $shortHash"
    jfrog rt use pwartifactory
    jfrog rt u --insecure-tls \
        --props "commitID=$shortHash;build.number=$buildNumber;branch=$branch" \
        --include-dirs=true \
        ${p_image_name} ${p_folder_to_upload}
}

VER=$(cat VERSION)
echo ${VER}

DOCKER_REPOSITORY=pw-docker-images
docker_tag=v${VER}-${BUILD_NUMBER}

upload_docker_img_to_artifactory()
{
     local CORE_ACCESS_DOCKER_IMAGE="pwartifactory.parallelwireless.net/${DOCKER_REPOSITORY}/ciph-app/$branch/${commit_hash_date}/ciph_app:${docker_tag}"
     jfrog rt use pwartifactory
     docker tag ciph_app:${docker_tag} ${CORE_ACCESS_DOCKER_IMAGE}
     docker push ${CORE_ACCESS_DOCKER_IMAGE}
     jfrog rt dp ${CORE_ACCESS_DOCKER_IMAGE} ${DOCKER_REPOSITORY} --build-name=${JOB_URL} --build-number=${BUILD_NUMBER}
     jfrog rt bp ${JOB_URL} ${BUILD_NUMBER}
}

echo "Checking current folder and content"
pwd
ls -la

if [ -d dist ]; then
    cd dist

    upload_docker_img_to_artifactory

    lxc_image=$(ls -a | grep -ie ciph_app*.rpm)
    upload_to_artifactory $lxc_image

    k8s_image=$(ls -a | grep -ie ciph_app-*.tgz)
    upload_to_artifactory $k8s_image

    k8s_test_image=$(ls -a | grep -ie ciph_app_test-*.tgz)
    upload_to_artifactory $k8s_test_image

    devel_image=$(ls -a | grep -ie ciph_app_devel*.tar.gz)
    upload_to_artifactory $devel_image
fi

echo "Done ci-publish.sh"
