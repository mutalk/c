cmake_minimum_required(VERSION 3.7)
project(mutalk_with_test)

enable_testing()

include_directories(include)

add_subdirectory( src build/src )
add_subdirectory( test build/test )