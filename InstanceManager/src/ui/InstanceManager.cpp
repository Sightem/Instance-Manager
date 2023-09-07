#include "ui/InstanceManager.h"

#include <opencv2/opencv.hpp>

#include "config/Config.hpp"
#include "imgui_stdlib.h"
#include "instance-control/InstanceControl.h"
#include "logging/CoreLogger.hpp"
#include "logging/FileLogger.hpp"
#include "roblox/Roblox.h"
#include "ui/AppLog.hpp"
#include "ui/CustomWidgets.hpp"
#include "ui/UI.h"
#include "utils/Utils.h"
#include "utils/string/StringUtils.h"
#include "utils/threadpool/ThreadPool.hpp"

std::vector<std::string> g_InstanceNames = g_InstanceControl.GetInstanceNames();
std::vector<bool> g_Selection;

InstanceManager::InstanceManager() : m_FileManagement(g_InstanceNames, g_Selection),
                                     m_AutoRelaunch(g_InstanceNames) {}

template<typename Func>
void ForEachSelectedInstance(Func func) {
	for (int i = 0; i < g_Selection.size(); ++i) {
		if (g_Selection[i]) {
			func(i);
		}
	}
}

void InstanceManager::StartUp() {
	std::ranges::sort(g_InstanceNames, [](const std::string& a, const std::string& b) {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
		                                    [](char ca, char cb) {
			                                    return std::tolower(ca) < std::tolower(cb);
		                                    });
	});

	// Create the Instances directory if it doesn't exist
	std::filesystem::create_directory("Instances");

	//if config.json doesn't exist, create it
	if (!std::filesystem::exists("config.json")) {
		std::ofstream ofs("config.json", std::ofstream::out | std::ofstream::trunc);
		nlohmann::json j;
		j["lastPlaceID"] = "";
		j["lastVip"] = "";
		j["lastDelay"] = "";
		j["lastInterval"] = "";
		j["lastInjectDelay"] = "";

		//save to file
		ofs << j.dump(4);

		ofs.flush();
		ofs.close();
	}

	// load config.json
	Config::getInstance().Load("config.json");

	if (!std::filesystem::exists("logs")) {
		std::filesystem::create_directory("logs");
	}

	std::string logPath = std::filesystem::current_path().string() + "\\logs";

	FileLogger& fileLogger = FileLogger::GetInstance();
	fileLogger.Initialize(logPath);
}

