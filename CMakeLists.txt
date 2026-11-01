# minimal required
cmake_minimum_required(VERSION 3.19.3 FATAL_ERROR)

# project definition
project(ITunesLibraryParser
    VERSION 0.2.0
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH 
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

################################
## CPP Compiler Configuration ##
################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

##############################
## Check AVX2 / Neon compatibility ##
##############################

include(FindAVX2)
include(FindARM)

if(NOT AVX2_FOUND AND NOT NEON_FOUND) 
    message(FATAL_ERROR "${PROJECT_NAME} requires AVX2 (for x64 chips) or Neon (for ARM / Apple Silicon) instruction set to be handled by compiler and CPU !")
endif()

#############
## Sources ##
#############

# library interface
add_library(itunes-library-parser INTERFACE)
target_include_directories(itunes-library-parser INTERFACE src)

# tracks are scanned on multiple threads
find_package(Threads REQUIRED)
target_link_libraries(itunes-library-parser INTERFACE Threads::Threads)

# header-only, SIMD instructions must be enabled on consumers
if(AVX2_FOUND)
    target_compile_options(itunes-library-parser
        INTERFACE ${AVX2_FLAGS}
    )
elseif(NEON_FOUND)
    target_link_options(itunes-library-parser INTERFACE -mfpu=neon -mfloat-abi=hard)
endif()

# debug flag
target_compile_definitions(itunes-library-parser 
    INTERFACE $<$<CONFIG:Debug>:_DEBUG>
)

# may produce tests ?
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()