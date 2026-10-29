cmake_minimum_required(VERSION 3.28.0)
PROJECT(2026_C_AV_FC_PRC_INTRANET)
SET(CMAKE_C_FLAGS "-g -Wall -Werror")
SET(CMAKE_CXX_FLAGS "-g -Wall -Werror -std=c++17")

#############################################################
################## PRC INTRANET CODEBASE ####################
#############################################################

# Add your source files here
SET(PRC_INTRANET_SOURCE_FILES
    src/prc_intranet/context.cpp
    src/prc_intranet/dispatch.cpp
    src/prc_intranet/transmit.cpp)

add_library(prc_intranet STATIC ${PRC_INTRANET_SOURCE_FILES})
target_include_directories(prc_intranet PUBLIC include)

#############################################################
######################## GOOGLE TEST ########################
#############################################################

# Host check only when this repo is the top level build, so the FC and PRC
# trees that vendor it never fetch googletest through it.
if (PROJECT_IS_TOP_LEVEL)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    add_executable(test_message_list tests/test_message_list.cpp)
    target_link_libraries(test_message_list PRIVATE prc_intranet GTest::gtest_main)
    gtest_discover_tests(test_message_list)
endif()
