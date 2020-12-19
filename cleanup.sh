
ROOT=$PWD 
echo $ROOT

echo ============== cleanup =====================
cd $ROOT
cd project/linux
make CFG=release clean

cd $ROOT
rm -f 3rdparty_artifactory/*
rm -rf 3rdparty/dpdk-20.05
rm -rf 3rdparty/fridmon-0.1.10

rm -rf lxc/ciph_app/rootfs
