CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(rcommon)

#SET(EXECUTABLE_OUTPUT_PATH ${RBASE_BINARY_ROOT}) 

INCLUDE_DIRECTORIES(include)

SET(SRC_LIB
        src/rbase.c
        src/rbuffer.c
        src/rfile.c
        src/rtime.c
        src/rstring.c
        src/rstring_ext.c
        src/rthread.c
        src/rarray.c
        src/rmemory.c
        src/rlog.c
        src/rdict.c
        src/rlist.c
        src/rtools.c
        )

SET(SRC_BIN
    ${SRC_LIB}
    test/rtest_rbuffer.c
    test/rtest_rpool.c
    test/rtest_collections.c
    test/rtest_rstring.c
    test/rtest_rthread.c
    test/rtest_rarray.c
    test/rtest_rdict.c
    test/rtest_rlog.c
    test/rtest_rfile.c
    test/rtest_rtools.c
    test/rtest_rtime.c
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

IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    ADD_EXECUTABLE(${PROJECT_NAME}_test ${SRC_BIN})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_test ${LINK_TEST_LIBS})
    #TARGET_COMPILE_OPTIONS(${PROJECT_NAME}_test PRIVATE -finput-charset=utf-8)
ELSE()
    ADD_EXECUTABLE(${PROJECT_NAME}_test${RBUILD_TYPE_POSTFIX} ${SRC_BIN})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_test${RBUILD_TYPE_POSTFIX} ${LINK_TEST_LIBS})
    #TARGET_COMPILE_OPTIONS(${PROJECT_NAME}_test${RBUILD_TYPE_POSTFIX} PRIVATE -finput-charset=utf-8)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

