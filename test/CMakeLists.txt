cmake_minimum_required(VERSION 3.7)
project(mutalk_tests)

project(tests)

set(CMAKE_C_STANDARD 99)


add_executable(${PROJECT_NAME} main.c )
target_link_libraries(${PROJECT_NAME} pthread mutalk)

add_test(NAME Test1 COMMAND ${PROJECT_NAME})

IF ((CMAKE_BUILD_TYPE MATCHES Debug) OR ( NOT CMAKE_BUILD_TYPE))
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
ENDIF ((CMAKE_BUILD_TYPE MATCHES Debug) OR ( NOT CMAKE_BUILD_TYPE))