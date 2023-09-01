#define _CRT_SECURE_NO_WARNINGS
#include "CLI11.hpp"
#include <Windows.h>
#include <iostream>
#include <ranges>
#include <vector>
#include <optional>
#include <string>

#define INJ_ERR_SUCCESS					0x00000000
#define INJ_ERR_INVALID_PROC_HANDLE		0x00000001	//GetHandleInformation		:	win32 error
#define INJ_ERR_FILE_DOESNT_EXIST		0x00000002	//GetFileAttributesW		:	win32 error
#define INJ_ERR_OUT_OF_MEMORY_EXT		0x00000003	//VirtualAllocEx			:	win32 error
#define INJ_ERR_OUT_OF_MEMORY_INT		0x00000004	//VirtualAlloc				:	win32 error
#define INJ_ERR_IMAGE_CANT_RELOC		0x00000005	//internal error			:	base relocation directory empty
#define INJ_ERR_LDRLOADDLL_MISSING		0x00000006	//GetProcAddressEx			:	can't find pointer to LdrLoadDll
#define INJ_ERR_REMOTEFUNC_MISSING		0x00000007	//LoadFunctionPointer		:	can't find remote function
#define INJ_ERR_CANT_FIND_MOD_PEB		0x00000008	//internal error			:	module not linked to PEB
#define INJ_ERR_WPM_FAIL				0x00000009	//WriteProcessMemory		:	win32 error
#define INJ_ERR_CANT_ACCESS_PEB			0x0000000A	//ReadProcessMemory			:	win32 error
#define INJ_ERR_CANT_ACCESS_PEB_LDR		0x0000000B	//ReadProcessMemory			:	win32 error
#define INJ_ERR_VPE_FAIL				0x0000000C	//VirtualProtectEx			:	win32 error
#define INJ_ERR_CANT_ALLOC_MEM			0x0000000D	//VirtualAllocEx			:	win32 error
#define INJ_ERR_CT32S_FAIL				0x0000000E	//CreateToolhelp32Snapshot	:	win32 error
#define	INJ_ERR_RPM_FAIL				0x0000000F	//ReadProcessMemory			:	win32 error
#define INJ_ERR_INVALID_PID				0x00000010	//internal error			:	process id is 0
#define INJ_ERR_INVALID_FILEPATH		0x00000011	//internal error			:	INJECTIONDATA::szDllPath is nullptr
#define INJ_ERR_CANT_OPEN_PROCESS		0x00000012	//OpenProcess				:	win32 error
#define INJ_ERR_PLATFORM_MISMATCH		0x00000013	//internal error			:	file error (0x20000001 - 0x20000003, check below)
#define INJ_ERR_NO_HANDLES				0x00000014	//internal error			:	no process handle to hijack
#define INJ_ERR_HIJACK_NO_NATIVE_HANDLE	0x00000015	//internal error			:	no compatible process handle to hijack
#define INJ_ERR_HIJACK_INJ_FAILED		0x00000016	//internal error			:	injecting injection module into handle owner process failed, additional errolog(s) created
#define INJ_ERR_HIJACK_CANT_ALLOC		0x00000017	//VirtualAllocEx			:	win32 error
#define INJ_ERR_HIJACK_CANT_WPM			0x00000018	//WriteProcessMemory		:	win32 error
#define INJ_ERR_HIJACK_INJMOD_MISSING	0x00000019	//internal error			:	can't find remote injection module
#define INJ_ERR_HIJACK_INJECTW_MISSING	0x0000001A	//internal error			:	can't find remote injection function
#define INJ_ERR_GET_MODULE_HANDLE_FAIL	0x0000001B	//GetModuleHandleA			:	win32 error
#define INJ_ERR_OUT_OF_MEMORY_NEW		0x0000001C	//operator new				:	internal memory allocation failed
#define INJ_ERR_REMOTE_CODE_FAILED		0x0000001D	//internal error			:	the remote code wasn't able to load the module

