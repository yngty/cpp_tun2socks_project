cmake_minimum_required(VERSION 3.10)
project(cpp_tun2socks_project)

set(CMAKE_CXX_STANDARD 11)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)
add_definitions(-DBOOST_BEAST_SEPARATE_COMPILATION)

# add_subdirectory(third-part/boost EXCLUDE_FROM_ALL)

add_subdirectory(third-part/lwip EXCLUDE_FROM_ALL)

set (LWIP_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/third-part/lwip/src/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-part/lwip/contrib/"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-part/lwip/contrib/ports/unix/port/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-part/lwip/contrib/examples/example_app"
)
# 源文件
file(GLOB CORE_SRC
    core/*.cpp
)

add_executable(tun2socks
    ${CORE_SRC}
    main.cpp
)

message(STATUS "LWIP_INCLUDE_DIRS: ${LWIP_INCLUDE_DIRS}")
target_link_libraries(tun2socks
     lwipcontribaddons
    #  lwipallapps
     lwipcontribportunix lwipcore
    #  lwipmbedtls

)
# // 打印lwipcore
message(STATUS "LWIP_CORE_LIBS: ${LWIP_CORE_LIBS}")

target_include_directories(tun2socks PRIVATE
    core
    ${LWIP_INCLUDE_DIRS}
)


# add_executable(example_app ${LWIP_DIR}/contrib/examples/example_app/test.c default_netif.c)
# target_include_directories(example_app PRIVATE ${LWIP_INCLUDE_DIRS})
# target_compile_options(example_app PRIVATE ${LWIP_COMPILER_FLAGS})
# target_compile_definitions(example_app PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
# # target_link_libraries(example_app ${LWIP_SANITIZER_LIBS} lwipcontribexamples lwipcontribapps lwipcontribaddons lwipallapps lwipcontribportunix lwipcore lwipmbedtls)

# add_executable(makefsdata ${lwipmakefsdata_SRCS})
# target_compile_options(makefsdata PRIVATE ${LWIP_COMPILER_FLAGS})
# target_include_directories(makefsdata PRIVATE ${LWIP_INCLUDE_DIRS})
# target_link_libraries(makefsdata ${LWIP_SANITIZER_LIBS})
