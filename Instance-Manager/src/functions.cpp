#define NOMINMAX
#include "functions.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <fmt/format.h>
#include <tinyxml2.h>
#include "md5.h"
#include <map>

namespace FS
{
    std::vector<std::string> enumerate_directories(const std::string& path, int depth, bool onlyNames) {
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

    std::string replace_pattern_in_file(const std::filesystem::path& filePath,
        const std::string& pattern,
        const std::string& replacement)
    {
        // Step 1: Read the file into a string
        std::ifstream inFile(filePath);
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string fileContent = buffer.str();
        inFile.close();

        std::cout << fileContent << std::endl;

        // Step 2: Replace the pattern with the replacement string
        size_t pos = 0;
        while ((pos = fileContent.find(pattern, pos)) != std::string::npos) {
            fileContent.replace(pos, pattern.size(), replacement);
            pos += replacement.size(); // Move the position after the replacement
        }

        std::cout << fileContent << std::endl;

        // Step 3: Return the modified content
        return fileContent;
    }

    std::string replace_pattern_in_content(const std::vector<char8_t>& contentVec, const std::string& pattern, const std::string& replacement)
    {
        // Step 1: Convert the vector to a string
        std::string fileContent(contentVec.begin(), contentVec.end());

        std::cout << fileContent << std::endl;

        // Step 2: Replace the pattern with the replacement string
        size_t pos = 0;
        while ((pos = fileContent.find(pattern, pos)) != std::string::npos) {
            fileContent.replace(pos, pattern.size(), replacement);
            pos += replacement.size(); // Move the position after the replacement
        }

        std::cout << fileContent << std::endl;

        // Step 3: Return the modified content
        return fileContent;
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

        auto nbEntries = zip.getNbEntries();

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

    std::vector<std::string> find_files(const std::string& path, const std::string& substring) {
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


    std::string run_powershell_command(const std::string& command) {
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

    std::string get_current_username() {
        char username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        if (GetUserName(username, &username_len)) {
            return std::string(username);
        }

        return "";
    }

    std::string get_user_sid() {
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

    std::string get_user_experience()
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
            return StringUtils::wstr_to_str(userExperience);
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
                return StringUtils::wstr_to_str(str2.substr(startIndex, endIndex - startIndex + 1));
            }
        }

        return StringUtils::wstr_to_str(userExperience);
    }

    std::string get_hex_datetime()
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

    void write_protocol_keys(const std::string& progId, const std::string& protocol, const std::string& progHash)
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

    std::map<std::string, std::string> get_progid_names()
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

    PVOID get_peb_address(HANDLE ProcessHandle)
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

        pebAddress = get_peb_address(processHandle);

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

    bool open_in_explorer(const std::string& path, bool isFile) noexcept {
        std::string cmd = isFile ? "/select," + path : path;
        HINSTANCE result = ShellExecute(0, "open", "explorer.exe", cmd.c_str(), 0, SW_SHOW);
        return (int)result > 32;
    }


    BOOL CALLBACK _enum_window_proc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            ShowWindow(hwnd, SW_MINIMIZE);
            return FALSE;
        }

        return TRUE;
    }

    void minimize_window(DWORD pid) {
        EnumWindows(_enum_window_proc, (LPARAM)pid);
    }

}

namespace StringUtils
{
    bool contains_only(const std::string& s, char c) {
        return s.find_first_not_of(c) == std::string::npos;
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

    std::string get_after_last_occurrence(const std::string& source, char ch) {
        size_t pos = source.rfind(ch);
        if (pos != std::string::npos) {
            return source.substr(pos + 1);
        }
        return "";
    }

    std::string base64_encode(const unsigned char* buffer, size_t length)
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

    std::string wstr_to_str(const std::wstring& wstr)
    {
        int numBytes = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::vector<char> buffer(numBytes);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer.data(), numBytes, NULL, NULL);
        return std::string(buffer.data());
    }

}

namespace Roblox
{
    void nuke_instance(const std::string name, const std::string path)
    {
        std::string cmd = "Get-AppxPackage -Name \"" + name + "\" | Remove-AppxPackage";
        Native::run_powershell_command(cmd);

        FS::remove_path(path);
    }

