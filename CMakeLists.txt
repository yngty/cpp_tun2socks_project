cmake_minimum_required(VERSION 3.10)
project(cpp_tun2socks_project)

set(CMAKE_CXX_STANDARD 11)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)
add_definitions(-DBOOST_BEAST_SEPARATE_COMPILATION)

add_subdirectory(third-part/boost EXCLUDE_FROM_ALL)

# 源文件
file(GLOB CORE_SRC
    core/*.cpp
)

add_executable(tun2socks
    ${CORE_SRC}
    main.cpp
)

message(${CMAKE_SOURCE_DIR})
add_custom_target(
    lwip_build ALL
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/third-part/lwip static
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/third-part/lwip
    COMMENT "Building lwIP static library"
)
add_dependencies(tun2socks lwip_build)

target_link_libraries(tun2socks
    ${CMAKE_SOURCE_DIR}/third-part/lwip/bin/liblwip.a
    Boost::asio
)

target_include_directories(tun2socks PRIVATE
    core
    third-part/lwip/src/include
    third-part/lwip/src/ports/include
)
