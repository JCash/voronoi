cmake_minimum_required(VERSION 3.15)

project(jc_voronoi VERSION 0.10.0 LANGUAGES C)

include(GNUInstallDirs)

add_library(jc_voronoi INTERFACE)
add_library(jc_voronoi::jc_voronoi ALIAS jc_voronoi)

target_compile_features(jc_voronoi INTERFACE c_std_99)
target_include_directories(
    jc_voronoi
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(NOT WIN32)
    target_link_libraries(jc_voronoi INTERFACE m)
endif()

option(JC_VORONOI_BUILD_EXAMPLES "Build the example programs" OFF)
option(JC_VORONOI_BUILD_TESTS "Build and register the test programs" OFF)

if(JC_VORONOI_BUILD_EXAMPLES)
    add_executable(jc_voronoi_main src/main.c src/stb_wrapper.c)
    target_link_libraries(jc_voronoi_main PRIVATE jc_voronoi::jc_voronoi)

    add_executable(jc_voronoi_simple src/examples/simple.c)
    target_link_libraries(jc_voronoi_simple PRIVATE jc_voronoi::jc_voronoi)
endif()

if(JC_VORONOI_BUILD_TESTS)
    enable_language(CXX)
    enable_testing()

    function(jc_voronoi_add_test target_name)
        add_executable(${target_name} test/test.cpp)
        target_compile_features(${target_name} PRIVATE cxx_std_11)
        target_include_directories(${target_name} PRIVATE test)
        target_link_libraries(${target_name} PRIVATE jc_voronoi::jc_voronoi)
        add_test(NAME ${target_name} COMMAND ${target_name})
    endfunction()

    jc_voronoi_add_test(jc_voronoi_test)
    jc_voronoi_add_test(jc_voronoi_test_double)
    target_compile_definitions(
        jc_voronoi_test_double
        PRIVATE
            TEST_USE_DOUBLE
            JCV_USE_DOUBLE=1
            JCV_REAL_TYPE=double
            JCV_ATAN2=atan2
            JCV_SQRT=sqrt
            JCV_REAL_TYPE_EPSILON=DBL_EPSILON
    )
endif()

install(TARGETS jc_voronoi EXPORT jc_voronoiTargets)
install(
    FILES src/jc_voronoi.h src/jc_voronoi_clip.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    EXPORT jc_voronoiTargets
    FILE jc_voronoiTargets.cmake
    NAMESPACE jc_voronoi::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jc_voronoi
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/jc_voronoiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/jc_voronoiConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jc_voronoi
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/jc_voronoiConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/jc_voronoiConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/jc_voronoiConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jc_voronoi
)
