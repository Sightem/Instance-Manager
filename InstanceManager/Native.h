#pragma once
#include <string>
#include <set>
#define NOMINMAX
#include <Windows.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Store.h>

#include <winrt/Windows.Storage.h>

namespace Native
{
    template <bool CaptureOutput = true>
    std::conditional_t<CaptureOutput, std::string, void> RunPowershellCommand(const std::string& command);
    winrt::com_ptr<IShellItemArray> CreateShellItemArrayFromProtocol(const winrt::hstring& protocolURI);
    DWORD LaunchUWPAppWithProtocol(const winrt::hstring& appID, const winrt::hstring& protocolURI);
    std::string GetCurrentUsername();
    PVOID GetPebAddress(HANDLE ProcessHandle);
    std::string GetCommandlineArguments(DWORD pid);
    bool OpenInExplorer(const std::string& path, bool isFile = false);
    bool SetProcessAffinity(DWORD processID, DWORD requestedCores);
    bool IsReadableMemory(const MEMORY_BASIC_INFORMATION& mbi);
    bool IsProcessRunning(DWORD targetPid, const CHAR* expectedName);

    std::set<DWORD> GetInstancesOf(const char* exeName);

    typedef std::string(*ExtractFunction)(const unsigned char*, size_t);
    std::string SearchEntireProcessMemory(HANDLE pHandle, const unsigned char* pattern, size_t patternSize, ExtractFunction extractFunction);
}