cmake_minimum_required(VERSION 3.0)
project(pbench VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(PEGTL EXCLUDE_FROM_ALL)
add_executable(pbench_evaluate pbench_evaluate.cpp)
target_link_libraries(pbench_evaluate PRIVATE taocpp::pegtl)
