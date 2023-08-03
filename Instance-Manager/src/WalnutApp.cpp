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
#include "Management.hpp"
#include "ThreadManager.hpp"

std::vector<UserInstance> instances = Roblox::process_roblox_packages();
std::vector<bool> selection;

class InstanceManager : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Instance Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav);

		static bool dont_ask_me_next_time = false;
		static int item_current_idx = 0;
		static int lastSelectedIndex = -1;
		if (selection.size() != instances.size())
		{
			selection.resize(instances.size());
		}

		static ImGuiTextFilter filter;
		filter.Draw("##Filter", -FLT_MIN);

		if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 20 * ImGui::GetTextLineHeightWithSpacing())))
		{
			if (!ImGui::IsAnyItemActive() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			{
				for (int n = 0; n < instances.size(); ++n)
				{
					const std::string& path_name = instances[n].Username;
					if (filter.PassFilter(path_name.c_str()))
					{
						selection[n] = true;
					}
					else
					{
						selection[n] = false;
					}
				}
			}

			for (int n = 0; n < instances.size(); ++n)
			{
				const std::string& path_name = instances[n].Username;

				if (filter.PassFilter(path_name.c_str()))
				{
					const bool is_selected = selection[n];

					if (instances[n].ProcessID != 0)
					{
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(40, 170, 40, 255));
					}

					if (ImGui::Selectable(instances[n].Username.c_str(), is_selected))
					{
						if (ImGui::GetIO().KeyShift && lastSelectedIndex != -1)
						{
							// Handle range selection
							int start = std::min(lastSelectedIndex, n);
							int end = std::max(lastSelectedIndex, n);
							for (int i = start; i <= end; ++i)
							{
								selection[i] = true; 
							}
						}
						else if (!ImGui::GetIO().KeyCtrl)
						{
							// Clear selection when CTRL is not held
							std::fill(selection.begin(), selection.end(), false);
							selection[n] = true;
						}
						else
						{
							selection[n] = !selection[n];
						}

						lastSelectedIndex = n;
					}

					if (instances[n].ProcessID != 0)
					{
						ImGui::PopStyleColor();
					}

					if (ImGui::BeginPopupContextItem())
					{
						std::string selectedNames = "Selected: ";
						bool isFirst = true;
						for (int i = 0; i < selection.size(); ++i)
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
						
						if (ImGui::TreeNode("Launch control"))
						{
							static std::string placeid = "";
							static std::string linkcode = "";

							ImGui::PushItemWidth(130.0f);

							ui::InputTextWithHint("##placeid", "PlaceID", &placeid);

							ImGui::SameLine();

							ui::InputTextWithHint("##linkcode", "VIP link code", &linkcode);

							ImGui::PopItemWidth();

							ImGui::SameLine();

							RenderLaunch(placeid, linkcode);
							
							ImGui::SameLine();
							
							RenderTerminate();
							ImGui::TreePop();
						};

						if (ImGui::TreeNode("Settings control"))
						{
							RenderSettings();

							ImGui::TreePop();
						};

						ImGui::Separator();


						RenderUpdateInstance();

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
								applog.add_log("Deleting selected instances...");

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

		applog.draw("Log");

		ImGui::End();
	}

private:
	std::vector<std::future<void>> instance_update_button_futures;
	ThreadManager thread_manager;
	QueuedThreadManager queued_thread_manager;

	void RenderLaunch(std::string placeid, std::string linkcode)
	{
		if (!AnyInstanceSelected())
			return;


		if (ui::GreenButton("Launch"))
		{
			ForEachSelectedInstance([this, placeid, linkcode](int idx) {
				auto callback = [idx, this]() {
					applog.add_log("Launched {}", instances[idx].Username);
				};

				applog.add_log("Launching {}...", instances[idx].Username);

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
			});
			
		}
	}

	void RenderSettings()
	{
		if (!AnyInstanceSelected())
			return;

		static int graphicsquality = 1;
		ImGui::InputInt("Graphics Quality", &graphicsquality);

		static float newmastervolume = 0.8f;
		ImGui::InputFloat("Master volume", &newmastervolume, 0.01f, 1.0f, "%.3f");

		static int newsavedquality = 1;
		ImGui::InputInt("Saved Graphics Quality", &newsavedquality);

		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f))) {
			ForEachSelectedInstance([this](int idx) {
				std::string path = fmt::format("C:\\Users\\{}\\AppData\\Local\\Packages\\{}\\LocalState\\GlobalBasicSettings_13.xml", Native::get_current_username(), instances[idx].PackageFamilyName);

				if (std::filesystem::exists(path))
					Roblox::modify_settings(path, graphicsquality, newmastervolume, newsavedquality);
			});
		}
	}

	void RenderTerminate()
	{
		if (!AnyInstanceSelected())
			return;

		if (ui::RedButton("Terminate"))
		{
			ForEachSelectedInstance([this](int idx) {
				if (instances[idx].ProcessID != 0)
				{
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, instances[idx].ProcessID);
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);

					instances[idx].ProcessID = 0;
				}
			});
		}
	}

	void DeleteInstances(const std::set<int>& indices)
	{
		applog.add_log("Deleting instances...");

		// It's safer to erase items from a vector in reverse order
		auto sorted_indices = indices;
		for (auto it = sorted_indices.rbegin(); it != sorted_indices.rend(); ++it)
		{
			int idx = *it;
			thread_manager.submit_task("deleteInstance", [idx, this]() {
				try
				{
					Roblox::nuke_instance(instances[idx].Name, instances[idx].InstallLocation);
					instances.erase(instances.begin() + idx);
				}
				catch (const std::exception& e)
				{
					applog.add_log("Failed to delete instance {}: {}", instances[idx].Name, e.what());
				}

			}, [&]() {
				applog.add_log("Instance Deleted");
			});
		}
	}

	void RenderRemoveInstances(const std::vector<bool>& selection, bool* dont_ask_me_next_time)
	{
		if (!AnyInstanceSelected())
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
			applog.add_log("Creating instance...");

			auto completionCallback = [&]() {
				applog.add_log("Instance created");
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
		if (!AnyInstanceSelected())
			return;


		if (ImGui::Button("Update Instance"))
		{
			ForEachSelectedInstance([this](int idx) {
				auto callback = [idx, this]() {
					std::string abs_path = std::filesystem::absolute(instances[idx].InstallLocation + "\\AppxManifest.xml").string();
					std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
					Native::run_powershell_command(cmd);

					applog.add_log("Update Done");
				};


				queued_thread_manager.submit_task(fmt::format("updateinstances{}", idx), [idx, this]() {
					applog.add_log("Updating {}...", instances[idx].Name);
					std::thread([idx, this]() {
						std::vector<char8_t> buffer;
						Request win10req("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Windows10Universal.zip");
						win10req.initalize();
						win10req.download_file<char8_t>(buffer);

						Utils::save_to_file("Windows10Universal.zip", buffer);

						std::string pathTow10uni = instances[idx].InstallLocation + "\\Windows10Universal.exe";

						if (std::filesystem::exists(pathTow10uni)) {
							std::filesystem::remove(pathTow10uni);
						}

						FS::decompress_zip("Windows10Universal.zip", instances[idx].InstallLocation);
						}).detach();

					std::thread([idx, this]() {
						std::vector<char8_t> buffer;
						Request crashreq("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Assets/CrashHandler.exe");
						crashreq.initalize();
						crashreq.download_file<char8_t>(buffer);

						Utils::save_to_file("CrashHandler.exe", buffer);

						std::string pathToCrashHandler = instances[idx].InstallLocation + "\\Assets\\CrashHandler.exe";

						if (std::filesystem::exists(pathToCrashHandler)) {
							std::filesystem::remove(pathToCrashHandler);
						}

						std::filesystem::copy_file("CrashHandler.exe", pathToCrashHandler);
						}).detach();

					std::thread([idx, this]() {
						std::vector<char8_t> buffer;

						Request appxml("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/AppxManifest.xml");
						appxml.initalize();
						appxml.download_file<char8_t>(buffer);

						std::string buf = FS::replace_pattern_in_content(buffer, "{INSTANCENAME}", instances[idx].Username);

						std::ofstream ofs(instances[idx].InstallLocation + "\\AppxManifest.xml", std::ofstream::out | std::ofstream::trunc);
						ofs << buf;
						ofs.flush();
						ofs.close();
						}).detach();

					}, callback);
			});


			//close the popup
			ImGui::CloseCurrentPopup();
		}
	}

	bool AnyInstanceSelected() const
	{
		return std::any_of(selection.begin(), selection.end(), [](bool selected) { return selected; });
	}

	std::set<int> GetSelectedIndices() const
	{
		std::set<int> selected_indices;
		for (int i = 0; i < selection.size(); ++i)
		{
			if (selection[i])
			{
				selected_indices.insert(i);
			}
		}
		return selected_indices;
	}

	template <typename Func>
	void ForEachSelectedInstance(Func func) const
	{
		auto selected_indices = GetSelectedIndices();
		for (auto idx : selected_indices)
		{
			func(idx);
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
		//system("pause");
		//g_ApplicationRunning = false;
	}


	Walnut::ApplicationSpecification spec;
	spec.Name = "Roblox UWP Instance Manager";
	spec.CustomTitlebar = false;

	Walnut::Application* app = new Walnut::Application(spec);
	std::shared_ptr<InstanceManager> instanceManager = std::make_shared<InstanceManager>();
	std::shared_ptr<Management> management = std::make_shared<Management>(instances, selection);
	app->PushLayer(instanceManager);
	app->PushLayer(management);
	return app;
}