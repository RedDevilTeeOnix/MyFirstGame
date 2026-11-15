cmake_minimum_required(VERSION 3.22)
project(MyFirstGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

add_executable(MyFirstGame main.cpp)
target_link_libraries(MyFirstGame PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)