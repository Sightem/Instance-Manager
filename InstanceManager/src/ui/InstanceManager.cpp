#include "ui/InstanceManager.h"


#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "ui/AppLog.hpp"
#include "utils/threadpool/ThreadPool.hpp"
#include "config/Config.hpp"
#include "mouse-controller/MouseController.hpp"
#include "ui/FileManagement.h"
#include "instance-control/InstanceControl.h"
#include "ui/AutoRelaunch.h"

#include "ui/UI.h"
#include "utils/filesystem/FS.h"
#include "native/Native.h"
#include "utils/Utils.h"
#include "utils/string/StringUtils.h"
#include "logging/CoreLogger.hpp"
#include "logging/FileLogger.h"
#include "roblox/Roblox.h"

std::vector<std::string> g_InstanceNames = g_InstanceControl.GetInstanceNames();
std::vector<bool> g_Selection;

MouseController& g_MouseController = MouseController::GetInstance();

InstanceManager::InstanceManager(): m_FileManagement(g_InstanceNames, g_Selection), m_AutoRelaunch(g_InstanceNames)
{}

template <typename Func>
void ForEachSelectedInstance(Func func)
{
	for (int i = 0; i < g_Selection.size(); ++i)
	{
		if (g_Selection[i])
		{
			func(i);
		}
	}
}

void InstanceManager::StartUp()
{
	std::ranges::sort(g_InstanceNames, [](const std::string& a, const std::string& b) {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
		[](char ca, char cb) {
				return std::tolower(ca) < std::tolower(cb);
			}
		);
	});

	// Create the instances directory if it doesn't exist
	std::filesystem::create_directory("instances");

	//if config.json doesn't exist, create it
	if (!std::filesystem::exists("config.json"))
	{
		std::ofstream ofs("config.json", std::ofstream::out | std::ofstream::trunc);
		nlohmann::json j;
		j["lastPlaceID"] = "";
		j["lastVip"] = "";
		j["lastDelay"] = "";
		j["lastInterval"] = "";

		//save to file
		ofs << j.dump(4);

		ofs.flush();
		ofs.close();
	}

	// load config.json
	Config::getInstance().Load("config.json");

	if (!std::filesystem::exists("logs"))
	{
		std::filesystem::create_directory("logs");
	}

	std::string logPath = std::filesystem::current_path().string() + "\\logs";

	FileLogger& fileLogger = FileLogger::GetInstance();
	fileLogger.Initialize(logPath);
}

