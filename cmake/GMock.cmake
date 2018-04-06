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
if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    add_library(gtest INTERFACE)
    target_include_directories(gtest
        INTERFACE "${source_dir}/googletest/include")
else()
    set(GTEST_SOURCES)
    add_library(gtest $(GTEST_SOURCES))
    set_target_properties(gtest PROPERTIES LINKER_LANGUAGE CXX)
    include_directories("${source_dir}/googletest/include")
endif()
target_link_libraries(gtest INTERFACE libgtest ${CMAKE_THREAD_LIBS_INIT})

add_library(libgmock IMPORTED STATIC GLOBAL)
if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    add_library(gmock INTERFACE)
    target_include_directories(gmock
        INTERFACE "${source_dir}/googletest/include")
else()
    set(GMOCK_SOURCES)
    add_library(gmock $(GMOCK_SOURCES))
    set_target_properties(gmock PROPERTIES LINKER_LANGUAGE CXX)
    include_directories("${source_dir}/googlemock/include")
endif()
target_link_libraries(gmock INTERFACE libgmock gtest)

add_library(libgmock_main IMPORTED STATIC GLOBAL)
if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    add_library(gmock_main INTERFACE)
else()
    set(GMOCK_MAIN_SOURCES)
    add_library(gmock_main $(GMOCK_MAIN_SOURCES))
    set_target_properties(gmock_main PROPERTIES LINKER_LANGUAGE CXX)
endif()
target_link_libraries(gmock_main INTERFACE libgmock_main gmock)

if (CMAKE_GENERATOR MATCHES "Xcode")
    set_target_properties(libgtest PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/gtest/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a"
        IMPORTED_LOCATION_DEBUG "${binary_dir}/googlemock/gtest/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gtestd.a")
    set_target_properties(libgmock PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a"
        IMPORTED_LOCATION_DEBUG "${binary_dir}/googlemock/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gmockd.a")
    set_target_properties(libgmock_main PROPERTIES
        IMPORTED_LOCATION_RELEASE "${binary_dir}/googlemock/Release/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a"
        IMPORTED_LOCATION_DEBUG "${binary_dir}/googlemock/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_maind.a")
else ()
    set_target_properties(libgtest PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a")
    set_target_properties(libgmock PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a")
    set_target_properties(libgmock_main PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a")
endif()
