# cont/k8s
K8S_ROOT=$PWD 
echo $K8S_ROOT

cd $K8S_ROOT

VER=$(cat ciph_app/VERSION)
echo $VER

docker build -t ciph_app:v$VER -f Dockerfile_ciph_app .
docker build -t ciph_app_test:v$VER -f Dockerfile_ciph_app_test .

cd $K8S_ROOT/deploy/helm/ciph_app_chart
./gen_chart.sh $VER
cd $K8S_ROOT/deploy/helm/ciph_app_test_chart
./gen_chart.sh $VER

cd $K8S_ROOT/deploy/helm/ciph_app_chart
tar czf ciph_app-$VER.tgz ciph_app

cd $K8S_ROOT/deploy/helm
mkdir ciph_app_test_chart/ciph_app_test/charts
cp ciph_app_chart/ciph_app-$VER.tgz ciph_app_test_chart/ciph_app_test/charts

cd $K8S_ROOT/deploy/helm/ciph_app_test_chart
tar czf ciph_app_test-$VER.tgz ciph_app_test
