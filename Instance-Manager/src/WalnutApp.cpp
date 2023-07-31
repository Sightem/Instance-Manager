#define NOMINMAX
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

#define NOMINMAX


class InstanceManager : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Instance Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav);

		static bool dont_ask_me_next_time = false;
		static int item_current_idx = 0;
		static std::string placeid = "";
		static std::string linkcode = "";
		if (selection.size() != instances.size())
		{
			selection.resize(instances.size());
		}

		static ImGuiTextFilter filter;
		filter.Draw("##Filter", -FLT_MIN);

		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 20 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (int n = 0; n < instances.size(); n++)
			{
				const std::string& path_name = instances[n].Username;

				if (filter.PassFilter(path_name.c_str()))
				{
					const bool is_selected = selection[n];
					if (ImGui::Selectable(instances[n].Username.c_str(), is_selected))
					{
						if (!ImGui::GetIO().KeyCtrl)  // Clear selection when CTRL is not held
							std::fill(selection.begin(), selection.end(), false);
						selection[n] = !selection[n];
					}

					if (ImGui::BeginPopupContextItem())
					{
						std::string selectedNames = "Selected: ";
						bool isFirst = true;
						for (int i = 0; i < selection.size(); i++)
						{
							if (selection[i])
							{
								if (!isFirst)
								{
									selectedNames += ", ";
								}
								selectedNames += instances[i].Username;
								isFirst = false;
							}
						}

						ImGui::Text(selectedNames.c_str());

						ImGui::Separator();

						ImGui::PushItemWidth(130.0f);

						ui::InputTextWithHint("##placeid", "PlaceID", &placeid);

						ImGui::SameLine();

						ui::InputTextWithHint("##linkcode", "VIP link code", &linkcode);

						ImGui::PopItemWidth();

						ImGui::SameLine();

						RenderLaunch(placeid, linkcode);

						ImGui::SameLine();
						
						RenderTerminate();

						RenderUpdateInstance();
	
						ImGui::SameLine();

						if (ImGui::Button("Copy installation path"))
						{
							StringUtils::copy_to_clipboard(instances[n].InstallLocation);
							ImGui::CloseCurrentPopup();
						}

						ImGui::SameLine();

						if (ImGui::Button("Copy AppData path"))
						{
							std::string path = FS::find_files("C:\\Users\\" + Native::get_current_username() + "\\AppData\\Local\\Packages", instances[item_current_idx].Name)[0];

							StringUtils::copy_to_clipboard(path);
						}

						ImGui::SameLine();

						RenderRemoveInstances(selection, &dont_ask_me_next_time);

						ImVec2 center = ImGui::GetMainViewport()->GetCenter();
						ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

						if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
						{
							int num_selected = std::count(selection.begin(), selection.end(), true);

							if (num_selected == 1) {
								ImGui::Text("This will delete all the files related to the selected instance and unregister it.\nThis operation cannot be undone!\n\n");
							}
							else {
								ImGui::Text("This will delete all the files related to the %d selected instances and unregister them.\nThis operation cannot be undone!\n\n", num_selected);
							}
							ImGui::Separator();

							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
							ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
							ImGui::PopStyleVar();

							if (ImGui::Button("OK", ImVec2(523.0f / 2.0f, 0))) {
								log.add_log("Deleting selected instances...");

								std::set<int> selected_indices;
								for (int i = 0; i < selection.size(); ++i) {
									if (selection[i]) {
										selected_indices.insert(i);
									}
								}
								DeleteInstances(selected_indices);

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

		RenderCreateInstance(instance_name_buf);

		ImGui::Separator();

		log.draw("Log");

		ImGui::End();

		ImGui::ShowDemoWindow();
	}

private:
	std::vector<UserInstance> instances = Roblox::process_roblox_packages();
	std::vector<bool> selection;
	std::vector<std::future<void>> instance_update_button_futures;
	ThreadManager thread_manager;
	QueuedThreadManager queued_thread_manager;
	AppLog log;

	void LaunchInstances(const std::set<int>& indices, std::string placeid, std::string linkcode)
	{
		log.add_log("Launching instances...");

		for (auto idx : indices)
		{
			auto callback = [idx, this]() {
				log.add_log("Launched {}", instances[idx].Username);
			};

			log.add_log("Launching {}...", instances[idx].Username);

			queued_thread_manager.submit_task("launchInstance" + std::to_string(idx), [idx, placeid, linkcode, this]() {
				std::string appid = instances[idx].AppID;

				if (linkcode.empty())
					Roblox::launch_roblox(instances[idx].AppID, placeid);
				else
					Roblox::launch_roblox(instances[idx].AppID, placeid, linkcode);

				//try 5 times to find the roblox process
				for (int i = 0; i < 5; i++)
				{
					auto pids = Roblox::get_roblox_instances();
					for (auto pid : pids)
					{
						std::string command_line = Native::get_commandline_arguments(pid);
						if (command_line.find(instances[idx].Username) != std::string::npos)
						{
							instances[idx].ProcessID = pid;
							return;
						}
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				}, callback);
		}
	}

	void RenderLaunch(std::string placeid, std::string linkcode)
	{
		bool any_selected = std::any_of(selection.begin(), selection.end(), [](bool selected) { return selected; });

		if (!any_selected)
			return;

		if (ui::GreenButton("Launch"))
		{
			std::set<int> selected_indices;
			for (int i = 0; i < selection.size(); ++i)
			{
				if (selection[i])
				{
					selected_indices.insert(i);
				}
			}

			LaunchInstances(selected_indices, placeid, linkcode);
			ImGui::CloseCurrentPopup();
		}
	}

	void RenderTerminate()
	{
		bool any_selected = std::any_of(selection.begin(), selection.end(), [](bool selected) { return selected; });

		if (!any_selected)
			return;

		if (ui::RedButton("Terminate"))
		{
			std::set<int> selected_indices;
			for (int i = 0; i < selection.size(); ++i)
			{
				if (selection[i])
				{
					selected_indices.insert(i);
				}
			}

			for (auto idx : selected_indices)
			{
				if (instances[idx].ProcessID != 0)
				{
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, instances[idx].ProcessID);
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);

					instances[idx].ProcessID = 0;
				}
			}
		}
	}

	void DeleteInstances(const std::set<int>& indices)
	{
		log.add_log("Deleting instances...");

		// It's safer to erase items from a vector in reverse order
		auto sorted_indices = indices;
		for (auto it = sorted_indices.rbegin(); it != sorted_indices.rend(); ++it)
		{
			int idx = *it;
			thread_manager.submit_task("deleteInstance", [idx, this]() {
				Roblox::nuke_instance(instances[idx].Name, instances[idx].InstallLocation);
				instances.erase(instances.begin() + idx);
				}, [&]() {
					log.add_log("Instance Deleted");
				});
		}
	}

	void RenderRemoveInstances(const std::vector<bool>& selection, bool* dont_ask_me_next_time)
	{
		bool any_selected = std::any_of(selection.begin(), selection.end(), [](bool selected) { return selected; });

		if (!any_selected)
			return;

		if (ui::ConditionalButton("Remove Selected Instances", true, ui::ButtonStyle::Red))
		{
			if (!(*dont_ask_me_next_time))
			{
				ImGui::OpenPopup("Delete?");
			}
			else
			{
				std::set<int> selected_indices;
				for (int i = 0; i < selection.size(); ++i)
				{
					if (selection[i])
					{
						selected_indices.insert(i);
					}
				}
				DeleteInstances(selected_indices);
			}
		}
	}

	void RenderCreateInstance(std::string instance_name_buf)
	{
		if (ui::ConditionalButton("Create instance", !(instance_name_buf.empty() || StringUtils::contains_only(instance_name_buf, '\0')), ui::ButtonStyle::Green))
		{
			log.add_log("Creating instance...");

			auto completionCallback = [&]() {
				log.add_log("Instance created");
				instances = Roblox::process_roblox_packages();
				selection.resize(instances.size(), false);
			};

			thread_manager.submit_task("createInstance", [instance_name_buf]() {
				std::string buf = FS::replace_pattern_in_file("Template\\AppxManifest.xml", "{INSTANCENAME}", instance_name_buf);

				std::filesystem::create_directory("instances\\" + instance_name_buf);

				FS::copy_directory("Template", "instances\\" + instance_name_buf);

				std::ofstream ofs("instances\\" + instance_name_buf + "\\AppxManifest.xml", std::ofstream::out | std::ofstream::trunc);
				ofs << buf;
				ofs.close();

				std::string abs_path = std::filesystem::absolute("instances\\" + instance_name_buf + "\\AppxManifest.xml").string();
				std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
 				Native::run_powershell_command(cmd);
				}, completionCallback);
		}
	}

	void RenderUpdateInstance()
	{
		bool any_selected = std::any_of(selection.begin(), selection.end(), [](bool selected) { return selected; });

		if (!any_selected)
			return;


		if (ImGui::Button("Update Instance"))
		{
			std::set<int> selected_indices;
			for (int i = 0; i < selection.size(); ++i)
			{
				if (selection[i])
				{
					selected_indices.insert(i);
				}
			}

			for (auto idx : selected_indices)
			{
				auto callback = [idx, this]() {
					std::string abs_path = std::filesystem::absolute(instances[idx].InstallLocation + "\\AppxManifest.xml").string();
					std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
					Native::run_powershell_command(cmd);

					log.add_log("Update Done");
				};


				queued_thread_manager.submit_task(fmt::format("updateinstances{}", idx), [idx, this]() {
					log.add_log("Updating {}...", instances[idx].Name);

					thread_manager.submit_task("updateInstance", [idx, this]() {
						Request win10req("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Windows10Universal.zip");
						win10req.initalize();
						win10req.download_file("Windows10Universal.zip");


						std::string pathTow10uni = instances[idx].InstallLocation + "\\Windows10Universal.exe";

						if (std::filesystem::exists(pathTow10uni)) {
							std::filesystem::remove(pathTow10uni);
						}

						FS::decompress_zip("Windows10Universal.zip", instances[idx].InstallLocation);
						});

					thread_manager.submit_task("updateInstance", [&]() {
						Request crashreq("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Assets/CrashHandler.exe");
						crashreq.initalize();
						crashreq.download_file("CrashHandler.exe");

						std::string pathToCrashHandler = instances[idx].InstallLocation + "\\Assets" + "\\CrashHandler.exe";

						if (std::filesystem::exists(pathToCrashHandler)) {
							std::filesystem::remove(pathToCrashHandler);
						}

						std::filesystem::copy_file("CrashHandler.exe", pathToCrashHandler);
						});

					thread_manager.submit_task("updateInstance", [&]() {
						//this is fucking stupid, gotta improve request.hpp's api
						Request appxml("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/AppxManifest.xml");
						appxml.initalize();
						appxml.download_file("AppxManifest.xml");

						std::string buf = FS::replace_pattern_in_file("AppxManifest.xml", "{INSTANCENAME}", instances[idx].Username);

						std::ofstream ofs(instances[idx].InstallLocation + "\\AppxManifest.xml", std::ofstream::out | std::ofstream::trunc);
						ofs << buf;
						ofs.flush();
						ofs.close();
						});
					}, callback);

			}
			//close the popup
			ImGui::CloseCurrentPopup();
		}
	}

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