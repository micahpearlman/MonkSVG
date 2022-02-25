cmake_minimum_required(VERSION 3.0.0)
project(monksvg VERSION 0.1.0)

option(MKSVG_DO_BUILD_EXAMPLES "Build examples" ON)
option(MKSVG_DO_MONKVG_BACKEND "Use MonkVG as the backend rendering" ON)

if(MKSVG_DO_MONKVG_BACKEND)
    # add the source code
    # set(BACKEND_SOURCE ${BACKEND_SOURCE} 
    #     ${CMAKE_CURRENT_SOURCE_DIR}/src/openvg/mkOpenVG_SVG.cpp)

    set(MKSVG_BACKEND_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/openvg/mkOpenVG_SVG.cpp)
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
    ${TINYXML_SOURCE}
    ${MKSVG_BACKEND_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mkSVG.cpp        
    )
add_dependencies(monksvg monkvg)

set(MONKSVG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(monksvg 
    PUBLIC
    ${MONKSVG_INCLUDE_DIRS}
    PRIVATE
    ${MONKVG_INCLUDE_DIRS}
    )

## Build Examples    
if (MKSVG_DO_BUILD_EXAMPLES)
    
    # external cmake project build support
    include(ExternalProject)
    include(build_external_proj.cmake)

    # OpenGL
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)

    # GLFW Library
    set(GLFW_OPTIONS "")
    list(APPEND GLFW_OPTIONS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw)
    list(APPEND GLFW_OPTIONS -DCMAKE_BUILD_TYPE=Release)
    build_external_git_project(glfw https://github.com/glfw/glfw.git ${GLFW_OPTIONS})

    # now add the build glfw3 OpenGL windowing package
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw/lib/cmake")
    find_package(glfw3 REQUIRED)
    set(GLFW_LIBRARIES
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})

    
    # GLEW library
    set(GLEW_OPTIONS "")
    list(APPEND GLEW_OPTIONS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew)
    list(APPEND GLEW_OPTIONS -DCMAKE_BUILD_TYPE=Release)
    build_external_git_project(glew https://github.com/Perlmint/glew-cmake.git ${GLEW_OPTIONS})

    # set(glew_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew/lib/cmake/glew)
    find_package(glew REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew/lib/cmake/glew)
    
    set(GLEW_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew/include)
    set(GLEW_LIBRARIES
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glew/lib/${CMAKE_STATIC_LIBRARY_PREFIX}GLEW${CMAKE_STATIC_LIBRARY_SUFFIX})

    # GLM Math Library
    set(GLM_OPTIONS "")
    list(APPEND GLM_OPTIONS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm)
    list(APPEND GLM_OPTIONS -DCMAKE_BUILD_TYPE=Release)
    list(APPEND GLM_OPTIONS -DBUILD_TESTING=0)
    build_external_git_project(glm https://github.com/g-truc/glm.git ${GLM_OPTIONS})

    # now add the build glm package
    set(glm_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm/lib/cmake/glm)
    find_package(glm REQUIRED)
    set(GLM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm/include)

    # GLFW Hello World
    add_executable(glfw_hello_world examples/glfw_hello_world.cpp)
    add_dependencies(glfw_hello_world monkvg monksvg)
    target_include_directories(glfw_hello_world 
                                PUBLIC
                                ${MONKSVG_INCLUDE_DIRS}
                                PRIVATE 
                                ${GLEW_INCLUDE_DIRS}
                                ${GLM_INCLUDE_DIRS}
                                ${MONKVG_INCLUDE_DIRS}                                
                                )
    target_link_libraries(glfw_hello_world  PUBLIC
                            monksvg
                            monkvg
                            ${GLU_LIBRARIES} # Required by MonkVG
                            ${GLFW_LIBRARIES}   
                            ${GLEW_LIBRARIES}                     
                            OpenGL::GL
                            OpenGL::GLX
                            ${CMAKE_DL_LIBS}
                            pthread
                            )
    

endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
