cmake_minimum_required(VERSION 3.0.0)
project(monksvg VERSION 0.1.0)

option(DO_BUILD_EXAMPLES "Build examples" ON)
option(DO_MONKVG_BACKEND "Use MonkVG as the backend rendering" ON)

if(DO_MONKVG_BACKEND)
    # add the source code
    set(BACKEND_SOURCE ${BACKEND_SOURCE} 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/openvg/mkOpenVG_SVG.cpp)

    # build MonkVG
    set(MKVG_DO_EXAMPLES OFF CACHE BOOL "Turn off MonkVG examples.")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/MonkVG)

    # set include dirs
    set(MONKVG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/MonkVG/include)

endif()

# TinyXML
add_compile_definitions(TIXML_USE_STL) # use STL
set(TINYXML_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tinyxml/tinystr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tinyxml/tinyxml.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tinyxml/tinyxmlerror.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tinyxml/tinyxmlparser.cpp)

add_library(monksvg 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mkSVG.cpp
    ${BACKEND_SOURCE}
    ${TINYXML_SOURCE}
    )
add_dependencies(monksvg monkvg)

# debug killme
message(STATUS "MonkVG Include Dirs: " ${MONKVG_INCLUDE_DIRS})
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

target_include_directories(monksvg 
    PRIVATE
    ${MONKVG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/openvg
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tinyxml 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
