
ROOT=$PWD 
echo $ROOT

echo ============== COPY ARTIFACTORY =====================
cd $ROOT
mkdir 3rdparty_artifactory
cp /tmp/dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz 3rdparty_artifactory
cp /tmp/rootfs_centos-7-amd64.tar.gz 3rdparty_artifactory
cp /tmp/libIPSec_MB.0.54.0.tar.gz 3rdparty_artifactory

echo ============== IPSec =====================
cd $ROOT
cd 3rdparty_artifactory
tar zxf libIPSec_MB.0.54.0.tar.gz
cp -f libIPSec_MB* /usr/lib

echo ============== DPDK =====================
cd $ROOT
cd 3rdparty_artifactory
tar zxf dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz -C $ROOT/3rdparty

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

#echo ============== PREP CONT ROOTFS =====================
#cd $ROOT
#cd 3rdparty_artifactory
#tar zxf rootfs_centos-7-amd64.tar.gz -C $ROOT/lxc/ciph_app

#echo =========== PREP CONT FILES ==============
#cd $ROOT
#cp VERSION $ROOT/lxc/ciph_app
#mkdir $ROOT/lxc/ciph_app/rootfs/home/ciph_app
#cp ciph_app/project/linux/dpdk-crypto-app $ROOT/lxc/ciph_app/rootfs/home/ciph_app
#cp ciph_app/project/linux/ciph_app.xml $ROOT/lxc/ciph_app/rootfs/home/ciph_app
#cp /usr/lib64/libnuma.so* $ROOT/lxc/ciph_app/rootfs/usr/lib64
#cp 3rdparty_artifactory/libIPSec_MB* $ROOT/lxc/ciph_app/rootfs/usr/lib64

#echo =========== PACK CONT ==============
#cd $ROOT/lxc
#tar czf ciph_app.tar-1.0.1.gz ciph_app
#mv ciph_app.tar-1.0.1.gz $ROOT/dist

echo =========== PREP DEVEL ==============
cd $ROOT
mkdir dist/ciph_app_devel
mkdir dist/ciph_app_devel/include
mkdir dist/ciph_app_devel/lib
mkdir dist/ciph_app_devel/examples

cp libdpdk_cryptodev_client/data_vectors.h dist/ciph_app_devel/include
cp libciph_agent/ciph_agent_c.h dist/ciph_app_devel/include

cp lib/linux/release/libciph_agent.a dist/ciph_app_devel/lib
cp lib/linux/release/libmemif_client.a dist/ciph_app_devel/lib

cp ciph_app_test/ciph_app_test.c dist/ciph_app_devel/examples
cp ciph_app_test/build.sh dist/ciph_app_devel/examples

cp VERSION dist/ciph_app_devel

echo =========== PACK DEVEL ==============
cd $ROOT/dist
tar czf ciph_app_devel.tar-1.0.1.gz ciph_app_devel