void InstanceManager::Update() {
	ImGui::Begin("Instance Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav);

	static auto& config = Config::getInstance().Get();
	static int lastSelectedIndex = -1;
	if (g_Selection.size() != g_InstanceNames.size()) {
		g_Selection.resize(g_InstanceNames.size());
	}

	static ImGuiTextFilter filter;
	filter.Draw("##Filter", -FLT_MIN);

	if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 25 * ImGui::GetTextLineHeightWithSpacing()))) {
		if (!ImGui::IsAnyItemActive() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A))) {
			for (int n = 0; n < g_InstanceNames.size(); ++n) {
				const std::string& path_name = g_InstanceNames[n];
				if (filter.PassFilter(path_name.c_str())) {
					g_Selection[n] = true;
				} else {
					g_Selection[n] = false;
				}
			}
		}

		for (int n = 0; n < g_InstanceNames.size(); ++n) {
			const std::string& instanceName = g_InstanceNames[n];

			if (filter.PassFilter(instanceName.c_str())) {
				auto color = g_InstanceControl.GetColor(g_InstanceNames[n]);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::Bullet();
				ImGui::PopStyleColor();
				ImGui::SameLine();

				const bool is_selected = g_Selection[n];
				if (ImGui::Selectable(g_InstanceNames[n].c_str(), is_selected)) {
					if (ImGui::GetIO().KeyShift && lastSelectedIndex != -1) {
						// Handle range selection
						int start = std::min(lastSelectedIndex, n);
						int end = std::max(lastSelectedIndex, n);
						for (int i = start; i <= end; ++i) {
							g_Selection[i] = true;
						}
					} else if (!ImGui::GetIO().KeyCtrl) {
						// Clear selection when CTRL is not held
						std::fill(g_Selection.begin(), g_Selection.end(), false);
						g_Selection[n] = true;
					} else {
						g_Selection[n] = !g_Selection[n];
					}

					lastSelectedIndex = n;
				}

				// This fixes an annoying bug where right clicking an instance right after launching it would make the program crash
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
					// If no m_Instances or only one instance is selected, or the right clicked instance isn't part of the selection
					if (std::count(g_Selection.begin(), g_Selection.end(), true) <= 1 || !g_Selection[n]) {
						std::fill(g_Selection.begin(), g_Selection.end(), false);// Clear other selections
						g_Selection[n] = true;                                   // Select the current instance
						lastSelectedIndex = n;                                   // Update the last selected index
					}
				}

				if (ImGui::BeginPopupContextItem()) {
					std::string selectedNames = "Selected: ";
					bool isFirst = true;
					for (int i = 0; i < g_Selection.size(); ++i) {
						if (g_Selection[i]) {
							if (!isFirst) {
								selectedNames += ", ";
							}
							selectedNames += g_InstanceNames[i];
							isFirst = false;
						}
					}

					ImGui::Text(selectedNames.c_str());

					ImGui::Separator();

					if (ImGui::TreeNode("Launch control")) {
						static std::string placeID = config["lastPlaceID"];
						static std::string linkCode = config["lastVip"];

						ImGui::PushItemWidth(130.0f);

						ImGui::InputTextWithHint("##placeid", "PlaceID", &placeID, ImGuiInputTextFlags_CharsDecimal);

						ImGui::SameLine();

						ImGui::InputTextWithHint("##linkcode", "VIP link code", &linkCode, ImGuiInputTextFlags_CharsDecimal);

						ImGui::SameLine();

						static double launchDelay = 0;
						ImGui::InputDouble("##delay", &launchDelay);

						ImGui::PopItemWidth();

						ImGui::SameLine();

						RenderLaunch(placeID, linkCode, launchDelay);

						ImGui::SameLine();

						RenderTerminate();
						ImGui::TreePop();
					}

					RenderSettings();

					if (std::count(g_Selection.begin(), g_Selection.end(), true) > 1 || !g_InstanceControl.IsInstanceRunning(g_InstanceNames[n]))
						ImGui::BeginDisabled();

					RenderProcessControl();

					RenderAutoLogin(n);

					if (std::count(g_Selection.begin(), g_Selection.end(), true) > 1 || !g_InstanceControl.IsInstanceRunning(g_InstanceNames[n]))
						ImGui::EndDisabled();

					ImGui::Separator();


					RenderUpdateInstance();

					ImGui::SameLine();

					RenderRemoveInstances();
				}
			}
		}
		ImGui::EndListBox();
	}
	RenderCreateInstance();

	ImGui::SameLine();

	RenderUpdateTemplate();

	ImGui::Separator();

	m_AppLog.Draw();

	ImGui::End();

	m_FileManagement.Draw("File Management");
	m_AutoRelaunch.Draw("Auto Relaunch");

	ImGui::ShowDemoWindow();
}

void InstanceManager::RenderProcessControl() {
	if (ImGui::TreeNode("Process Control")) {
		static int cpucores = 1;
		ImGui::InputInt("CPU Cores", &cpucores);

		ImGui::SameLine();

		ui::HelpMarker("This will set the affinity of the process to the selected amount of cores.");

		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f))) {
			ForEachSelectedInstance([&](int idx) {
				auto& mgr = g_InstanceControl.GetManager(g_InstanceNames[idx]);

				if (!Native::SetProcessAffinity(mgr.GetPID(), cpucores)) {
					CoreLogger::Log(LogLevel::ERR, "Failed to set affinity for instance {}", g_InstanceNames[idx]);
				} else {
					CoreLogger::Log(LogLevel::INFO, "Set affinity for instance {}", g_InstanceNames[idx]);
				}
			});
		}

		ImGui::TreePop();
	};
}
void InstanceManager::RenderAutoLogin(int n) {
	if (ImGui::TreeNode("Auto Login")) {
		static std::string cookie;

		ImGui::InputTextWithHint("##cookie", "Cookie", &cookie);

		if (cookie.empty())
			ImGui::BeginDisabled();

		if (ImGui::Button("Login", ImVec2(320.0f, 0.0f))) {
			std::thread([this, n]() {
				CoreLogger::Log(LogLevel::INFO, "Beginning auto login for {}", g_InstanceNames[n]);

				auto start = std::chrono::high_resolution_clock::now();

				auto it = std::ranges::find_if(g_Selection, [](bool val) { return val; });

				DWORD pid = 0;
				if (it != g_Selection.end()) {
					int64_t index = std::distance(g_Selection.begin(), it);
					pid = g_InstanceControl.GetManager(g_InstanceNames[index]).GetPID();
				}

				HWND hWnd = FindWindow(NULL, g_InstanceControl.GetInstance(g_InstanceNames[n]).DisplayName.c_str());

				SetForegroundWindow(hWnd);

				Utils::SleepFor(std::chrono::milliseconds(300));

				int x_mid, y_mid;
				std::tie(x_mid, y_mid) = Utils::MatchTemplate("images\\login.png", 0.80);

				if (x_mid != -1 && y_mid != -1) {
					Native::PerformMouseAction(x_mid, y_mid, 150);

					Utils::SleepFor(std::chrono::milliseconds(300));

					Roblox::HandleCodeValidation(pid, cookie);
				} else {
					std::tie(x_mid, y_mid) = Utils::MatchTemplate("images\\anotherdev.png", 0.80);
					if (x_mid != -1 && y_mid != -1) {
						Native::PerformMouseAction(x_mid, y_mid);

						Utils::SleepFor(std::chrono::milliseconds(300));

						Roblox::HandleCodeValidation(pid, cookie);
					}
				}

				auto end = std::chrono::high_resolution_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				CoreLogger::Log(LogLevel::INFO, "Auto login took {} milliseconds", dur.count());
			}).detach();
		}

		if (cookie.empty())
			ImGui::EndDisabled();

		ImGui::TreePop();
	}
}

