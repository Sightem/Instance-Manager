#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "imgui.h"

#define NOMINMAX
#include <windows.h>

namespace Roblox
{
    struct Instance {
        std::string Name;
        std::string PackageID;
        std::string AppID;
        std::string InstallLocation;
        std::string PackageFamilyName;
        std::string Version;
        std::string PackageFullName;
        std::string DisplayName;
    };

    void NukeInstance(const std::string& packagefullname, const std::string& path);
    void HandleCodeValidation(DWORD pid, const std::string& cookie);
    std::unordered_map<std::string, std::tuple<Roblox::Instance, ImU32>> ProcessRobloxPackages();

    enum ModifyXMLError {
        Success = 0,
        LoadError,
        NotFound,
        SaveError
    };

    ModifyXMLError ModifySettings(std::string filePath, int newGraphicsQualityValue, float newMasterVolumeValue, int newSavedQualityValue);
    std::string GetCSRF(std::string cookie);
    std::string EnterCode(const std::string& code, std::string cookie);
    std::string ValidateCode(const std::string& code, std::string cookie);
    std::string ExtractCode(const unsigned char* data, size_t dataSize);
    std::string FindCodeValue(HANDLE pHandle);
    std::vector<std::string> GetNewInstances(const std::vector<std::string>& old_instances);
}