
<h1 align="center">
  <br>
      <img src="https://i.ibb.co/rp9HkvM/instance-manager.png">
  <br>
    Instance Manager
  <br>
</h1>

<h5 align="center">A sleek Instance Manager for the UWP version of Roblox</h5>

<p align="center">
<a href="#introduction"><b>Introduction</b></a> •
  <a href="#download"><b>Download</b></a> •
  <a href="#building"><b>Building</b></a> •
  <a href="#key-features"><b>Key Features</b></a> •
  <a href="#credits"><b>Credits</b></a> • 
  <a href="#contributors"><b>Contributors</b></a>
</p>

<h4 align="center">
  <a href="https://discord.gg/hVdMzb7KUn">
    <img src="https://img.shields.io/badge/discord-7289da.svg?style=flat-square" alt="discord">


  </a>
</h4>

![screenshot](https://cdn.discordapp.com/attachments/1145882469900496966/1146221397828435998/Instance_Manager_bkTrF21Mgy.gif)

# Introduction
`Instance Manager` is a tool that allows you to manage multiple instances of Roblox at once. It is designed to be easy to use and intuitive. It is also designed to be as lightweight as possible, so it can run on any computer as long as it has Windows 10 installed, and has a GPU that supports OpenGL.

## Key Features
* Instance Creation
* Instance Deletion
* Update Instances
* Launch Instances
  - Launch any instance into any game and vip server
* In-game Settings Control
* Auto Relaunch
  - Avoid crashes by automatically relaunching instances
* Auto Inject
* Join any game with multiple accounts at once, works with Vip Servers too
* File Management
  - Easily clone workspace and/or autoexec folders from any instance to any instance
* CPU Limiter
* Auto Cookie Login

## Download
Download the latest version of Instance Manager [here](https://example.com).

> **Note**
> Make sure to have [developer mode](https://learn.microsoft.com/en-us/windows/apps/get-started/developer-mode-features-and-debugging) enabled on your Windows 10 device.

## Building

To build you require following prerequisites:
- CMake
- Visual Studio 17+ installed on your system with the Win 10 SDK package
- vcpkg (optional)

Install all the following packages in any way:
- OpenCV
- OpenGl
- Glfw3
- Nlohmann JSON
- Cpr
- libzippp
- tinyxml2

> **Note**
> [vcpkg](https://vcpkg.io/en/getting-started) is recommended for installing the packages.

Add the `-G "Visual Studio 17 2022"` to your command line.

If using VCPKG also include `-DCMAKE_TOOLCHAIN_FILE=<vcpkg_installion_path>/scripts/buildsystems/vcpkg.cmake`

a full command line would look something like this (assuming in root of project):
```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_TOOLCHAIN_FILE=<vcpkg_installion_path>/scripts/buildsystems/vcpkg.cmake ..`
cmake --build .
```



## Credits

This software uses the following open source packages:

- [imgui](https://github.com/ocornut/imgui)
- [CPR](https://github.com/libcpr/cpr)
- [OpenCV](https://github.com/opencv/opencv)
- [OpenGL](https://www.opengl.org/)
- [GLFW](https://www.glfw.org/)
- [fmt](https://github.com/fmtlib/fmt)
- [nlohmann json](https://github.com/nlohmann/json)
- [WinRT](https://github.com/microsoft/cppwinrt)

## Contributors

<a href="https://github.com/Sightem"><img src="https://avatars.githubusercontent.com/u/67830794?v=4" width="50" height="50"></a>
<a href="https://github.com/nowilltolife"><img src="https://avatars.githubusercontent.com/u/55301990?v=4" width="50" height="50"></a>

# License
GNU Affero General Public License v3.0
```