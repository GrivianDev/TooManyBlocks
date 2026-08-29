cmake_minimum_required(VERSION 3.10)

project(TooManyBlocks VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure CMAKE_BUILD_TYPE is set for single-config generators
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

# Gather all .cpp files in the src directory
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(TooManyBlocks ${SOURCES})

# Define App name and disable GLU for GLEW for including glew.h
target_compile_definitions(TooManyBlocks PRIVATE APP_NAME="TooManyBlocks" APP_VERSION="${PROJECT_VERSION}" GLEW_NO_GLU)
# Enable DEBUG_MODE macro in Debug mode
target_compile_definitions(TooManyBlocks PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)

# Include directories
target_include_directories(TooManyBlocks PRIVATE src/core src/core/foundation/log)

# Load configuration of all librarys from dependency folder wich contains intermediate cmake file
add_subdirectory(dependencies)
target_link_libraries(TooManyBlocks PRIVATE glew_s glfw imgui glm stb_image miniaudio JsonParser)

# Additional configuration based on the current platform
if(UNIX AND NOT APPLE)
    message(STATUS "Detected Linux - Linking additional Unix-specific libraries")
    target_link_libraries(TooManyBlocks PRIVATE GL X11 pthread)
elseif(WIN32)
    message(STATUS "Detected Windows - Linking additional Windows-specific libraries")
    target_link_libraries(TooManyBlocks PRIVATE opengl32 glu32 User32 gdi32 Shell32 psapi)

    # In Release mode, define WinMain Entrypoint to get rid of console window
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release Build: Enabling Windows GUI subsystem (No console)")
        # Properties for WinMain Entrypoint
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_definitions(TooManyBlocks PRIVATE $<$<CONFIG:Release>:WIN_MAIN>)
            set_target_properties(TooManyBlocks PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            # Does not need to define WIN_MAIN cause main() is still used with this compiler
            set_target_properties(TooManyBlocks PROPERTIES LINK_FLAGS "-mwindows")
        endif()
    else()
        message(STATUS "Building as default subsystem")
    endif()
else()
    message(STATUS "Unknown Platform... no specific configuration done!")
endif()

# Set output directories
set_target_properties(TooManyBlocks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin
)

# Set up resource folder copying
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/res)
set(DESTINATION_DIR ${CMAKE_BINARY_DIR}/bin/res)

add_custom_target(copy_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_DIR} ${DESTINATION_DIR}
    COMMENT "Copying resource files to the build directory"
)

# Make sure it runs before the main build
add_dependencies(TooManyBlocks copy_resources)

# Copy third party licenses to output dir
file(COPY "${CMAKE_SOURCE_DIR}/third_party.txt" DESTINATION "${CMAKE_BINARY_DIR}/bin")