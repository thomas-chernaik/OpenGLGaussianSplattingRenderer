cmake_minimum_required(VERSION 3.26)
project(diss)

set(CMAKE_CXX_STANDARD 17)
# Find packages (OpenGL, GLEW, and GLFW)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GTest REQUIRED)

# Add the source files to the project
add_executable(diss main.cpp
        utils.h
        sort.h)

add_executable(tests tests/sortTests.cpp
        utils.h
        sort.h)

# Include directories for GLEW and GLFW
target_include_directories(diss PRIVATE ${GLEW_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})

target_include_directories(tests PRIVATE ${GTEST_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})

# Link libraries (OpenGL, GLEW, and GLFW)
target_link_libraries(diss PRIVATE ${OPENGL_LIBRARIES} GLEW::GLEW glfw)

target_link_libraries(tests PRIVATE ${GTEST_LIBRARIES} GLEW::GLEW glfw)
# Set C++ standard to C++11 (or higher if needed)
set_property(TARGET diss PROPERTY CXX_STANDARD 11)