void InstanceManager::RenderLaunch(std::string placeid, std::string linkcode, double launchdelay) {
	if (!AnyInstanceSelected())
		return;


	if (ui::GreenButton("Launch")) {
		ForEachSelectedInstance([this, placeid, linkcode, launchdelay](int idx) {
			auto callback = [idx]() {
				CoreLogger::Log(LogLevel::INFO, "Launched {}", g_InstanceNames[idx]);
			};

			CoreLogger::Log(LogLevel::INFO, "Launching {}...", g_InstanceNames[idx]);

			this->m_QueuedThreadManager.SubmitTask("delay" + std::to_string(idx), [launchdelay]() {
				std::this_thread::sleep_for(std::chrono::milliseconds((int) (launchdelay * 1000)));
			});

			this->m_QueuedThreadManager.SubmitTask(
			        "launchInstance" + std::to_string(idx), [idx, placeid, linkcode]() {
				        g_InstanceControl.LaunchInstance(g_InstanceNames[idx], placeid, linkcode);
			        },
			        callback);
		});
	}

	auto& config = Config::getInstance().Get();
	config["lastPlaceID"] = placeid;
	config["lastVip"] = linkcode;
}

void InstanceManager::RenderSettings() {
	if (ImGui::TreeNode("Settings control")) {
		if (!AnyInstanceSelected())
			return;

		static int graphicsQuality = 1;
		ImGui::InputScalarWithRange("Graphics Quality", &graphicsQuality, 3, 21, 1, 1, "%d");

		static float newMasterVolume = 0.0f;
		ImGui::InputScalarWithRange("Master volume", &newMasterVolume, 0.0f, 1.0f, 0.01f, 0.1f, "%.1f");

		static int newSavedQuality = 1;
		ImGui::InputScalarWithRange("Saved Graphics Quality", &newSavedQuality, 1, 10, 1, 1, "%d");


		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f))) {
			ForEachSelectedInstance([](int idx) {
				std::string path = fmt::format(R"({}\AppData\Local\Packages\{}\LocalState\GlobalBasicSettings_13.xml)", Native::GetUserProfilePath(), g_InstanceControl.GetInstance(g_InstanceNames[idx]).PackageFamilyName);

				if (std::filesystem::exists(path))
					Roblox::ModifySettings(path, graphicsQuality, newMasterVolume, newSavedQuality);
				else {
					CoreLogger::Log(LogLevel::ERR, "Failed to modify settings for instance {}, Launch the instance before modifying the settings", g_InstanceNames[idx]);
					return;
				}
			});
		}
		ImGui::TreePop();
	}
}

void InstanceManager::RenderTerminate() {
	if (!AnyInstanceSelected())
		return;

	if (ui::RedButton("Terminate")) {
		ForEachSelectedInstance([](int idx) {
			CoreLogger::Log(LogLevel::INFO, "Terminating {}", g_InstanceNames[idx]);
			g_InstanceControl.TerminateInstance(g_InstanceNames[idx]);
		});
	}
}

void InstanceManager::SubmitDeleteTask(int idx) {
	const std::string instanceName = g_InstanceNames[idx];
	this->m_ThreadManager.SubmitTask(
	        "deleteInstance" + std::to_string(idx), [idx, instanceName]() {
        try
        {
            g_InstanceControl.DeleteInstance(instanceName);
            g_InstanceNames.erase(g_InstanceNames.begin() + idx);
        }
        catch (const std::exception& e)
        {
            CoreLogger::Log(LogLevel::WARNING, "Failed to delete {}: {}", instanceName, e.what());
        } }, [instanceName]() { CoreLogger::Log(LogLevel::INFO, "{} has been deleted", instanceName); });
}


void InstanceManager::DeleteInstances(const std::set<int>& indices) {
	CoreLogger::Log(LogLevel::INFO, "Deleting {} instances...", indices.size());

	auto sortedIndices = indices;
	for (int idx: std::ranges::reverse_view(sortedIndices)) {
		SubmitDeleteTask(idx);
	}
}

