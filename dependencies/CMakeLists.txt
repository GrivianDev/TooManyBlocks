# ========================== GLFW ==========================
# GLFW has its own CMake file that exposes "glfw" as a static library
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable doc generation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Disable installation target" FORCE)
add_subdirectory(glfw)

# ========================== GLEW ==========================
# GLEW is tricky, so checking if user has generated the nessecary files for building from source
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glew/src/glew.c" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glew/include/GL/glew.h")
    message(FATAL_ERROR 
        "GLEW generated files are missing!\n"
        "Run the following commands in the 'glew/auto' directory to generate them:\n"
        "cd ${CMAKE_CURRENT_SOURCE_DIR}/glew/auto\n"
        "make"
    )
else()
    # Configuration: Disable shared libraries and utility builds
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libraries")  # Seems to not prevent shared build completely
    set(BUILD_UTILS OFF CACHE BOOL "Disable building glewinfo and visualinfo" FORCE)
    set(GLEW_NO_GLU ON CACHE BOOL "Disable GLU support" FORCE)

    # GLEW has a large CMake file
    add_subdirectory(glew/build/cmake)

    target_compile_definitions(glew_s PRIVATE GLEW_STATIC)
    # Ensure the correct include directories for GLEW
    target_include_directories(glew_s INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glew/include>)
endif()

# ========================== IMGUI ==========================
# Create a static library for ImGui with OpenGL and GLFW backends.
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# Include the core and backend directories for ImGui.
target_include_directories(imgui PUBLIC imgui imgui/backends)

# Link against GLFW (Glew is not needed here if the main program links it).
target_link_libraries(imgui PRIVATE glfw)

# ========================== GLM ==========================
# Configuration: Enable fast math and allow output messages
set(GLM_QUIET OFF CACHE BOOL "Disable building silently" FORCE)
set(GLM_ENABLE_FAST_MATH ON CACHE BOOL "Enable GLM fast math" FORCE)

# GLM has its own CMake file exposing "glm" as a static library
add_subdirectory(glm)

# ======================== miniaudio ======================
# Configuration: Excluding unessecary features / backends
set(MINIAUDIO_NO_ENGINE ON CACHE BOOL "High level api is not used" FORCE)
set(MINIAUDIO_NO_RESOURCE_MANAGER ON CACHE BOOL "Ressources are managed manually" FORCE)
set(MINIAUDIO_NO_NODE_GRAPH ON CACHE BOOL "No Nodes since mixing is done manually" FORCE)
set(MINIAUDIO_NO_EXTRA_NODES ON CACHE BOOL "No advanced effets needed" FORCE)
set(MINIAUDIO_NO_FLAC ON CACHE BOOL "MP3 and WAV suffice" FORCE)
set(MINIAUDIO_NO_JACK ON CACHE BOOL "Linux music production backend" FORCE)
set(MINIAUDIO_NO_OPENSL ON CACHE BOOL "Old android api is not needed" FORCE)
set(MINIAUDIO_NO_WEBAUDIO ON CACHE BOOL "Web based audio is not needed" FORCE)

# miniaudio has its own CMake file exposing "miniaudio" as a static library
add_subdirectory(miniaudio)

# ========================== STB ==========================
# Expose stb_image as a static library
add_library(stb_image STATIC libs/stb_image.cpp)

# Include the stb header directory
target_include_directories(stb_image PUBLIC stb)

# ====================== JsonParser =======================
# JsonParser lib also has its on cmake
add_subdirectory(JsonParser)