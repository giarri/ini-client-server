cmake_minimum_required(VERSION 4.2)
project(ini_server)

set(CMAKE_CXX_STANDARD 20)

add_library(ini_resource SHARED ini_resource.cpp)

target_include_directories(ini_resource PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
install(TARGETS ini_resource
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES ini_resource.cpp
        DESTINATION src/ini_server      # convention: /usr/local/src/ini_server
)
install(FILES ini_resource.h
        DESTINATION include)

add_executable(ini_resource_test test.cpp)
target_link_libraries(ini_resource_test PUBLIC ini_resource)
