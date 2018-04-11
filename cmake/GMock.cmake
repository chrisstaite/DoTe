find_package(Threads REQUIRED)

include(ExternalProject)

ExternalProject_Add(
    gtest-project
    URL https://github.com/google/googletest/archive/release-1.8.0.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND "")
ExternalProject_Get_Property(gtest-project source_dir binary_dir)

# Make the directories that will be made when it's downloaded to stop CMake complaining
file(MAKE_DIRECTORY "${source_dir}/googlemock/include")
file(MAKE_DIRECTORY "${source_dir}/googletest/include")

add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest-project)
set_target_properties(libgtest PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/googletest/include"
    INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")

add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock libgtest)
set_target_properties(libgmock PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/googlemock/include"
    INTERFACE_LINK_LIBRARIES "libgtest")

add_library(libgmock_main IMPORTED STATIC GLOBAL)
add_dependencies(libgmock_main libgmock)
set_target_properties(libgmock_main PROPERTIES
    INTERFACE_LINK_LIBRARIES "libgmock")

if (CMAKE_VERSION VERSION_LESS "2.8.12")
    # Old CMake is stupid
    include_directories("${source_dir}/googletest/include")
    include_directories("${source_dir}/googlemock/include")
endif ()

if (CMAKE_GENERATOR MATCHES "Xcode")
    set_target_properties(libgtest PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/gtest/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a"
        IMPORTED_LOCATION_DEBUG "${binary_dir}/googlemock/gtest/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a")
    set_target_properties(libgmock PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a"
        IMPORTED_LOCATION_DEBUG "${binary_dir}/googlemock/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a")
    set_target_properties(libgmock_main PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a"
        IMPORTED_LOCATION_DEBUG "${binary_dir}/googlemock/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a")
else ()
    set_target_properties(libgtest PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a")
    set_target_properties(libgmock PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a")
    set_target_properties(libgmock_main PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a")
endif()
