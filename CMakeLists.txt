
cmake_minimum_required(VERSION 2.8)

project(DoTe CXX)

# Allow C code to be compiled for CMake tests
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
    include/openssl/i_ssl_factory.h
    include/openssl/ssl_factory.h
    src/openssl/ssl_factory.cpp
    include/openssl/context.h
    src/openssl/context.cpp
    include/openssl/i_ssl_connection.h
    include/openssl/ssl_connection.h
    src/openssl/ssl_connection.cpp
    include/openssl/hostname_verifier.h
    src/openssl/hostname_verifier.cpp
    include/openssl/base64.h
    src/openssl/base64.cpp
    include/socket.h
    src/socket.cpp
    include/forwarder_connection.h
    src/forwarder_connection.cpp
    include/i_forwarder_config.h
    include/forwarder_config.h
    src/forwarder_config.cpp
    include/i_forwarders.h
    include/client_forwarders.h
    src/client_forwarders.cpp
    include/i_loop.h
    include/loop.h
    src/loop.cpp
    include/server.h
    src/server.cpp
    include/config_parser.h
    src/config_parser.cpp
    include/pid_file.h
    src/pid_file.cpp
    include/log.h
    src/log.cpp
    include/noop_log.h
    include/stream_log.h
    src/stream_log.cpp
    include/i_logger.h
    include/syslog_logger.h
    src/syslog_logger.cpp
    include/dote.h
    src/dote.cpp)

# Set up the sources for the binary
set(BinarySources
    src/main.cpp)

# Set up the sources for the tests
set(TestSources
    test/mock_logger.h
    test/mock_loop.h
    test/mock_forwarders.h
    test/mock_forwarder_config.h
    test/openssl/mock_ssl_connection.h
    test/openssl/mock_ssl_factory.h
    test/openssl/test_context.cpp
    test/openssl/test_ssl_connection.cpp
    test/openssl/test_hostname_verifier.cpp
    test/openssl/test_base64.cpp
    test/openssl/test_ssl_factory.cpp
    test/test_socket.cpp
    test/test_forwarder_connection.cpp
    test/test_forwarder_config.cpp
    test/test_client_forwarders.cpp
    test/test_loop.cpp
    test/test_server.cpp
    test/parse_inet.h
    test/parse_inet.cpp
    test/test_config_parser.cpp
    test/test_pid_file.cpp
    test/test_log.cpp)

# Remove RTTI because we don't need it and it bloats the binary
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# Set up the library of the code that can be tested and compiled
# into the real binary
find_package(OpenSSL REQUIRED)
add_library(dote_static ${CommonSources})
if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    target_include_directories(dote_static
        PUBLIC ${OPENSSL_INCLUDE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    include_directories(${OPENSSL_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
target_link_libraries(dote_static ${OPENSSL_LIBRARIES})

# Strip release
set(CMAKE_EXE_LINKER_FLAGS_RELEASE
    "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

# Set up the binary application
add_executable(dote ${BinarySources})
target_link_libraries(dote dote_static)

# Import Google test and GoogleMock
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GMock.cmake)

# Set up the test binary
add_executable(test_dote ${TestSources})
target_link_libraries(test_dote
    dote_static
    libgmock_main libgmock libgtest ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME test_dote COMMAND test_dote)
