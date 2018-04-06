
cmake_minimum_required(VERSION 2.8)

project(DoTe CXX)

# Alloc C code to be compiled for CMake tests
enable_language(C)

# Enable C++11 support
if(CMAKE_VERSION VERSION_LESS "3.1")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

enable_testing()

# Set up the sources common between the tests and binaries
set(CommonSources
    include/openssl/context.h
    src/openssl/context.cpp
    include/openssl/ssl_connection.h
    src/openssl/ssl_connection.cpp
    include/openssl/hostname_verifier.h
    src/openssl/hostname_verifier.cpp
    include/socket.h
    src/socket.cpp
    include/forwarder_connection.h
    src/forwarder_connection.cpp
    include/forwarder_config.h
    src/forwarder_config.cpp
    include/i_forwarders.h
    include/client_forwarders.h
    src/client_forwarders.cpp
    include/loop.h
    src/loop.cpp
    include/dote.h
    src/dote.cpp)

# Set up the sources for the binary
set(BinarySources
    src/main.cpp)

# Set up the sources for the tests
set(TestSources
    test/openssl/test_client.cpp
    test/openssl/test_wrapped_bio.cpp
    test/openssl/test_context.cpp)

# Set up the library of the code that can be tested and compiled
# into the real binary
find_package(OpenSSL REQUIRED)
add_library(dote_static ${CommonSources})
if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    target_include_directories(dote_static
        PRIVATE ${OPENSSL_INCLUDE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    include_directories(${OPENSSL_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
target_link_libraries(dote_static ${OPENSSL_LIBRARIES})

# Set up the binary application
add_executable(dote ${BinarySources})
target_link_libraries(dote dote_static)

# Import Google test and GoogleMock
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GMock.cmake)

# Set up the test binary
add_executable(test_dote ${TestSources})
target_link_libraries(test_dote dote_static libgmock_main)
add_test(NAME test_dote COMMAND test_dote)