    std::vector<UserInstance> process_roblox_packages()
    {
        auto ProgIdNames = Native::get_progid_names();
        std::vector<UserInstance> userInstancesVec;

        // Run the PowerShell command
        std::string output = Native::run_powershell_command("Get-AppxPackage ROBLOXCORPORATION.ROBLOX.* | Format-List -Property Name, PackageFullName, InstallLocation, PackageFamilyName, Version");

        // Split the output into lines
        std::stringstream ss(output);
        std::string line;

        std::regex pattern(R"(Name\s*:\s*(.+)\s*PackageFullName\s*:\s*(.+)\s*InstallLocation\s*:\s*(.+)\s*PackageFamilyName\s*:\s*(.+)\s*Version\s*:\s*(.+))");
        std::smatch match;

        std::string::const_iterator searchStart(output.cbegin());

        while (std::regex_search(searchStart, output.cend(), match, pattern)) {
            std::string name = match[1].str();
            std::string packageFullName = match[2].str();
            std::string installLocation = match[3].str();
            std::string packageFamilyName = match[4].str();
            std::string version = match[5].str();

            std::regex nameRegex("\\.ROBLOX\\.([\\w+-]+)");
            std::smatch nameMatch;

            if (std::regex_search(name, nameMatch, nameRegex) && nameMatch.size() == 2) {
                std::string username = nameMatch[1].str();

                // Using a lambda to check if a user with the same username already exists in the vector
                auto userExists = [&userInstancesVec, &username]() {
                    return std::any_of(userInstancesVec.begin(), userInstancesVec.end(), [&username](const UserInstance& instance) {
                        return instance.Username == username;
                        });
                };

                if (ProgIdNames.find(packageFullName) != ProgIdNames.end() && !userExists()) {
                    UserInstance instance;
                    instance.Name = name;
                    instance.Username = username;
                    instance.PackageID = packageFullName;
                    instance.AppID = ProgIdNames.at(packageFullName);
                    instance.InstallLocation = installLocation;
                    instance.PackageFamilyName = packageFamilyName;
                    instance.Version = version;
                    userInstancesVec.push_back(instance);
                }
            }

            searchStart = match.suffix().first;
        }

        return userInstancesVec;
    }

    void launch_roblox(std::string AppID, const std::string& placeid)
    {
        std::string userSid = Native::get_user_sid();
        std::string userExperience = Native::get_user_experience();
        std::string hexDateTime = Native::get_hex_datetime();

        static std::string protocol = "roblox";

        std::string lower = protocol + userSid + AppID + hexDateTime + userExperience;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        Native::write_protocol_keys(AppID, protocol, Utils::get_hash(lower));
        //call SHChangeNotify
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

        system(fmt::format("start roblox://placeId={}/", placeid).c_str());
    }

    void launch_roblox(std::string AppID, const std::string& placeid, const std::string& linkcode)
    {
        std::string userSid = Native::get_user_sid();
        std::string userExperience = Native::get_user_experience();
        std::string hexDateTime = Native::get_hex_datetime();

        static std::string protocol = "roblox";

        std::string lower = protocol + userSid + AppID + hexDateTime + userExperience;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        Native::write_protocol_keys(AppID, protocol, Utils::get_hash(lower));
        //call SHChangeNotify
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

        //system(fmt::format("start roblox://placeId={}/", placeid).c_str());
        //roblox://placeId=2414851778&linkCode=99685865445527485243539729691730/
        std::string cmd = fmt::format("start roblox://placeId={}^&linkCode={}/", placeid, linkcode);
        system(cmd.c_str());
    }


    std::set<DWORD> get_roblox_instances()
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

    ModifyXMLError modify_settings(std::string filePath, int newGraphicsQualityValue, float newMasterVolumeValue, int newSavedQualityValue)
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

}

