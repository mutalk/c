cmake_minimum_required(VERSION 3.7)
project(mutalk C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall -Wextra -pedantic")


add_library(${PROJECT_NAME} SHARED mutalk.c)
add_library(${PROJECT_NAME}Static STATIC mutalk.c)
set_target_properties(${PROJECT_NAME}Static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

IF ((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    target_compile_definitions(${PROJECT_NAME}Static PRIVATE DEBUG=1)
ENDIF ((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}Static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)