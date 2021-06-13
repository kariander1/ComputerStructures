cmake_minimum_required(VERSION 3.0.0)
project(MultiCore VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(MultiCore main.c sim_api.c core_api.cpp)
add_executable(MultiCore_debug main.c sim_api.c core_api.cpp)
list(APPEND flags "-std=c++11" "-O0" "-Wall" "-pedantic-errors")
list(APPEND debug_flags "-std=c++11" "-O0" "-Wall" "-pedantic-errors" "-DDEBUG")
target_compile_options(MultiCore_debug
  PRIVATE
    ${debug_flags}
  )
  target_compile_options(MultiCore
  PRIVATE
    ${flags}
  )


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


