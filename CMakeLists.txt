cmake_minimum_required(VERSION 3.16)
project(jester-gb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    src/main.cpp
    src/cpu/cpu.cpp
    src/bus/bus.cpp
    src/cartridge/cartridge.cpp
    src/ppu/ppu.cpp
    src/apu/apu.cpp
    src/tui/terminal.cpp
    src/tui/renderer.cpp
    src/tui/menu.cpp
    src/input/input.cpp
)

# Header files
set(HEADERS
    src/cpu/cpu.hpp
    src/cpu/opcodes.hpp
    src/bus/bus.hpp
    src/cartridge/cartridge.hpp
    src/ppu/ppu.hpp
    src/apu/apu.hpp
    src/tui/terminal.hpp
    src/tui/renderer.hpp
    src/tui/menu.hpp
    src/input/input.hpp
    src/types.hpp
)

# Create executable
add_executable(jester-gb ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(jester-gb PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Platform-specific settings
if(WIN32)
    # Windows
    target_compile_definitions(jester-gb PRIVATE _WIN32_WINNT=0x0601 NOMINMAX)
    target_link_libraries(jester-gb PRIVATE winmm comdlg32)
    
    # Disable audio on Windows (no PulseAudio)

    
    # Console application
    set_target_properties(jester-gb PROPERTIES WIN32_EXECUTABLE FALSE)
    
else()
    # Linux/Unix
    add_compile_options(-Wall -Wextra -O2)
    
    # Find PulseAudio
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(PULSE libpulse-simple)
        if(PULSE_FOUND)
            target_include_directories(jester-gb PRIVATE ${PULSE_INCLUDE_DIRS})
            target_link_libraries(jester-gb PRIVATE ${PULSE_LIBRARIES})
        else()
            message(STATUS "PulseAudio not found - audio disabled")
            target_compile_definitions(jester-gb PRIVATE JESTER_NO_AUDIO=1)
        endif()
    endif()
    
    target_link_libraries(jester-gb PRIVATE pthread)
endif()

# Copy roms directory to build folder for convenience  
add_custom_command(TARGET jester-gb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink 
        ${CMAKE_SOURCE_DIR}/roms 
        ${CMAKE_BINARY_DIR}/roms
    COMMENT "Creating symlink to roms directory"
)
