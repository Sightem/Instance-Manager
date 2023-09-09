#include "instance-control/InstanceControl.h"

#include <fmt/format.h>

#include <fstream>

#include "utils/filesystem/FS.h"

InstanceControl& GetPrivateInstance() {
	static InstanceControl instance;
	return instance;
}

InstanceControl& g_InstanceControl = GetPrivateInstance();

bool InstanceControl::LaunchInstance(const std::string& username, const std::string& placeid, const std::string& linkcode) {
	auto it = m_Instances.find(username);
	Roblox::Instance& instance = std::get<0>(it->second);
	auto manager = std::make_unique<Manager>(instance, username, placeid, linkcode);
	if (!manager->start()) {
		return false;
	}

	m_LaunchedInstances[username] = std::move(manager);
	std::get<1>(it->second) = IM_COL32(40, 170, 40, 255);

	return true;
}

bool InstanceControl::TerminateInstance(const std::string& username) {
	auto it = m_Instances.find(username);
	if (m_LaunchedInstances.find(username) == m_LaunchedInstances.end()) {
		for (auto& group: m_Groups) {
			group.second->RemoveAccount(username);
		}
	} else {
		m_LaunchedInstances[username]->terminate();
		m_LaunchedInstances.erase(username);
	}

	std::get<1>(it->second) = IM_COL32(77, 77, 77, 255);

	return true;
}

bool InstanceControl::IsInstanceRunning(const std::string& username) {
	if (m_LaunchedInstances.find(username) == m_LaunchedInstances.end() || OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_LaunchedInstances[username]->GetPID()) == NULL) {
		return false;
	}

	return true;
}

void InstanceControl::TerminateGroup(const std::string& groupname) {
	auto it = m_Groups.find(groupname);
	if (it == m_Groups.end()) {
		return;
	}

	std::vector<std::string> accs = it->second->GetAccounts();

	for (const auto& username: accs) {
		auto instanceIt = m_Instances.find(username);
		if (instanceIt != m_Instances.end()) {
			std::get<1>(instanceIt->second) = IM_COL32(77, 77, 77, 255);
		}
	}

	m_Groups.erase(it);
}

std::vector<std::string> InstanceControl::GetInstanceNames() {
	std::vector<std::string> names;
	for (auto& instance: m_Instances) {
		names.emplace_back(instance.first);
	}

	return names;
}

bool InstanceControl::CreateInstance(const std::string& username) {
	std::filesystem::path path(fmt::format("Instances\\{}", username));
	std::filesystem::create_directory(path);
	FS::CopyDirectory("Template", path);

	//std::string manifestPath = path + "\\AppxManifest.xml";
	std::filesystem::path manifestPath = path / "AppxManifest.xml";
	Utils::ModifyAppxManifest(manifestPath, username);

	std::string abs_path = std::filesystem::absolute(manifestPath).string();
	Native::InstallUWPApp(winrt::to_hstring(abs_path));

	std::set<std::string> oldInstances;
	for (const auto& pair: m_Instances)
		oldInstances.insert(pair.first);

	m_Instances = Roblox::ProcessRobloxPackages();

	std::vector<std::string> newInstances;
	for (const auto& pair: m_Instances) {
		if (oldInstances.find(pair.first) == oldInstances.end())
			newInstances.push_back(pair.first);
	}

	std::thread(&InstanceControl::AnimateThread, this, newInstances).detach();

	return true;
}


void InstanceControl::AnimateThread(const std::vector<std::string>& newInstances) {
	auto startTime = std::chrono::high_resolution_clock::now();
	double duration = 2.0;

	std::vector<std::reference_wrapper<ImU32>> instanceColors;
	for (const auto& instanceName: newInstances) {
		auto it = m_Instances.find(instanceName);
		if (it != m_Instances.end()) {
			instanceColors.emplace_back(std::get<1>(it->second));
		}
	}

	constexpr static auto sine_period = 3 * 3.14159265358979323846;
	constexpr static auto base_color = 77;
	constexpr static auto offset_color = 255 - 77;
	constexpr static auto wait_period = std::chrono::milliseconds(10);

	while (true) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsed = std::chrono::duration<double>(currentTime - startTime).count();

		if (elapsed > duration)
			break;

		double y = std::abs(sin(sine_period * (elapsed / duration)));
		ImU32 greenValue = base_color + offset_color * y;

		for (auto& colorRef: instanceColors) {
			colorRef.get() = IM_COL32(base_color, greenValue, base_color, 0xFF);
		}

		Utils::SleepFor(std::chrono::milliseconds(wait_period));
	}
}

void InstanceControl::DeleteInstance(const std::string& name) {
	Roblox::NukeInstance(std::get<Roblox::Instance>(m_Instances[name]).PackageFullName, std::get<Roblox::Instance>(m_Instances[name]).InstallLocation);
}

void InstanceControl::CreateGroup(const GroupCreationInfo& info) {
	std::unordered_map<std::string, std::unique_ptr<Manager>> managers;

	for (const auto& username: info.usernames) {
		auto it = m_Instances.find(username);
		if (it != m_Instances.end()) {
			auto& instance = std::get<0>(it->second);
			managers.emplace(username, std::make_unique<Manager>(instance, username, info.placeid, info.linkcode));

			std::get<1>(it->second) = info.color;
		}
	}

	auto [group_it, inserted] = m_Groups.emplace(info.groupname, std::make_unique<Group>(std::move(managers), info.relaunchinterval, info.launchdelay, info.injectdelay, info.dllpath, info.mode, info.method));

	if (inserted) {
		group_it->second->Start();
	}
}


Manager& InstanceControl::GetManager(const std::string& username) {
	return *m_LaunchedInstances[username].get();
}

const Roblox::Instance& InstanceControl::GetInstance(const std::string& username) {
	return std::get<Roblox::Instance>(m_Instances[username]);
}
