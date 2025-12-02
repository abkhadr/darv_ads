cmake_minimum_required(VERSION 3.10)
project(DARV_ADS)

# C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Thread library
find_package(Threads REQUIRED)

# ==================== Original DARV ====================
add_executable(darv main.cpp)
target_link_libraries(darv ${CMAKE_THREAD_LIBS_INIT})

# ==================== Autograd Tests ====================
add_executable(test_autograd test_autograd.cpp)
target_link_libraries(test_autograd ${CMAKE_THREAD_LIBS_INIT})

# ==================== Complete NN Tests ====================
add_executable(test_complete_nn test_complete_nn.cpp)
target_link_libraries(test_complete_nn ${CMAKE_THREAD_LIBS_INIT})

# ==================== Debug Gradients ====================
add_executable(test_debug_gradients test_debug_gradients.cpp)
target_link_libraries(test_debug_gradients ${CMAKE_THREAD_LIBS_INIT})

# ==================== Dual-Path System ====================
add_executable(test_dual_path test_dual_path.cpp)
target_link_libraries(test_dual_path ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_dual_simple test_dual_simple.cpp)
target_link_libraries(test_dual_simple ${CMAKE_THREAD_LIBS_INIT})

# ==================== Installation ====================
install(TARGETS darv test_autograd test_complete_nn test_dual_path
        RUNTIME DESTINATION bin)

# ==================== Documentation ====================
message(STATUS "DARV Dual-Path System Configuration:")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - Source Dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "")
message(STATUS "Available Targets:")
message(STATUS "  - darv: Original DARV cycle")
message(STATUS "  - test_autograd: Autograd engine tests")
message(STATUS "  - test_complete_nn: Complete neural network tests")
message(STATUS "  - test_debug_gradients: Gradient debugging tool")
message(STATUS "  - test_dual_path: Dual-Path system integration test")
message(STATUS "")
message(STATUS "Build with: make <target>")
message(STATUS "Run with: ./build/<target>")