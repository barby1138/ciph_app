TAG=v$1

cat > values.yaml <<EOF
# replicaCount number of replicas 
replicaCount: 1
# repository docker repo name
repository: ciph_app_test
# tag docker docker image tag 
tag: $TAG
EOF
