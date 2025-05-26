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

# --- FreeType (Explicit Setup) ---
set(FREETYPE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include)
set(FREETYPE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/lib/freetype.lib)

if(NOT EXISTS ${FREETYPE_INCLUDE_DIR}/ft2build.h)
    message(FATAL_ERROR "FreeType include directory not found or ft2build.h is missing: ${FREETYPE_INCLUDE_DIR}")
endif()
if(NOT EXISTS ${FREETYPE_LIBRARY})
    message(FATAL_ERROR "FreeType library not found: ${FREETYPE_LIBRARY}")
endif()

message(STATUS "Using FreeType include directory: ${FREETYPE_INCLUDE_DIR}")
message(STATUS "Using FreeType library: ${FREETYPE_LIBRARY}")

# --- Source Files ---
set(APP_SOURCES
    src/main.cpp
    src/Shader.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/Chunk.cpp    # Added Chunk.cpp
    src/World.cpp    # Added World.cpp
    src/TextRenderer.cpp # Added TextRenderer.cpp
)

# --- Executable ---
enable_language(C) # For GLAD (glad.c)
add_executable(${PROJECT_NAME} ${APP_SOURCES})

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE glfw_lib glad_lib ${FREETYPE_LIBRARY})

# --- Include Directories ---
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src        # So we can use #include "Shader.h" etc.
    ${GLAD_INCLUDE_DIRS}                   # For glad/glad.h
    ${GLFW_DIR}/include                   # For GLFW/glfw3.h (redundant if using glfw_lib INTERFACE_INCLUDE_DIRECTORIES)
    ${GLM_INCLUDE_DIR}                     # For glm headers
    ${FREETYPE_INCLUDE_DIR}
)

# --- Copy DLLs and Assets (Windows Specific) ---
if(WIN32)
    # Copy glfw3.dll
    set(GLFW_DLL_PATH ${GLFW_DIR}/lib/glfw3.dll)
    if(NOT EXISTS ${GLFW_DLL_PATH})
        message(FATAL_ERROR "GLFW DLL not found at ${GLFW_DLL_PATH}")
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GLFW_DLL_PATH}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying glfw3.dll to build directory"
    )

    # Copy shaders directory
    set(SHADERS_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
    if(NOT EXISTS ${SHADERS_DIR_PATH})
        message(FATAL_ERROR "Shaders directory not found at ${SHADERS_DIR_PATH}")
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${SHADERS_DIR_PATH}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMENT "Copying shaders to build directory's shaders folder"
    )

    # Copy fonts directory
    set(ASSETS_FONTS_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts)
    if(NOT EXISTS ${ASSETS_FONTS_DIR_PATH})
        message(WARNING "Assets/fonts directory not found at ${ASSETS_FONTS_DIR_PATH}. Font loading might fail if using relative paths from here.")
        # Not making this fatal, as user might use absolute system font paths
    else()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${ASSETS_FONTS_DIR_PATH}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/fonts
            COMMENT "Copying fonts to build directory's assets/fonts folder"
        )
    endif()

    # --- Copy freetype.dll ---
    set(FREETYPE_DLL_EXPECTED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/bin/freetype.dll)
    
    # This check runs at CMake configure time.
    if(NOT EXISTS ${FREETYPE_DLL_EXPECTED_PATH})
        message(FATAL_ERROR "CRITICAL: freetype.dll was NOT FOUND by CMake at the expected path: ${FREETYPE_DLL_EXPECTED_PATH}. "
                            "Please verify this exact file exists. The build process cannot continue without it "
                            "if FreeType was compiled as a DLL.")
    else()
        message(STATUS "SUCCESS: freetype.dll FOUND by CMake at ${FREETYPE_DLL_EXPECTED_PATH}. The rule to copy it will be added.")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FREETYPE_DLL_EXPECTED_PATH} # Use the verified path
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying freetype.dll to build directory"
        )
    endif()

endif() # End of if(WIN32)

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