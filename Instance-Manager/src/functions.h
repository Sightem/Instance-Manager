#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <array>
#include <regex>
#include <sstream>
#include <Windows.h>
#include <libzippp/libzippp.h>


namespace FS
{
    std::vector<std::string> enumerate_directories(const std::string& path, int depth, bool onlyNames = false) {
        std::vector<std::string> directories;

        if (depth < 0) {
            return directories;
        }

        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (std::filesystem::is_directory(entry.status())) {
                    directories.push_back(onlyNames ? entry.path().filename().string() : entry.path().string());
                    if (depth > 1) {
                        auto subDirs = enumerate_directories(entry.path().string(), depth - 1, onlyNames);
                        directories.insert(directories.end(), subDirs.begin(), subDirs.end());
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        return directories;
    }

    bool copy_directory(const std::filesystem::path& src, const std::filesystem::path& dst)
    {
        auto abs_src = std::filesystem::absolute(src);
        auto abs_dst = std::filesystem::absolute(dst);

        if (!std::filesystem::exists(abs_src) || !std::filesystem::is_directory(abs_src)) {
            std::cerr << "Source directory " << abs_src << " does not exist or is not a directory.\n";
            return false;
        }

        if (!std::filesystem::exists(abs_dst)) {
            std::filesystem::create_directories(abs_dst);
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(abs_src)) {
            const auto& src_path = entry.path();
            auto dst_path = abs_dst / src_path.lexically_relative(abs_src);

            if (entry.is_directory()) {
                std::filesystem::create_directories(dst_path);
            }
            else if (entry.is_regular_file()) {
                try {
                    std::filesystem::copy_file(src_path, dst_path, std::filesystem::copy_options::overwrite_existing);
                }
                catch (const std::filesystem::filesystem_error& e) {
                    std::cerr << "Error copying file " << src_path << " to " << dst_path << ": " << e.what() << '\n';
                    return false;
                }
            }
            else {
                std::cerr << "Skipping non-regular file " << src_path << '\n';
            }
        }

        return true;
    }

    std::string replace_pattern_in_file(const std::filesystem::path& filePath, const std::string& pattern, const std::string& replacement)
    {
        std::ifstream fileStream(filePath);

        if (!fileStream) {
            std::cerr << "Failed to open the file: " << filePath << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        std::string fileContents = buffer.str();

        fileStream.close();

        size_t pos = 0;
        while ((pos = fileContents.find(pattern, pos)) != std::string::npos) {
            fileContents.replace(pos, pattern.length(), replacement);
            pos += replacement.length();
        }

        return fileContents;
    }

    bool remove_path(const std::filesystem::path& path_to_delete)
    {
        std::error_code ec;

        // Check if the path exists
        if (std::filesystem::exists(path_to_delete, ec))
        {
            try
            {
                if (std::filesystem::is_regular_file(path_to_delete))
                {
                    return std::filesystem::remove(path_to_delete, ec);
                }
                else if (std::filesystem::is_directory(path_to_delete))
                {
                    return std::filesystem::remove_all(path_to_delete, ec) > 0;
                }
                else
                {
                    std::cerr << "Error: Unsupported file type." << std::endl;
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }
        else
        {
            // Path does not exist
            std::cerr << "Error: Path does not exist." << std::endl;
            return false;
        }
    }

    bool decompress_zip(const std::string& zipPath, const std::string& destination) {
        using namespace libzippp;

        ZipArchive zip(zipPath);
        if (!zip.open(ZipArchive::ReadOnly)) {
            std::cerr << "Failed to open zip archive: " << zipPath << std::endl;
            return false;
        }


        int nbEntries = zip.getNbEntries();
        for (int i = 0; i < nbEntries; ++i) {
            ZipEntry entry = zip.getEntry(i);
            if (!entry.isNull()) {
                std::string outputPath = destination + "\\" + entry.getName();

                if (entry.isDirectory()) {
                    CreateDirectoryA(outputPath.c_str(), NULL);
                }
                else {
                    std::string content = entry.readAsText();
                    std::ofstream outputFile(outputPath, std::ios::binary);
                    if (outputFile) {
                        outputFile.write(content.c_str(), content.size());
                        outputFile.close();
                    }
                    else {
                        std::cerr << "Failed to write file: " << outputPath << std::endl;
                    }
                }
            }
        }

        zip.close();

        return true;
    }

    bool decompress_zip_to_file(const std::string& zipPath, const std::string& destination) {
        using namespace libzippp;

        ZipArchive zip(zipPath);
        if (!zip.open(ZipArchive::ReadOnly)) {
            std::cerr << "Failed to open zip archive: " << zipPath << std::endl;
            return false;
        }

        int nbEntries = zip.getNbEntries();

        if (nbEntries != 1) {
            std::cerr << "Zip archive must contain only one file" << std::endl;
            zip.close();
            return false;
        }

        ZipEntry entry = zip.getEntry(0);
        if (!entry.isNull() && !entry.isDirectory()) {
            std::string content = entry.readAsText();
            std::ofstream outputFile(destination, std::ios::binary);
            if (outputFile) {
                outputFile.write(content.c_str(), content.size());
                outputFile.close();
            }
            else {
                std::cerr << "Failed to write file: " << destination << std::endl;
                zip.close();
                return false;
            }
        }
        else {
            std::cerr << "Invalid zip entry" << std::endl;
            zip.close();
            return false;
        }

        zip.close();

        return true;
    }

}

namespace ui
{
    static const char* DISALLOWED_CHARS = "<>:\"/\\|?*\t\n\r ";

    static int _MyResizeCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            std::string* my_str = (std::string*)data->UserData;
            IM_ASSERT(&my_str->front() == data->Buf);
            my_str->resize(data->BufSize - 1);
            data->Buf = &my_str->front();
        }
        return 0;
    }

    static int _WindowsNameFilter(ImGuiInputTextCallbackData* data)
    {
        if (strchr(DISALLOWED_CHARS, (char)data->EventChar))
            return 1;
        return 0;
    }

    static bool InputTextWithHint(const char* label, const char* hint, std::string* my_str, ImGuiInputTextFlags flags = 0)
    {
        IM_ASSERT((flags & (ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCharFilter)) == 0);
        // Combine the resize callback and character filter callback
        auto combinedCallback = [](ImGuiInputTextCallbackData* data) -> int
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
                return _MyResizeCallback(data);
            else if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
                return _WindowsNameFilter(data);
            return 0;
        };

        return ImGui::InputTextWithHint(label, hint, &my_str->front(), my_str->size() + 1, flags | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCharFilter, combinedCallback, (void*)my_str);
    }

}

namespace Native
{
    bool enable_developer_mode()
    {
        HKEY hKey = nullptr;
        const wchar_t* subkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock";
        DWORD allowAllTrustedApps = 1;
        DWORD disposition;

        LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, &disposition);
        if (result != ERROR_SUCCESS)
        {
            std::wcerr << L"Failed to open or create the registry key. Error code: " << result << std::endl;
            return false;
        }

        result = RegSetValueExW(hKey, L"AllowAllTrustedApps", 0, REG_DWORD, (const BYTE*)&allowAllTrustedApps, sizeof(DWORD));
        if (result != ERROR_SUCCESS)
        {
            std::wcerr << L"Failed to set the registry value. Error code: " << result << std::endl;
            RegCloseKey(hKey);
            return false;
        }

        RegCloseKey(hKey);
        return true;
    }


    std::string run_powershell_command(const std::string& command) noexcept {
        SECURITY_ATTRIBUTES security_attributes;
        ZeroMemory(&security_attributes, sizeof(security_attributes));
        security_attributes.nLength = sizeof(security_attributes);
        security_attributes.bInheritHandle = TRUE;

        HANDLE stdout_read = NULL;
        HANDLE stdout_write = NULL;

        if (!CreatePipe(&stdout_read, &stdout_write, &security_attributes, 0)) {
            std::cerr << "Failed to create pipe" << std::endl;
            return "";
        }

        STARTUPINFO startup_info;
        ZeroMemory(&startup_info, sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);
        startup_info.hStdError = stdout_write;
        startup_info.hStdOutput = stdout_write;
        startup_info.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION process_info;
        ZeroMemory(&process_info, sizeof(process_info));

        std::string cmd_line = "powershell.exe -Command \"" + command + "\"";
        if (!CreateProcess(NULL, &cmd_line[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_info)) {
            std::cerr << "Failed to create process" << std::endl;
            return "";
        }

        CloseHandle(stdout_write);

        constexpr size_t buffer_size = 4096;
        std::array<char, buffer_size> buffer;
        std::string output;

        DWORD bytes_read;
        while (ReadFile(stdout_read, buffer.data(), buffer_size, &bytes_read, NULL)) {
            output.append(buffer.data(), bytes_read);
        }

        CloseHandle(stdout_read);
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);

        return output;
    }
}

namespace StringUtils
{
    inline bool contains_only(const std::string& s, char c) {
        return s.find_first_not_of(c) == std::string::npos;
    }

