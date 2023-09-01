#include "ui/AutoRelaunch.h"
#include "imgui_stdlib.h"

void AutoRelaunch::Draw(const char* title, bool* p_open)
{
	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	{
		static int lastSelectedIndex = -1;
		if (m_InstanceSelection.size() != m_InstanceNames.size())
		{
			m_InstanceSelection.resize(m_InstanceNames.size());
		}

		if (ImGui::BeginListBox("##listbox instances", ImVec2(ImGui::GetContentRegionAvail().x * 0.30f, ImGui::GetContentRegionAvail().y * 0.5f)))
		{
			if (!ImGui::IsAnyItemActive() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			{
				for (int n = 0; n < m_InstanceNames.size(); ++n)
				{
					m_InstanceSelection[n] = true;
				}
			}

			for (int n = 0; n < m_InstanceNames.size(); ++n)
			{
				const bool is_selected = m_InstanceSelection[n];

				if (g_InstanceControl.IsInstanceRunning(m_InstanceNames[n]))
				{
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(40, 170, 40, 255));
					ImGui::Bullet();
					ImGui::PopStyleColor();
					ImGui::SameLine();

					// Check if the bullet is hovered
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Instance has been launched.");
						ImGui::EndTooltip();
					}
				}
				else // fucking cursed
				{
					auto groupColor = g_InstanceControl.IsGrouped(m_InstanceNames[n]);

					ImGui::PushStyleColor(ImGuiCol_Text, groupColor);
					ImGui::Bullet();
					ImGui::PopStyleColor();
					ImGui::SameLine();

					//check if color is 77,77,77,255 (grey)
					if (groupColor == IM_COL32(77, 77, 77, 255))
					{
						if (ImGui::BeginItemTooltip())
						{
							ImGui::Text("Instance has not been launched.");

							ImGui::EndTooltip();
						}
					}
					else
					{
						// Check if the bullet is hovered
						if (ImGui::BeginItemTooltip())
						{
							ImGui::Text("Instance has been launched.");

							ImGui::EndTooltip();
						}
					}
				}

				if (ImGui::Selectable(m_InstanceNames[n].c_str(), is_selected))
				{
					if (ImGui::GetIO().KeyShift && lastSelectedIndex != -1)
					{
						// Handle range selection
						int start = std::min(lastSelectedIndex, n);
						int end = std::max(lastSelectedIndex, n);
						for (int i = start; i <= end; ++i)
						{
							m_InstanceSelection[i] = true;
						}
					}
					else if (!ImGui::GetIO().KeyCtrl)
					{
						// Clear selection when CTRL is not held
						std::fill(m_InstanceSelection.begin(), m_InstanceSelection.end(), false);
						m_InstanceSelection[n] = true;
					}
					else
					{
						m_InstanceSelection[n] = !m_InstanceSelection[n];
					}

					lastSelectedIndex = n;
				}

				// This fixes an annoying bug where right clicking an instance right after launching it would make the program crash
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
				{
					// If no instances or only one instance is selected, or the right clicked instance isn't part of the selection
					if (std::count(m_InstanceSelection.begin(), m_InstanceSelection.end(), true) <= 1 || !m_InstanceSelection[n])
					{
						std::fill(m_InstanceSelection.begin(), m_InstanceSelection.end(), false); // Clear other selections
						m_InstanceSelection[n] = true; // Select the current instance
						lastSelectedIndex = n; // Update the last selected index
					}
				}
			}

			ImGui::EndListBox();
		}

		ImGui::SameLine();

		ImGui::BeginGroup();

		static std::string groupName;
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##group name", "Group name", &groupName);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
		static std::string PlaceID;
		ImGui::InputTextWithHint("##placeidar", "Place ID", &PlaceID);

		static std::string vipCode;
		ImGui::SameLine();
		ImGui::InputTextWithHint("##vipcodear", "VIP Code", &vipCode);

		ImGui::SameLine();
		static std::string launchDelay;
		ImGui::InputTextWithHint("##launchdelayar", "Launch Delay (Seconds)", &launchDelay);

		ImGui::SameLine();
		static std::string relaunchInterval;
		ImGui::InputTextWithHint("##relaunchintervalar", "Relaunch Interval (Minutes)", &relaunchInterval);

		static bool isLoaded = false;
        if (!isLoaded)
        {
            auto loadConfigValue = [&](const std::string& key, std::string& value) {
                if (auto optValue = Config::getInstance().GetStringForKey(key); optValue) {
                    value = *optValue;
                }
            };

            loadConfigValue("lastPlaceID", PlaceID);
            loadConfigValue("lastVip", vipCode);
            loadConfigValue("lastDelay", launchDelay);
            loadConfigValue("lastInterval", relaunchInterval);

            isLoaded = true;
        }

		ImGui::PopItemWidth();

		static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
		if (ImGui::TreeNode("Group Color"))
		{
			float availableWidth = ImGui::GetContentRegionAvail().x;
			float widgetWidth = (availableWidth - ImGui::GetStyle().ItemSpacing.y) * 0.40f;
			float totalWidgetWidth = 2 * widgetWidth + ImGui::GetStyle().ItemSpacing.y;

			float spacing = (availableWidth - totalWidgetWidth) / 2.0f;

			ImGui::Dummy(ImVec2(spacing, 0));
			ImGui::SameLine();

			ImGui::SetNextItemWidth(widgetWidth);
			ImGui::ColorPicker3("##MyColor##5", (float*)&color, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(widgetWidth);
			ImGui::ColorPicker3("##MyColor##6", (float*)&color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Auto Inject"))
		{
			ImVec2 buttonSize = ImGui::CalcTextSize("Open DLL File");
			float padding = 20.0f; // Some padding between the InputText and the button
			float inputTextWidth = ImGui::GetContentRegionAvail().x - buttonSize.x - padding;
			ImGui::PushItemWidth(inputTextWidth);

            std::vector<std::string> injectionModes = { "LoadLibrary", "ManualMap" };
            static int selectedInjectionMode = 0;
            ui::RenderCombo("##Injection Mode", injectionModes, selectedInjectionMode);
            this->m_InjectionMode = injectionModes[selectedInjectionMode];

            std::vector<std::string> injectionMethods = { "NtCreateThreadEx", "HijackThread", "SetWindowsHookEx", "QueueUserAPC", "SetWindowLong" };
            static int selectedInjectionMethod = 0;
            ui::RenderCombo("##Injection Method", injectionMethods, selectedInjectionMethod);
            this->m_InjectionMethod = injectionMethods[selectedInjectionMethod];


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

				if (GetOpenFileName(&ofn) == TRUE) {
				}

			}

			ImGui::TreePop();
		}

		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(21, 82, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(25, 92, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(33, 123, 33, 255));

		if (groupName.empty() || PlaceID.empty() || relaunchInterval.empty() || std::count(m_InstanceSelection.begin(), m_InstanceSelection.end(), true) == 0)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Create Group", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			std::vector<std::string> selectedInstances;
			for (int i = 0; i < m_InstanceSelection.size(); ++i)
			{
				if (m_InstanceSelection[i])
				{
					selectedInstances.push_back(m_InstanceNames[i]);
				}
			}

			if (std::find(m_Groups.begin(), m_Groups.end(), groupName) != m_Groups.end())
			{
				// TODO: handle inserting into existing group
			}
			else
			{
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

                g_InstanceControl.CreateGroup(groupInfo);
			}

			Config::getInstance().UpdateConfig<std::string>("lastPlaceID", PlaceID);
			Config::getInstance().UpdateConfig<std::string>("lastVip", vipCode);
			Config::getInstance().UpdateConfig<std::string>("lastDelay", launchDelay);
			Config::getInstance().UpdateConfig<std::string>("lastInterval", relaunchInterval);
		}

		if (groupName.empty() || PlaceID.empty() || relaunchInterval.empty() || std::count(m_InstanceSelection.begin(), m_InstanceSelection.end(), true) == 0)
		{
			ImGui::EndDisabled();
		}

		ImGui::PopStyleColor(3);


		ImGui::EndGroup();
	}


	ImGui::Separator();

	{
		if (ui::BeginSizedListBox("##listbox groups", 0.30f, 1.0f))
		{
			for (int n = 0; n < m_Groups.size(); n++)
			{
				const bool is_selected = m_GroupSelection[n];

				auto groupColor = g_InstanceControl.GetGroupColor(m_Groups[n]);
				ImGui::PushStyleColor(ImGuiCol_Text, groupColor);
				ImGui::Bullet();
				ImGui::PopStyleColor();
				ImGui::SameLine();

				if (ImGui::Selectable(m_Groups[n].c_str(), is_selected))
				{
					std::fill(m_GroupSelection.begin(), m_GroupSelection.end(), false);
					m_GroupSelection[n] = true;
				}
			}

			ImGui::EndListBox();
		}

		ImGui::SameLine();

		ImGui::BeginGroup();

		bool groupPresent = std::none_of(m_GroupSelection.begin(), m_GroupSelection.end(), [](bool b) { return b; });
		ImGui::BeginDisabled(groupPresent);

		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(82, 21, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 25, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(123, 33, 33, 255));

		if (ImGui::Button("Terminate Group", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			for (int i = 0; i < m_GroupSelection.size(); ++i)
			{
				if (m_GroupSelection[i])
				{
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