# projects/example_project/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(example)

# بدون optimization في البداية (سيضيفه DARV)
set(CMAKE_CXX_STANDARD 11)

add_executable(example main.cpp)