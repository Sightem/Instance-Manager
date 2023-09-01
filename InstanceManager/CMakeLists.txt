cmake_minimum_required(VERSION 3.26)
project(Instance_Manager)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# List of required packages
set(REQUIRED_PACKAGES
        OpenCV
        OpenGL
        GLFW3
        libzippp
        fmt
        nlohmann_json
        cpr
        tinyxml2
)

# Find packages
foreach(PACKAGE ${REQUIRED_PACKAGES})
    find_package(${PACKAGE} REQUIRED)
endforeach()

# Add include directories
include_directories(${PROJECT_NAME} PRIVATE
        include
        libs/stb
        libs/imgui
)

# Add executable
add_executable(Instance_Manager
        src/config/Config.cpp
        src/group/Group.cpp
        src/instance-control/InstanceControl.cpp
        src/manager/Manager.cpp
        src/native/Native.cpp
        src/roblox/Roblox.cpp
        src/ui/AppLog.cpp
        src/ui/AutoRelaunch.cpp
        src/ui/FileManagement.cpp
        src/ui/InstanceManager.cpp
        src/ui/UI.cpp
        src/utils/filesystem/FS.cpp
        src/utils/string/StringUtils.cpp
        src/utils/Utils.cpp

        libs/imgui/imgui.cpp
        libs/imgui/imgui_demo.cpp
        libs/imgui/imgui_draw.cpp
        libs/imgui/imgui_impl_glfw.cpp
        libs/imgui/imgui_impl_opengl3.cpp
        libs/imgui/imgui_tables.cpp
        libs/imgui/imgui_widgets.cpp
        libs/imgui/imgui_stdlib.h
        libs/imgui/imgui_stdlib.cpp

        main.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${OpenCV_LIBS}
        WindowsApp.lib
        ${OPENGL_LIBRARIES}
        glfw
        libzippp::libzippp
        fmt::fmt
        nlohmann_json::nlohmann_json
        cpr::cpr
        tinyxml2::tinyxml2
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:CONSOLE")

# Post-build commands
add_custom_command(
        TARGET Instance_Manager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Template
        ${CMAKE_CURRENT_BINARY_DIR}/Template
)

add_custom_command(
        TARGET Instance_Manager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_BINARY_DIR}/Template/Windows10Universal.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Template
)

add_custom_command(
        TARGET Instance_Manager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/images
        ${CMAKE_CURRENT_BINARY_DIR}/images
)