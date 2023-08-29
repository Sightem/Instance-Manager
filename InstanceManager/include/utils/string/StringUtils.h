#pragma once
#include <string>

namespace StringUtils
{
    bool ContainsOnly(const std::string& s, char c);
    bool CopyToClipboard(const std::string& data);
    std::string WStrToStr(const std::wstring& wstr);
}