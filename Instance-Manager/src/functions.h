#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <array>
#include <regex>
#include <sstream>
#include <imgui.h>
#include <Windows.h>
#include <libzippp/libzippp.h>



namespace FS
{
    std::vector<std::string> enumerate_directories(const std::string& path, int depth, bool onlyNames = false);

    bool copy_directory(const std::filesystem::path& src, const std::filesystem::path& dst);

    std::string replace_pattern_in_file(const std::filesystem::path& filePath, const std::string& pattern, const std::string& replacement);

    bool remove_path(const std::filesystem::path& path_to_delete);

    bool decompress_zip(const std::string& zipPath, const std::string& destination);

    bool decompress_zip_to_file(const std::string& zipPath, const std::string& destination);

}

namespace ui
{
    bool InputTextWithHint(const char* label, const char* hint, std::string* my_str, ImGuiInputTextFlags flags = 0);
}

namespace Native
{
    bool enable_developer_mode();


    std::string run_powershell_command(const std::string& command);
}

namespace StringUtils
{
    bool contains_only(const std::string& s, char c);

    std::vector<std::pair<std::string, std::string>> extract_paths_and_folders(const std::string& input) noexcept;

    std::string replace_all(std::string str, const std::string& from, const std::string& to);

    bool copy_to_clipboard(const std::string& data);

}

namespace Roblox
{
    void nuke_instance(const std::string& name, const std::string& path);
}