#include "ui/InstanceManager.h"

#include <opencv2/opencv.hpp>

#include "config/Config.hpp"
#include "imgui_stdlib.h"
#include "instance-control/InstanceControl.h"
#include "logging/CoreLogger.hpp"
#include "logging/FileLogger.hpp"
#include "roblox/Roblox.h"
#include "ui/AppLog.h"
#include "ui/CustomWidgets.hpp"
#include "ui/UI.h"
#include "utils/Utils.hpp"
#include "utils/string/StringUtils.h"

std::vector<std::string> g_InstanceNames = g_InstanceControl.GetInstanceNames();
std::vector<bool> g_Selection;

InstanceManager::InstanceManager() : m_FileManagement(g_InstanceNames, g_Selection),
                                     m_AutoRelaunch(g_InstanceNames),
                                     m_QueuedThreadPool(1),
                                     m_ThreadPool(4) {}

void InstanceManager::StartUp() {
	std::ranges::sort(g_InstanceNames, [](const std::string& a, const std::string& b) {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
		                                    [](char ca, char cb) {
			                                    return std::tolower(ca) < std::tolower(cb);
		                                    });
	});

	std::filesystem::create_directory("Instances");

	if (!std::filesystem::exists("config.json")) {
		std::ofstream ofs("config.json", std::ofstream::out | std::ofstream::trunc);

		const nlohmann::json j = {
		        {"lastPlaceID", ""},
		        {"lastVip", ""},
		        {"lastDelay", ""},
		        {"lastInterval", ""},
		        {"lastInjectDelay", ""},
		};

		ofs << j.dump(4);

		ofs.flush();
		ofs.close();
	}

	Config::getInstance().Load("config.json");

	if (!std::filesystem::exists("logs")) {
		std::filesystem::create_directory("logs");
	}

	const std::filesystem::path logPath = std::filesystem::current_path() / "logs";
	FileLogger& fileLogger = FileLogger::GetInstance();
	fileLogger.Initialize(logPath);
}

