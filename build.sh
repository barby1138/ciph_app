
ROOT=$PWD 
echo $ROOT

echo ============== COPY ARTIFACTORY =====================
cd $ROOT
mkdir 3rdparty_artifactory
cp /work/dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz 3rdparty_artifactory
cp /work/rootfs_centos-7-amd64.tar.gz 3rdparty_artifactory

echo ============== DPDK =====================
cd $ROOT
cd 3rdparty_artifactory
tar zxf dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz -C $ROOT/3rdparty

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

echo ============== ROOTFS =====================
#cd $ROOT
#cd 3rdparty_artifactory
#tar zxf rootfs_centos-7-amd64.tar.gz -C $ROOT/lxc/ciph_app

echo =========== INASTALL TO CONT ==============
#cd $ROOT
#cp ciph_app/project/linux/dpdk-crypto-app $ROOT/lxc/ciph_app/rootfs/tmp
#cp /usr/lib/libnuma.so* $ROOT/lxc/ciph_app/rootfs/usr/lib
#tar zxf 3rdparty_artifactory/Intel-IpSec-MB-54.tar.gz -C $ROOT/lxc/ciph_app/rootfs/usr/lib
