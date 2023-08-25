#define NOMINMAX
#include "Functions.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <fmt/format.h>
#include <cpr/cpr.h>
#include <tinyxml2.h>
#include <nlohmann/json.hpp>
#include "md5.h"
#include <map>
#include "request.hpp"
#include "AppLog.hpp"
#include "InstanceControl.h"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace FS
{
    bool CopyDirectory(const std::filesystem::path& src, const std::filesystem::path& dst)
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

    bool RemovePath(const std::filesystem::path& path_to_delete)
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

    bool DecompressZip(const std::string& zipPath, const std::string& destination) {
        using namespace libzippp;

        ZipArchive zip(zipPath);
        if (!zip.open(ZipArchive::ReadOnly)) {
            AppLog::GetInstance().addLog("Failed to open zip archive: {}", zipPath);
            return false;
        }


        auto nbEntries = zip.getNbEntries();
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
                        AppLog::GetInstance().addLog("Failed to write file: {}", outputPath);
                    }
                }
            }
        }

        zip.close();

        return true;
    }

    bool DecompressZipToFile(const std::string& zipPath, const std::string& destination) {
        using namespace libzippp;

        ZipArchive zip(zipPath);
        if (!zip.open(ZipArchive::ReadOnly)) {
            AppLog::GetInstance().addLog("Failed to open zip archive: {}", zipPath);
            return false;
        }

        auto nbEntries = zip.getNbEntries();

        if (nbEntries != 1) {
            AppLog::GetInstance().addLog("Zip archive must contain only one file: {}", zipPath);
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
                AppLog::GetInstance().addLog("Failed to write file: {}", destination);
                zip.close();
                return false;
            }
        }
        else {
            AppLog::GetInstance().addLog("Invalid zip entry: {}", zipPath);
            zip.close();
            return false;
        }

        zip.close();

        return true;
    }

    std::vector<std::string> FindFiles(const std::string& path, const std::string& substring) {
        std::vector<std::string> result;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if ((entry.is_regular_file() || entry.is_directory()) && entry.path().string().find(substring) != std::string::npos) {
                    result.push_back(entry.path().string());
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        return result;
    }

}

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

    // Check for underscore and replace with hyphen
    if (data->EventChar == '_')
    {
        data->EventChar = '-';
        return 0; // Allow the new character to pass through
    }

    return 0;
}

namespace ui
{
    bool InputTextWithHint(const char* label, const char* hint, std::string* my_str, ImGuiInputTextFlags flags)
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

        if (my_str->empty()) {
            my_str->resize(1);
        }
        bool result = ImGui::InputTextWithHint(label, hint, &my_str->front(), my_str->size() + 1, flags | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCharFilter, combinedCallback, (void*)my_str);

        // Resize the string to its actual length after the InputTextWithHint call
        size_t actual_length = strlen(&my_str->front());
        my_str->resize(actual_length);

        return result;
    }


    bool ConditionalButton(const char* label, bool condition, ButtonStyle style)
    {
        bool beginDisabledCalled = false;

        if (!condition) {
            ImGui::BeginDisabled(true);
            beginDisabledCalled = true;
        }

        switch (style) {
        case ButtonStyle::Red:
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(82, 21, 21, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 25, 25, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(123, 33, 33, 255));
            break;
        case ButtonStyle::Green:
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(21, 82, 21, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(25, 92, 25, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(33, 123, 33, 255));
            break;
        }

        bool pressed = ImGui::Button(label);

        ImGui::PopStyleColor(3);

        if (beginDisabledCalled) {
            ImGui::EndDisabled();
        }

        return pressed;
    }

    bool GreenButton(const char* label)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(21, 82, 21, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(25, 92, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(33, 123, 33, 255));

        bool pressed = ImGui::Button(label);

        ImGui::PopStyleColor(3);

        return pressed;
    }

    bool RedButton(const char* label)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(82, 21, 21, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(123, 33, 33, 255));

        bool pressed = ImGui::Button(label);

        ImGui::PopStyleColor(3);

        return pressed;
    }

    void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool BeginSizedListBox(const char* label, float width_ratio, float height_ratio)
    {
        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x * width_ratio, ImGui::GetContentRegionAvail().y * height_ratio);
        return ImGui::BeginListBox(label, size);
    }

    unsigned int ImVec4ToUint32(const ImVec4& color)
    {
        uint8_t r = static_cast<uint8_t>(color.x * 255.0f);
        uint8_t g = static_cast<uint8_t>(color.y * 255.0f);
        uint8_t b = static_cast<uint8_t>(color.z * 255.0f);
        uint8_t a = static_cast<uint8_t>(color.w * 255.0f);
        return (a << 24) | (b << 16) | (g << 8) | r;
    }
}


