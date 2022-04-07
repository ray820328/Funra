CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(rcommon)

#SET(EXECUTABLE_OUTPUT_PATH ${RBASE_BINARY_ROOT}) 

INCLUDE_DIRECTORIES(include)

SET(SRC_LIB
        src/dict.c
        src/mt19937-64.c
        src/siphash.c
        src/rdict.c
        src/rlist.c
        src/rtime.c
        src/rlog.c
        )

#LIST(APPEND uv_test_sources src/win/snprintf.c test/runner-win.c)${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM}

SET(SRC_BIN
    ${SRC_LIB}
    test/rtest_collections.c
    test/rtest_rdict.c
    test/rtest.c
    )

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_LIB})
#ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_LIB})

#SET(LIBRARY_OUTPUT_PATH ${RBASE_LIB_ROOT}) 

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR}")

IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    SET(LINK_TEST_LIBS ${LINK_LIBS} 3rd/cmockad)
ELSE()
    SET(LINK_TEST_LIBS ${LINK_LIBS} libcmockad.so)
ENDIF()

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR}")

ADD_EXECUTABLE(${PROJECT_NAME}_test ${SRC_BIN})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}_test ${LINK_TEST_LIBS})

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

