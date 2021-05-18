cmake_minimum_required(VERSION 3.0.0)
project(dflow_calc VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(dflow_calc dflow_main.c dflow_calc.cpp)
add_executable(dflow_calc_debug dflow_main.c dflow_calc.cpp)
list(APPEND flags "-std=c99" "-O2" "-std=c++11" "-Wall" "-pedantic-errors")
list(APPEND debug_flags "-std=c99" "-g" "-O0" "-DDEBUG" "-std=c++11" "-Wall" "-pedantic-errors")
target_compile_options(dflow_calc_debug
  PRIVATE
    ${debug_flags}
  )
  target_compile_options(dflow_calc
  PRIVATE
    ${flags}
  )
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
