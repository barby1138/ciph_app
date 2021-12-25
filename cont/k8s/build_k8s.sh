# cont/k8s
K8S_ROOT=$PWD 
echo $K8S_ROOT

K8S_DEPLOY=$K8S_ROOT/deploy/helm

cd $K8S_ROOT

VER=$(cat ciph_app/VERSION)
echo $VER

docker_tag=v${VER}-${BUILD_NUMBER}

#tmp
docker build -t ciph_app:${docker_tag} -f Dockerfile_ciph_app .

cd $K8S_DEPLOY/ciph_app_chart
chmod +x gen_chart.sh && ./gen_chart.sh $VER
cd $K8S_DEPLOY/ciph_app_test_chart
chmod +x gen_chart.sh && ./gen_chart.sh $VER

cd $K8S_DEPLOY/ciph_app_chart
tar czf ciph_app-$VER.tgz ciph_app

cd $K8S_DEPLOY/ciph_app_test_chart
tar czf ciph_app_test-$VER.tgz ciph_app_test
