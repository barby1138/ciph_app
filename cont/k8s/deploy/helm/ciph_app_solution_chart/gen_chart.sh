VER=$1

cat > ciph_app_solution/Chart.yaml <<EOF
apiVersion: v2
name: ciph_app_solution
description: A Helm chart for Kubernetes
version: $VER
EOF