namespace Native
{
    std::string RunPowershellCommand(const std::string& command) {
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

    std::string GetCurrentUsername() {
        char username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        if (GetUserName(username, &username_len)) {
            return std::string(username);
        }

        return "";
    }

    std::string GetUserSID() {
        HANDLE hToken = NULL;
        DWORD dwSize = 0;
        TOKEN_USER* pTokenUser = NULL;
        LPTSTR StringSid = NULL;
        std::string sSid;

        auto Cleanup = [&]() {
            if (StringSid) {
                LocalFree(StringSid);
            }
            if (pTokenUser) {
                free(pTokenUser);
            }
            if (hToken) {
                CloseHandle(hToken);
            }
        };

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            Cleanup();
            return sSid; // Return empty 
        }

        GetTokenInformation(hToken, TokenUser, NULL, dwSize, &dwSize);
        pTokenUser = (TOKEN_USER*)malloc(dwSize);

        if (pTokenUser == NULL) {
            Cleanup();
            return sSid; // Return empty
        }

        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
            Cleanup();
            return sSid; // Return empty
        }

        if (!ConvertSidToStringSid(pTokenUser->User.Sid, &StringSid)) {
            Cleanup();
            return sSid; // Return empty
        }

        sSid = StringSid;

        std::transform(sSid.begin(), sSid.end(), sSid.begin(), ::tolower);

        Cleanup();

