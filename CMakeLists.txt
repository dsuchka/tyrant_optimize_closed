cmake_minimum_required(VERSION 2.6)
# This file is part of MXE. See LICENSE.md for licensing information.

# partial module - included by src/cmake/CMakeLists.txt

#set(TGT test-${PKG}-cmake)
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Ofast")
set( BUILD_SHARED_LIBRARIES OFF)
set( BOOST_USE_STATIC_LIBS ON )
set( CMAKE_EXE_LINKER_FLAGS "-static")
add_definitions(-DTYRANT_OPTIMIZER_VERSION="${VERSION}" ${DEBUG} -DNQUEST)

project(tuo)
file(GLOB TUO_SOURCES "*.cpp")

enable_language(CXX)
add_executable(tuo ${TUO_SOURCES})

find_package(Boost EXACT COMPONENTS system thread filesystem regex REQUIRED)

target_link_libraries(tuo ${Boost_LIBRARIES})

#install(TARGETS ${TGT} DESTINATION bin)

