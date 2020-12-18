
ROOT=$PWD 
echo $ROOT

echo ============== cleanup =====================
cd $ROOT
rm -rf cd 3rdparty/fridmon-0.1.10
cd project/linux
make CFG=release clean
