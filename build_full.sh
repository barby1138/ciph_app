
ROOT=$PWD 
echo $ROOT

#echo ============== NASM =====================
#cd $ROOT
#tar -xf 3rdparty/nasm_2.15.02.orig.tar.xz -C ./3rdparty
#cd 3rdparty/nasm-2.15.02
#./configure
#make
#make install

echo ============== IPSEC =====================
IPSECVER=0.54
echo $IPSECVER

cd $ROOT
unzip 3rdparty/intel-ipsec-mb-$IPSECVER.zip -d ./3rdparty
#cp 3rdparty/snow3g_common_NO_8.patch 3rdparty/intel-ipsec-mb-$IPSECVER/include
cd 3rdparty/intel-ipsec-mb-$IPSECVER
#patch include/snow3g_common.h < include/snow3g_common_NO_8.patch
make SAFE_DATA=y SAFE_PARAM=y
make install PREFIX=$PWD/intel-ipsec-mb-$IPSECVER-install NOLDCONFIG=y

echo ============== DPDK =====================
#20.05
DPDK_VER=20.05
echo $DPDK_VER

cd $ROOT
tar -xf 3rdparty/dpdk-$DPDK_VER.tar.gz -C ./3rdparty
cp 3rdparty/enable_PMD_AESNI_MB.patch 3rdparty/dpdk-$DPDK_VER/config
cp 3rdparty/rte_snow3g_pmd_N_BUFF.patch 3rdparty/dpdk-$DPDK_VER/drivers/crypto/snow3g
#cp 3rdparty/rte_snow3g_pmd_7.patch 3rdparty/dpdk-$DPDK_VER/drivers/crypto/snow3g
cd 3rdparty/dpdk-$DPDK_VER
patch config/common_base < config/enable_PMD_AESNI_MB.patch
patch drivers/crypto/snow3g/rte_snow3g_pmd.c < drivers/crypto/snow3g/rte_snow3g_pmd_N_BUFF.patch
#patch drivers/crypto/snow3g/rte_snow3g_pmd.c < drivers/crypto/snow3g/rte_snow3g_pmd_7.patch
export DESTDIR=$ROOT/3rdparty/dpdk-$DPDK_VER/distr/x86_64-native-linuxapp-gcc
make install T=x86_64-native-linuxapp-gcc \
    EXTRA_CFLAGS=-I$ROOT/3rdparty/intel-ipsec-mb-$IPSECVER/intel-ipsec-mb-$IPSECVER-install/include \
    EXTRA_LDFLAGS=-L$ROOT/3rdparty/intel-ipsec-mb-$IPSECVER/intel-ipsec-mb-$IPSECVER-install/lib
    
#echo ============== FRIDMON =====================
#cd $ROOT
#unzip 3rdparty/fridmon-0.1.10.zip -d ./3rdparty

#cd 3rdparty/fridmon-0.1.10/meson_1_0_1/project/redhat
#make CFG=release

#cd $ROOT
#cd 3rdparty/fridmon-0.1.10/hyperon_1_0_1/project/redhat
#make CFG=release

echo ============== CIPH_APP =====================
cd $ROOT
cd project/linux
make CFG=release clean
make CFG=release \
    EXTRA_CFLAGS=-I$ROOT/3rdparty/intel-ipsec-mb-$IPSECVER/intel-ipsec-mb-$IPSECVER-install/include \
    EXTRA_LDFLAGS=-L$ROOT/3rdparty/intel-ipsec-mb-$IPSECVER/intel-ipsec-mb-$IPSECVER-install/lib
