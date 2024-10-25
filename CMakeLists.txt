cmake_minimum_required(VERSION 3.30.4)
project(emu8086rocket)


# Add the library directory to the include path
# include_directories(lib/include)
add_executable(rocket main.cpp)

# Link the library to the executable
# target_link_libraries(my_executable my_library)