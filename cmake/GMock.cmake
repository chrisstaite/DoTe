find_package(Threads REQUIRED)

include(ExternalProject)

ExternalProject_Add(
    gtest-project
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND "")
ExternalProject_Get_Property(gtest-project source_dir binary_dir)

add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest-project)
add_library(gtest INTERFACE)
target_include_directories(gtest
    INTERFACE "${source_dir}/googletest/include")
target_link_libraries(gtest INTERFACE libgtest ${CMAKE_THREAD_LIBS_INIT})

add_library(libgmock IMPORTED STATIC GLOBAL)
add_library(gmock INTERFACE)
add_dependencies(libgmock gtest)
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/$<CONFIG>/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a")

add_library(libgmock_main IMPORTED STATIC GLOBAL)
add_library(gmock_main INTERFACE)
target_link_libraries(gmock_main INTERFACE libgmock_main gmock)
target_include_directories(gmock
    INTERFACE "${source_dir}/googlemock/include")
target_link_libraries(gmock INTERFACE libgmock gtest)
add_dependencies(libgmock_main gmock)
set_target_properties(libgmock_main PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/$<CONFIG>/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a")

if (CMAKE_GENERATOR MATCHES "Xcode")
    set_target_properties(libgtest PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/gtest/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a")
    set_target_properties(libgmock PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a")
    set_target_properties(libgmock_main PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a")
else ()
    set_target_properties(libgtest PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a")
    set_target_properties(libgmock PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a")
    set_target_properties(libgmock_main PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a")
endif()
