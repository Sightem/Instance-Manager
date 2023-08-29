#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#define NOMINMAX
#include <Windows.h>

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
    };

    void NukeInstane(const std::string packagefullname, const std::string path);
    std::unordered_map<std::string, Roblox::Instance> ProcessRobloxPackages();

    enum ModifyXMLError {
        Success = 0,
        LoadError,
        NotFound,
        SaveError
    };

    ModifyXMLError ModifySettings(std::string filePath, int newGraphicsQualityValue, float newMasterVolumeValue, int newSavedQualityValue);
    std::string GetCSRF(std::string cookie);
    std::string EnterCode(std::string code, std::string cookie);
    std::string ValidateCode(std::string code, std::string cookie);
    std::string ExtractCode(const unsigned char* data, size_t dataSize);
    std::string FindCodeValue(HANDLE pHandle, const std::string& name);
    std::vector<std::string> GetNewInstances(const std::vector<std::string>& old_instances);
}