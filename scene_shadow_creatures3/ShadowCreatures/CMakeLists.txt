cmake_minimum_required(VERSION 3.20)
project(ShadowCreatures LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.2.5
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL " " FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL " " FORCE)
FetchContent_MakeAvailable(assimp)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Local 3rdParty
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/glad")
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/glfw")
set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty/glm")

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR} glfw_build)

# GLAD
add_library(glad STATIC "${GLAD_DIR}/src/glad.c")
target_include_directories(glad PUBLIC "${GLAD_DIR}/include")

add_executable(ShadowCreatures
    src/main.cpp
    src/Camera.cpp
    src/ShadowCreature.cpp
    src/AILightController.cpp
    src/LoadedModel.cpp
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)

target_include_directories(ShadowCreatures PRIVATE 
    src
    "${GLAD_DIR}/include"
    "${GLFW_DIR}/include"
    "${GLM_DIR}"
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)

target_link_libraries(ShadowCreatures 
    glad
    glfw
    assimp::assimp
    ${CMAKE_DL_LIBS}
)

# Copy runtime assets and DLLs next to the executable.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/models/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/models)
add_custom_command(TARGET ShadowCreatures POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:ShadowCreatures>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        $<TARGET_FILE_DIR:ShadowCreatures>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:glfw>
        $<TARGET_FILE_DIR:ShadowCreatures>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:assimp>
        $<TARGET_FILE_DIR:ShadowCreatures>
)
