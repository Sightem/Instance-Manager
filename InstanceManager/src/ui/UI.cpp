#include "ui/UI.h"

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