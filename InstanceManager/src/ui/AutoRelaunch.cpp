#include "ui/AutoRelaunch.h"

#include "imgui_stdlib.h"

void AutoRelaunch::Draw(const char* title, bool* p_open) {
	if (!ImGui::Begin(title, p_open)) {
		ImGui::End();
		return;
	}

	{
		if (m_InstanceSelection.size() != m_InstanceNames.size()) {
			m_InstanceSelection.resize(m_InstanceNames.size());
		}

		if (ImGui::BeginListBox("##listbox m_Instances", ImVec2(ImGui::GetContentRegionAvail().x * 0.30f, ImGui::GetContentRegionAvail().y * 0.5f))) {
			ui::HandleSelectAll(m_InstanceNames, m_InstanceSelection);

			for (int n = 0; n < m_InstanceNames.size(); ++n) {
				const auto color = g_InstanceControl.GetColor(m_InstanceNames[n]);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::Bullet();
				ImGui::PopStyleColor();
				ImGui::SameLine();

				static int lastSelectedIndex = -1;
				if (ImGui::Selectable(m_InstanceNames[n].c_str(), m_InstanceSelection[n])) {
					ui::HandleMultiSelection(m_InstanceSelection, n, lastSelectedIndex);
				}

				ui::HandleRightClickSelection(m_InstanceSelection, n, lastSelectedIndex);
			}

			ImGui::EndListBox();
		}


		ImGui::SameLine();

		ImGui::BeginGroup();

		static std::string groupName;
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##group name", "Group name", &groupName);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.20f);
		static std::string PlaceID;
		ImGui::InputTextWithHint("##placeidar", "Place ID", &PlaceID, ImGuiInputTextFlags_CharsDecimal);

		static std::string vipCode;
		ImGui::SameLine();
		ImGui::InputTextWithHint("##vipcodear", "VIP Code", &vipCode, ImGuiInputTextFlags_CharsDecimal);

		ImGui::SameLine();
		static std::string launchDelay;
		ImGui::InputTextWithHint("##launchdelayar", "Launch Delay (Seconds)", &launchDelay, ImGuiInputTextFlags_CharsDecimal);

		ImGui::SameLine();
		static std::string relaunchInterval;
		ImGui::InputTextWithHint("##relaunchintervalar", "Relaunch Interval (Minutes)", &relaunchInterval, ImGuiInputTextFlags_CharsDecimal);

		ImGui::SameLine();
		static std::string injectDelay;
		ImGui::InputTextWithHint("##injectdelayar", "Inject Delay (Seconds)", &injectDelay, ImGuiInputTextFlags_CharsDecimal);

		static bool isLoaded = false;
		if (!isLoaded) {
			auto loadConfigValue = [](const std::string& key) -> std::optional<std::string> {
				if (auto optValue = Config::getInstance().GetStringForKey(key); optValue) {
					return *optValue;
				}
				return std::nullopt;
			};

			auto loadValue = [loadConfigValue](auto& variable, const std::string& key) {
				if (const auto value = loadConfigValue(key)) {
					variable = *value;
				}
			};

			loadValue(PlaceID, "lastPlaceID");
			loadValue(vipCode, "lastVip");
			loadValue(launchDelay, "lastDelay");
			loadValue(relaunchInterval, "lastInterval");
			loadValue(injectDelay, "lastInjectDelay");

			isLoaded = true;
		}

		ImGui::PopItemWidth();

		static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
		if (ImGui::TreeNode("Group Color")) {
			const float availableWidth = ImGui::GetContentRegionAvail().x;
			const float widgetWidth = (availableWidth - ImGui::GetStyle().ItemSpacing.y) * 0.40f;
			const float totalWidgetWidth = 2 * widgetWidth + ImGui::GetStyle().ItemSpacing.y;

			const float spacing = (availableWidth - totalWidgetWidth) / 2.0f;

			ImGui::Dummy(ImVec2(spacing, 0));
			ImGui::SameLine();

			ImGui::SetNextItemWidth(widgetWidth);
			ImGui::ColorPicker3("##MyColor##5", reinterpret_cast<float*>(&color), ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(widgetWidth);
			ImGui::ColorPicker3("##MyColor##6", reinterpret_cast<float*>(&color), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Auto Inject")) {
			ImVec2 buttonSize = ImGui::CalcTextSize("Open DLL File");
			constexpr float padding = 20.0f;// Some padding between the InputText and the button
			const float inputTextWidth = ImGui::GetContentRegionAvail().x - buttonSize.x - padding;
			ImGui::PushItemWidth(inputTextWidth);

			constexpr std::array<const char*, 2> injectionModes = {"LoadLibrary", "ManualMap"};
			static int selectedInjectionMode = 0;
			if (ui::RenderCombo("##Injection Mode", injectionModes, selectedInjectionMode)) {
				this->m_InjectionMode = injectionModes[selectedInjectionMode];
			}

			constexpr std::array<const char*, 5> injectionMethods = {"NtCreateThreadEx", "HijackThread", "SetWindowsHookEx", "QueueUserAPC", "SetWindowLong"};
			static int selectedInjectionMethod = 0;
			if (ui::RenderCombo("##Injection Method", injectionMethods, selectedInjectionMethod)) {
				this->m_InjectionMethod = injectionMethods[selectedInjectionMethod];
			}


			// Set the width for the InputText
			ImGui::InputText("##Selected DLL", szFile, sizeof(szFile));
			ImGui::PopItemWidth();

			ImGui::SameLine();

			if (ImGui::Button("Open DLL File")) {
				OPENFILENAME ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;
				ofn.lpstrFile = szFile;
				ofn.lpstrFile[0] = '\0';
				ofn.nMaxFile = sizeof(szFile);
				ofn.lpstrFilter = "DLL Files\0*.dll\0All\0*.*\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFileTitle = NULL;
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = NULL;
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

				GetOpenFileName(&ofn);
			}

			ImGui::TreePop();
		}

		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(21, 82, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(25, 92, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(33, 123, 33, 255));

		if (groupName.empty() || PlaceID.empty() || relaunchInterval.empty() || std::count(m_InstanceSelection.begin(), m_InstanceSelection.end(), true) == 0) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Create Group", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			std::vector<std::string> selectedInstances;
			for (int i = 0; i < m_InstanceSelection.size(); ++i) {
				if (m_InstanceSelection[i]) {
					selectedInstances.push_back(m_InstanceNames[i]);
				}
			}

			if (std::find(m_Groups.begin(), m_Groups.end(), groupName) != m_Groups.end()) {
				// TODO: handle inserting into existing group
			} else {
				m_Groups.emplace_back(groupName);
				m_GroupSelection.push_back(false);

				InstanceControl::GroupCreationInfo groupInfo;
				groupInfo.groupname = groupName;
				groupInfo.usernames = selectedInstances;
				groupInfo.placeid = PlaceID;
				groupInfo.linkcode = vipCode;
				groupInfo.dllpath = std::string(szFile);
				groupInfo.mode = this->m_InjectionMode;
				groupInfo.method = this->m_InjectionMethod;
				groupInfo.launchdelay = launchDelay.empty() ? 0 : std::stoi(launchDelay);
				groupInfo.relaunchinterval = std::stoi(relaunchInterval);
				groupInfo.color = ui::ImVec4ToUint32(color);
				groupInfo.injectdelay = injectDelay.empty() ? 0 : std::stoi(injectDelay);

				g_InstanceControl.CreateGroup(groupInfo);
			}

			Config::getInstance().UpdateConfig<std::string>("lastPlaceID", PlaceID);
			Config::getInstance().UpdateConfig<std::string>("lastVip", vipCode);
			Config::getInstance().UpdateConfig<std::string>("lastDelay", launchDelay);
			Config::getInstance().UpdateConfig<std::string>("lastInterval", relaunchInterval);
			Config::getInstance().UpdateConfig<std::string>("lastInjectDelay", injectDelay);
		}

		if (groupName.empty() || PlaceID.empty() || relaunchInterval.empty() || std::count(m_InstanceSelection.begin(), m_InstanceSelection.end(), true) == 0) {
			ImGui::EndDisabled();
		}

		ImGui::PopStyleColor(3);


		ImGui::EndGroup();
	}


	ImGui::Separator();

	{
		if (ui::BeginSizedListBox("##listbox groups", 0.30f, 1.0f)) {
			for (int n = 0; n < m_Groups.size(); n++) {
				const bool is_selected = m_GroupSelection[n];

				if (ImGui::Selectable(m_Groups[n].c_str(), is_selected)) {
					std::fill(m_GroupSelection.begin(), m_GroupSelection.end(), false);
					m_GroupSelection[n] = true;
				}
			}

			ImGui::EndListBox();
		}

		ImGui::SameLine();

		ImGui::BeginGroup();

		const bool groupPresent = std::none_of(m_GroupSelection.begin(), m_GroupSelection.end(), [](bool b) { return b; });
		ImGui::BeginDisabled(groupPresent);

		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(82, 21, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 25, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(123, 33, 33, 255));

		if (ImGui::Button("Terminate Group", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			for (int i = 0; i < m_GroupSelection.size(); ++i) {
				if (m_GroupSelection[i]) {
					g_InstanceControl.TerminateGroup(m_Groups[i]);
					m_Groups.erase(m_Groups.begin() + i);
					m_GroupSelection.erase(m_GroupSelection.begin() + i);
					break;
				}
			}
		}

		ImGui::EndDisabled();

		ImGui::PopStyleColor(3);

		ImGui::EndGroup();
	}

	ImGui::End();
}