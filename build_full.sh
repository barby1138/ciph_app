
ROOT=$PWD 
echo $ROOT

echo ============== NASM =====================
cd $ROOT
tar -xf 3rdparty/nasm_2.15.02.orig.tar.xz -C ./3rdparty
cd 3rdparty/nasm-2.15.02
./configure
make
make install

echo ============== IPSEC =====================
cd $ROOT
unzip 3rdparty/intel-ipsec-mb-0.54.zip -d ./3rdparty
cd 3rdparty/intel-ipsec-mb-0.54
./configure
make
make install

echo ============== DPDK =====================
cd $ROOT
tar -xf 3rdparty/dpdk-20.05.tar.gz -C ./3rdparty
cp 3rdparty/enable_PMD_AESNI_MB.patch 3rdparty/dpdk-20.05/config
cd 3rdparty/dpdk-20.05
patch config/common_base < config/enable_PMD_AESNI_MB.patch
export DESTDIR=$ROOT/3rdparty/dpdk-20.05/distr/x86_64-native-linuxapp-gcc
make install T=x86_64-native-linuxapp-gcc

echo ============== LIBMEMIF =====================
cd $ROOT
cd libmemif
mkdir build
cd build
cmake ..
make
make install

echo ============== FRIDMON =====================
cd $ROOT
unzip 3rdparty/fridmon-0.1.10.zip -d ./3rdparty

cd 3rdparty/fridmon-0.1.10/meson_1_0_1/project/redhat
make CFG=release

cd $ROOT
cd 3rdparty/fridmon-0.1.10/hyperon_1_0_1/project/redhat
make CFG=release

echo ============== CIPH_APP =====================
cd $ROOT
cd project/linux
make CFG=release
