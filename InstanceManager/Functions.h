#pragma once
#define NOMINMAX
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <array>
#include <regex>
#include <sstream>
#include <map>
#include <set>
#include "imgui.h"
#include <Lmcons.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include <sddl.h>
#include <libzippp/libzippp.h>

#include "ntdll.h"


struct RobloxPackage {
    std::string Name;
    std::string Username;
    std::string PackageID;
    std::string AppID;
    std::string InstallLocation;
    std::string PackageFamilyName;
    std::string Version;
};

struct RobloxInstance {
    RobloxPackage Package;
    DWORD ProcessID = {};
};

namespace FS
{
    bool CopyDirectory(const std::filesystem::path& src, const std::filesystem::path& dst);
    bool RemovePath(const std::filesystem::path& path_to_delete);
    bool DecompressZip(const std::string& zipPath, const std::string& destination);
    bool DecompressZipToFile(const std::string& zipPath, const std::string& destination);
    std::vector<std::string> FindFiles(const std::string& path, const std::string& substring);
}

namespace ui
{
    enum class ButtonStyle
    {
        Red,
        Green,
    };

    bool InputTextWithHint(const char* label, const char* hint, std::string* my_str, ImGuiInputTextFlags flags = 0);
    bool ConditionalButton(const char* label, bool condition, ButtonStyle style);
    bool GreenButton(const char* label);
    bool RedButton(const char* label);
    void HelpMarker(const char* desc);
}

namespace Native
{
    std::string RunPowershellCommand(const std::string& command);
    std::string GetCurrentUsername();
    std::string GetUserExperience();
    std::string GetUserSID();
    std::string GetHexDatetime();
    void WriteProtocolKeys(const std::string& progId, const std::string& protocol, const std::string& progHash);
    std::map<std::string, std::string> GetProgIDNames();
    PVOID GetPebAddress(HANDLE ProcessHandle);
    std::string get_commandline_arguments(DWORD pid);
    bool OpenInExplorer(const std::string& path, bool isFile = false);
    bool SetProcessAffinity(DWORD processID, DWORD requestedCores);
    bool IsReadableMemory(const MEMORY_BASIC_INFORMATION& mbi);

    typedef std::string(*ExtractFunction)(const unsigned char*, size_t);
    std::string SearchEntireProcessMemory(HANDLE pHandle, const unsigned char* pattern, size_t patternSize, ExtractFunction extractFunction);
}

namespace StringUtils
{
    bool contains_only(const std::string& s, char c);
    bool CopyToClipboard(const std::string& data);
    std::string Base64Encodde(const unsigned char* buffer, size_t length);
    std::string WStrToStr(const std::wstring& wstr);
}

namespace Roblox
{
    void NukeInstane(const std::string name, const std::string path);
    std::vector<RobloxPackage> ProcessRobloxPackages();
    std::vector<RobloxInstance> WrapPackages();
    void LaunchRoblox(std::string AppID, const std::string& placeid);
    void LaunchRoblox(std::string AppID, const std::string& placeid, const std::string& linkcode);

    std::set<DWORD> GetRobloxInstances();

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
    std::vector<RobloxInstance> GetNewInstances(const std::vector<RobloxInstance>& old_instances);
}

namespace Utils
{
    long GetShiftRight(long value, int count);
    int ToInt32(const unsigned char* bytes, int offset);
    std::vector<unsigned char> ParsePattern(const std::string& pattern);
    const unsigned char* ToBytes(int64_t value);
    const unsigned char* TakeBytes(const unsigned char* input, int count);
    std::string GetHash(const std::string& baseInfo);
    std::string ModifyAppxManifest(std::string inputXML, std::string name);
    bool SaveToFile(const std::string& file_path, const std::vector<char8_t>& buffer);
    uintptr_t BoyerMooreHorspool(const unsigned char* signature, size_t signatureSize, const unsigned char* data, size_t dataSize);
    void DownloadAndSave(const std::string& url, const std::string& localFileName);
    void DecompressZip(const std::string& zipFile, const std::string& destination);
    void CopyFileToDestination(const std::string& source, const std::string& destination);
    void WriteAppxManifest(const std::string& url, const std::string& localPath, const std::string name = "");
    void UpdatePackage(const std::string& baseFolder, const std::string& instanceName = "");
    bool SaveScreenshotAsPng(const char* filename);

}