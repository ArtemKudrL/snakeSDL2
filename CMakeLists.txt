cmake_minimum_required(VERSION 3.10)

Project(snake)

find_package(SDL2 REQUIRED)

add_executable(snake src/main.cxx)
target_include_directories(snake PRIVATE include/SDL2)
target_compile_options(snake PRIVATE -O2)
target_link_libraries(snake PRIVATE SDL2 SDL2_mixer SDL2_ttf SDL2_image)
