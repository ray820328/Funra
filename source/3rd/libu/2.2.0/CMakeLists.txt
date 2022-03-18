CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(u)

SET(CMAKE_C_STANDARD 99)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Set debug library postfix" FORCE)
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Set release library postfix" FORCE)

#set flag and subfix
IF (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET(PROJECT_FLAGS_COMMON "-fPIC -fexceptions -Wall -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -g3 -ggdb -O0 -rdynamic -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -g0 -O3 -rdynamic -DNDEBUG")
ELSEIF (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    SET(PROJECT_FLAGS_COMMON "-GF -MP -EHsc -Gd -W3 -D_CRT_SECURE_NO_WARNINGS")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -MDd -Ob0 -Od -RTC1 -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -MD -Ob2 -O2 -DNDEBUG")
ENDIF ()
SET(CMAKE_C_FLAGS_DEBUG "${PROJECT_C_FLAGS_DEBUG}" CACHE STRING "Set debug build flags" FORCE)
SET(CMAKE_C_FLAGS_RELEASE "${PROJECT_C_FLAGS_RELEASE}" CACHE STRING "Set relese build flags" FORCE)

INCLUDE_DIRECTORIES(
    include
)

aux_source_directory(srcs/missing _SOURCES)
set(SRC_CORE ${SRC_CORE} ${_SOURCES})

aux_source_directory(srcs/toolbox _SOURCES)
set(SRC_CORE ${SRC_CORE} ${_SOURCES})

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_CORE})

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR} - ${R3RD_LIB_ROOT}")
IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM}
    )
    
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
        legacy_stdio_definitions 
        ws2_32
    )
ELSE()
    LINK_DIRECTORIES(
         ${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM}
    )
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
        pthread dl 
        m
    )
ENDIF()

