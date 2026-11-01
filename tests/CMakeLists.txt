# fetch
if(NOT TARGET Catch2::Catch2)
    message("Including [Catch2] !")
    Include(FetchContent)

    FetchContent_Declare(Catch2
        GIT_REPOSITORY "https://github.com/catchorg/Catch2"
        GIT_TAG "v2.x"
    )

    FetchContent_MakeAvailable(Catch2)
    FetchContent_GetProperties(Catch2
        SOURCE_DIR CATCH_SOURCE_DIR
    )
endif()

# move and extract files against tests compute
file(ARCHIVE_EXTRACT 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/samples/iTunes Music Library (L).zip"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
configure_file(
    "samples/iTunes Music Library (S).xml" 
    "iTunes Music Library (S).xml" 
    COPYONLY
)

#
add_executable(${PROJECT_NAME}_tests
    tests.cpp
)

set_target_properties(${PROJECT_NAME}_tests PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# link
target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    itunes-library-parser
    Catch2::Catch2
)

target_compile_definitions(${PROJECT_NAME}_tests 
    PRIVATE CATCH_CONFIG_ENABLE_BENCHMARKING
)

# per-stage benchmark
add_executable(${PROJECT_NAME}_bench
    bench.cpp
)

set_target_properties(${PROJECT_NAME}_bench PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME}_bench PRIVATE
    itunes-library-parser
)

#tests handling
include(CTest)
list(APPEND CMAKE_MODULE_PATH ${CATCH_SOURCE_DIR}/contrib)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_tests)