        return sSid;
    }

    std::string GetUserExperience()
    {
        const std::wstring userExperience = L"User Choice set via Windows User Experience {D18B6DD5-6124-4341-9318-804003BAFA0B}";
        const std::wstring str1 = L"User Choice set via Windows User Experience";

        wchar_t systemPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_SYSTEMX86, NULL, 0, systemPath);
        std::wstring fullPath = std::wstring(systemPath) + L"\\Shell32.dll";

        std::ifstream file(fullPath, std::ios::binary);
        if (!file)
        {
            std::cerr << "Failed to open file." << std::endl;
            return StringUtils::WStrToStr(userExperience);
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> bytes(fileSize);
        file.read(bytes.data(), fileSize);
        file.close();

        std::wstring str2(reinterpret_cast<wchar_t*>(bytes.data()), fileSize / sizeof(wchar_t));

        size_t startIndex = str2.find(str1);
        if (startIndex != std::wstring::npos)
        {
            size_t endIndex = str2.find(L"}", startIndex);
            if (endIndex != std::wstring::npos)
            {
                return StringUtils::WStrToStr(str2.substr(startIndex, endIndex - startIndex + 1));
            }
        }

        return StringUtils::WStrToStr(userExperience);
    }

    std::string GetHexDatetime()
    {
        using namespace std::chrono;
        system_clock::time_point now = system_clock::now();
        time_t tt = system_clock::to_time_t(now);
        tm local_tm;
        localtime_s(&local_tm, &tt);

        local_tm.tm_sec = 0;

        tt = mktime(&local_tm);

        LONGLONG ll = Int32x32To64(tt, 10000000) + 116444736000000000;
        FILETIME ft;
        ft.dwLowDateTime = (DWORD)ll;
        ft.dwHighDateTime = ll >> 32;

        unsigned long long fileTime = ((unsigned long long)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
        unsigned long num1 = fileTime >> 32;
        unsigned long num2 = fileTime & 0xFFFFFFFF;

        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << num1
            << std::setw(8) << std::setfill('0') << num2;

        return ss.str();
    }

    void WriteProtocolKeys(const std::string& progId, const std::string& protocol, const std::string& progHash)
    {
        HKEY hKey;
        std::string keyName = "Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\" + protocol + "\\UserChoice";
        DWORD disposition;

        if (RegCreateKeyExA(HKEY_CURRENT_USER, keyName.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, &disposition) == ERROR_SUCCESS)
        {
            RegSetKeyValueA(hKey, nullptr, "Hash", REG_SZ, progHash.c_str(), progHash.size() + 1);

            RegSetKeyValueA(hKey, nullptr, "ProgId", REG_SZ, progId.c_str(), progId.size() + 1);

            RegCloseKey(hKey);
        }
    }

    std::map<std::string, std::string> GetProgIDNames()
    {
        HKEY hKey;
        LONG lResult;
        std::map<std::string, std::string> ProgIdNames;

        // Open the registry key
        lResult = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes"), 0, KEY_READ, &hKey);
        if (lResult != ERROR_SUCCESS) {
            // Handle error
            return ProgIdNames; // Return an empty map
        }

        DWORD dwIndex = 0; // Index of the subkey.
        DWORD dwSize = MAX_PATH; // Size of name buffer.
        TCHAR szSubKeyName[MAX_PATH]; // Buffer for subkey name.

        while (RegEnumKeyEx(hKey, dwIndex, szSubKeyName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY hSubKey;
            // Open the subkey
            if (RegOpenKeyEx(hKey, szSubKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                HKEY hApplicationKey, hShellOpenKey;
                // Check for the presence of "Application" subkey
                if (RegOpenKeyEx(hSubKey, TEXT("Application"), 0, KEY_READ, &hApplicationKey) == ERROR_SUCCESS) {
                    // Check for the presence of "shell\open" subkey
                    if (RegOpenKeyEx(hSubKey, TEXT("shell\\open"), 0, KEY_READ, &hShellOpenKey) == ERROR_SUCCESS) {
                        // Check for the presence of "PackageId" value
                        TCHAR szPackageId[MAX_PATH];
                        DWORD dwBufferSize = sizeof(szPackageId);
                        if (RegQueryValueEx(hShellOpenKey, TEXT("PackageId"), NULL, NULL, (LPBYTE)szPackageId, &dwBufferSize) == ERROR_SUCCESS) {
                            std::string key = szPackageId;
                            if (ProgIdNames.find(key) == ProgIdNames.end()) {
                                std::string value = szSubKeyName;
                                ProgIdNames[key] = value.substr(value.find_last_of('\\') + 1);
                            }
                        }
                        RegCloseKey(hShellOpenKey);
                    }
                    RegCloseKey(hApplicationKey);
                }
                RegCloseKey(hSubKey);
            }

            // Reset variables for next iteration
            dwSize = MAX_PATH;
            dwIndex++;
        }

        // Close the main registry key
        RegCloseKey(hKey);

        return ProgIdNames;
    }

    PVOID GetPebAddress(HANDLE ProcessHandle)
    {
        _NtQueryInformationProcess NtQueryInformationProcess =
            (_NtQueryInformationProcess)GetProcAddress(
                GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
        PROCESS_BASIC_INFORMATION pbi;

        NtQueryInformationProcess(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);

        return pbi.PebBaseAddress;
    }

    std::string get_commandline_arguments(DWORD pid)
    {
        HANDLE processHandle;
        PVOID pebAddress;
        PVOID rtlUserProcParamsAddress;
        UNICODE_STRING commandLine;
        WCHAR* commandLineContents;

        if ((processHandle = OpenProcess(
            PROCESS_QUERY_INFORMATION | /* required for NtQueryInformationProcess */
            PROCESS_VM_READ, /* required for ReadProcessMemory */
            FALSE, pid)) == 0)
        {
            return std::string();
        }

        pebAddress = GetPebAddress(processHandle);

        if (!ReadProcessMemory(processHandle,
            &(((_PEB*)pebAddress)->ProcessParameters),
            &rtlUserProcParamsAddress,
            sizeof(PVOID), NULL))
        {
            return std::string();
        }

        if (!ReadProcessMemory(processHandle,
            &(((_RTL_USER_PROCESS_PARAMETERS*)rtlUserProcParamsAddress)->CommandLine),
            &commandLine, sizeof(commandLine), NULL))
        {
            return std::string();
        }

        commandLineContents = (WCHAR*)malloc(commandLine.Length);

        if (!ReadProcessMemory(processHandle, commandLine.Buffer,
            commandLineContents, commandLine.Length, NULL))
        {
            return std::string();
        }

        std::string command_line_str(commandLineContents, commandLineContents + commandLine.Length / 2);
        CloseHandle(processHandle);
        free(commandLineContents);

        return command_line_str;
    }

    bool OpenInExplorer(const std::string& path, bool isFile) {
        std::string cmd = isFile ? "/select," + path : path;
        HINSTANCE result = ShellExecute(0, "open", "explorer.exe", cmd.c_str(), 0, SW_SHOW);
        return (int)result > 32;
    }

    bool SetProcessAffinity(DWORD processID, DWORD requestedCores)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        DWORD numberOfCores = sysInfo.dwNumberOfProcessors;

        if (requestedCores <= 0 || requestedCores > numberOfCores) {
            std::cerr << "Invalid number of cores requested." << std::endl;
            return false;
        }

        DWORD_PTR mask = 0;
        for (DWORD i = 0; i < requestedCores; i++) {
            mask |= (1 << i);
        }

        HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processID);
        if (hProcess == NULL) {
            std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl;
            return false;
        }

        if (SetProcessAffinityMask(hProcess, mask) == 0) {
            std::cerr << "Failed to set process affinity. Error code: " << GetLastError() << std::endl;
            CloseHandle(hProcess);
            return false;
        }

        CloseHandle(hProcess);
        return true;
    }

    bool IsReadableMemory(const MEMORY_BASIC_INFORMATION& mbi) {
        return mbi.State == MEM_COMMIT &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD);
    }

    bool IsProcessRunning(DWORD targetPid, const CHAR* expectedName)
    {
        DWORD aProcesses[1024], cbNeeded, cProcesses;

        if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
            return false;
        }

        cProcesses = cbNeeded / sizeof(DWORD);

        for (unsigned int i = 0; i < cProcesses; i++) {
            if (aProcesses[i] != targetPid) {
                continue;
            }

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetPid);
            if (NULL == hProcess) {
                return false;
            }

            CHAR szProcessName[MAX_PATH] = "<unknown>";
            HMODULE hMod;

            if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(CHAR));
            }

            CloseHandle(hProcess);

            if (strcmp(szProcessName, expectedName) == 0) {
                return true;
            }
            else {
                return false;
            }
        }


        return false;
    }

    std::string SearchEntireProcessMemory(HANDLE pHandle, const unsigned char* pattern, size_t patternSize, ExtractFunction extractFunction) {
        uintptr_t address = 0;
        MEMORY_BASIC_INFORMATION mbi = {};

        while (VirtualQueryEx(pHandle, (void*)address, &mbi, sizeof(mbi))) {
            if (IsReadableMemory(mbi)) {
                std::vector<unsigned char> buffer(mbi.RegionSize);

                if (ReadProcessMemory(pHandle, (void*)address, buffer.data(), mbi.RegionSize, nullptr)) {
                    uintptr_t offset = Utils::BoyerMooreHorspool(pattern, patternSize, buffer.data(), mbi.RegionSize);
                    if (offset != 0) {
                        std::string value = extractFunction(buffer.data() + offset + patternSize, mbi.RegionSize - offset - patternSize);
                        if (!value.empty()) {
                            return value;
                        }
                    }
                }
            }

            // Move to the next memory region
            address += mbi.RegionSize;
        }

        return "";
    }
}

