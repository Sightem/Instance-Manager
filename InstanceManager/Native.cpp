#include "Native.h"
#include "ntdll.h"

#include <TlHelp32.h>
#include <lmcons.h>

#include "AppLog.h"
#include "Utils.h"


namespace Native
{
    template <bool CaptureOutput>
    std::conditional_t<CaptureOutput, std::string, void> RunPowershellCommand(const std::string& command) {
        SECURITY_ATTRIBUTES security_attributes;
        ZeroMemory(&security_attributes, sizeof(security_attributes));
        security_attributes.nLength = sizeof(security_attributes);
        security_attributes.bInheritHandle = TRUE;

        HANDLE stdout_read = NULL;
        HANDLE stdout_write = NULL;

        if constexpr (CaptureOutput) {
            if (!CreatePipe(&stdout_read, &stdout_write, &security_attributes, 0)) {
                AppLog::GetInstance().AddLog("Failed to create pipe");
                if constexpr (CaptureOutput) return "";
                else return;
            }
        }

        STARTUPINFO startup_info;
        ZeroMemory(&startup_info, sizeof(startup_info));
        startup_info.cb = sizeof(startup_info);
        if constexpr (CaptureOutput) {
            startup_info.hStdError = stdout_write;
            startup_info.hStdOutput = stdout_write;
        }
        startup_info.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION process_info;
        ZeroMemory(&process_info, sizeof(process_info));

        std::string cmd_line = "powershell.exe -Command \"" + command + "\"";

        if (!CreateProcess(NULL, &cmd_line[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_info)) {
            AppLog::GetInstance().AddLog("Failed to create process");
            if constexpr (CaptureOutput) return "";
            else return;
        }

        if constexpr (CaptureOutput) {
            CloseHandle(stdout_write);

            constexpr size_t buffer_size = 4096;
            std::array<char, buffer_size> buffer;
            std::string output;

            DWORD bytes_read;
            while (ReadFile(stdout_read, buffer.data(), buffer_size, &bytes_read, NULL)) {
                output.append(buffer.data(), bytes_read);
            }

            CloseHandle(stdout_read);

            return output;
        }

        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);

        if constexpr (!CaptureOutput) return;
    }


    winrt::com_ptr<IShellItemArray> CreateShellItemArrayFromProtocol(const winrt::hstring& protocolURI) {
        winrt::com_ptr<IShellItem> shellItem;
        HRESULT hr = SHCreateItemFromParsingName(protocolURI.c_str(), nullptr, IID_PPV_ARGS(&shellItem));
        if (FAILED(hr)) {
            return nullptr;
        }

        winrt::com_ptr<IShellItemArray> shellItemArray;
        hr = SHCreateShellItemArrayFromShellItem(shellItem.get(), IID_PPV_ARGS(&shellItemArray));
        if (FAILED(hr)) {
            return nullptr;
        }

        return shellItemArray;
    }

    DWORD LaunchUWPAppWithProtocol(const winrt::hstring& appID, const winrt::hstring& protocolURI) {
        winrt::com_ptr<IApplicationActivationManager> pAAM;
        HRESULT hr = CoCreateInstance(CLSID_ApplicationActivationManager, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&pAAM));
        if (FAILED(hr)) {
            AppLog::GetInstance().AddLog("Failed to create IApplicationActivationManager instance. Error code: {}", hr);
            return 0;
        }

        auto shellItemArray = CreateShellItemArrayFromProtocol(protocolURI);
        if (!shellItemArray) {
            AppLog::GetInstance().AddLog("Failed to create IShellItemArray from protocol URI.");
            return 0;
        }

        DWORD dwPID = 0;
        hr = pAAM->ActivateForProtocol(appID.c_str(), shellItemArray.get(), &dwPID);

        if (FAILED(hr)) {
            AppLog::GetInstance().AddLog("Failed to activate UWP app. Error code: {}", hr);
            return 0;
        }

        return dwPID;
    }

    std::string GetCurrentUsername() {
        char username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        if (GetUserName(username, &username_len)) {
            return std::string(username);
        }

        return "";
    }

    std::set<DWORD> GetInstancesOf(const char* exeName)
    {
        std::set<DWORD> pidSet;
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry) == TRUE)
        {
            while (Process32Next(snapshot, &entry) == TRUE)
            {
                if (strcmp(entry.szExeFile, exeName) == 0)
                {
                    pidSet.insert(entry.th32ProcessID);
                }
            }
        }

        CloseHandle(snapshot);

        return pidSet;
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

    std::string GetCommandlineArguments(DWORD pid)
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

        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, commandLineContents, -1, NULL, 0, NULL, NULL);
        char* convertedStr = new char[requiredSize];
        WideCharToMultiByte(CP_UTF8, 0, commandLineContents, -1, convertedStr, requiredSize, NULL, NULL);
        std::string command_line_str(convertedStr);

        CloseHandle(processHandle);
        free(commandLineContents);
        delete[] convertedStr;

        return command_line_str;
    }



    bool OpenInExplorer(const std::string& path, bool isFile) {
        std::string cmd = isFile ? "/select," + path : path;
        HINSTANCE result = ShellExecute(0, "open", "explorer.exe", cmd.c_str(), 0, SW_SHOW);
        return (INT_PTR)result > 32;
    }

    bool SetProcessAffinity(DWORD processID, DWORD requestedCores)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        DWORD numberOfCores = sysInfo.dwNumberOfProcessors;

        if (requestedCores <= 0 || requestedCores > numberOfCores) {
            AppLog::GetInstance().AddLog("Invalid number of cores requested.");
            return false;
        }

        DWORD_PTR mask = 0;
        for (DWORD i = 0; i < requestedCores; i++) {
            mask |= (1 << i);
        }

        HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processID);
        if (hProcess == NULL) {
            AppLog::GetInstance().AddLog("Failed to open process. Error code: {}", GetLastError());
            return false;
        }

        if (SetProcessAffinityMask(hProcess, mask) == 0) {
            AppLog::GetInstance().AddLog("Failed to set process affinity. Error code: {}", GetLastError());
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

//i hate this fucking language
template std::string Native::RunPowershellCommand<true>(const std::string& command);
template void Native::RunPowershellCommand<false>(const std::string& command);