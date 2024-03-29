CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
INCLUDE(funra)

PROJECT(rserver)

SET(RPROJECT_ROOT ${PROJECT_SOURCE_DIR}/..)
SET(RSERVER_SOURCE_ROOT ${PROJECT_SOURCE_DIR})

IF (CMAKE_BUILD_TYPE MATCHES "Debug")
	MESSAGE(STATUS "Build as type of Debug")
	SET(RBUILD_TYPE_POSTFIX ${CMAKE_DEBUG_POSTFIX})
ELSEIF (CMAKE_BUILD_TYPE MATCHES "Release")
	MESSAGE(STATUS "Build as type of Release")
	SET(RBUILD_TYPE_POSTFIX ${CMAKE_RELEASE_POSTFIX})
ELSE()
	MESSAGE(STATUS "Build as type [${CMAKE_BUILD_TYPE}], set as Debug.")
	SET(RBUILD_TYPE_POSTFIX ${CMAKE_DEBUG_POSTFIX})
ENDIF ()

#set flag and subfix
IF (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET(PROJECT_FLAGS_COMMON "-fPIC -fexceptions -Wall -D__STDC_FORMAT_MACROS -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -g3 -ggdb -O0 -rdynamic -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -g0 -O2 -rdynamic -DNDEBUG")
ELSEIF (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    SET(PROJECT_FLAGS_COMMON "/GF /MP /EHsc /Gd /W3 -D_CRT_SECURE_NO_WARNINGS")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} /MTd /Ob0 /Od /RTC1 -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} /MT /Ob2 /O2 -DNDEBUG")
    SET(CMAKE_EXE_LINKER_FLAGS /NODEFAULTLIB:"MSVCRT.lib;MSVCRTD.lib")
ENDIF ()
SET(CMAKE_C_FLAGS_DEBUG "${PROJECT_C_FLAGS_DEBUG}" CACHE STRING "Set debug build flags" FORCE)
SET(CMAKE_C_FLAGS_RELEASE "${PROJECT_C_FLAGS_RELEASE}" CACHE STRING "Set relese build flags" FORCE)
SET(CompilerFlags
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_C_FLAGS
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    )
FOREACH(CompilerFlag ${CompilerFlags})
    STRING(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
ENDFOREACH()

# 64位
IF (CMAKE_CL_64)
	MESSAGE(STATUS "Arch type is 64.")
	SET(ARCH_TYPE 64)
# 32
ELSE()
	MESSAGE(STATUS "Arch type is 32.")
	SET(ARCH_TYPE 32)
ENDIF()

IF (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    SET(PROJECT_DEFINES "-DOS_LINUX -DLUA_USE_POSIX")
    SET(DIR_PLATFORM Linux_${CMAKE_SYSTEM_PROCESSOR})
ELSEIF (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    SET(PROJECT_DEFINES "-DWIN32 -DOS_Windows")
    SET(DIR_PLATFORM Windows_${CMAKE_SYSTEM_PROCESSOR})

    ADD_COMPILE_OPTIONS("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")
ELSE()
    SET(PROJECT_DEFINES)
    SET(DIR_PLATFORM ${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
ENDIF ()
ADD_DEFINITIONS(${PROJECT_DEFINES})


SET(RSERVER_LIB_ROOT ${PROJECT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM})
SET(RSERVER_BINARY_ROOT ${PROJECT_SOURCE_DIR}/../../build/bin/${DIR_PLATFORM})

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    LINK_DIRECTORIES(
        ${RSERVER_LIB_ROOT}
    )
    
    SET(LINK_LIBS 
		legacy_stdio_definitions
		3rd/cmocka${RBUILD_TYPE_POSTFIX}
        3rd/lua${RBUILD_TYPE_POSTFIX} 
        3rd/uv${RBUILD_TYPE_POSTFIX}
        3rd/uv_a${RBUILD_TYPE_POSTFIX} 
        3rd/iconv${RBUILD_TYPE_POSTFIX} 
        rcommon${RBUILD_TYPE_POSTFIX} 
        ripc${RBUILD_TYPE_POSTFIX}  
	)
else()
    LINK_DIRECTORIES(
        /usr/local/lib
        ${RSERVER_LIB_ROOT}
        ${RSERVER_LIB_ROOT}/3rd
    )
    SET(LINK_LIBS 
        dl 
        m 
        pthread
        liblua${RBUILD_TYPE_POSTFIX}.a 
        libuv${RBUILD_TYPE_POSTFIX}.a 
        libuv_a${RBUILD_TYPE_POSTFIX}.a 
        librcommon${RBUILD_TYPE_POSTFIX}.a
        libripc${RBUILD_TYPE_POSTFIX}.a    
    )
    SET(PROJECT_LINK_OPTIONS
        #-ljemalloc 
        -luv
        #-liconv
        -lstdc++
        -static-libgcc
        -static-libstdc++
    )
endif()

MESSAGE(STATUS "Build system: ${CMAKE_HOST_SYSTEM_NAME}")
MESSAGE(STATUS "Link dirs: ${RSERVER_LIB_ROOT}")
MESSAGE(STATUS "Link libs: ${LINK_LIBS}")
MESSAGE(STATUS "Link opts: ${PROJECT_LINK_OPTIONS}")

ADD_LINK_OPTIONS(${PROJECT_LINK_OPTIONS})

SET(CMAKE_INSTALL_PREFIX					${RSERVER_BINARY_ROOT})

# 指定可执行程序输出目录
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG 	${RSERVER_BINARY_ROOT})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE 	${RSERVER_BINARY_ROOT})

# 指定 库文件输出目录
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG 	${RSERVER_LIB_ROOT})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE 	${RSERVER_LIB_ROOT})


ADD_SUBDIRECTORY(commonsvr)
ADD_SUBDIRECTORY(funrasvr)
ADD_SUBDIRECTORY(funrasvr/test)
