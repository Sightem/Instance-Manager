#pragma once
#include "imgui.h"
#include "Base.hpp"

#include "instance-control/InstanceControl.h"
#include "utils/filesystem/FS.h"

namespace fs = std::filesystem;

class FileManagement
{
public:
    FileManagement(std::vector<std::string>& instances, std::vector<bool>& selection)
        : instances(instances), selection(selection)
    {}

    ~FileManagement() = default;

    void Draw(const char* title, bool* p_open = NULL);

private:
    std::vector<std::string>& instances;
    std::vector<bool>& selection;

    struct DirectoryEntryInfo {
        fs::directory_entry entry;
        std::string filename;
        std::string uniqueId;
    };
    std::unordered_map<std::string, std::vector<DirectoryEntryInfo>> cache;

    void DisplayFilesAndDirectories(std::string packageFamilyName, const std::filesystem::path& directory, bool forceRefresh = false);
    void CloneDir(std::string packageFamilyName, const std::filesystem::path& full_src_path);
};