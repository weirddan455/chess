rm -rf win-build
mkdir win-build
cd win-build
CC=x86_64-w64-mingw32-gcc cmake -DCMAKE_SYSTEM_NAME=Windows ..
make
ln -s ../assets assets
