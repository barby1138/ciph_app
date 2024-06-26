DEPLPATH=$PWD
echo $DEPLPATH

LXCPATH=$(lxc-config lxc.lxcpath)
echo $LXCPATH

echo ============== CLEANUP =====================
lxc-stop -n ciph_app -k
rm -rf $LXCPATH/ciph_app

echo ============== UNTAR ROOTFS =====================
cd $DEPLPATH/lxc
mkdir $LXCPATH/ciph_app
tar zxf rootfs_centos-7-amd64.tar.gz -C $LXCPATH/ciph_app

echo =========== PREPARE APP ==============
cd $DEPLPATH
CPU_NUM=$(grep -c ^processor /proc/cpuinfo)
echo $CPU_NUM
if [[ $CPU_NUM -eq 64 ]]; then
    echo "lxc.cgroup.cpuset.cpus = 30,62" >> config
elif [[ $CPU_NUM -eq 48 ]]; then
    echo "lxc.cgroup.cpuset.cpus = 22,46" >> config
elif [[ $CPU_NUM -eq 28 ]]; then
    echo "lxc.cgroup.cpuset.cpus = 1,15" >> config
else
    echo "WARNING: unknown arch - do nothing"
fi
cp -f config $LXCPATH/ciph_app

cp VERSION $LXCPATH/ciph_app
mkdir $LXCPATH/ciph_app/rootfs/home/ciph_app
mkdir $LXCPATH/ciph_app/rootfs/var/ciph_app
# dpdk-crypto-app, ciph_app.xml
cp bin/* $LXCPATH/ciph_app/rootfs/home/ciph_app
chmod 0755 $LXCPATH/ciph_app/rootfs/home/ciph_app/dpdk-crypto-app
# libnuma, libIPSec_MB
cp lib/* $LXCPATH/ciph_app/rootfs/usr/lib64

echo =========== PREPARE SERVICE ==============
cd $LXCPATH/ciph_app/rootfs
cp $DEPLPATH/svc/ciph_app.service etc/systemd/system
cp $DEPLPATH/svc/ciph_app.sh home/ciph_app
chmod 0755 home/ciph_app/ciph_app.sh

echo =========== PREPARE LOGROTATE ==============
cp $DEPLPATH/log/syslog etc/logrotate.d/

# enable
#if [ ! -L etc/systemd/system/ciph_app.service ]; then
echo "create ciph_app.service link"
ln -s -r etc/systemd/system/ciph_app.service etc/systemd/system/multi-user.target.wants/ciph_app.service
#fi
ls -lrt etc/systemd/system/multi-user.target.wants/ciph_app.service

echo =========== CREATE DIRs AT HOST ==============
mkdir /var/ciph_app
mkdir /var/log/ciph_app

echo =========== START ==============
lxc-start -n ciph_app