void InstanceManager::Update()
{
	ImGui::Begin("Instance Manager", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav);

	static auto& config = Config::getInstance().Get();
	static int lastSelectedIndex = -1;
	if (g_Selection.size() != g_InstanceNames.size())
	{
		g_Selection.resize(g_InstanceNames.size());
	}

	static ImGuiTextFilter filter;
	filter.Draw("##Filter", -FLT_MIN);

	if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 25 * ImGui::GetTextLineHeightWithSpacing())))
	{
		if (!ImGui::IsAnyItemActive() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
		{
			for (int n = 0; n < g_InstanceNames.size(); ++n)
			{
				const std::string& path_name = g_InstanceNames[n];
				if (filter.PassFilter(path_name.c_str()))
				{
					g_Selection[n] = true;
				}
				else
				{
					g_Selection[n] = false;
				}
			}
		}

		for (int n = 0; n < g_InstanceNames.size(); ++n)
		{
			const std::string& instanceName = g_InstanceNames[n];

			if (filter.PassFilter(instanceName.c_str()))
			{
				const bool is_selected = g_Selection[n];

				if (g_InstanceControl.IsInstanceRunning(g_InstanceNames[n]))
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
					auto groupColor = g_InstanceControl.IsGrouped(g_InstanceNames[n]);

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
							ImGui::Text("Instance is being Auto Relaunched");

							ImGui::EndTooltip();
						}
					}
				}

				if (ImGui::Selectable(g_InstanceNames[n].c_str(), is_selected))
				{
					if (ImGui::GetIO().KeyShift && lastSelectedIndex != -1)
					{
						// Handle range selection
						int start = std::min(lastSelectedIndex, n);
						int end = std::max(lastSelectedIndex, n);
						for (int i = start; i <= end; ++i)
						{
							g_Selection[i] = true;
						}
					}
					else if (!ImGui::GetIO().KeyCtrl)
					{
						// Clear selection when CTRL is not held
						std::fill(g_Selection.begin(), g_Selection.end(), false);
						g_Selection[n] = true;
					}
					else
					{
						g_Selection[n] = !g_Selection[n];
					}

					lastSelectedIndex = n;
				}

				// This fixes an annoying bug where right clicking an instance right after launching it would make the program crash
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
				{
					// If no instances or only one instance is selected, or the right clicked instance isn't part of the selection
					if (std::count(g_Selection.begin(), g_Selection.end(), true) <= 1 || !g_Selection[n])
					{
						std::fill(g_Selection.begin(), g_Selection.end(), false); // Clear other selections
						g_Selection[n] = true; // Select the current instance
						lastSelectedIndex = n; // Update the last selected index
					}
				}

				if (ImGui::BeginPopupContextItem())
				{
					std::string selectedNames = "Selected: ";
					bool isFirst = true;
					for (int i = 0; i < g_Selection.size(); ++i)
					{
						if (g_Selection[i])
						{
							if (!isFirst)
							{
								selectedNames += ", ";
							}
							selectedNames += g_InstanceNames[i];
							isFirst = false;
						}
					}

					ImGui::Text(selectedNames.c_str());

					ImGui::Separator();

					if (ImGui::TreeNode("Launch control"))
					{
						static std::string placeID = config["lastPlaceID"];
						static std::string linkCode = config["lastVip"];

						ImGui::PushItemWidth(130.0f);

						ui::InputTextWithHint("##placeid", "PlaceID", &placeID);

						ImGui::SameLine();

						ui::InputTextWithHint("##linkcode", "VIP link code", &linkCode);

						ImGui::SameLine();

						static double launchDelay = 0;
						ImGui::InputDouble("##delay", &launchDelay);

						ImGui::PopItemWidth();

						ImGui::SameLine();

						RenderLaunch(placeID, linkCode, launchDelay);

						ImGui::SameLine();

						RenderTerminate();
						ImGui::TreePop();
					};

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

	AppLog::GetInstance().Draw("Log");

	ImGui::End();

	m_FileManagement.Draw("File Management");
	m_AutoRelaunch.Draw("Auto Relaunch");
}

void InstanceManager::RenderProcessControl()
{
	if (ImGui::TreeNode("Process Control"))
	{
		static int cpucores = 1;
		ImGui::InputInt("CPU Cores", &cpucores);

		ImGui::SameLine();

		ui::HelpMarker("This will set the affinity of the process to the selected amount of cores.");

		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f)))
		{
			ForEachSelectedInstance([&](int idx)
				{
					auto mgr = g_InstanceControl.GetManager(g_InstanceNames[idx]);

					if (!Native::SetProcessAffinity(mgr->GetPID(), cpucores))
					{
						CoreLogger::Log(LogLevel::ERR, "Failed to set affinity for instance {}", g_InstanceNames[idx]);
					}
					else
					{
						CoreLogger::Log(LogLevel::INFO, "Set affinity for instance {}", g_InstanceNames[idx]);
					}
				}
			);
		}

		ImGui::TreePop();
	};
}

std::pair<int, int> InstanceManager::MatchTemplate(const std::string& template_path, double threshold) {
	Utils::SaveScreenshotAsPng("screenshot.png");

	cv::Mat screen = cv::imread("screenshot.png");
	cv::Mat templateIMG = cv::imread(template_path);

	cv::Mat result;
	cv::matchTemplate(screen, templateIMG, result, cv::TM_CCOEFF_NORMED);

	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

	if (maxVal > threshold) {
		int xMid = maxLoc.x + (templateIMG.cols / 2);
		int yMid = maxLoc.y + (templateIMG.rows / 2);
		CoreLogger::Log(LogLevel::INFO, "Button found from template {} at location: ({}, {})", template_path.c_str(), xMid, yMid);
		return { xMid, yMid };
	}
	else {
		return { -1, -1 };
	}
}

void InstanceManager::PerformMouseAction(int x_mid, int y_mid, std::optional<int> y_offset)
{
	g_MouseController.MoveMouse(x_mid, y_mid);
	g_MouseController.ClickMouse();

	if (y_offset) {
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		g_MouseController.MoveMouse(x_mid, y_mid + *y_offset);
		g_MouseController.ClickMouse();
	}
}

void InstanceManager::HandleCodeValidation(DWORD pid, const std::string& username, const char* cookie) 
{
	HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	std::string codeValue = Roblox::FindCodeValue(pHandle, username);

	if (codeValue == "") {
		CoreLogger::Log(LogLevel::INFO, "Code value not found");
	}
	else {
		CoreLogger::Log(LogLevel::INFO, "Code value found: {}", codeValue);
		Roblox::EnterCode(codeValue, cookie);
		Roblox::ValidateCode(codeValue, cookie);
	}
}

