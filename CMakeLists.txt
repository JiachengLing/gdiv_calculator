cmake_minimum_required(VERSION 3.15)
project(gdiv_calculator LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Find GDAL (adjust if your GDAL is a custom install)
# ------------------------------------------------------------------
find_package(GDAL REQUIRED)          # provides target GDAL::GDAL

# ------------------------------------------------------------------
# Library (DLL) sources — list explicitly (clearer than GLOB)
# ------------------------------------------------------------------
set(GDIV_SOURCES
        src/gdiv_toolbox.cpp
        src/gdiv_utils.cpp
        src/msr.cpp
        src/shdi.cpp
        src/gdiv_lsi.cpp
)

# ------------------------------------------------------------------
# Build the DLL
# ------------------------------------------------------------------
add_library(gdiv_toolbox SHARED ${GDIV_SOURCES})

# Export macro: when building the DLL we export symbols
target_compile_definitions(gdiv_toolbox PRIVATE GDIV_EXPORTS)

# Public headers (consumers include from this dir)
target_include_directories(gdiv_toolbox
        PUBLIC  ${PROJECT_SOURCE_DIR}/include
        PRIVATE ${PROJECT_SOURCE_DIR}/src
)

# Link GDAL
target_link_libraries(gdiv_toolbox PRIVATE GDAL::GDAL)

# Put DLL and import lib into dist/
set_target_properties(gdiv_toolbox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist    # .dll/.exe on Windows
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist    # .so/.dylib
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist    # .lib/.a
)

# ------------------------------------------------------------------
# Test executable (links to the DLL)
# ------------------------------------------------------------------
add_executable(test_basic tests/test_basic.cpp)

# The test needs to see the public header
target_include_directories(test_basic PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link against our DLL target (brings GDAL transitively on most setups)
target_link_libraries(test_basic PRIVATE gdiv_toolbox)

# Put the test exe next to the DLL so it can find it at runtime
set_target_properties(test_basic PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist
)