void InstanceManager::Update() {
	ImGui::Begin("Instance Manager", nullptr);

	if (g_Selection.size() != g_InstanceNames.size()) {
		g_Selection.resize(g_InstanceNames.size());
	}

	static ImGuiTextFilter filter;
	filter.Draw("##Filter", -FLT_MIN);

	if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y * (2.0f / 3.0f)))) {
		ui::HandleSelectAll(g_InstanceNames, g_Selection, filter);

		for (int n = 0; n < g_InstanceNames.size(); ++n) {
			const std::string& instanceName = g_InstanceNames[n];

			if (filter.PassFilter(instanceName.c_str())) {
				auto color = g_InstanceControl.GetColor(g_InstanceNames[n]);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::Bullet();
				ImGui::PopStyleColor();
				ImGui::SameLine();

				static int lastSelectedIndex = -1;
				if (ImGui::Selectable(g_InstanceNames[n].c_str(), g_Selection[n])) {
					ui::HandleMultiSelection(g_Selection, n, lastSelectedIndex);
				}

				ui::HandleRightClickSelection(g_Selection, n, lastSelectedIndex);

				RenderContextMenu(n);
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
}

void InstanceManager::RenderContextMenu(int n) {
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

		RenderLaunch();

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

void InstanceManager::RenderProcessControl() {
	if (ImGui::TreeNode("Process Control")) {
		static int cpucores = 1;
		ImGui::InputInt("CPU Cores", &cpucores);

		ImGui::SameLine();

		ui::HelpMarker("This will set the affinity of the process to the selected amount of cores.");

		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f))) {
			Utils::ForEachSelectedInstance(g_Selection, [&](int idx) {
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
			std::thread([n]() {
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

void InstanceManager::RenderLaunch() {
	static auto& config = Config::getInstance().Get();
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

		RenderLaunchButton(placeID, linkCode, launchDelay);

		ImGui::SameLine();

		RenderTerminate();
		ImGui::TreePop();
	}
}

void InstanceManager::RenderLaunchButton(const std::string& placeid, const std::string& linkcode, double launchdelay) {
	if (!AnyInstanceSelected())
		return;


	if (ui::GreenButton("Launch")) {
		Utils::ForEachSelectedInstance(g_Selection, [this, placeid, linkcode, launchdelay](int idx) {
			auto callback = [idx]() {
				CoreLogger::Log(LogLevel::INFO, "Launched {}", g_InstanceNames[idx]);
			};

			CoreLogger::Log(LogLevel::INFO, "Launching {}...", g_InstanceNames[idx]);

			this->m_QueuedThreadPool.SubmitTask([launchdelay]() {
				std::this_thread::sleep_for(std::chrono::milliseconds((int) (launchdelay * 1000)));
			});

			this->m_QueuedThreadPool.SubmitTask(callback, [idx, placeid, linkcode]() {
				g_InstanceControl.LaunchInstance(g_InstanceNames[idx], placeid, linkcode);
			});
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
		ImGui::InputScalarWithRange("Master volume", &newMasterVolume, 0.0f, 1.0f, 0.1f, 0.1f, "%.1f");

		static int newSavedQuality = 1;
		ImGui::InputScalarWithRange("Saved Graphics Quality", &newSavedQuality, 1, 10, 1, 1, "%d");

		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f))) {
			Utils::ForEachSelectedInstance(g_Selection, [](int idx) {
				const std::string path = fmt::format(R"({}\AppData\Local\Packages\{}\LocalState\GlobalBasicSettings_13.xml)", Native::GetUserProfilePath(), g_InstanceControl.GetInstance(g_InstanceNames[idx]).PackageFamilyName);

				if (std::filesystem::exists(path)) {
					std::vector<Roblox::Setting> settings = {
					        {"int", "GraphicsQualityLevel", graphicsQuality},
					        {"float", "MasterVolume", newMasterVolume},
					        {"token", "SavedQualityLevel", newSavedQuality}};

					try {
						Roblox::ModifySettings(path, settings);
					} catch (const std::exception& e) {
						CoreLogger::Log(LogLevel::ERR, "Failed to modify settings for instance {}: {}", g_InstanceNames[idx], e.what());
					}
				} else {
					CoreLogger::Log(LogLevel::ERR, "Failed to modify settings for instance {}, Launch the instance before modifying the settings", g_InstanceNames[idx]);
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
		Utils::ForEachSelectedInstance(g_Selection, [](int idx) {
			CoreLogger::Log(LogLevel::INFO, "Terminating {}", g_InstanceNames[idx]);
			g_InstanceControl.TerminateInstance(g_InstanceNames[idx]);
		});
	}
}

void InstanceManager::SubmitDeleteTask(int idx) {
	const std::string instanceName = g_InstanceNames[idx];

	auto callback = [instanceName, idx]() {
		g_InstanceNames.erase(g_InstanceNames.begin() + idx);
		CoreLogger::Log(LogLevel::INFO, "{} has been deleted", instanceName);
	};

	this->m_QueuedThreadPool.SubmitTask(callback, [instanceName]() {
		try {
			g_InstanceControl.DeleteInstance(instanceName);
		} catch (const std::exception& e) {
			CoreLogger::Log(LogLevel::WARNING, "Failed to delete {}: {}", instanceName, e.what());
		}
	});
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
			Utils::ForEachSelectedInstance(g_Selection, [this](int idx) {
				SubmitDeleteTask(idx);
			});
		}
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("This will delete all the files related to the selected instances and unregister it.\nThis operation cannot be undone!\n\n");
		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Checkbox("Don't ask me next time", &removeDontAskMeNextTime);
		ImGui::PopStyleVar();

		if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x / 2.0f, 0))) {
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
		if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x, 0))) { ImGui::CloseCurrentPopup(); }
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

		this->m_QueuedThreadPool.SubmitTask(completionCallback, []() {
			g_InstanceControl.CreateInstance(instanceNameBuf);
		});
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

		this->m_QueuedThreadPool.SubmitTask(callback, []() {
			Utils::UpdatePackage("Template");
		});
	}
}


void InstanceManager::RenderUpdateInstance() {
	if (!AnyInstanceSelected())
		return;

	if (ImGui::Button("Update Instance")) {
		Utils::ForEachSelectedInstance(g_Selection, [this](int idx) {
			auto callback = [idx]() {
				std::string abs_path = std::filesystem::absolute(g_InstanceControl.GetInstance(g_InstanceNames[idx]).InstallLocation + "\\AppxManifest.xml").string();
				std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
				Native::RunPowershellCommand<false>(cmd);
				CoreLogger::Log(LogLevel::INFO, "Update Done");
			};
			this->m_QueuedThreadPool.SubmitTask(callback, [idx]() {
				CoreLogger::Log(LogLevel::INFO, "Updating {}...", g_InstanceNames[idx]);
				Utils::UpdatePackage(g_InstanceControl.GetInstance(g_InstanceNames[idx]).InstallLocation, g_InstanceNames[idx]);
			});
		});
		ImGui::CloseCurrentPopup();
	}
}

bool InstanceManager::AnyInstanceSelected() {
	return std::any_of(g_Selection.begin(), g_Selection.end(), [](bool selected) { return selected; });
}