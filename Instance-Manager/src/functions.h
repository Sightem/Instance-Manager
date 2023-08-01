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
#include <imgui.h>
#include <Lmcons.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include <sddl.h>
#include <libzippp/libzippp.h>

#include "ntdll.h"


struct UserInstance {
    std::string Name;
    std::string Username;
    std::string PackageID;
    std::string AppID;
    std::string InstallLocation;
    std::string PackageFamilyName;
    std::string Version;
    DWORD ProcessID = 0;
};

namespace FS
{
    std::vector<std::string> enumerate_directories(const std::string& path, int depth, bool onlyNames = false);

    bool copy_directory(const std::filesystem::path& src, const std::filesystem::path& dst);

    std::string replace_pattern_in_file(const std::filesystem::path& filePath, const std::string& pattern, const std::string& replacement);

    bool remove_path(const std::filesystem::path& path_to_delete);

    bool decompress_zip(const std::string& zipPath, const std::string& destination);

    bool decompress_zip_to_file(const std::string& zipPath, const std::string& destination);

    std::vector<std::string> find_files(const std::string& path, const std::string& substring);
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

}

namespace Native
{
    bool enable_developer_mode();

    std::string run_powershell_command(const std::string& command);

    std::string get_current_username();

    std::string get_user_experience();

    std::string get_user_sid();

    std::string get_hex_datetime();

    void write_protocol_keys(const std::string& progId, const std::string& protocol, const std::string& progHash);

    std::map<std::string, std::string> get_progid_names();

    PVOID get_peb_address(HANDLE ProcessHandle);

    std::string get_commandline_arguments(DWORD pid);

    bool open_in_explorer(const std::string& path, bool isFile = false) noexcept;

    void minimize_window(DWORD pid);
}

namespace StringUtils
{
    bool contains_only(const std::string& s, char c);

    std::string replace_all(std::string str, const std::string& from, const std::string& to);

    bool copy_to_clipboard(const std::string& data);

    std::string get_after_last_occurrence(const std::string& source, char ch);

    std::string base64_encode(const unsigned char* buffer, size_t length);

    std::string wstr_to_str(const std::wstring& wstr);
}

namespace Roblox
{
    void nuke_instance(const std::string name, const std::string path);
    std::vector<UserInstance> process_roblox_packages();
    void launch_roblox(std::string AppID, const std::string& placeid);
    void launch_roblox(std::string AppID, const std::string& placeid, const std::string& linkcode);

    std::set<DWORD> get_roblox_instances();
}

namespace Utils
{
    long get_shift_right(long value, int count);
    int to_int32(const unsigned char* bytes, int offset);

    const unsigned char* to_bytes(int64_t value);
    const unsigned char* take_bytes(const unsigned char* input, int count);
    std::string get_hash(const std::string& baseInfo);

}