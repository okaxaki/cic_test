cmake_minimum_required(VERSION 3.0)
project(cic_test)

set(CMAKE_C_FLAGS "-O3 -Wall")

add_executable(cic src/main.c)

if(NOT CMAKE_COMPILER_IS_GNUCC)
	target_link_libraries(cic)
else()
	target_link_libraries(cic m)
endif()