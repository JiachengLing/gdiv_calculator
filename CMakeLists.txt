cmake_minimum_required(VERSION 3.15)
project(gdiv_calculator VERSION 0.1.0 LANGUAGES CXX)

# ================================================================
# Options
# ================================================================
option(GDIV_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(GDIV_WINDOWS_EXPORT_ALL "Automatically export all symbols on Windows" OFF)
option(GDIV_STATIC_MSVC_RUNTIME "Use static MSVC runtime (/MT)" OFF)
option(BUILD_TESTING "Enable CTest" ON)
option(BUILD_GDIV_TESTS "Build gdiv_calculator test programs" ON)

# ================================================================
# C++ Standard
# ================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC runtime (CMake 3.15+ supports CMP0091)
if(MSVC)
    cmake_policy(SET CMP0091 NEW)
    if(GDIV_STATIC_MSVC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# ================================================================
# Dependencies
# ================================================================
find_package(GDAL REQUIRED)   # Provides target GDAL::GDAL

# ================================================================
# Source files
# ================================================================
set(GDIV_SOURCES
        src/gdiv_toolbox.cpp
        src/gdiv_utils.cpp
        src/msr.cpp
        src/shdi.cpp
        src/gdiv_lsi.cpp
        src/runner/gdal_io.cpp
        src/runner/tiler.cpp
        src/runner/runner.cpp
)

# ================================================================
# Build the shared library (DLL / SO)
# ================================================================
add_library(gdiv_toolbox SHARED ${GDIV_SOURCES})

# Define export macro (used in headers for __declspec(dllexport/dllimport))
target_compile_definitions(gdiv_toolbox PRIVATE GDIV_EXPORTS)

# Include directories
target_include_directories(gdiv_toolbox
        PUBLIC  ${PROJECT_SOURCE_DIR}/include
        PRIVATE ${PROJECT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(gdiv_toolbox PRIVATE GDAL::GDAL)

# Output directories for all build configurations
set_target_properties(gdiv_toolbox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY           ${PROJECT_SOURCE_DIR}/dist/$<CONFIG>
        LIBRARY_OUTPUT_DIRECTORY           ${PROJECT_SOURCE_DIR}/dist/$<CONFIG>
        ARCHIVE_OUTPUT_DIRECTORY           ${PROJECT_SOURCE_DIR}/dist/$<CONFIG>
        OUTPUT_NAME                        gdiv_toolbox
)

# Automatically export all symbols on Windows if enabled
if(WIN32 AND GDIV_WINDOWS_EXPORT_ALL)
    set_target_properties(gdiv_toolbox PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Enable compiler warnings (optional)
if(GDIV_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(gdiv_toolbox PRIVATE /W4)
    else()
        target_compile_options(gdiv_toolbox PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# ================================================================
# Installation and export configuration
# ================================================================
include(GNUInstallDirs)

# Install library and headers
install(TARGETS gdiv_toolbox
        EXPORT gdiv_calculatorTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export package configuration for find_package()
install(EXPORT gdiv_calculatorTargets
        NAMESPACE gdiv::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gdiv_calculator
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/gdiv_calculatorConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gdiv_calculatorConfig.cmake"
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/gdiv_calculatorTargets.cmake\")\n"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/gdiv_calculatorConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/gdiv_calculatorConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gdiv_calculator
)

# ================================================================
# Test build (optional)
# ================================================================
if(BUILD_TESTING)
    include(CTest)
endif()

if(BUILD_GDIV_TESTS)
    add_executable(test_basic tests/test_basic.cpp)

    # Include headers from project
    target_include_directories(test_basic PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Link against the shared library
    target_link_libraries(test_basic PRIVATE gdiv_toolbox)

    # Place test executable in the same dist/<config> folder as the DLL
    set_target_properties(test_basic PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist/$<CONFIG>
    )

    # Add warnings to test build (optional)
    if(GDIV_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(test_basic PRIVATE /W4)
        else()
            target_compile_options(test_basic PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()

    # Register test for ctest
    if(BUILD_TESTING)
        add_test(NAME test_basic_runs COMMAND test_basic)
    endif()
endif()


