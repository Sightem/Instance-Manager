#include "Roblox.h"

#include <regex>
#include <iostream>

#include "Native.h"
#include "FS.h"
#include "Utils.h"

#include "tinyxml2.h"
#include "cpr/cpr.h"
#include "nlohmann/json.hpp"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36"

#include "InstanceControl.h"

namespace Roblox
{
    void NukeInstane(const std::string packagefullname, const std::string path)
    {
        Native::RemoveUWPApp(winrt::to_hstring(packagefullname));

        std::thread([path]() {
			FS::RemovePath(path);
        }).detach();

    }

    std::unordered_map<std::string, Roblox::Instance> ProcessRobloxPackages() {
        std::unordered_map<std::string, Roblox::Instance> instancesMap;

        winrt::Windows::Management::Deployment::PackageManager packageManager;

        try {
            for (const auto& package : packageManager.FindPackages()) {
                // Check Publisher ID
                std::wstring publisherID = package.Id().PublisherId().c_str();
                if (publisherID == L"55nm5eh3cm0pr") {
                    Roblox::Instance instance;

                    instance.PackageID = winrt::to_string(package.Id().FullName().c_str());
                    instance.PackageFullName = winrt::to_string(package.Id().FullName().c_str());
                    instance.PackageFamilyName = winrt::to_string(package.Id().FamilyName().c_str());
                    instance.Version = std::to_string(package.Id().Version().Major) + "." +
                        std::to_string(package.Id().Version().Minor) + "." +
                        std::to_string(package.Id().Version().Revision) + "." +
                        std::to_string(package.Id().Version().Build);
                    if (package.InstalledLocation()) {
                        instance.InstallLocation = winrt::to_string(package.InstalledLocation().Path().c_str());
                    }
                    else {
                        instance.InstallLocation = "Not Available";
                    }

                    // Getting AppListEntry information
                    auto appListEntries = package.GetAppListEntriesAsync().get();
                    for (const auto& appListEntry : appListEntries) {
                        std::string fullName = winrt::to_string(package.Id().FullName().c_str());
                        size_t underscorePosition = fullName.find('_');
                        if (underscorePosition != std::string::npos) {
                            instance.Name = fullName.substr(0, underscorePosition);
                        }
                        else {
                            instance.Name = fullName;
                        }

                        instance.AppID = winrt::to_string(appListEntry.AppUserModelId().c_str());
                    }

                    // Extract the desired key using regex
                    std::string fullName = winrt::to_string(package.Id().Name().c_str());
                    std::regex r(R"(([^.]+)$)");  // Match any substring that doesn't contain a dot, at the end of the string
                    std::smatch match;
                    if (std::regex_search(fullName, match, r) && match.size() > 1) {
                        std::string key = match.str(1);
                        instancesMap[key] = instance;
                    }
                }
            }
        }
        catch (const winrt::hresult_error& ex) {
            std::wcerr << L"WinRT error: " << ex.message().c_str() << std::endl;
            std::wcerr << L"Error code: " << ex.code() << std::endl;
        }

        if (instancesMap.find("ROBLOX") != instancesMap.end()) {
            instancesMap.erase("ROBLOX");
        }

        return instancesMap;
    }

    ModifyXMLError ModifySettings(std::string filePath, int newGraphicsQualityValue, float newMasterVolumeValue, int newSavedQualityValue)
    {
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLError eResult = doc.LoadFile(filePath.c_str());
        if (eResult != tinyxml2::XML_SUCCESS) {
            return ModifyXMLError::LoadError;
        }

        tinyxml2::XMLElement* itemElement = doc.FirstChildElement("roblox")->FirstChildElement("Item");
        if (!itemElement || std::string(itemElement->Attribute("class")) != "UserGameSettings") {
            return ModifyXMLError::NotFound;
        }

        tinyxml2::XMLElement* propertiesElement = itemElement->FirstChildElement("Properties");
        if (!propertiesElement) {
            return ModifyXMLError::NotFound;
        }

        tinyxml2::XMLElement* graphicsQualityElement = propertiesElement->FirstChildElement("int");
        while (graphicsQualityElement && std::string(graphicsQualityElement->Attribute("name")) != "GraphicsQualityLevel") {
            graphicsQualityElement = graphicsQualityElement->NextSiblingElement("int");
        }
        if (graphicsQualityElement) {
            graphicsQualityElement->SetText(newGraphicsQualityValue);
        }

        tinyxml2::XMLElement* masterVolumeElement = propertiesElement->FirstChildElement("float");
        while (masterVolumeElement && std::string(masterVolumeElement->Attribute("name")) != "MasterVolume") {
            masterVolumeElement = masterVolumeElement->NextSiblingElement("float");
        }
        if (masterVolumeElement) {
            masterVolumeElement->SetText(newMasterVolumeValue);
        }

        tinyxml2::XMLElement* savedQualityElement = propertiesElement->FirstChildElement("token");
        while (savedQualityElement && std::string(savedQualityElement->Attribute("name")) != "SavedQualityLevel") {
            savedQualityElement = savedQualityElement->NextSiblingElement("token");
        }
        if (savedQualityElement) {
            savedQualityElement->SetText(newSavedQualityValue);
        }

        eResult = doc.SaveFile(filePath.c_str());
        if (eResult != tinyxml2::XML_SUCCESS) {
            return ModifyXMLError::SaveError;
        }

        return ModifyXMLError::Success;
    }

