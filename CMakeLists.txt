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
