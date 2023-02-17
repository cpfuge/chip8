set(SOURCE_FILES
    "chip8.cpp"
    "cpu.cpp"
    "display.cpp"
    "main.cpp"
    )

set(CMAKE_CXX_STANDARD 17)

add_executable(chip8 WIN32 ${APPLICATION_TYPE} ${SOURCE_FILES})

target_link_libraries(chip8
    SDL2::SDL2
    SDL2::SDL2main
    )

target_include_directories(chip8
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

set_target_properties(chip8
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
