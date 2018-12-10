cd source/external/gitsubmodules/glfw

mkdir build_dll
cd build_dll
cmake -DBUILD_SHARED_LIBS=ON -G "Unix Makefiles" ../
make

cp src/Debug/*.so ../../../lib/unix
