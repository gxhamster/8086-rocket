cmake_minimum_required(VERSION 3.30)
project(emu8086rocket)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/raylib)

add_executable(rocket src/main.cpp)

target_link_libraries(rocket PRIVATE raylib)


# Move resources into build directory
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/src/resources
    ${PROJECT_BINARY_DIR}/Debug/resources
    COMMENT "Copying textures into binary directory")   

add_dependencies(rocket copy_resources)

set(CMAKE_INSTALL_PREFIX "C:/emu8086")
install(TARGETS rocket DESTINATION DEVICES)
# Current working directory when running from emulator is C:/emu8086/MyBuild
# So move the sprites and textures, shaders into that folder
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/resources
        DESTINATION MyBuild)