namespace StringUtils
{
    bool contains_only(const std::string& s, char c) {
        return s.find_first_not_of(c) == std::string::npos;
    }

    bool CopyToClipboard(const std::string& data) {
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
            std::copy(data.begin(), data.end(), pMem);
            pMem[data.size()] = '\0'; // null terminator
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

    std::string Base64Encode(const unsigned char* buffer, size_t length)
    {
        BIO* bio, * b64;
        long len;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, buffer, length);
        BIO_flush(bio);

        len = BIO_get_mem_data(bio, &buffer);

        std::string result(reinterpret_cast<const char*>(buffer), len);

        BIO_free_all(bio);
        return result;
    }

    std::string WStrToStr(const std::wstring& wstr)
    {
        int numBytes = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::vector<char> buffer(numBytes);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer.data(), numBytes, NULL, NULL);
        return std::string(buffer.data());
    }

}

namespace Roblox
{
    void NukeInstane(const std::string name, const std::string path)
    {
        std::string cmd = "Get-AppxPackage -Name \"" + name + "\" | Remove-AppxPackage";
        Native::RunPowershellCommand(cmd);

        std::thread([path]() {
            FS::RemovePath(path);
        }).detach();
    }

    std::unordered_map<std::string, Roblox::Instance> ProcessRobloxPackages()
    {
        auto ProgIdNames = Native::GetProgIDNames();
        std::unordered_map<std::string, Roblox::Instance> userInstancesMap;

        // Run the PowerShell command
        std::string output = Native::RunPowershellCommand("Get-AppxPackage ROBLOXCORPORATION.ROBLOX.* | Format-List -Property Name, PackageFullName, InstallLocation, PackageFamilyName, Version");

        // Split the output into lines
        std::stringstream ss(output);
        std::string line;

        std::regex pattern(R"(Name\s*:\s*(.+)\s*PackageFullName\s*:\s*(.+)\s*InstallLocation\s*:\s*([^\n]+(?:\n\s{20}[^\n]+)*)\s*PackageFamilyName\s*:\s*(.+)\s*Version\s*:\s*(.+))");
        std::smatch match;

        std::string::const_iterator searchStart(output.cbegin());

        while (std::regex_search(searchStart, output.cend(), match, pattern)) {
            std::string name = match[1].str();
            std::string packageFullName = match[2].str();
            std::string installLocationRaw = match[3].str();
            std::string installLocation = std::regex_replace(installLocationRaw, std::regex("\n\\s{20}"), "");
            installLocation.erase(std::remove(installLocation.begin(), installLocation.end(), '\r'), installLocation.end());
            std::string packageFamilyName = match[4].str();
            std::string version = match[5].str();

            std::regex nameRegex("\\.ROBLOX\\.([\\w+-]+)");
            std::smatch nameMatch;

            if (std::regex_search(name, nameMatch, nameRegex) && nameMatch.size() == 2) {
                std::string username = nameMatch[1].str();

                if (ProgIdNames.find(packageFullName) != ProgIdNames.end() && userInstancesMap.find(username) == userInstancesMap.end()) {
                    Instance instance;
                    instance.Name = name;
                    instance.PackageID = packageFullName;
                    instance.AppID = ProgIdNames.at(packageFullName);
                    instance.InstallLocation = installLocation;
                    instance.PackageFamilyName = packageFamilyName;
                    instance.Version = version;
                    userInstancesMap[username] = instance;
                }
            }

            searchStart = match.suffix().first;
        }

        return userInstancesMap;
    }

    /*
    std::vector<RobloxInstance> WrapPackages()
    {
        std::vector<RobloxPackage> packages = ProcessRobloxPackages();

        std::vector<RobloxInstance> instances;

        for (const auto& package : packages) {
            RobloxInstance instance;
            instance.Package.Name = package.Name;
            instance.Package.Username = package.Username;
            instance.Package.PackageID = package.PackageID;
            instance.Package.AppID = package.AppID;
            instance.Package.InstallLocation = package.InstallLocation;
            instance.Package.PackageFamilyName = package.PackageFamilyName;
            instance.Package.Version = package.Version;
            instance.ProcessID = 0;
            instances.push_back(instance);
        }

        return instances;
    }
    */

