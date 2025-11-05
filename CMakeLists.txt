cmake_minimum_required(VERSION 3.20)
project(MyProject LANGUAGES CXX)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(Project_manager (Project_manager/main.cpp))

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(Project_manager PRIVATE nlohmann_json::nlohmann_json)

