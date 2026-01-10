cmake_minimum_required(VERSION 3.15)
project(twii_miner)

find_package(fmt CONFIG REQUIRED)

add_executable(twii_miner)

set_target_properties(twii_miner PROPERTIES
    CXX_STANDARD 20
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<OR:$<STREQUAL:${VCPKG_TARGET_TRIPLET},x64-windows>>:DLL>"
)

target_sources(twii_miner PRIVATE
    "src/main.cpp"
    "src/xml_loader.cpp"
    "src/arg_parser.cpp"
    "src/skill_loader.cpp"
    "src/skill_input.cpp"
    "src/skill_output.cpp"
)
target_include_directories(twii_miner PRIVATE
    "src"
)

target_compile_definitions(twii_miner PUBLIC
    "WIN32_LEAN_AND_MEAN"
    "UNICODE" "_UNICODE"
    "NOMINMAX"
    "TOML_EXCEPTIONS=0"
    "TOML_HEADER_ONLY=0"
)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    target_compile_definitions(twii_miner PUBLIC
        "_ITERATOR_DEBUG_LEVEL=2"
    )
endif()

target_include_directories(twii_miner PUBLIC
   "../tomlplusplus/include")

# vcpkg used for lib fmt
target_link_libraries(twii_miner PUBLIC
    "fmt::fmt" "fmt::fmt-header-only"
)
