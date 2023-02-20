tag=$1

DOCKER_REPOSITORY=pw-docker-images
docker_tag=v$tag

cat > ciph_app/values.yaml <<EOF
# replicaCount number of replicas 
replicaCount: 1
# repository docker repo name
repository: pwartifactory.parallelwireless.net/${DOCKER_REPOSITORY}/ciph-app/$BRANCH/ciph_app
# tag docker docker image tag 
tag: $docker_tag
namespace: pw
EOF

cat > ciph_app/Chart.yaml <<EOF
apiVersion: v2
name: ciph_app
description: A Helm chart for Kubernetes
version: $tag
EOF