std::unordered_map<DWORD, std::string> errorMessages = {
        {INJ_ERR_INVALID_PROC_HANDLE, "Invalid process handle. (GetHandleInformation: win32 error)"},
        {INJ_ERR_FILE_DOESNT_EXIST, "File doesn't exist. (GetFileAttributesW: win32 error)"},
        {INJ_ERR_OUT_OF_MEMORY_EXT, "Out of memory externally. (VirtualAllocEx: win32 error)"},
        {INJ_ERR_OUT_OF_MEMORY_INT, "Out of memory internally. (VirtualAlloc: win32 error)"},
        {INJ_ERR_IMAGE_CANT_RELOC, "Image can't be relocated. (Internal error: base relocation directory empty)"},
        {INJ_ERR_LDRLOADDLL_MISSING, "LdrLoadDll missing. (GetProcAddressEx: can't find pointer to LdrLoadDll)"},
        {INJ_ERR_REMOTEFUNC_MISSING, "Remote function missing. (LoadFunctionPointer: can't find remote function)"},
        {INJ_ERR_CANT_FIND_MOD_PEB, "Can't find module in PEB. (Internal error: module not linked to PEB)"},
        {INJ_ERR_WPM_FAIL, "WriteProcessMemory failed. (WriteProcessMemory: win32 error)"},
        {INJ_ERR_CANT_ACCESS_PEB, "Can't access PEB. (ReadProcessMemory: win32 error)"},
        {INJ_ERR_CANT_ACCESS_PEB_LDR, "Can't access PEB LDR. (ReadProcessMemory: win32 error)"},
        {INJ_ERR_VPE_FAIL, "VirtualProtectEx failed. (VirtualProtectEx: win32 error)"},
        {INJ_ERR_CANT_ALLOC_MEM, "Can't allocate memory. (VirtualAllocEx: win32 error)"},
        {INJ_ERR_CT32S_FAIL, "CreateToolhelp32Snapshot failed. (CreateToolhelp32Snapshot: win32 error)"},
        {INJ_ERR_RPM_FAIL, "ReadProcessMemory failed. (ReadProcessMemory: win32 error)"},
        {INJ_ERR_INVALID_PID, "Invalid process ID. (Internal error: process id is 0)"},
        {INJ_ERR_INVALID_FILEPATH, "Invalid file path. (Internal error: INJECTIONDATA::szDllPath is nullptr)"},
        {INJ_ERR_CANT_OPEN_PROCESS, "Can't open process. (OpenProcess: win32 error)"},
        {INJ_ERR_PLATFORM_MISMATCH, "Platform mismatch. (Internal error: file error)"},
        {INJ_ERR_NO_HANDLES, "No handles available. (Internal error: no process handle to hijack)"},
        {INJ_ERR_HIJACK_NO_NATIVE_HANDLE, "No native handle for hijacking. (Internal error: no compatible process handle to hijack)"},
        {INJ_ERR_HIJACK_INJ_FAILED, "Hijack injection failed. (Internal error: injecting injection module into handle owner process failed)"},
        {INJ_ERR_HIJACK_CANT_ALLOC, "Hijack can't allocate memory. (VirtualAllocEx: win32 error)"},
        {INJ_ERR_HIJACK_CANT_WPM, "Hijack WriteProcessMemory failed. (WriteProcessMemory: win32 error)"},
        {INJ_ERR_HIJACK_INJMOD_MISSING, "Hijack injection module missing. (Internal error: can't find remote injection module)"},
        {INJ_ERR_HIJACK_INJECTW_MISSING, "Hijack InjectW function missing. (Internal error: can't find remote injection function)"},
        {INJ_ERR_GET_MODULE_HANDLE_FAIL, "GetModuleHandleA failed. (GetModuleHandleA: win32 error)"},
        {INJ_ERR_OUT_OF_MEMORY_NEW, "Out of memory when using 'new'. (Operator new: internal memory allocation failed)"},
        {INJ_ERR_REMOTE_CODE_FAILED, "The remote code wasn't able to load the module. (Internal error)"}
};

enum LAUNCH_METHOD
{
    LM_NtCreateThreadEx,
    LM_HijackThread,
    LM_SetWindowsHookEx,
    LM_QueueUserAPC,
    LM_SetWindowLong
};

enum INJECTION_MODE
{
    IM_LoadLibrary,
    IM_LdrLoadDll
};

