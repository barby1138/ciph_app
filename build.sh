
ROOT=$PWD 
echo $ROOT

echo ============== DPDK =====================
cd $ROOT
cd 3rdparty_artifactory
mkdir $ROOT/3rdparty/dpdk-20.05
tar zxf dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz -C $ROOT/3rdparty/dpdk-20.05

echo ============== LIBMEMIF =====================
cd $ROOT
cd libmemif
mkdir build
cd build
cmake ..
make
#make install

echo ============== FRIDMON =====================
cd $ROOT
unzip 3rdparty/fridmon-0.1.10.zip  -d ./3rdparty

cd 3rdparty/fridmon-0.1.10/meson_1_0_1/project/redhat
make CFG=release

cd $ROOT
cd 3rdparty/fridmon-0.1.10/hyperon_1_0_1/project/redhat
make CFG=release

echo ============== CIPH_APP =====================
cd $ROOT
cd project/linux
make CFG=release
