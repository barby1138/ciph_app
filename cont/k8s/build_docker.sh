ln -s -r ~/rpmbuild/BUILD .
ls .

VER=$(cat VERSION)
echo $VER
docker build -t ciph_app:v$VER
