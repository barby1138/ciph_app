VER=$1
TAG=v$1

cat > values.yaml <<EOF
# replicaCount number of replicas 
replicaCount: 1
# repository docker repo name
repository: ciph_app
# tag docker docker image tag 
tag: $TAG
EOF

cat > Chart.yaml <<EOF
apiVersion: v2
name: ciph_app
description: A Helm chart for Kubernetes
version: $VER
EOF
