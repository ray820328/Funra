#!/bin/sh

# -j to enable parallel compiling
coreCount=`cat /proc/cpuinfo| grep "processor"| wc -l`
ROOT_PATH=$(pwd)

echo "Build($0 ${BUILD_PACKAGE_TYPE}): ${ROOT_PATH}"

mkdir -p build/Debug
mkdir -p build/Release

#cmake -S ${ROOT_PATH} -B ${ROOT_PATH}/build/Debug -DCMAKE_BUILD_TYPE=Debug ${LUA_BS_BUILD_OPTIONS} && cmake --build ${ROOT_PATH}/build/debug --target install -j ${CPU_CORE_NUM}
#cmake -S ${ROOT_PATH} -B ${ROOT_PATH}/build/Release -DCMAKE_BUILD_TYPE=Release ${LUA_BS_BUILD_OPTIONS} && cmake --build ${ROOT_PATH}/build/release --target install -j ${CPU_CORE_NUM}
if [[ ${BUILD_PACKAGE_TYPE} = "All" || ${BUILD_PACKAGE_TYPE} = "Debug" ]];then
    cd ${ROOT_PATH}/build/Debug
    cmake -DCMAKE_BUILD_TYPE=Debug ../..
    make -j $coreCount $@
fi

if [[ ${BUILD_PACKAGE_TYPE} = "All" || ${BUILD_PACKAGE_TYPE} = "Release" ]];then
    cd ${ROOT_PATH}/build/Release
    cmake -DCMAKE_BUILD_TYPE=Release ../..
    make -j $coreCount $@
fi

if [[ ${BUILD_PACKAGE_TYPE} != "All" && ${BUILD_PACKAGE_TYPE} != "Debug" && ${BUILD_PACKAGE_TYPE} != "Release" ]];then
    echo "Error, build params must as: $0 [All|Debug|Release]"
fi

cd ${ROOT_PATH}
rm -rf build
