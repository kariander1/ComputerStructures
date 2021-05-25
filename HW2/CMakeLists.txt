cmake_minimum_required(VERSION 3.0.0)
project(CacheSim VERSION 0.1.0)

include(CTest)
enable_testing()

list(APPEND flags "-std=c++11")

add_executable(CacheSim cacheSim.cpp)
add_executable(CacheSimRoy cacheSimRoy.cpp)
add_executable(cacheSimShelly cacheSimShelly.cpp)
target_compile_options(cacheSimShelly
PRIVATE
  ${flags}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
