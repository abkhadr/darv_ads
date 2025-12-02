# CMakeLists.txt (الجذر)
cmake_minimum_required(VERSION 3.10)
project(DARV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# إضافة مجلد الـ headers
include_directories(${CMAKE_SOURCE_DIR})

# البرنامج الرئيسي
add_executable(darv main.cpp)

# ربط مكتبات الـ threads (مطلوبة لـ std::this_thread)
find_package(Threads REQUIRED)
target_link_libraries(darv Threads::Threads)

# ربط مكتبة filesystem
target_link_libraries(darv stdc++fs)