
VER=$(cat ciph_app/VERSION)
echo $VER
docker build -t ciph_app:v$VER .
