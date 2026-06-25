cmake_minimum_required(VERSION 3.16)

project(mo_ecat_pc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MO_ECAT_PC_BUILD_APP "Build Qt upper computer application" ON)

add_subdirectory(mo_ecat_core)

if(MO_ECAT_PC_BUILD_APP)
    add_subdirectory(app)
endif()

