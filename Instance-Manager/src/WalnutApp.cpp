#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

#include "functions.h"
#include "request.hpp"

class InstanceManager : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Instance Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav);

		static std::vector<std::pair<std::string, std::string>> paths_and_names = StringUtils::extract_paths_and_folders(Native::run_powershell_command("Get-AppxPackage *Roblox* | Select-Object Name, InstallLocation | Format-List Name, InstallLocation"));

		static bool dont_ask_me_next_time = false;
		static int item_current_idx = 0; // Here we store our selection data as an index.
		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 20 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (int n = 0; n < paths_and_names.size(); n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(paths_and_names[n].first.c_str(), is_selected))
					item_current_idx = n;

				if (ImGui::BeginPopupContextItem())
				{
					ImGui::Text(paths_and_names[n].first.c_str());

					if (ImGui::Button("Update Instance"))
					{
						Request uni10req("https://raw.githubusercontent.com/Sightem/UWP-Instance-Manager/dev/WalnutApp/exes.zip");
						uni10req.initalize();


						uni10req.download_file("exes.zip");

						FS::decompress_zip("exes.zip", std::filesystem::current_path().string());

						std::string pathTow10uni = paths_and_names[item_current_idx].second + "\\Windows10Universal.exe";
						std::string pathToCrashHandler = paths_and_names[item_current_idx].second + "\\Assets" + "\\CrashHandler.exe";

						//delete the files in the instance folder if they exist
						if (std::filesystem::exists(pathTow10uni)) {
							std::filesystem::remove(pathTow10uni);
						}

						if (std::filesystem::exists(pathToCrashHandler)) {
							std::filesystem::remove(pathToCrashHandler);
						}

						//copy the files to the instance folder
						std::filesystem::copy_file("Windows10Universal.exe", pathTow10uni);
						std::filesystem::copy_file("CrashHandler.exe", pathToCrashHandler);
					}

					ImGui::SameLine();

					//remove item button to test removing items from the list
					ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(82, 21, 21, 255));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 25, 25, 255));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(123, 33, 33, 255));
					if (ImGui::Button("Remove Instance"))
					{
						if (!dont_ask_me_next_time) {
							ImGui::OpenPopup("Delete?");
						}
						else {
							Roblox::nuke_instance(paths_and_names[item_current_idx].first, paths_and_names[item_current_idx].second);

							paths_and_names.erase(paths_and_names.begin() + item_current_idx);
						}
					}
					ImGui::PopStyleColor(3);

					ImVec2 center = ImGui::GetMainViewport()->GetCenter();
					ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

					if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
					{
						ImGui::Text("This will delete all the files related to the instance and unrigester it.\nThis operation cannot be undone!\n\n");
						ImGui::Separator();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
						ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
						ImGui::PopStyleVar();

						if (ImGui::Button("OK", ImVec2(523.0f / 2.0f, 0))) {
							Roblox::nuke_instance(paths_and_names[item_current_idx].first, paths_and_names[item_current_idx].second);

							paths_and_names.erase(paths_and_names.begin() + item_current_idx);

							ImGui::CloseCurrentPopup();
						}

						ImGui::SetItemDefaultFocus();
						ImGui::SameLine();
						if (ImGui::Button("Cancel", ImVec2(523.0f / 2.0f, 0))) { ImGui::CloseCurrentPopup(); }
						ImGui::EndPopup();
					}
					ImGui::EndPopup();
				}
			}
			ImGui::EndListBox();
		}

		static std::string instance_name_buf;

		ImGui::PushItemWidth(ImGui::GetWindowWidth() - 162.0f);

		ui::InputTextWithHint("##NameStr", "Input the instance name here...", &instance_name_buf);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(R"(Disallowed characters: <>:\"/\\|?*\t\n\r )");


		ImGui::SameLine();
		ImGui::PopItemWidth();

		//ImGui::PopItemWidth();

		if (instance_name_buf.empty() || StringUtils::contains_only(instance_name_buf, '\0'))
			ImGui::BeginDisabled(true);

		//add item button to test adding items to the list
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(21, 82, 21, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(25, 92, 25, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(33, 123, 33, 255));
		if (ImGui::Button("Create Instance"))
		{

			std::string buf = FS::replace_pattern_in_file("Template\\AppxManifest.xml", "{INSTANCENAME}", instance_name_buf);

			std::filesystem::create_directory("instances\\" + instance_name_buf);

			FS::copy_directory("Template", "instances\\" + instance_name_buf);

			std::ofstream ofs("instances\\" + instance_name_buf + "\\AppxManifest.xml", std::ofstream::out | std::ofstream::trunc);
			ofs << buf;
			ofs.close();

			std::string abs_path = std::filesystem::absolute("instances\\" + instance_name_buf + "\\AppxManifest.xml").string();
			std::string cmd = "Add-AppxPackage -path \"" + abs_path + "\" -register";
			Native::run_powershell_command(cmd);

			//add the new instance to the listbox
			paths_and_names.push_back(std::make_pair("ROBLOXCORPORATION.ROBLOX." + instance_name_buf, std::filesystem::absolute("instances\\" + instance_name_buf).string()));

		}
		ImGui::PopStyleColor(3);

		if (instance_name_buf.empty() || StringUtils::contains_only(instance_name_buf, '\0'))
			ImGui::EndDisabled();

		ImGui::Separator();

		bool isInstanceButtonDisabled = paths_and_names.empty() || item_current_idx < 0 || item_current_idx >= paths_and_names.size();

		//ImGui::Text("Selected: %s", paths_and_names[item_current_idx].first.c_str());
		ImGui::SameLine();

		ImGui::End();



		ImGui::ShowDemoWindow();
	}
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	// Create the instances directory if it doesn't exist
	std::filesystem::create_directory("instances");


	if (!std::filesystem::exists("Template"))
	{
		if (!std::filesystem::exists("Template.zip"))
		{
			std::cout << "Downloading template..." << std::endl;
			Request req("https://cdn.discordapp.com/attachments/975002988764594226/1132863713322467368/Template.zip");
			req.initalize();

			req.download_file("Template.zip");


			FS::decompress_zip("Template.zip", std::filesystem::current_path().string());

		}
		else
		{
			FS::decompress_zip("Template.zip", std::filesystem::current_path().string());
		}

		Request req("https://raw.githubusercontent.com/Sightem/UWP-Instance-Manager/dev/WalnutApp/exes.zip");
		req.initalize();

		req.download_file("exes.zip");

		FS::decompress_zip("exes.zip", std::filesystem::current_path().string());

		//copy it to the template folder
		std::filesystem::copy_file("Windows10Universal.exe", "Template\\Windows10Universal.exe");

		//copy it to the template folder
		std::filesystem::copy_file("CrashHandler.exe", "Template\\Assets\\CrashHandler.exe");


	}

	if (!Native::enable_developer_mode())
	{
		std::cout << "Failed to enable developer mode" << std::endl;
		system("pause");
		g_ApplicationRunning = false;
	}


	Walnut::ApplicationSpecification spec;
	spec.Name = "Roblox UWP Instance Manager";
	spec.CustomTitlebar = false;

	Walnut::Application* app = new Walnut::Application(spec);
	std::shared_ptr<InstanceManager> instanceManager = std::make_shared<InstanceManager>();
	app->PushLayer(instanceManager);
	return app;
}