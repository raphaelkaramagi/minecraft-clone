cmake_minimum_required(VERSION 3.10)

project(MinecraftClone VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Project Directories ---
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# --- GLFW ---
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
add_library(glfw_lib STATIC IMPORTED)
set_target_properties(glfw_lib PROPERTIES
    IMPORTED_LOCATION ${GLFW_DIR}/lib/glfw3.lib  # Adjusted path to MSVC .lib file
    INTERFACE_INCLUDE_DIRECTORIES ${GLFW_DIR}/include
)

# --- GLAD ---
set(GLAD_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src)
set(GLAD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)
add_library(glad_lib STATIC ${GLAD_SOURCE_DIR}/glad.c)
target_include_directories(glad_lib PUBLIC ${GLAD_INCLUDE_DIRS})

# --- GLM ---
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glm)

# --- Source Files ---
set(APP_SOURCES
    src/main.cpp
    src/Shader.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/Chunk.cpp    # Added Chunk.cpp
    src/World.cpp    # Added World.cpp
)

# --- Executable ---
enable_language(C) # For GLAD (glad.c)
add_executable(${PROJECT_NAME} ${APP_SOURCES})

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE glfw_lib glad_lib)

# --- Include Directories ---
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src        # So we can use #include "Shader.h" etc.
    ${GLAD_INCLUDE_DIRS}                   # For glad/glad.h
    ${GLFW_DIR}/include                   # For GLFW/glfw3.h (redundant if using glfw_lib INTERFACE_INCLUDE_DIRECTORIES)
    ${GLM_INCLUDE_DIR}                     # For glm headers
)

# --- Copy DLLs and Assets (Windows Specific) ---
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GLFW_DIR}/lib/glfw3.dll # Adjusted path to DLL
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying glfw3.dll to build directory"
    )
    # Copy shaders directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMENT "Copying shaders to build directory's shaders folder"
    )
endif()

# --- Output/Build Directory ---
# Place executables directly in build/bin or build/bin/Debug etc.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Message for user ---
message(STATUS "MinecraftClone configured.")
message(STATUS "Executable will be in: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/<config>/ or ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
message(STATUS "Build with 'cmake --build . --config Debug' or 'cmake --build . --config Release'")
message(WARNING "If you are on Windows and used a GLFW library other than for VS2022 (e.g., lib-vc2019), please ensure the path to glfw3.lib and glfw3.dll in CMakeLists.txt (lines 11-12) is updated accordingly.")