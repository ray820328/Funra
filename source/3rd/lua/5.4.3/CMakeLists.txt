CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lua)

SET(CMAKE_C_STANDARD 99)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Set debug library postfix" FORCE)
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Set release library postfix" FORCE)

#set flag and subfix
IF (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET(PROJECT_FLAGS_COMMON "-fPIC -fexceptions -Wall -D__STDC_FORMAT_MACROS -DLUA_COMPAT_BITLIB -DLUA_COMPAT_APIINTCASTS -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -g3 -ggdb -O0 -rdynamic -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -g0 -O2 -rdynamic -DNDEBUG")
ELSEIF (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    SET(PROJECT_FLAGS_COMMON "-GF -MP -EHsc -Gd -W3 -D_CRT_SECURE_NO_WARNINGS -DLUA_COMPAT_BITLIB -DLUA_COMPAT_APIINTCASTS -DLUA_BUILD_AS_DLL")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -MDd -Ob0 -Od -RTC1 -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -MD -Ob2 -O2 -DNDEBUG")
ENDIF ()
SET(CMAKE_C_FLAGS_DEBUG "${PROJECT_C_FLAGS_DEBUG}" CACHE STRING "Set debug build flags" FORCE)
SET(CMAKE_C_FLAGS_RELEASE "${PROJECT_C_FLAGS_RELEASE}" CACHE STRING "Set relese build flags" FORCE)

#OPTION (USE_LUA_SOCKET "Use provided lua socket implementation" ON)
#IF (USE_LUA_SOCKET)
#    ADD_SUBDIRECTORY (src/luasocket)
#    SET (EXTRA_LIBS ${EXTRA_LIBS} luasocket)
#ENDIF (USE_LUA_SOCKET)

INCLUDE_DIRECTORIES(src include src/luasocket)

SET(SRC_CORE
        src/lapi.c
        src/lcode.c
        src/lctype.c
        src/ldebug.c
        src/ldo.c
        src/ldump.c
        src/lfunc.c
        src/lgc.c
        src/llex.c
        src/lmem.c
        src/lobject.c
        src/lopcodes.c
        src/lparser.c
        src/lstate.c
        src/lstring.c
        src/ltable.c
        src/ltm.c
        src/lundump.c
        src/lvm.c
        src/lzio.c
        #src/rlog.c
        #src/rprofiler.c
        #src/rmemory_leak_checker.c
        )

SET(SRC_LIB
        src/lauxlib.c
        src/lbaselib.c
        #src/lbitlib.c
        src/lcorolib.c
        src/ldblib.c
        src/liolib.c
        src/lmathlib.c
        src/loslib.c
        src/lstrlib.c
        src/ltablib.c
        src/lutf8lib.c
        src/loadlib.c
        src/linit.c
        )

SET(SRC_LUA
        src/lua.c
        )

SET(SRC_LUAC
        src/luac.c
        )

SET(SRC_LIB_LUASOCKET
        src/luasocket/auxiliar.c
        src/luasocket/buffer.c
        src/luasocket/except.c
        src/luasocket/inet.c
        src/luasocket/io.c
        src/luasocket/luasocket.c
        src/luasocket/options.c
        src/luasocket/select.c
        src/luasocket/tcp.c
        src/luasocket/timeout.c
        src/luasocket/udp.c
        src/luasocket/mime.c
)

IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    SET(SRCS_LIB_OS
        src/luasocket/wsocket.c
    )
ELSE()
    SET(SRCS_LIB_OS
        src/luasocket/usocket.c
    )
ENDIF ()

#ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_CORE} ${SRC_LIB} ${SRC_LIB_LUASOCKET} ${SRCS_LIB_OS})
#ADD_EXECUTABLE(${PROJECT_NAME}_bin ${SRC_LIB} ${SRC_CORE} ${SRC_LUA} ${SRC_LIB_LUASOCKET} ${SRCS_LIB_OS})

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_CORE} ${SRC_LIB})
ADD_EXECUTABLE(${PROJECT_NAME}_bin ${SRC_LIB} ${SRC_CORE} ${SRC_LUA})

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR} - ${R3RD_LIB_ROOT}")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM}
    )
    
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
        legacy_stdio_definitions 
        ws2_32 
        #${CMAKE_CURRENT_SOURCE_DIR}/lib/raylibcd.lib 
        ${EXTRA_LIBS}
    )
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_bin 
        legacy_stdio_definitions 
        ws2_32 
        #${CMAKE_CURRENT_SOURCE_DIR}/lib/raylibcd.lib 
        ${EXTRA_LIBS})
else()
    LINK_DIRECTORIES(
         ${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM}
    )
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
        pthread dl 
        m 
        #${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylibc.a 
        ${EXTRA_LIBS}
    )
    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME}_bin
#        -Wl,--start-group
        pthread
        dl
        m
        #${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylibc.a
        ${EXTRA_LIBS}
#        -Wl,--end-group
    )
endif()

