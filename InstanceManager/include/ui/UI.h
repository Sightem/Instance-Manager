#pragma once
#include "imgui.h"
#include <string>
#include <utility>
#include <vector>
#include <functional>

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

    template <std::size_t N>
    bool RenderCombo(const char* label, const std::array<std::string, N>& items, int& currentItemIdx) {
        if (items.size() == 0) return false;

        const char* combo_preview_value = items[currentItemIdx].c_str();

        bool itemChanged = false;

        if (ImGui::BeginCombo(label, combo_preview_value, NULL)) {
            for (int n = 0; n < items.size(); n++) {
                const bool is_selected = (currentItemIdx == n);
                if (ImGui::Selectable(items[n].c_str(), is_selected)) {
                    currentItemIdx = n;
                    itemChanged = true;
                }

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        return itemChanged;
    }

}