void InstanceManager::RenderAutoLogin(int n)
{
	if (ImGui::TreeNode("Auto Login"))
	{
		static char cookie[3000] = "";

		ImGui::InputTextWithHint("##cookie", "Cookie", cookie, IM_ARRAYSIZE(cookie));

		if (strlen(cookie) == 0)
			ImGui::BeginDisabled();

		if (ImGui::Button("Login", ImVec2(320.0f, 0.0f)))
		{
			std::thread([this, n]() {
				CoreLogger::Log(LogLevel::INFO, "Beginning auto login for {}", g_InstanceNames[n]);

				auto start = std::chrono::high_resolution_clock::now();

				auto it = std::ranges::find_if(g_Selection, [](bool val) { return val; });

				DWORD pid = 0;
				if (it != g_Selection.end())
				{
					int64_t index = std::distance(g_Selection.begin(), it);
					pid = g_InstanceControl.GetManager(g_InstanceNames[index])->GetPID();
				}

				int x_mid, y_mid;
				std::tie(x_mid, y_mid) = MatchTemplate("images\\login.png", 0.80);

				if (x_mid != -1 && y_mid != -1) {
					PerformMouseAction(x_mid, y_mid, 150);

					std::this_thread::sleep_for(std::chrono::milliseconds(300));

					HandleCodeValidation(pid, g_InstanceNames[n], cookie);
				}
				else {
					std::tie(x_mid, y_mid) = MatchTemplate("images\\anotherdev.png", 0.80);
					if (x_mid != -1 && y_mid != -1) {
						PerformMouseAction(x_mid, y_mid);

						std::this_thread::sleep_for(std::chrono::milliseconds(300));

						HandleCodeValidation(pid, g_InstanceNames[n], cookie);
					}
				}

				auto end = std::chrono::high_resolution_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				CoreLogger::Log(LogLevel::INFO, "Auto login took {} milliseconds", dur.count());
				}).detach();
		}

		if (strlen(cookie) == 0)
			ImGui::EndDisabled();

		ImGui::TreePop();
	}
}

void InstanceManager::RenderLaunch(std::string placeid, std::string linkcode, double launchdelay)
{
	if (!AnyInstanceSelected())
		return;


	if (ui::GreenButton("Launch"))
	{
		ForEachSelectedInstance([this, placeid, linkcode, launchdelay](int idx) {
			auto callback = [idx]() {
				CoreLogger::Log(LogLevel::INFO, "Launched {}", g_InstanceNames[idx]);
			};

			CoreLogger::Log(LogLevel::INFO, "Launching {}...", g_InstanceNames[idx]);

			this->m_QueuedThreadManager.SubmitTask("delay" + std::to_string(idx), [launchdelay]() {
				std::this_thread::sleep_for(std::chrono::milliseconds((int)(launchdelay * 1000)));
				}
			);

			this->m_QueuedThreadManager.SubmitTask("launchInstance" + std::to_string(idx), [idx, placeid, linkcode]() {
				g_InstanceControl.LaunchInstance(g_InstanceNames[idx], placeid, linkcode);

				}, callback);
			});

	}

	auto& config = Config::getInstance().Get();
	config["lastPlaceID"] = placeid;
	config["lastVip"] = linkcode;
}

void InstanceManager::RenderSettings()
{
	if (ImGui::TreeNode("Settings control"))
	{
		if (!AnyInstanceSelected())
			return;

		static int graphicsQuality = 1;
		ImGui::InputInt("Graphics Quality", &graphicsQuality);

		static float newMasterVolume = 0.8f;
		ImGui::InputFloat("Master volume", &newMasterVolume, 0.01f, 1.0f, "%.3f");

		static int newSavedQuality = 1;
		ImGui::InputInt("Saved Graphics Quality", &newSavedQuality);

		if (ImGui::Button("Apply", ImVec2(250.0f, 0.0f))) {
			ForEachSelectedInstance([](int idx) {
				std::string path = fmt::format("C:\\Users\\{}\\AppData\\Local\\Packages\\{}\\LocalState\\GlobalBasicSettings_13.xml", Native::GetCurrentUsername(), g_InstanceControl.GetInstance(g_InstanceNames[idx]).PackageFamilyName);

				if (std::filesystem::exists(path))
					Roblox::ModifySettings(path, graphicsQuality, newMasterVolume, newSavedQuality);
				});
		}
		ImGui::TreePop();
	}
}

void InstanceManager::RenderTerminate()
{
	if (!AnyInstanceSelected())
		return;

	if (ui::RedButton("Terminate"))
	{
		ForEachSelectedInstance([](int idx) {
			CoreLogger::Log(LogLevel::INFO, "Terminating {}", g_InstanceNames[idx]);
			g_InstanceControl.TerminateInstance(g_InstanceNames[idx]);
			});
	}
}

