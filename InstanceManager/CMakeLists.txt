cmake_minimum_required(VERSION 3.26)
project(Instance_Manager)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)
include_directories(libs/stb)
include_directories(libs/imgui)

find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(libzippp REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpr REQUIRED)
find_package(tinyxml2 REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS} )

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

        main.cpp)

target_link_libraries( ${PROJECT_NAME} ${OpenCV_LIBS} WindowsApp.lib ${OPENGL_LIBRARIES} glfw libzippp::libzippp fmt::fmt nlohmann_json::nlohmann_json cpr::cpr tinyxml2::tinyxml2)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:CONSOLE")
