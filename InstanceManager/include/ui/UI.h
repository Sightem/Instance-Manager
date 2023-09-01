#pragma once
#include "imgui.h"
#include <string>
#include <vector>

namespace ui
{
    enum class ButtonStyle
    {
        Red,
        Green,
    };

    bool InputTextWithHint(const char* label, const char* hint, std::string* my_str, ImGuiInputTextFlags flags = 0);
    bool ConditionalButton(const char* label, bool condition, ButtonStyle style);
    bool GreenButton(const char* label);
    bool RedButton(const char* label);
    void HelpMarker(const char* desc);
    bool BeginSizedListBox(const char* label, float width_ratio, float height_ratio);

    unsigned int ImVec4ToUint32(const ImVec4& color);
    bool RenderCombo(const char* label, std::vector<std::string>& items, int& currentItemIdx);
}