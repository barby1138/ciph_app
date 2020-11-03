

#tar -xvf 3rdparty/dpdk-19.11.3.tar.xz  -C ./3rdparty

ROOT=$PWD 

echo ============== FRIDMON =====================
#unzip 3rdparty/fridmon-0.1.10.zip  -d ./3rdparty

#cd 3rdparty/fridmon-0.1.10/meson_1_0_1/project/redhat
#make CFG=release

#cd $ROOT
#cd 3rdparty/fridmon-0.1.10/hyperon_1_0_1/project/redhat
#make CFG=release

echo ============== NASM =====================
#cd $ROOT
#tar -xvf 3rdparty/nasm_2.15.02.orig.tar.xz  -C ./3rdparty
#cd 3rdparty/nasm-2.15.02
#./configure
#make
#make install

echo ============== IPSEC =====================
cd $ROOT
unzip 3rdparty/intel-ipsec-mb-0.54.zip  -d ./3rdparty
cd 3rdparty/intel-ipsec-mb-0.54
./configure
make
#make install
