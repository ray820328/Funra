SET(CMAKE_C_STANDARD 99)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Set debug librbased postfix" FORCE)
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Set release librbase postfix" FORCE)

INCLUDE_DIRECTORIES(
    ../
    ../3rd/lua/5.4.3/src
    ../rbase/common/include
    ../rserver/commonsvr/include
)
