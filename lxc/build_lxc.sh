DEPLPATH=$PWD
echo $DEPLPATH

LXCPATH=$(lxc-config lxc.lxcpath)
echo $LXCPATH

echo ============== CLEANUP =====================
lxc-stop -n ciph_app -k
rm -rf $LXCPATH/ciph_app

echo ============== CREATE =====================
lxc-create -n ciph_app -t download -- -d centos -r 7 -a amd64 --keyserver hkp://p80.pool.sks-keyservers.net:80

echo =========== PREPARE ==============
cd $DEPLPATH
cp -f config $LXCPATH/ciph_app
cp VERSION $LXCPATH/ciph_app
mkdir $LXCPATH/ciph_app/rootfs/home/ciph_app
# dpdk-crypto-app, ciph_app.xml
cp bin/* $LXCPATH/ciph_app/rootfs/home/ciph_app
chmod 0755 $LXCPATH/ciph_app/rootfs/home/ciph_app/dpdk-crypto-app
# libnuma, libIPSec_MB
cp lib/* $LXCPATH/ciph_app/rootfs/usr/lib64
# TODO put app to startup

echo =========== START ==============
lxc-start -n ciph_app