struct INJECTIONDATAW
{
    DWORD			LastErrorCode;
    wchar_t			szDllPath[MAX_PATH * 2];
    wchar_t* szTargetProcessExeFileName;
    DWORD			ProcessID;
    INJECTION_MODE	Mode;
    LAUNCH_METHOD	Method;
    DWORD			Flags;
    DWORD			hHandleValue;
    HINSTANCE		hDllOut;
};

std::string getErrorMessage(DWORD errorCode) {
    if (errorMessages.find(errorCode) != errorMessages.end()) {
        return errorMessages[errorCode];
    }
    return "Unknown error.";
}

int main(int argc, char* argv[])
{
    CLI::App app("Command-line DLL Injector");

    int processID;
    std::string dllPath;
    std::string mode;
    std::string launchMethod;

    app.add_option("-p,--pid", processID, "Target Process ID")->required();
    app.add_option("-d,--dll", dllPath, "Path to the DLL")->required()->check(CLI::ExistingFile);
    app.add_option("-m,--mode", mode, "Injection mode (LoadLibrary, LdrLoadDll, ManualMap)")->required();
    app.add_option("-l,--launch-method", launchMethod, "Launch method (NtCreateThreadEx, HijackThread, SetWindowsHookEx, QueueUserAPC, SetWindowLong)")->required();

    CLI11_PARSE(app, argc, argv);

    // Load the GH Injector library
    HMODULE hModule = LoadLibrary(TEXT("GH Injector - x86.dll"));
    if (!hModule)
    {
        std::cerr << "Failed to load the GH Injector library. Error: " << GetLastError() << std::endl;
        return -1;
    }

    // Get the InjectW function
    typedef DWORD(__stdcall* InjectW_t)(INJECTIONDATAW*);
    InjectW_t pInjectW = (InjectW_t)GetProcAddress(hModule, "InjectW");
    if (!pInjectW)
    {
        std::cerr << "Failed to retrieve the InjectW function. Error: " << GetLastError() << std::endl;
        FreeLibrary(hModule);
        return -1;
    }

    INJECTION_MODE injMode;
    LAUNCH_METHOD injMethod;

    std::unordered_map<std::string, INJECTION_MODE> modeMap = {
            {"LoadLibrary", IM_LoadLibrary},
            {"LdrLoadDll", IM_LdrLoadDll}
    };

    if (modeMap.find(mode) != modeMap.end())
    {
        injMode = modeMap[mode];
    }
    else
    {
        std::cerr << "Invalid injection mode specified." << std::endl;
        FreeLibrary(hModule);
        return -1;
    }

    std::unordered_map<std::string, LAUNCH_METHOD> methodMap = {
            {"NtCreateThreadEx", LM_NtCreateThreadEx},
            {"HijackThread", LM_HijackThread},
            {"SetWindowsHookEx", LM_SetWindowsHookEx},
            {"QueueUserAPC", LM_QueueUserAPC},
            {"SetWindowLong", LM_SetWindowLong}
    };

    if (methodMap.find(launchMethod) != methodMap.end())
    {
        injMethod = methodMap[launchMethod];
    }
    else
    {
        std::cerr << "Invalid launch method specified." << std::endl;
        FreeLibrary(hModule);
        return -1;
    }

    INJECTIONDATAW injData;

    injData.LastErrorCode = 0;
    wcscpy(injData.szDllPath, std::wstring(dllPath.begin(), dllPath.end()).c_str());
    injData.ProcessID = processID;
    injData.Mode = injMode;
    injData.Method = injMethod;
    injData.Flags = 0;
    injData.hHandleValue = 0;
    injData.hDllOut = NULL;
    injData.szTargetProcessExeFileName = NULL;

    DWORD result = pInjectW(&injData);
    if (result == INJ_ERR_SUCCESS)
    {
        std::cout << "Injection successful! DLL Base: " << injData.hDllOut << std::endl;
    }
    else
    {
        std::cerr << "Injection failed: " << getErrorMessage(result) << " (Error code: " << std::hex << result << ")" << std::endl;
    }

    // Unload the GH Injector library
    FreeLibrary(hModule);

    return 0;
}