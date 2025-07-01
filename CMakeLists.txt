cmake_minimum_required(VERSION 3.20)

project("PHYParser" CXX)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_CXX_STANDARD 20)

add_library(PHYParser
        PHYParser.hpp
        src/phy.hpp
        src/phy.cpp
        src/structs/common.hpp
        src/structs/phy.hpp
        src/helpers/check-bounds.hpp
        src/helpers/offset-data-view.hpp
        src/helpers/offset-data-view.cpp
        src/enums.hpp
        src/errors.hpp
        src/structs/mopp.hpp
        src/helpers/text-section-parser.hpp
        src/helpers/text-section-parser.cpp
        src/helpers/surface-parser.hpp
        src/helpers/surface-parser.cpp
)

target_include_directories(
        PHYParser PRIVATE
        "src"
)
