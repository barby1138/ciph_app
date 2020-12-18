
ROOT=$PWD 
echo $ROOT

echo ============== ROOTFS =====================
cd $ROOT
cd 3rdparty_artifactory
tar zxf rootfs_centos-7-amd64.tar.gz -C $ROOT/lxc/ciph_app

echo =========== INASTALL TO CONT ==============
cd $ROOT
#app
cp ciph_app/project/linux/dpdk-crypto-app $ROOT/lxc/ciph_app/rootfs/tmp
#libnuma
tar zxf 3rdparty_artifactory/libnuma-10.tar.gz -C $ROOT/lxc/ciph_app/rootfs/usr/lib
#driver
tar zxf 3rdparty_artifactory/Intel-IpSec-MB-54.tar.gz -C $ROOT/lxc/ciph_app/rootfs/usr/lib
