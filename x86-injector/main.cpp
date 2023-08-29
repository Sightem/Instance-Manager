#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <windows.h>
#include <span>
#include <psapi.h>
#include <string>
#include <format>

HANDLE OpenProc(DWORD pid)
{
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE,
        pid
    );

    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_INVALID_PARAMETER) {
            std::cerr << std::format("[ERROR] [{}] Opening the process failed. Is the process still running?\n", pid);
            return NULL;
        }
        std::cerr << std::format("[ERROR] [{}] Opening the process failed: 0x{:x}\n", pid, err);
        return NULL;
    }

    return hProcess;
}


bool IsCompatible(HANDLE hProcess)
{
    BOOL isTargetWow64 = FALSE;
    IsWow64Process(hProcess, &isTargetWow64);

    BOOL isInjectorWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &isInjectorWow64);

    return isTargetWow64 == isInjectorWow64;
}

LPVOID WriteIntoProcess(HANDLE hProcess, LPBYTE buffer, SIZE_T buffer_size, DWORD protect)
{
    LPVOID remoteAddress = VirtualAllocEx(hProcess, NULL, buffer_size, MEM_COMMIT | MEM_RESERVE, protect);
    if (remoteAddress == NULL) {
        std::cerr << "Could not allocate memory in the remote process\n";
        return NULL;
    }
    if (!WriteProcessMemory(hProcess, remoteAddress, buffer, buffer_size, NULL)) {
        VirtualFreeEx(hProcess, remoteAddress, buffer_size, MEM_FREE);
        return NULL;
    }
    return remoteAddress;
}

bool InjectWithLoadLibraryW(HANDLE hProcess, const wchar_t* injectpath)
{
    if (!injectpath) {
        return false;
    }

    HMODULE hModule = GetModuleHandleW(L"kernel32.dll");
    if (!hModule) 
        return false;

    FARPROC hLoadLib = GetProcAddress(hModule, "LoadLibraryW");
    if (!hLoadLib) 
        return false;

    std::wstring_view injectpathView(injectpath);
    SIZE_T injectPathSize = (injectpathView.size() + 1) * sizeof(wchar_t);

    PVOID remote_ptr = WriteIntoProcess(hProcess, (BYTE*)injectpath, injectPathSize, PAGE_READWRITE);
    if (!remote_ptr) {
        std::cerr << std::format("Writing to process failed: {:x}\n", GetLastError());
        return false;
    }

    DWORD ret = WAIT_FAILED;
    HANDLE hndl = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)hLoadLib, remote_ptr, NULL, NULL);
    if (hndl) [[likely]] {
        ret = WaitForSingleObject(hndl, 100000);
        CloseHandle(hndl); hndl = NULL;
    }
    else [[unlikely]] {
        std::cout << "Creating thread failed!\n";
    }

    VirtualFreeEx(hProcess, remote_ptr, 0, MEM_RELEASE);
    if (ret == WAIT_OBJECT_0) {
        return true;
    }
    return false;
}

size_t EnumModules(HANDLE hProcess, HMODULE hMods[], const DWORD hModsMax, DWORD filters)
{
    if (hProcess == nullptr) {
        return 0;
    }

    DWORD cbNeeded;

    if (!EnumProcessModules(hProcess, hMods, hModsMax, &cbNeeded)) {
        return 0;
    }

    const size_t modules_count = cbNeeded / sizeof(HMODULE);
    return modules_count;
}

__forceinline WCHAR ToLower(WCHAR c1)
{
    if (c1 <= L'Z' && c1 >= L'A') {
        c1 = (c1 - L'A') + L'a';
    }
    return c1;
}

bool IsWantedModule(wchar_t* currname, wchar_t* wantedname)
{
    if (wantedname == NULL || currname == NULL)
        return false;

    WCHAR* currEndPointer = currname;
    while (*currEndPointer != L'\0') {
        currEndPointer++;
    }

    if (currEndPointer == currname) 
        return false;

    wchar_t* wantedEndPointer = wantedname;
    while (*wantedEndPointer != L'\0') {
        wantedEndPointer++;
    }

    if (wantedEndPointer == wantedname) 
        return false;

    while ((currEndPointer != currname) && (wantedEndPointer != wantedname)) {

        if (ToLower(*wantedEndPointer) != ToLower(*currEndPointer)) {
            return false;
        }
        wantedEndPointer--;
        currEndPointer--;
    }
    return true;
}

HMODULE SearchModByName(HANDLE hProcess, const std::wstring& searchedName)
{
    const DWORD hModsMax = 0x1000;
    std::vector<HMODULE> hMods(hModsMax, 0);

    size_t modulesCount = EnumModules(hProcess, hMods.data(), hModsMax, LIST_MODULES_32BIT | LIST_MODULES_64BIT);

    wchar_t nameBuf[MAX_PATH] = { 0 };

    for (size_t i = 0; i < modulesCount; ++i) {

        HMODULE hMod = hMods[i];
        if (!hMod || hMod == INVALID_HANDLE_VALUE) break;

        memset(nameBuf, 0, sizeof(nameBuf));
        if (GetModuleFileNameExW(hProcess, hMod, nameBuf, MAX_PATH)) 
        {
            if (IsWantedModule(nameBuf, (wchar_t*)searchedName.c_str())) 
            {
                return hMod;
            }
        }
    }
    return NULL;
}


bool InjectProcess(DWORD pid, const wchar_t* dll_path)
{
    HANDLE hProcess = OpenProc(pid);
    if (!hProcess) 
        return false;

    if (!IsCompatible(hProcess)) {
        CloseHandle(hProcess);
        std::cerr << "[" << std::dec << pid << "] Injector bitness mismatch, skipping" << std::endl;
        return false;
    }

    bool isLoaded = false;
    bool isInjected = InjectWithLoadLibraryW(hProcess, dll_path);
    if (SearchModByName(hProcess, dll_path)) {
        isLoaded = true;
    }
    CloseHandle(hProcess);

    return isLoaded;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <PID> <DLL_PATH>\n";
        return 1;
    }

    int pid = std::stoi(argv[1]);

    std::string dllPath = argv[2];

    std::wstring wpath = std::wstring(dllPath.begin(), dllPath.end());

    if (InjectProcess(pid, wpath.c_str())) {
        std::cout << "Injection successful\n";
    }
    else {
        std::cerr << "Injection failed\n";
        return 2;
    }

    return 0;
}