namespace Utils
{
    long get_shift_right(long value, int count)
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
    int to_int32(const unsigned char* bytes, int offset)
    {
        return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    const unsigned char* to_bytes(int64_t value)
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

    const unsigned char* take_bytes(const unsigned char* input, int count)
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

    std::string get_hash(const std::string& baseInfo)
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
        int length = (((lengthBase & 4) <= 1) ? 1 : 0) + Utils::get_shift_right(lengthBase, 2) - 1;
        std::string base64Hash;

        if (length > 1) {
            HashMap map;
            map.CACHE = 0;
            map.OUTHASH1 = 0;
            map.PDATA = 0;
            map.MD51 = (to_int32(md5Result, 0) | 1) + static_cast<int>(0x69FB0000L);
            map.MD52 = (to_int32(md5Result, 4) | 1) + static_cast<int>(0x13DB0000L);
            map.INDEX = static_cast<int>(get_shift_right(length - 2, 1));
            map.COUNTER = map.INDEX + 1;

            while (map.COUNTER > 0) {
                map.R0 = to_int32(to_bytes(to_int32(bytes.data(), static_cast<int>(map.PDATA)) + map.OUTHASH1), 0);
                map.R1[0] = to_int32(to_bytes(to_int32(bytes.data(), static_cast<int>(map.PDATA) + 4)), 0);
                map.PDATA = map.PDATA + 8;
                map.R2[0] = to_int32(to_bytes((map.R0 * map.MD51) - (0x10FA9605L * get_shift_right(map.R0, 16))), 0);
                map.R2[1] = to_int32(to_bytes((0x79F8A395L * map.R2[0]) + (0x689B6B9FL * get_shift_right(map.R2[0], 16))), 0);
                map.R3 = to_int32(to_bytes((0xEA970001L * map.R2[1]) - (0x3C101569L * get_shift_right(map.R2[1], 16))), 0);
                map.R4[0] = to_int32(to_bytes(map.R3 + map.R1[0]), 0);
                map.R5[0] = to_int32(to_bytes(map.CACHE + map.R3), 0);
                map.R6[0] = to_int32(to_bytes((map.R4[0] * map.MD52) - (0x3CE8EC25L * get_shift_right(map.R4[0], 16))), 0);
                map.R6[1] = to_int32(to_bytes((0x59C3AF2DL * map.R6[0]) - (0x2232E0F1L * get_shift_right(map.R6[0], 16))), 0);
                map.OUTHASH1 = to_int32(to_bytes((0x1EC90001L * map.R6[1]) + (0x35BD1EC9L * get_shift_right(map.R6[1], 16))), 0);
                map.OUTHASH2 = to_int32(to_bytes(map.R5[0] + map.OUTHASH1), 0);
                map.CACHE = map.OUTHASH2;
                map.COUNTER = map.COUNTER - 1;
            }

            std::vector<unsigned char> outHash(16, 0x00);
            auto fullbuffer = to_bytes(map.OUTHASH1);
            auto buffer = take_bytes(fullbuffer, 4);
            std::copy(buffer, buffer + 4, outHash.begin());
            fullbuffer = to_bytes(map.OUTHASH2);
            buffer = take_bytes(fullbuffer, 4);
            std::copy(buffer, buffer + 4, outHash.begin() + 4);

            map.reset();

            map.CACHE = 0;
            map.OUTHASH1 = 0;
            map.PDATA = 0;
            map.MD51 = to_int32(md5Result, 0) | 1;
            map.MD52 = to_int32(md5Result, 4) | 1;
            map.INDEX = static_cast<int>(get_shift_right(length - 2, 1));
            map.COUNTER = map.INDEX + 1;
            while (map.COUNTER > 0)
            {
                map.R0 = to_int32(bytes.data(), map.PDATA) + map.OUTHASH1;
                map.PDATA = map.PDATA + 8;
                map.R1[0] = to_int32(to_bytes(map.R0 * map.MD51), 0);
                map.R1[1] = to_int32(to_bytes((0xB1110000L * map.R1[0]) - (0x30674EEFL * get_shift_right(map.R1[0], 16))), 0);
                map.R2[0] = to_int32(to_bytes((0x5B9F0000L * map.R1[1]) - (0x78F7A461L * get_shift_right(map.R1[1], 16))), 0);
                map.R2[1] = to_int32(to_bytes((0x12CEB96DL * get_shift_right(map.R2[0], 16)) - (0x46930000L * map.R2[0])), 0);
                map.R3 = to_int32(to_bytes((0x1D830000L * map.R2[1]) + (0x257E1D83L * get_shift_right(map.R2[1], 16))), 0);
                map.R4[0] = to_int32(to_bytes(map.MD52 * (map.R3 + to_int32(bytes.data(), map.PDATA - 4))), 0);
                map.R4[1] = to_int32(to_bytes((0x16F50000L * map.R4[0]) - (0x5D8BE90BL * get_shift_right(map.R4[0], 16))), 0);
                map.R5[0] = to_int32(to_bytes((0x96FF0000L * map.R4[1]) - (0x2C7C6901L * get_shift_right(map.R4[1], 16))), 0);
                map.R5[1] = to_int32(to_bytes((0x2B890000L * map.R5[0]) + (0x7C932B89L * get_shift_right(map.R5[0], 16))), 0);
                map.OUTHASH1 = to_int32(to_bytes((0x9F690000L * map.R5[1]) - (0x405B6097L * get_shift_right(map.R5[1], 16))), 0);
                map.OUTHASH2 = to_int32(to_bytes(map.OUTHASH1 + map.CACHE + map.R3), 0);
                map.CACHE = map.OUTHASH2;
                map.COUNTER = map.COUNTER - 1;
            }

            buffer = to_bytes(map.OUTHASH1);
            std::copy(buffer, buffer + 4, outHash.begin() + 8);
            buffer = to_bytes(map.OUTHASH2);
            std::copy(buffer, buffer + 4, outHash.begin() + 12);

            std::vector<unsigned char> outHashBase(8, 0x00);
            int hashValue1 = to_int32(outHash.data(), 8) ^ to_int32(outHash.data(), 0);
            int hashValue2 = to_int32(outHash.data(), 12) ^ to_int32(outHash.data(), 4);

            buffer = to_bytes(hashValue1);
            std::copy(buffer, buffer + 4, outHashBase.begin());
            buffer = to_bytes(hashValue2);
            std::copy(buffer, buffer + 4, outHashBase.begin() + 4);
            base64Hash = StringUtils::base64_encode(outHashBase.data(), outHashBase.size());

            return base64Hash;
        }
        else {
            throw std::runtime_error("Missing base info");
        }
    }

    bool save_to_file(const std::string& file_path, const std::vector<char8_t>& buffer)
    {
        std::ofstream ofs(file_path, std::ios::out | std::ios::binary);

        if (!ofs.is_open())
            return false;

        ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        ofs.close();

        return true;
    }
}
