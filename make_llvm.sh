#! /bin/bash

if [ ! -d "build" ]; then
	mkdir build
fi

cd build

cmake  ../llvm -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/home/user/Desktop/llvm-project-build -DBUILD_SHARED_LIBS=on \
-DLLVM_ENABLE_PROJECTS=clang -DLLVM_USE_LINKER=gold -DCMAKE_BUILD_TYPE=Release

num_of_processors=$("nproc")
make -j$num_of_processors