    DWORD LaunchRoblox(std::string AppID, std::string Username, const std::string& placeid)
    {
        std::string userSid = Native::GetUserSID();
        std::string userExperience = Native::GetUserExperience();
        std::string hexDateTime = Native::GetHexDatetime();

        static std::string protocol = "roblox";

        std::string lower = protocol + userSid + AppID + hexDateTime + userExperience;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        Native::WriteProtocolKeys(AppID, protocol, Utils::GetHash(lower));
        //call SHChangeNotify
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

        system(fmt::format("start roblox://placeId={}/", placeid).c_str());

        for (int i = 0; i < 20; i++)
        {
            auto pids = Roblox::GetRobloxInstances();
            for (auto pid : pids)
            {
                std::string command_line = Native::get_commandline_arguments(pid);
                if (command_line.find(Username) != std::string::npos)
                {
                    return pid;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    DWORD LaunchRoblox(std::string AppID, std::string Username, const std::string& placeid, const std::string& linkcode)
    {
        std::string userSid = Native::GetUserSID();
        std::string userExperience = Native::GetUserExperience();
        std::string hexDateTime = Native::GetHexDatetime();

        static std::string protocol = "roblox";

        std::string lower = protocol + userSid + AppID + hexDateTime + userExperience;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        Native::WriteProtocolKeys(AppID, protocol, Utils::GetHash(lower));
        //call SHChangeNotify
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

        std::string cmd = fmt::format("start roblox://placeId={}^&linkCode={}/", placeid, linkcode);
        system(cmd.c_str());

        for (int i = 0; i < 5; i++)
        {
            auto pids = Roblox::GetRobloxInstances();
            for (auto pid : pids)
            {
                std::string command_line = Native::get_commandline_arguments(pid);
                if (command_line.find(Username) != std::string::npos)
                {
                    return pid;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    std::set<DWORD> GetRobloxInstances()
    {
        std::set<DWORD> pidSet;
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry) == TRUE)
        {
            while (Process32Next(snapshot, &entry) == TRUE)
            {
                if (strcmp(entry.szExeFile, "Windows10Universal.exe") == 0)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                    BOOL debugged = FALSE;
                    CheckRemoteDebuggerPresent(hProcess, &debugged);
                    if (debugged)
                    {
                        TerminateProcess(hProcess, 9);
                    }
                    else
                    {
                        pidSet.insert(entry.th32ProcessID);
                    }
                    CloseHandle(hProcess);
                }
            }
        }

        CloseHandle(snapshot);

        return pidSet;
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

namespace Utils
{
    std::vector<unsigned char> ParsePattern(const std::string& patternString) {
        std::vector<unsigned char> pattern;
        size_t start = 0;
        size_t end = patternString.find(' ');

        while (end != std::string::npos) {
            std::string byteString = patternString.substr(start, end - start);
            if (byteString == "??") {
                pattern.push_back('?');
            }
            else {
                unsigned long byteValue = std::strtol(byteString.c_str(), nullptr, 16);
                pattern.push_back(static_cast<unsigned char>(byteValue));
            }
            start = end + 1;
            end = patternString.find(' ', start);
        }

        // Handle the last token in the string (after the last space)
        std::string byteString = patternString.substr(start, end);
        if (byteString == "??") {
            pattern.push_back('?');
        }
        else {
            unsigned long byteValue = std::strtol(byteString.c_str(), nullptr, 16);
            pattern.push_back(static_cast<unsigned char>(byteValue));
        }

        return pattern;
    }



    long GetShiftRight(long value, int count)
    {
        if ((value & 0x80000000) == 0)
        {
            return (value >> count);
        }
        else // Negative number
        {
            return (value >> count) ^ 0xFFFF0000;
        }
    }

    //this is some Cnile shit, gotta fix it
    int ToInt32(const unsigned char* bytes, int offset)
    {
        return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    const unsigned char* ToBytes(int64_t value)
    {
        static unsigned char bytes[8];
        bytes[0] = value & 0xFF;
        bytes[1] = (value >> 8) & 0xFF;
        bytes[2] = (value >> 16) & 0xFF;
        bytes[3] = (value >> 24) & 0xFF;
        bytes[4] = (value >> 32) & 0xFF;
        bytes[5] = (value >> 40) & 0xFF;
        bytes[6] = (value >> 48) & 0xFF;
        bytes[7] = (value >> 56) & 0xFF;
        return bytes;
    }

    const unsigned char* TakeBytes(const unsigned char* input, int count)
    {
        static unsigned char result[8];
        for (int i = 0; i < count && i < 8; ++i) {
            result[i] = input[i];
        }
        return result;
    }

    class HashMap {
    public:
        long PDATA = 0, CACHE = 0, COUNTER = 0, INDEX = 0,
            MD51 = 0, MD52 = 0, OUTHASH1 = 0, OUTHASH2 = 0,
            R0 = 0, R3 = 0;
        std::map<int, long> R1 = { {0, 0}, {1, 0} },
            R2 = { {0, 0}, {1, 0} },
            R4 = { {0, 0}, {1, 0} },
            R5 = { {0, 0}, {1, 0} },
            R6 = { {0, 0}, {1, 0} };

        void reset() {
            PDATA = 0;
            CACHE = 0;
            COUNTER = 0;
            INDEX = 0;
            MD51 = 0;
            MD52 = 0;
            OUTHASH1 = 0;
            OUTHASH2 = 0;
            R0 = 0;
            R3 = 0;

            R1 = { {0, 0}, {1, 0} };
            R2 = { {0, 0}, {1, 0} };
            R4 = { {0, 0}, {1, 0} };
            R5 = { {0, 0}, {1, 0} };
            R6 = { {0, 0}, {1, 0} };
        }
    };

    std::string GetHash(const std::string& baseInfo)
    {
        std::vector<unsigned char> bytes(baseInfo.begin(), baseInfo.end());

        for (int i = 0; i < bytes.size(); i += 2) {
            bytes.insert(bytes.begin() + i + 1, 0x00);
        }

        bytes.push_back(0x00);
        bytes.push_back(0x00);

        Chocobo1::MD5 md5;
        md5.addData(reinterpret_cast<char*>(&bytes[0]), bytes.size());
        md5.finalize();
        std::array<unsigned char, 16> bytesn = md5.toArray();
        unsigned char md5Result[16];
        std::copy(bytesn.begin(), bytesn.end(), md5Result);


        int lengthBase = baseInfo.length() * 2 + 2;
        int length = (((lengthBase & 4) <= 1) ? 1 : 0) + Utils::GetShiftRight(lengthBase, 2) - 1;
        std::string base64Hash;

        if (length > 1) {
            HashMap map;
            map.CACHE = 0;
            map.OUTHASH1 = 0;
            map.PDATA = 0;
            map.MD51 = (ToInt32(md5Result, 0) | 1) + static_cast<int>(0x69FB0000L);
            map.MD52 = (ToInt32(md5Result, 4) | 1) + static_cast<int>(0x13DB0000L);
            map.INDEX = static_cast<int>(GetShiftRight(length - 2, 1));
            map.COUNTER = map.INDEX + 1;

            while (map.COUNTER > 0) {
                map.R0 = ToInt32(ToBytes(ToInt32(bytes.data(), static_cast<int>(map.PDATA)) + map.OUTHASH1), 0);
                map.R1[0] = ToInt32(ToBytes(ToInt32(bytes.data(), static_cast<int>(map.PDATA) + 4)), 0);
                map.PDATA = map.PDATA + 8;
                map.R2[0] = ToInt32(ToBytes((map.R0 * map.MD51) - (0x10FA9605L * GetShiftRight(map.R0, 16))), 0);
                map.R2[1] = ToInt32(ToBytes((0x79F8A395L * map.R2[0]) + (0x689B6B9FL * GetShiftRight(map.R2[0], 16))), 0);
                map.R3 = ToInt32(ToBytes((0xEA970001L * map.R2[1]) - (0x3C101569L * GetShiftRight(map.R2[1], 16))), 0);
                map.R4[0] = ToInt32(ToBytes(map.R3 + map.R1[0]), 0);
                map.R5[0] = ToInt32(ToBytes(map.CACHE + map.R3), 0);
                map.R6[0] = ToInt32(ToBytes((map.R4[0] * map.MD52) - (0x3CE8EC25L * GetShiftRight(map.R4[0], 16))), 0);
                map.R6[1] = ToInt32(ToBytes((0x59C3AF2DL * map.R6[0]) - (0x2232E0F1L * GetShiftRight(map.R6[0], 16))), 0);
                map.OUTHASH1 = ToInt32(ToBytes((0x1EC90001L * map.R6[1]) + (0x35BD1EC9L * GetShiftRight(map.R6[1], 16))), 0);
                map.OUTHASH2 = ToInt32(ToBytes(map.R5[0] + map.OUTHASH1), 0);
                map.CACHE = map.OUTHASH2;
                map.COUNTER = map.COUNTER - 1;
            }

            std::vector<unsigned char> outHash(16, 0x00);
            auto fullbuffer = ToBytes(map.OUTHASH1);
            auto buffer = TakeBytes(fullbuffer, 4);
            std::copy(buffer, buffer + 4, outHash.begin());
            fullbuffer = ToBytes(map.OUTHASH2);
            buffer = TakeBytes(fullbuffer, 4);
            std::copy(buffer, buffer + 4, outHash.begin() + 4);

            map.reset();

            map.CACHE = 0;
            map.OUTHASH1 = 0;
            map.PDATA = 0;
            map.MD51 = ToInt32(md5Result, 0) | 1;
            map.MD52 = ToInt32(md5Result, 4) | 1;
            map.INDEX = static_cast<int>(GetShiftRight(length - 2, 1));
            map.COUNTER = map.INDEX + 1;
            while (map.COUNTER > 0)
            {
                map.R0 = ToInt32(bytes.data(), map.PDATA) + map.OUTHASH1;
                map.PDATA = map.PDATA + 8;
                map.R1[0] = ToInt32(ToBytes(map.R0 * map.MD51), 0);
                map.R1[1] = ToInt32(ToBytes((0xB1110000L * map.R1[0]) - (0x30674EEFL * GetShiftRight(map.R1[0], 16))), 0);
                map.R2[0] = ToInt32(ToBytes((0x5B9F0000L * map.R1[1]) - (0x78F7A461L * GetShiftRight(map.R1[1], 16))), 0);
                map.R2[1] = ToInt32(ToBytes((0x12CEB96DL * GetShiftRight(map.R2[0], 16)) - (0x46930000L * map.R2[0])), 0);
                map.R3 = ToInt32(ToBytes((0x1D830000L * map.R2[1]) + (0x257E1D83L * GetShiftRight(map.R2[1], 16))), 0);
                map.R4[0] = ToInt32(ToBytes(map.MD52 * (map.R3 + ToInt32(bytes.data(), map.PDATA - 4))), 0);
                map.R4[1] = ToInt32(ToBytes((0x16F50000L * map.R4[0]) - (0x5D8BE90BL * GetShiftRight(map.R4[0], 16))), 0);
                map.R5[0] = ToInt32(ToBytes((0x96FF0000L * map.R4[1]) - (0x2C7C6901L * GetShiftRight(map.R4[1], 16))), 0);
                map.R5[1] = ToInt32(ToBytes((0x2B890000L * map.R5[0]) + (0x7C932B89L * GetShiftRight(map.R5[0], 16))), 0);
                map.OUTHASH1 = ToInt32(ToBytes((0x9F690000L * map.R5[1]) - (0x405B6097L * GetShiftRight(map.R5[1], 16))), 0);
                map.OUTHASH2 = ToInt32(ToBytes(map.OUTHASH1 + map.CACHE + map.R3), 0);
                map.CACHE = map.OUTHASH2;
                map.COUNTER = map.COUNTER - 1;
            }

            buffer = ToBytes(map.OUTHASH1);
            std::copy(buffer, buffer + 4, outHash.begin() + 8);
            buffer = ToBytes(map.OUTHASH2);
            std::copy(buffer, buffer + 4, outHash.begin() + 12);

            std::vector<unsigned char> outHashBase(8, 0x00);
            int hashValue1 = ToInt32(outHash.data(), 8) ^ ToInt32(outHash.data(), 0);
            int hashValue2 = ToInt32(outHash.data(), 12) ^ ToInt32(outHash.data(), 4);

            buffer = ToBytes(hashValue1);
            std::copy(buffer, buffer + 4, outHashBase.begin());
            buffer = ToBytes(hashValue2);
            std::copy(buffer, buffer + 4, outHashBase.begin() + 4);
            base64Hash = StringUtils::Base64Encode(outHashBase.data(), outHashBase.size());

            return base64Hash;
        }
        else {
            throw std::runtime_error("Missing base info");
        }
    }

    bool SaveToFile(const std::string& file_path, const std::vector<char8_t>& buffer)
    {
        std::ofstream ofs(file_path, std::ios::out | std::ios::binary);

        if (!ofs.is_open())
            return false;

        ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        ofs.close();

        return true;
    }

    uintptr_t BoyerMooreHorspool(const unsigned char* signature, size_t signatureSize, const unsigned char* data, size_t dataSize)
    {
        size_t maxShift = signatureSize;
        size_t maxIndex = signatureSize - 1;
        size_t wildCardIndex = 0;
        for (size_t i = 0; i < maxIndex; i++) {
            if (signature[i] == '?') {
                maxShift = maxIndex - i;
                wildCardIndex = i;
            }
        }

        size_t shiftTable[256];
        for (size_t i = 0; i <= 255; i++) {
            shiftTable[i] = maxShift;
        }

        for (size_t i = wildCardIndex + 1; i < maxIndex - 1; i++) {
            shiftTable[signature[i]] = maxIndex - i;
        }

        for (size_t currentIndex = 0; currentIndex < dataSize - signatureSize;) {
            for (size_t sigIndex = maxIndex; sigIndex >= 0; sigIndex--) {
                if (data[currentIndex + sigIndex] != signature[sigIndex] && signature[sigIndex] != '?') {
                    currentIndex += shiftTable[data[currentIndex + maxIndex]];
                    break;
                }
                else if (sigIndex == 0) {
                    return currentIndex;
                }
            }
        }

        return 0;
    }

    void DownloadAndSave(const std::string& url, const std::string& localFileName) {
        std::vector<char8_t> buffer;
        Request req(url);
        req.initalize();
        req.download_file<char8_t>(buffer);
        Utils::SaveToFile(localFileName, buffer);
    }

    void DecompressZip(const std::string& zipFile, const std::string& destination) {
        if (std::filesystem::exists(destination)) {
            std::filesystem::remove(destination);
        }
        FS::DecompressZipToFile(zipFile, destination);
    }

    void CopyFileToDestination(const std::string& source, const std::string& destination) {
        if (std::filesystem::exists(destination)) {
            std::filesystem::remove(destination);
        }
        std::filesystem::copy_file(source, destination);
    }

    std::string ModifyAppxManifest(std::string inputXML, std::string name)
    {
        tinyxml2::XMLDocument doc;
        doc.Parse(inputXML.c_str());

        // <Identity> element's Name attribute
        tinyxml2::XMLElement* identityElement = doc.FirstChildElement("Package")->FirstChildElement("Identity");
        if (identityElement) {
            const char* nameValue = identityElement->Attribute("Name");
            if (nameValue) {
                std::string newNameValue = std::string(nameValue) + "." + name;
                identityElement->SetAttribute("Name", newNameValue.c_str());
            }
        }

        // <uap:VisualElements> tag
        tinyxml2::XMLElement* visualElement = doc.FirstChildElement("Package")->FirstChildElement("Applications")->FirstChildElement("Application")->FirstChildElement("uap:VisualElements");
        if (visualElement) {
            const char* displayNameValue = visualElement->Attribute("DisplayName");
            if (displayNameValue) {
                std::string newDisplayNameValue = std::string(displayNameValue) + " " + name;
                visualElement->SetAttribute("DisplayName", newDisplayNameValue.c_str());
            }
        }

        // <uap:DefaultTile> tag
        tinyxml2::XMLElement* defaultTile = visualElement->FirstChildElement("uap:DefaultTile");
        if (defaultTile) {
            const char* shortNameValue = defaultTile->Attribute("ShortName");
            if (shortNameValue) {
                std::string newShortNameValue = std::string(shortNameValue) + " " + name;
                defaultTile->SetAttribute("ShortName", newShortNameValue.c_str());
            }
        }

        // <DisplayName> tag under <Properties>
        tinyxml2::XMLElement* propertiesDisplayName = doc.FirstChildElement("Package")->FirstChildElement("Properties")->FirstChildElement("DisplayName");
        if (propertiesDisplayName) {
            const char* displayNameText = propertiesDisplayName->GetText();
            if (displayNameText) {
                std::string newDisplayNameText = std::string(displayNameText) + " " + name;
                propertiesDisplayName->SetText(newDisplayNameText.c_str());
            }
        }

        tinyxml2::XMLPrinter printer;
        doc.Print(&printer);
        return printer.CStr();
    }


    void WriteAppxManifest(const std::string& url, const std::string& localPath, const std::string name) {
        std::vector<char8_t> buffer;
        Request req(url);
        req.initalize();
        req.download_file<char8_t>(buffer);

        std::string strbuf(buffer.begin(), buffer.end());

        if (!name.empty())
        {
            strbuf = Utils::ModifyAppxManifest(strbuf, name);
        }


        std::ofstream ofs(localPath, std::ofstream::out | std::ofstream::trunc);
        ofs.write(strbuf.c_str(), strbuf.size());
        ofs.flush();
    }

    void UpdatePackage(const std::string& baseFolder, const std::string& instanceName) {
        // For Windows10Universal.zip
        std::thread win10t([&, baseFolder]() {
            DownloadAndSave("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Windows10Universal.zip", "Windows10Universal.zip");
            DecompressZip("Windows10Universal.zip", baseFolder + "\\Windows10Universal.exe");
        });

        // For CrashHandler.exe
        std::thread crasht([&, baseFolder]() {
            DownloadAndSave("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Assets/CrashHandler.exe", "CrashHandler.exe");
            CopyFileToDestination("CrashHandler.exe", baseFolder + "\\Assets\\CrashHandler.exe");
        });

        // For AppxManifest.xml
        std::thread appxt([&, baseFolder, instanceName]() {
            WriteAppxManifest("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/AppxManifest.xml", baseFolder + "\\AppxManifest.xml");
        });

        win10t.join();

        AppLog::GetInstance().addLog("Updated Windows10Universal");

        crasht.join();

        AppLog::GetInstance().addLog("Updated CrashHandler");

        appxt.join();

        AppLog::GetInstance().addLog("Updated AppxManifest");
    }

    bool SaveScreenshotAsPng(const char* filename)
    {
        HDC screenDC = GetDC(NULL);
        HDC memoryDC = CreateCompatibleDC(screenDC);

        // Get the dimensions of the entire virtual screen (all monitors)
        int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        HBITMAP bitmap = CreateCompatibleBitmap(screenDC, screenWidth, screenHeight);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memoryDC, bitmap);

        // Copy the entire virtual screen
        BitBlt(memoryDC, 0, 0, screenWidth, screenHeight, screenDC, screenX, screenY, SRCCOPY);

        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = screenWidth;
        bi.biHeight = -screenHeight; // Top-down orientation
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        DWORD dwBmpSize = ((screenWidth * bi.biBitCount + 31) / 32) * 4 * screenHeight;
        std::vector<BYTE> lpBitmapBits(dwBmpSize);
        GetDIBits(screenDC, bitmap, 0, screenHeight, lpBitmapBits.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        // Convert BGR to RGB
        for (size_t i = 0; i < dwBmpSize; i += 3)
        {
            std::swap(lpBitmapBits[i], lpBitmapBits[i + 2]);
        }

        SelectObject(memoryDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);

        int result = stbi_write_png(filename, screenWidth, screenHeight, 3, lpBitmapBits.data(), screenWidth * 3);
        return result != 0;
    }

}
