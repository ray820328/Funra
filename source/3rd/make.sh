#!/bin/sh

# -j to enable parallel compiling
coreCount=`cat /proc/cpuinfo| grep "processor"| wc -l`
ROOT_PATH=$(pwd)

mkdir -p build/Debug
mkdir -p build/Release

#cmake -S ${ROOT_PATH} -B ${ROOT_PATH}/build/Debug -DCMAKE_BUILD_TYPE=Debug ${LUA_BS_BUILD_OPTIONS} && cmake --build ${ROOT_PATH}/build/debug --target install -j ${CPU_CORE_NUM}
#cmake -S ${ROOT_PATH} -B ${ROOT_PATH}/build/Release -DCMAKE_BUILD_TYPE=Release ${LUA_BS_BUILD_OPTIONS} && cmake --build ${ROOT_PATH}/build/release --target install -j ${CPU_CORE_NUM}

if [[ ${BUILD_PACKAGE_TYPE} = "All" || ${BUILD_PACKAGE_TYPE} = "Debug" ]];then
    cd build/Debug
    cmake -DCMAKE_BUILD_TYPE=Debug ../..
    make -j $coreCount $@
fi

if [[ ${BUILD_PACKAGE_TYPE} = "All" || ${BUILD_PACKAGE_TYPE} = "Release" ]];then
    cd ../Release
    cmake -DCMAKE_BUILD_TYPE=Release ../..
    make -j $coreCount $@
fi

cd ${ROOT_PATH}
rm -rf build
