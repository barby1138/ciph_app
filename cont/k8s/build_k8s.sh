# cont/k8s
K8S_ROOT=$PWD 
echo $K8S_ROOT

cd $K8S_ROOT

VER=$(cat ciph_app/VERSION)
echo $VER

docker build -t ciph_app:v$VER -f Dockerfile_ciph_app .
docker build -t ciph_app_test:v$VER -f Dockerfile_ciph_app_test .

cd $K8S_ROOT/deploy/helm/ciph_app_chart
./gen_values.sh $VER
cd $K8S_ROOT/deploy/helm/ciph_app_test_chart
./gen_values.sh $VER

cd $K8S_ROOT/deploy/helm
mkdir ciph_app_test_chart/charts
cp -R ciph_app_chart ciph_app_test_chart/charts
