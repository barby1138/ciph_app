#!/bin/bash

echo "Executing ci-publish.sh ..."

vmname=$(hostname)
uname=$(whoami)
commit_hash_date=$(git log --format=format:%cd-%h --date=format:%Y%m%d.%H%M -n 1)
shortHash=$(git rev-parse --short HEAD)
buildNumber=$BUILD_NUMBER

branch=${push_changes_0_new_name}
#branch=develop

export PATH=$PATH:/opt/swtools/bin/depot_tools/jfrog_CLI/

upload_to_artifactory()
{
    local p_image_name="${1}"
    local p_folder_to_upload="pw-products/ciph-app/${branch}/${commit_hash_date}/"
    echo "Upload ${p_image_name} to folder ${p_folder_to_upload} with the tag $shortHash"
    jfrog rt use pwartifactory
    jfrog rt u --insecure-tls --props "commitID=$shortHash" --include-dirs=true \
          ${p_image_name} ${p_folder_to_upload}
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
