cmake_minimum_required(VERSION 3.15)
project(Mimir VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(yaml-cpp INTERFACE)
target_include_directories(yaml-cpp INTERFACE ${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(MIMIR_LIB_SOURCES
    src/parser.cpp
    src/dag.cpp
    src/signature.cpp
    src/executor.cpp
    src/cache.cpp
    src/target.cpp
    src/command_runner.cpp
)

add_library(libmimir STATIC ${MIMIR_LIB_SOURCES})
target_include_directories(libmimir PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(libmimir PUBLIC pthread)

add_executable(mimir src/main.cpp)
target_link_libraries(mimir PRIVATE libmimir)

enable_testing()
add_subdirectory(tests)