    std::vector<std::pair<std::string, std::string>> extract_paths_and_folders(const std::string& input) noexcept {
        std::vector<std::pair<std::string, std::string>> namesAndPaths;

        std::regex pattern(R"(Name\s*:\s*(.+)\s*InstallLocation\s*:\s*(.+))");
        std::smatch match;

        std::string::const_iterator searchStart(input.cbegin());

        while (std::regex_search(searchStart, input.cend(), match, pattern)) {
            std::string name = match[1].str();
            std::string path = match[2].str();

            namesAndPaths.push_back(std::make_pair(name, path));

            searchStart = match.suffix().first;
        }

        return namesAndPaths;
    }

    std::string replace_all(std::string str, const std::string& from, const std::string& to) {
        size_t startPos = 0;
        while ((startPos = str.find(from, startPos)) != std::string::npos) {
            str.replace(startPos, from.length(), to);
            startPos += to.length();  // Move past the replaced part in case 'to' contains 'from'
        }
        return str;
    }

    bool copy_to_clipboard(const std::string& data) {
        if (!OpenClipboard(nullptr)) {
            return false;
        }

        EmptyClipboard();

        // +1 for the null terminator
        size_t size = (data.size() + 1) * sizeof(char);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hMem) {
            CloseClipboard();
            return false;
        }

        // Copy the string to the allocated memory
        char* pMem = static_cast<char*>(GlobalLock(hMem));
        if (pMem) {
            strcpy(pMem, data.c_str());
            GlobalUnlock(hMem);
        }
        else {
            GlobalFree(hMem);
            CloseClipboard();
            return false;
        }

        SetClipboardData(CF_TEXT, hMem);
        if (GetLastError() != ERROR_SUCCESS) {
            GlobalFree(hMem);
            CloseClipboard();
            return false;
        }

        CloseClipboard();

        // Do not free the hMem after SetClipboardData, as the system will take ownership of it.
        return true;
    }

}

namespace Roblox
{
    void nuke_instance(const std::string& name, const std::string& path)
    {
        std::string cmd = "Get-AppxPackage -Name \"" + name + "\" | Remove-AppxPackage";
        Native::run_powershell_command(cmd);

        FS::remove_path(path);
    }
}