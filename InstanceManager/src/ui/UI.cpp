#include "ui/UI.h"

namespace ui {
	int FilterCallback(ImGuiInputTextCallbackData* data) {
		const char* DISALLOWED_CHARS = "<>:\"/\\|?*\t\n\r ";

		char character = data->EventChar;

		if (character == '_') {
			data->EventChar = '-';
			return 0;
		}


		for (int i = 0; DISALLOWED_CHARS[i]; ++i) {
			if (character == DISALLOWED_CHARS[i])
				return 1;
		}

		return 0;
	}

	bool ConditionalButton(const char* label, bool condition, ButtonStyle style) {
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

	bool GreenButton(const char* label) {
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(21, 82, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(25, 92, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(33, 123, 33, 255));

		bool pressed = ImGui::Button(label);

		ImGui::PopStyleColor(3);

		return pressed;
	}

	bool RedButton(const char* label) {
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(82, 21, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 25, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(123, 33, 33, 255));

		bool pressed = ImGui::Button(label);

		ImGui::PopStyleColor(3);

		return pressed;
	}

	void HelpMarker(const char* desc) {
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	bool BeginSizedListBox(const char* label, float width_ratio, float height_ratio) {
		ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x * width_ratio, ImGui::GetContentRegionAvail().y * height_ratio);
		return ImGui::BeginListBox(label, size);
	}

	unsigned int ImVec4ToUint32(const ImVec4& color) {
		auto r = static_cast<uint8_t>(color.x * 255.0f);
		auto g = static_cast<uint8_t>(color.y * 255.0f);
		auto b = static_cast<uint8_t>(color.z * 255.0f);
		auto a = static_cast<uint8_t>(color.w * 255.0f);
		return (a << 24) | (b << 16) | (g << 8) | r;
	}

	void HandleMultiSelection(std::vector<bool>& selection, int n, int& lastSelectedIndex) {
		if (ImGui::GetIO().KeyShift && lastSelectedIndex != -1) {
			// Handle range selection
			int start = std::min(lastSelectedIndex, n);
			int end = std::max(lastSelectedIndex, n);
			for (int i = start; i <= end; ++i) {
				selection[i] = true;
			}
		} else if (!ImGui::GetIO().KeyCtrl) {
			// Clear selection when CTRL is not held
			std::fill(selection.begin(), selection.end(), false);
			selection[n] = true;
		} else {
			selection[n] = !selection[n];
		}

		lastSelectedIndex = n;
	}

	void HandleRightClickSelection(std::vector<bool>& selection, int n, int& lastSelectedIndex) {
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
			if (std::count(selection.begin(), selection.end(), true) <= 1 || !selection[n]) {
				std::fill(selection.begin(), selection.end(), false);// Clear other selections
				selection[n] = true;                                 // Select the current instance
				lastSelectedIndex = n;                               // Update the last selected index
			}
		}
	}

	void HandleSelectAll(const std::vector<std::string>& instanceNames,
	                     std::vector<bool>& selection,
	                     const std::optional<ImGuiTextFilter>& filterOpt) {

		if (!ImGui::IsAnyItemActive() && ImGui::GetIO().KeyCtrl &&
		    ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A))) {

			std::transform(instanceNames.begin(), instanceNames.end(), selection.begin(),
			               [&filterOpt](const std::string& path_name) {
				               return !filterOpt.has_value() || filterOpt.value().PassFilter(path_name.c_str());
			               });
		}
	}

}// namespace ui