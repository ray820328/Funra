SET(CMAKE_C_STANDARD 99)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Set debug librbased postfix" FORCE)
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Set release librbase postfix" FORCE)

INCLUDE_DIRECTORIES(
    ../
    ../3rd/lua/5.3.6/src
    #../3rd/lua/5.4.3/src
    ../3rd/lua-protobuf
    ../3rd/libuv/1.42.0/include
    ../3rd/win-iconv
    ../rbase/common/include
    ../rbase/ipc/include
    ../rbase/rpc/include
    ../rbase/ecs/include
    ../rserver/commonsvr/include
)
