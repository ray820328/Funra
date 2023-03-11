CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lua)

SET(CMAKE_C_STANDARD 99)
SET(SGE_SOURCE_ROOT ${PROJECT_SOURCE_DIR})
SET(SGE_BINARY_ROOT ${PROJECT_BINARY_DIR})

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Set debug library postfix" FORCE)
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Set release library postfix" FORCE)

#set flag and subfix
IF (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET(PROJECT_FLAGS_COMMON "-fPIC -fexceptions -Wall -D__STDC_FORMAT_MACROS -DLUA_USE_LINUX -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -g3 -O0 -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -g0 -O2 -DNDEBUG")
ELSEIF (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    SET(PROJECT_FLAGS_COMMON "-GF -MP -EHsc -Gd -W3 -D_CRT_SECURE_NO_WARNINGS -DLUA_LIB -DLUA_BUILD_AS_DLL")
    SET(PROJECT_C_FLAGS_DEBUG "${PROJECT_FLAGS_COMMON} -MTd -Ob0 -Od -RTC1 -D_DEBUG -DCHECK_MEM_LEAK")
    SET(PROJECT_C_FLAGS_RELEASE "${PROJECT_FLAGS_COMMON} -MT -Ob2 -O2 -DNDEBUG")
ENDIF ()
SET(CMAKE_C_FLAGS_DEBUG "${PROJECT_C_FLAGS_DEBUG}" CACHE STRING "Set debug build flags" FORCE)
SET(CMAKE_C_FLAGS_RELEASE "${PROJECT_C_FLAGS_RELEASE}" CACHE STRING "Set relese build flags" FORCE)

IF (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    SET(PROJECT_DEFINES "-DOS_LINUX -DLUA_USE_POSIX")
ELSEIF (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    SET(PROJECT_DEFINES "-DWIN32 -DOS_Windows")
ELSE()
    SET(PROJECT_DEFINES)
ENDIF ()
ADD_DEFINITIONS(${PROJECT_DEFINES})

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

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
)

SET(SRC_LIB
    src/lauxlib.c
    src/lbaselib.c
    src/lbitlib.c
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

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_CORE} ${SRC_LIB})
ADD_EXECUTABLE(${PROJECT_NAME}_bin ${SRC_LIB} ${SRC_CORE} ${SRC_LUA})

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR}")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
    )
    
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} legacy_stdio_definitions ws2_32 ${EXTRA_LIBS})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_bin 
        legacy_stdio_definitions 
        ws2_32
        ${EXTRA_LIBS})
else()
    LINK_DIRECTORIES(
         ${CMAKE_CURRENT_SOURCE_DIR}/lib/
    )
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread dl m ${EXTRA_LIBS})
    TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME}_bin
#        -Wl,--start-group
        pthread
        dl
        m
        ${EXTRA_LIBS}
#        -Wl,--end-group
    )
endif()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )

INSTALL(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )

