cmake_minimum_required(VERSION 3.24)
project(Plataformero2D LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Plataformero2D)

# raylib ya incluida para evitar reinstalaciones y enlaces manuales.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib-5.5 EXCLUDE_FROM_ALL)

add_executable(Plataformero2D
    src/main.cpp
    src/Game.cpp
    src/Presentation.cpp
    src/Game.h
    src/Presentation.h
)
target_link_libraries(Plataformero2D PRIVATE raylib)
set_target_properties(Plataformero2D PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Plataformero2D>")

# El sprite debe estar junto al .exe para funcionar desde Visual Studio.
add_custom_command(TARGET Plataformero2D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Plataformero2D>/assets")
if(MSVC)
    target_compile_options(Plataformero2D PRIVATE /W4 /utf-8)
endif()
