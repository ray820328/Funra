CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(ripc)

#SET(EXECUTABLE_OUTPUT_PATH ${RBASE_BINARY_ROOT}) 

INCLUDE_DIRECTORIES(
    include
    ../../3rd/libuv/1.42.0/include
)

SET(SRC_LIB
    src/ripc.c
    src/rsocket_select.c
    src/rsocket_s.c
    src/rsocket_c.c
    src/rsocket_uv_s.c
    src/rsocket_uv_c.c
    src/rcodec_default.c
)

SET(SRC_BIN
    ${SRC_LIB}
    test/rtest_rcodec_default.c
    test/rtest.c
    test/rtest_rsocket_uv_s.c
    test/rtest_rsocket_uv_c.c
    test/rtest_rsocket_s.c
    test/rtest_rsocket_c.c
)

#LIST(APPEND uv_test_sources src/win/snprintf.c test/runner-win.c)${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib/${DIR_PLATFORM}

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_LIB})
#ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_LIB})

#SET(LIBRARY_OUTPUT_PATH ${RBASE_LIB_ROOT}) 

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(STATUS "Link dirs (${PROJECT_NAME}): ${RSERVER_LIB_ROOT}")
MESSAGE(STATUS "Link libs (${PROJECT_NAME}): ${LINK_LIBS}")
MESSAGE(STATUS "Link opts (${PROJECT_NAME}): ${PROJECT_LINK_OPTIONS}")

IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    SET(LINK_TEST_LIBS ${LINK_LIBS} 3rd/cmockad)
ELSE()
    SET(LINK_TEST_LIBS ${LINK_LIBS} libcmockad.so)
ENDIF()

MESSAGE(STATUS "Platform: ${CMAKE_HOST_SYSTEM_NAME} - ${CMAKE_CURRENT_SOURCE_DIR}")

IF(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    ADD_EXECUTABLE(${PROJECT_NAME}_test ${SRC_BIN})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_test ${LINK_TEST_LIBS})
ELSE()
    ADD_EXECUTABLE(${PROJECT_NAME}_test${RBUILD_TYPE_POSTFIX} ${SRC_BIN})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}_test${RBUILD_TYPE_POSTFIX} ${LINK_TEST_LIBS})
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

