-- premake5.lua
workspace "Instance-Manager"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Instance-Manager"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/Build-Walnut-External.lua"

include "Instance-Manager"