    std::string EnterCode(std::string code, std::string cookie)
    {
        nlohmann::json j;
        j["code"] = code;

        cpr::Response r = cpr::Post(
            cpr::Url{ "https://apis.roblox.com/auth-token-service/v1/login/enterCode" },
            cpr::Header{ {"Content-Type", "application/json"}, { "Referer", "https://www.roblox.com/" }, { "User-Agent", USER_AGENT }, { "x-csrf-token", GetCSRF(cookie) }, { "Accept", "application/json" }, { "Origin", "https://www.roblox.com" } },
            cpr::Cookies{ {".ROBLOSECURITY", cookie} },
            cpr::Body{ j.dump() }
        );

        return r.text;
    }

    std::string ValidateCode(std::string code, std::string cookie)
    {
        nlohmann::json j;
        j["code"] = code;

        cpr::Response r = cpr::Post(
            cpr::Url{ "https://apis.roblox.com/auth-token-service/v1/login/validateCode" },
            cpr::Header{ {"Content-Type", "application/json"}, { "Referer", "https://www.roblox.com/" }, { "User-Agent", USER_AGENT }, { "x-csrf-token", GetCSRF(cookie) }, { "Accept", "application/json" }, { "Origin", "https://www.roblox.com" } },
            cpr::Cookies{ {".ROBLOSECURITY", cookie} },
            cpr::Body{ j.dump() }
        );

        return r.text;
    }

    std::string GetCSRF(std::string cookie)
    {
        cpr::Response r = cpr::Post(
            cpr::Url{ "https://auth.roblox.com/v1/authentication-ticket" },
            cpr::Header{ {"Content-Type", "application/json"}, { "Referer", "https://www.roblox.com/" } },
            cpr::Cookies{ {".ROBLOSECURITY", cookie} }
        );

        return r.header["x-csrf-token"];
    }

    std::string ExtractCode(const unsigned char* dataStart, size_t dataSize) {
        std::string code;
        const unsigned char* dataEnd = dataStart + dataSize;
        while (dataStart != dataEnd && *dataStart != 0x22) {
            code += static_cast<char>(*dataStart);
            dataStart++;
        }
        return code;
    }

    std::string FindCodeValue(HANDLE pHandle, const std::string& name) {

        std::string windowName = std::string("Roblox ") + name;
        HWND hWnd = FindWindow(NULL, windowName.c_str());

        SetForegroundWindow(hWnd);

        // First pattern: key=???????-????-????-????-????????????&code=
        auto pattern1 = Utils::ParsePattern("6B 65 79 3D ?? ?? ?? ?? ?? ?? ?? ?? 2D ?? ?? ?? ?? 2D ?? ?? ?? ?? 2D ?? ?? ?? ?? 2D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 26 63 6F 64 65 3D");
        std::string codeValue = Native::SearchEntireProcessMemory(pHandle, pattern1.data(), pattern1.size(), Roblox::ExtractCode);

        if (!codeValue.empty()) return codeValue;

        // Second pattern: "\"code\":\""
        unsigned char pattern2[] = { 0x22, 0x63, 0x6F, 0x64, 0x65, 0x22, 0x3A, 0x22 };
        size_t patternSize2 = sizeof(pattern2) / sizeof(unsigned char);
        codeValue = Native::SearchEntireProcessMemory(pHandle, pattern2, patternSize2, Roblox::ExtractCode);

        return codeValue;
    }

    std::vector<std::string> GetNewInstances(const std::vector<std::string>& old_instances) {
        std::vector<std::string> new_instances = g_InstanceControl.GetInstanceNames();
        std::vector<std::string> result;

        // Check if an instance in new_instances is not present in old_instances
        for (const auto& new_instance : new_instances) {
            bool found = false;
            for (const auto& old_instance : old_instances) {
                if (new_instance == old_instance) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.push_back(new_instance);
            }
        }

        return result;
    }
}