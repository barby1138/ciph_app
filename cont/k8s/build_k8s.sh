# cont/k8s
K8S_ROOT=$PWD 
echo $K8S_ROOT

K8S_DEPLOY=$K8S_ROOT/deploy/helm

cd $K8S_ROOT

VER=$(cat ciph_app/VERSION)
echo $VER

tag=${VER}-${BUILD_NUMBER}
docker_tag=v$tag

#tmp
docker build -t ciph_app:${docker_tag} -f Dockerfile_ciph_app .

docker save ciph_app:${docker_tag} | gzip > ciph_app-img-${tag}.tgz

cd $K8S_DEPLOY/ciph_app_chart
chmod +x gen_chart.sh && ./gen_chart.sh ${tag}
cd $K8S_DEPLOY/ciph_app_test_chart
chmod +x gen_chart.sh && ./gen_chart.sh ${tag}

cd $K8S_DEPLOY/ciph_app_chart
tar czf ciph_app-helm-${tag}.tgz ciph_app

cd $K8S_DEPLOY/ciph_app_test_chart
tar czf ciph_app_test-helm-${tag}.tgz ciph_app_test
