ls ./ciph_app

VER=$(cat VERSION)
echo $VER
docker build -t ciph_app:v$VER
