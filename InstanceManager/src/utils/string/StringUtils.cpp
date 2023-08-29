#include "utils/string/StringUtils.h"

#define NOMINMAX
#include <windows.h>

#include <vector>

namespace StringUtils
{
    bool ContainsOnly(const std::string& s, char c) {
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

    std::string WStrToStr(const std::wstring& wstr)
    {
        int numBytes = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::vector<char> buffer(numBytes);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer.data(), numBytes, NULL, NULL);
        return std::string(buffer.data());
    }
}