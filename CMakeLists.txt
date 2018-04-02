
cmake_minimum_required(VERSION 3.1)

project(DoTe CXX)
set(CMAKE_CXX_STANDARD 11)
enable_testing()

# Set up the sources common between the tests and binaries
set(CommonSources
    include/openssl/client.h
    src/openssl/client.cpp
    include/openssl/wrapped_bio.h
    src/openssl/wrapped_bio.cpp
    include/openssl/context.h
    src/openssl/context.cpp
    include/socket.h
    src/socket.cpp
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
target_include_directories(dote_static
    PRIVATE ${OPENSSL_INCLUDE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dote_static ${OPENSSL_LIBRARIES})

# Set up the binary application
add_executable(dote ${BinarySources})
target_link_libraries(dote dote_static)

# Import Google test and GoogleMock
find_package(Threads REQUIRED)
include(ExternalProject)
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND "")
ExternalProject_Get_Property(gtest source_dir binary_dir)
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/googletest/include"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/googlemock/include")
add_library(libgmock_main IMPORTED STATIC GLOBAL)
add_dependencies(libgmock_main gmock)
set_target_properties(libgmock_main PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock_main.a")

# Set up the test binary
add_executable(test_dote ${TestSources})
target_link_libraries(test_dote
    dote_static libgtest libgmock libgmock_main)
add_test(NAME test_dote COMMAND test_dote)