void InstanceManager::RenderRemoveInstances() {
	static bool removeDontAskMeNextTime = false;

	if (!AnyInstanceSelected())
		return;

	if (ui::ConditionalButton("Remove Selected Instances", true, ui::ButtonStyle::Red)) {
		if (!removeDontAskMeNextTime) {
			ImGui::OpenPopup("Delete?");
		} else {
			ForEachSelectedInstance([this](int idx) {
				SubmitDeleteTask(idx);
			});
		}
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		int64_t num_selected = std::count(g_Selection.begin(), g_Selection.end(), true);

		if (num_selected == 1) {
			ImGui::Text("This will delete all the files related to the selected instance and unregister it.\nThis operation cannot be undone!\n\n");
		} else {
			ImGui::Text("This will delete all the files related to the %d selected m_Instances and unregister them.\nThis operation cannot be undone!\n\n", num_selected);
		}
		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Checkbox("Don't ask me next time", &removeDontAskMeNextTime);
		ImGui::PopStyleVar();

		if (ImGui::Button("OK", ImVec2(523.0f / 2.0f, 0))) {
			std::set<int> selectedIndices;
			for (int i = 0; i < g_Selection.size(); ++i) {
				if (g_Selection[i]) {
					selectedIndices.insert(i);
				}
			}
			DeleteInstances(selectedIndices);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(523.0f / 2.0f, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	ImGui::EndPopup();
}

void InstanceManager::RenderCreateInstance() {
	static std::string instanceNameBuf;

	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 315.0f);

	ImGui::InputTextWithHint("##NameStr", "Input the instance name here...", &instanceNameBuf, ImGuiInputTextFlags_CallbackCharFilter, ui::FilterCallback);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(R"(Disallowed characters: <>:\"/\\|?*\t\n\r )");

	ImGui::PopItemWidth();

	ImGui::SameLine();

	if (ui::ConditionalButton("Create instance", !(instanceNameBuf.empty() || StringUtils::ContainsOnly(instanceNameBuf, '\0')), ui::ButtonStyle::Green)) {
		CoreLogger::Log(LogLevel::INFO, "Creating instance...");

		auto completionCallback = [=]() {
			CoreLogger::Log(LogLevel::INFO, "Instance created");
			std::vector<std::string> new_instances = Roblox::GetNewInstances(g_InstanceNames);

			for (const auto& str: new_instances) {
				auto pos = std::lower_bound(g_InstanceNames.begin(), g_InstanceNames.end(), str,
				                            [](const std::string& a, const std::string& b) {
					                            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
					                                                                [](char ca, char cb) {
						                                                                return std::tolower(ca) < std::tolower(cb);
					                                                                });
				                            });

				g_InstanceNames.insert(pos, str);
			}

			g_Selection.resize(g_InstanceNames.size(), false);
		};

		this->m_ThreadManager.SubmitTask(
		        "createInstance", []() {
			        g_InstanceControl.CreateInstance(instanceNameBuf);
		        },
		        completionCallback);
	}
}

void InstanceManager::RenderUpdateTemplate() {
	auto callback = []() {
		CoreLogger::Log(LogLevel::INFO, "Template updated");
	};
	if (ImGui::Button("Update Template")) {
		CoreLogger::Log(LogLevel::INFO, "Updating template...");

		if (!std::filesystem::exists("Template")) {
			std::filesystem::create_directories("Template\\Assets");
		}

		this->m_QueuedThreadManager.SubmitTask(
		        "updatetemplate", [&]() {
			        Utils::UpdatePackage("Template");
		        },
		        callback);
	}
}


void InstanceManager::RenderUpdateInstance() {
	if (!AnyInstanceSelected())
		return;

	if (ImGui::Button("Update Instance")) {
		ForEachSelectedInstance([this](int idx) {
			auto callback = [idx]() {
				std::string abs_path = std::filesystem::absolute(g_InstanceControl.GetInstance(g_InstanceNames[idx]).InstallLocation + "\\AppxManifest.xml").string();
				std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
				Native::RunPowershellCommand<false>(cmd);
				CoreLogger::Log(LogLevel::INFO, "Update Done");
			};
			this->m_QueuedThreadManager.SubmitTask(
			        fmt::format("updateinstances{}", idx), [idx]() {
				        CoreLogger::Log(LogLevel::INFO, "Updating {}...", g_InstanceNames[idx]);
				        Utils::UpdatePackage(g_InstanceControl.GetInstance(g_InstanceNames[idx]).InstallLocation, g_InstanceNames[idx]);
			        },
			        callback);
		});
		ImGui::CloseCurrentPopup();
	}
}

bool InstanceManager::AnyInstanceSelected() {
	return std::any_of(g_Selection.begin(), g_Selection.end(), [](bool selected) { return selected; });
}