void InstanceManager::DeleteInstances(const std::set<int>& indices)
{
	CoreLogger::Log(LogLevel::INFO, "Deleting instances...");

	// It's safer to erase items from a vector in reverse order
	auto sortedIndices = indices;
	for (auto it = sortedIndices.rbegin(); it != sortedIndices.rend(); ++it)
	{
		int idx = *it;
		this->m_QueuedThreadManager.SubmitTask("deleteInstance" + std::to_string(idx), [idx]() {
			try
			{
				g_InstanceControl.DeleteInstance(g_InstanceNames[idx]);
				g_InstanceNames.erase(g_InstanceNames.begin() + idx);
			}
			catch (const std::exception& e)
			{
				CoreLogger::Log(LogLevel::ERR, "Failed to delete instance {}: {}", g_InstanceControl.GetInstance(g_InstanceNames[idx]).Name, e.what());
			}

			}, [&]() {
				CoreLogger::Log(LogLevel::INFO, "Instance Deleted");
			});
	}
}


void InstanceManager::RenderRemoveInstances()
{
	static bool removeDontAskMeNextTime = false;

	if (!AnyInstanceSelected())
		return;

	if (ui::ConditionalButton("Remove Selected Instances", true, ui::ButtonStyle::Red))
	{
		if (!removeDontAskMeNextTime)
		{
			ImGui::OpenPopup("Delete?");
		}
		else
		{
			ForEachSelectedInstance([this](int idx) {
				this->m_ThreadManager.SubmitTask("deleteInstance", [idx]() {
					try
					{
						g_InstanceControl.DeleteInstance(g_InstanceNames[idx]);
						g_InstanceNames.erase(g_InstanceNames.begin() + idx);
					}
					catch (const std::exception& e)
					{
						CoreLogger::Log(LogLevel::WARNING, "Failed to delete instance {}: {}", g_InstanceNames[idx], e.what());
					}
					}, [&]() {
						CoreLogger::Log(LogLevel::INFO, "Instance Deleted");
					});
				});
		}
	}


	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		int64_t num_selected = std::count(g_Selection.begin(), g_Selection.end(), true);

		if (num_selected == 1) {
			ImGui::Text("This will delete all the files related to the selected instance and unregister it.\nThis operation cannot be undone!\n\n");
		}
		else {
			ImGui::Text("This will delete all the files related to the %d selected instances and unregister them.\nThis operation cannot be undone!\n\n", num_selected);
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

void InstanceManager::RenderCreateInstance()
{
	static std::string instanceNameBuf;

	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 315.0f);

	ui::InputTextWithHint("##NameStr", "Input the instance name here...", &instanceNameBuf);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(R"(Disallowed characters: <>:\"/\\|?*\t\n\r )");

	ImGui::PopItemWidth();

	ImGui::SameLine();

	if (ui::ConditionalButton("Create instance", !(instanceNameBuf.empty() || StringUtils::ContainsOnly(instanceNameBuf, '\0')), ui::ButtonStyle::Green))
	{
		CoreLogger::Log(LogLevel::INFO, "Creating instance...");

		auto completionCallback = [=]() {
			CoreLogger::Log(LogLevel::INFO, "Instance created");
			std::vector<std::string> new_instances = Roblox::GetNewInstances(g_InstanceNames);

			for (const auto& str : new_instances) {
				auto pos = std::lower_bound(g_InstanceNames.begin(), g_InstanceNames.end(), str,
					[](const std::string& a, const std::string& b) {
						return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
						[](char ca, char cb) {
								return std::tolower(ca) < std::tolower(cb);
							}
						);
					}
				);

				g_InstanceNames.insert(pos, str);
			}

			g_Selection.resize(g_InstanceNames.size(), false);
		};

		this->m_ThreadManager.SubmitTask("createInstance", []() {
			g_InstanceControl.CreateInstance(instanceNameBuf);
		}, completionCallback);
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

		this->m_QueuedThreadManager.SubmitTask("updatetemplate", [&]() {
			Utils::UpdatePackage("Template");
		}, callback);
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
			this->m_QueuedThreadManager.SubmitTask(fmt::format("updateinstances{}", idx), [idx]() {
				CoreLogger::Log(LogLevel::INFO, "Updating {}...", g_InstanceNames[idx]);
				Utils::UpdatePackage(g_InstanceControl.GetInstance(g_InstanceNames[idx]).InstallLocation, g_InstanceNames[idx]);
				}, callback);
			});
		ImGui::CloseCurrentPopup();
	}
}

bool InstanceManager::AnyInstanceSelected()
{
	return std::any_of(g_Selection.begin(), g_Selection.end(), [](bool selected) { return selected; });
}