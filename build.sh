
ROOT=$PWD 
echo $ROOT
VER=$(cat VERSION)
echo ${VER}

echo ============== COPY ARTIFACTORY ==================
cd $ROOT
mkdir 3rdparty_artifactory
cp /tmp/dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz 3rdparty_artifactory
#cp /tmp/rootfs_centos-7-amd64.tar.gz 3rdparty_artifactory
cp /tmp/libIPSec_MB.0.54.0.tar.gz 3rdparty_artifactory

# done in Dockerfile
#echo ============== PREP IPSec ========================
cd $ROOT
cd 3rdparty_artifactory
tar zxf libIPSec_MB.0.54.0.tar.gz
#cp -f libIPSec_MB* /usr/lib

echo ============== PREP DPDK =========================
cd $ROOT
cd 3rdparty_artifactory
tar zxf dpdk-20.05-x86_64-native-linuxapp-gcc.tar.gz -C $ROOT/3rdparty

echo ============== BUILD FRIDMON =====================
cd $ROOT
unzip 3rdparty/fridmon-0.1.10.zip -d ./3rdparty

cd 3rdparty/fridmon-0.1.10/meson_1_0_1/project/redhat
make CFG=release

cd $ROOT
cd 3rdparty/fridmon-0.1.10/hyperon_1_0_1/project/redhat
make CFG=release

echo ============== BUILD CIPH_APP ====================
cd $ROOT
cd project/linux
make CFG=release

echo ============== PREP DEVEL ========================
cd $ROOT
mkdir dist/ciph_app_devel
mkdir dist/ciph_app_devel/include
mkdir dist/ciph_app_devel/lib
mkdir dist/ciph_app_devel/examples

cp libdpdk_cryptodev_client/data_vectors.h dist/ciph_app_devel/include
cp libciph_agent/ciph_agent_c.h dist/ciph_app_devel/include

cp lib/linux/release/libciph_agent.so dist/ciph_app_devel/lib

cp ciph_app_test/ciph_app_test.c dist/ciph_app_devel/examples
cp ciph_app_test/build.sh dist/ciph_app_devel/examples

cp VERSION dist/ciph_app_devel
cp dist/readme dist/ciph_app_devel

echo =========== PACK DEVEL ===========================
cd $ROOT/dist
tar czf ciph_app_devel-$VER.tar.gz ciph_app_devel

echo =========== LXC ==================================
echo =========== PREP FILES FOR RPM ===================
RPMBUILD=~/rpmbuild/BUILD
cd $ROOT
echo $RPMBUILD
rm -rf $RPMBUILD/ciph_app
mkdir -p ~/rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
mkdir $RPMBUILD/ciph_app
mkdir $RPMBUILD/ciph_app/bin
mkdir $RPMBUILD/ciph_app/lib
mkdir $RPMBUILD/ciph_app/svc
mkdir $RPMBUILD/ciph_app/log

cp VERSION $RPMBUILD/ciph_app
cp cont/lxc/config $RPMBUILD/ciph_app
cp cont/lxc/install_lxc.sh $RPMBUILD/ciph_app
cp cont/lxc/ciph_app.service $RPMBUILD/ciph_app/svc
cp cont/ciph_app.sh $RPMBUILD/ciph_app/svc
cp cont/syslog $RPMBUILD/ciph_app/log
cp ciph_app/project/linux/dpdk-crypto-app $RPMBUILD/ciph_app/bin
cp ciph_app/project/linux/ciph_app.xml $RPMBUILD/ciph_app/bin
cp /usr/lib64/libnuma.so* $RPMBUILD/ciph_app/lib
cp 3rdparty_artifactory/libIPSec_MB* $RPMBUILD/ciph_app/lib

mkdir $RPMBUILD/ciph_app/lxc
cp /tmp/rootfs_centos-7-amd64.tar.gz $RPMBUILD/ciph_app/lxc

echo =========== BUILD RPM ============================
cd $ROOT/cont/lxc
rpmbuild -bb --define "_ver $VER" ciph_app.spec

echo =========== PACK RPM =============================
cd $ROOT/dist
cp ~/rpmbuild/RPMS/x86_64/ciph_app-$VER-rel.x86_64.rpm .

echo =========== K8s ==================================
echo =========== PREP FILES FOR K8s ===================
cd $ROOT
CONT_K8S_DIR=$ROOT/cont/k8s

mkdir $CONT_K8S_DIR/ciph_app
mkdir $CONT_K8S_DIR/ciph_app/bin
mkdir $CONT_K8S_DIR/ciph_app/lib
mkdir $CONT_K8S_DIR/ciph_app/svc
mkdir $CONT_K8S_DIR/ciph_app/log

cp VERSION $CONT_K8S_DIR/ciph_app
cp cont/ciph_app.sh $CONT_K8S_DIR/ciph_app/svc
cp cont/syslog $CONT_K8S_DIR/ciph_app/log
cp ciph_app/project/linux/dpdk-crypto-app $CONT_K8S_DIR/ciph_app/bin
cp ciph_app/project/linux/ciph_app.xml $CONT_K8S_DIR/ciph_app/bin
cp /usr/lib64/libnuma.so* $CONT_K8S_DIR/ciph_app/lib
cp 3rdparty_artifactory/libIPSec_MB* $CONT_K8S_DIR/ciph_app/lib

mkdir $CONT_K8S_DIR/ciph_app_test
cp bin/linux/release/ciph_app_test $CONT_K8S_DIR/ciph_app_test
cp lib/linux/release/libciph_agent.so $CONT_K8S_DIR/ciph_app_test

echo =========== K8S BUILD ===========================
cd $CONT_K8S_DIR
chmod +x build_k8s.sh && ./build_k8s.sh

echo =========== K8S TO DIST =========================
cd $ROOT
cp $CONT_K8S_DIR/deploy/helm/ciph_app_test_chart/ciph_app_test-$VER.tgz dist
cp $CONT_K8S_DIR/deploy/helm/ciph_app_chart/ciph_app-$VER.tgz dist

