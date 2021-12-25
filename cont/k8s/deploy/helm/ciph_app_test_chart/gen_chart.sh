VER=$1

DOCKER_REPOSITORY=pw-docker-images
docker_tag=v$VER-${BUILD_NUMBER}

cat > ciph_app_test/values.yaml <<EOF
# replicaCount number of replicas 
replicaCount: 1
# repository docker repo name
repository: pwartifactory.parallelwireless.net/${DOCKER_REPOSITORY}/ciph-app/$BRANCH/ciph_app
# tag docker docker image tag 
tag: $docker_tag
namespace: parallelwireless
EOF

cat > ciph_app_test/Chart.yaml <<EOF
apiVersion: v2
name: ciph_app_test
description: A Helm chart for Kubernetes
version: $VER
EOF
