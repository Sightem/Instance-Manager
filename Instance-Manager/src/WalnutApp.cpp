#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <future>
#include <thread>
#include <fmt/format.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

#include "functions.h"
#include "request.hpp"
#include "AppLog.hpp"
#include "ThreadManager.hpp"


class InstanceManager : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Instance Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav);

		static auto paths_and_names = StringUtils::extract_paths_and_folders(Native::run_powershell_command("Get-AppxPackage ROBLOXCORPORATION.ROBLOX.* | Format-List -Property Name, PackageFullName, InstallLocation"));

		static bool dont_ask_me_next_time = false;
		static int item_current_idx = 0; // Here we store our selection data as an index.

		static ImGuiTextFilter filter;
		filter.Draw("##Filter", -FLT_MIN);

		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 20 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (int n = 0; n < paths_and_names.size(); n++)
			{
				const std::string& path_name = std::get<0>(paths_and_names[n]);

				if (filter.PassFilter(path_name.c_str()))
				{

					const bool is_selected = (item_current_idx == n);
					if (ImGui::Selectable(std::get<0>(paths_and_names[n]).c_str(), is_selected))
						item_current_idx = n;

					if (ImGui::BeginPopupContextItem())
					{
						ImGui::Text(std::get<0>(paths_and_names[n]).c_str());

						if (ImGui::Button("Update Instance"))
						{
							log.add_log("Updating {}...", std::get<0>(paths_and_names[item_current_idx]));

							auto completionCallback = [&]() {
								log.add_log("Update Done");
								thread_manager.clear_task_data("updateInstance");
							};

							thread_manager.submit_task("updateInstance", [&]() {
								Request win10req("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Windows10Universal.zip");
								win10req.initalize();
								win10req.download_file("Windows10Universal.zip");

								FS:: decompress_zip("Windows10Universal.zip", std::get<2>(paths_and_names[item_current_idx]));

								std::string pathTow10uni = std::get<2>(paths_and_names[item_current_idx]) + "\\Windows10Universal.exe";

								if (std::filesystem::exists(pathTow10uni)) {
									std::filesystem::remove(pathTow10uni);
								}

								std::filesystem::copy_file("Windows10Universal.exe", pathTow10uni);
								}, completionCallback);

							thread_manager.submit_task("updateInstance", [&]() {
								Request crashreq("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Assets/CrashHandler.exe");
								crashreq.initalize();
								crashreq.download_file("CrashHandler.exe");

								std::string pathToCrashHandler = std::get<2>(paths_and_names[item_current_idx]) + "\\Assets" + "\\CrashHandler.exe";

								if (std::filesystem::exists(pathToCrashHandler)) {
									std::filesystem::remove(pathToCrashHandler);
								}

								std::filesystem::copy_file("CrashHandler.exe", pathToCrashHandler);
							}, completionCallback);


							//close the popup
							ImGui::CloseCurrentPopup();
						}

						ImGui::SameLine();

						if (ImGui::Button("Copy installation path"))
						{
							StringUtils::copy_to_clipboard(std::get<2>(paths_and_names[item_current_idx]));
							ImGui::CloseCurrentPopup();
						}

						ImGui::SameLine();

						if (ImGui::Button("Copy AppData path"))
						{

							std::string path = FS::find_files("C:\\Users\\" + Native::get_current_username() + "\\AppData\\Local\\Packages", std::get<0>(paths_and_names[item_current_idx]))[0];

							StringUtils::copy_to_clipboard(path);
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
								Roblox::nuke_instance(std::get<0>(paths_and_names[item_current_idx]), std::get<2>(paths_and_names[item_current_idx]));

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
								Roblox::nuke_instance(std::get<0>(paths_and_names[item_current_idx]), std::get<2>(paths_and_names[item_current_idx]));

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
			}
			ImGui::EndListBox();
		}

		static std::string instance_name_buf;

		ImGui::PushItemWidth(ImGui::GetWindowWidth() - 162.0f);

		ui::InputTextWithHint("##NameStr", "Input the instance name here...", &instance_name_buf);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(R"(Disallowed characters: <>:\"/\\|?*\t\n\r )");

		ImGui::PopItemWidth();

		ImGui::SameLine();

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
			std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
			Native::run_powershell_command(cmd);

			//do something else here
			paths_and_names.push_back(std::make_tuple("ROBLOXCORPORATION.ROBLOX." + instance_name_buf, "ROBLOXCORPORATION.ROBLOX." + instance_name_buf + "_2.586.0.0_x86__55nm5eh3cm0pr", std::filesystem::absolute("instances\\" + instance_name_buf).string()));

		}
		ImGui::PopStyleColor(3);

		if (instance_name_buf.empty() || StringUtils::contains_only(instance_name_buf, '\0'))
			ImGui::EndDisabled();

		ImGui::Separator();

		log.draw("Log");

		ImGui::End();

		ImGui::ShowDemoWindow();
	}

private:
	std::vector<std::future<void>> instance_update_button_futures;
	ThreadManager thread_manager;
	AppLog log;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	// Create the instances directory if it doesn't exist
	std::filesystem::create_directory("instances");

	if (!Native::enable_developer_mode())
	{
		std::cout << "Failed to enable developer mode" << std::endl;
		system("pause");
		//g_ApplicationRunning = false;
	}


	Walnut::ApplicationSpecification spec;
	spec.Name = "Roblox UWP Instance Manager";
	spec.CustomTitlebar = false;

	Walnut::Application* app = new Walnut::Application(spec);
	std::shared_ptr<InstanceManager> instanceManager = std::make_shared<InstanceManager>();
	app->PushLayer(instanceManager);
	return app;
}