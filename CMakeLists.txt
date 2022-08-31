cmake_minimum_required(VERSION 3.0)
project(cic_test)

set(CMAKE_C_FLAGS "-O3 -Wall")

add_executable(cic src/main.c)
add_executable(cic2 src/main2.c src/cic.c)

if(CMAKE_COMPILER_IS_GNUCC)
  target_link_libraries(cic m)
  target_link_libraries(cic2 m)
endif()