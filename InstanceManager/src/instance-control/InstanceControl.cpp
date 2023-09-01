#include "instance-control/InstanceControl.h"
#include <fmt/format.h>

#include "utils/filesystem/FS.h"
#include "utils/utils.h"

#include <fstream>

InstanceControl& GetPrivateInstance()
{
	static InstanceControl instance;
	return instance;
}

InstanceControl& g_InstanceControl = GetPrivateInstance();

bool InstanceControl::LaunchInstance(const std::string& username, const std::string& placeid, const std::string& linkcode)
{
    auto instance = instances[username];
	auto manager = std::make_shared<Manager>(instance, username, placeid, linkcode);
	if (!manager->start())
	{
		return false;
	}

	m_LaunchedInstances[username] = manager;
	return true;
}

bool InstanceControl::TerminateInstance(const std::string& username)
{
    if (m_LaunchedInstances.find(username) == m_LaunchedInstances.end())
    {
        if (IsGrouped(username))
        {
            for (auto& group : m_Groups)
            {
                if (group.second->GetColorForManagedAccount(username).has_value())
                {
                    group.second->RemoveAccount(username);
                }
            }
        }
    }
    else
    {
        m_LaunchedInstances[username]->terminate();
        m_LaunchedInstances.erase(username);
    }

    return true;
}

bool InstanceControl::IsInstanceRunning(const std::string& username)
{
	if (m_LaunchedInstances.find(username) == m_LaunchedInstances.end() || OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_LaunchedInstances[username]->GetPID()) == NULL)
	{
		return false;
	}

	return true;
}

void InstanceControl::TerminateGroup(const std::string& groupname)
{
	if (m_Groups.find(groupname) == m_Groups.end())
	{
		return;
	}

	m_Groups.erase(groupname);
}

ImU32 InstanceControl::IsGrouped(const std::string& username)
{
	for (auto& group : m_Groups)
	{
		auto color = group.second->GetColorForManagedAccount(username);
		if (color.has_value())
		{
			return color.value();
		}
	}

	return IM_COL32(77, 77, 77, 255);
}

std::vector<std::string> InstanceControl::GetInstanceNames()
{
    std::vector<std::string> names;
    for (auto& instance : instances)
    {
		names.emplace_back(instance.first);
	}

    return names;
}

bool InstanceControl::CreateInstance(const std::string& username)
{
	std::ifstream file("Template\\AppxManifest.xml", std::ios::in | std::ios::binary | std::ios::ate);
	std::string inputXML;
	inputXML.resize(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(&inputXML[0], inputXML.size());
	file.close();

	std::string buf = Utils::ModifyAppxManifest(inputXML, username);

	std::string path = fmt::format("instances\\{}", username);

	std::filesystem::create_directory(path);

	FS::CopyDirectory("Template", path);

	std::ofstream ofs(path + "\\AppxManifest.xml", std::ofstream::out | std::ofstream::trunc);
	ofs << buf;
	ofs.flush();
	ofs.close();
	std::string abs_path = std::filesystem::absolute(path + "\\AppxManifest.xml").string();

	Native::InstallUWPApp(winrt::to_hstring(abs_path));

	instances = Roblox::ProcessRobloxPackages();

	return true;
}

void InstanceControl::DeleteInstance(const std::string& name)
{
    Roblox::NukeInstance(instances[name].PackageFullName, instances[name].InstallLocation);
}

void InstanceControl::CreateGroup(const GroupCreationInfo& info)
{
	std::unordered_map<std::string, std::shared_ptr<Manager>> managers;
	for (const auto& username : info.usernames)
	{
		auto it = instances.find(username);
		if (it != instances.end())
		{
			auto& instance = it->second;
			auto manager = std::make_shared<Manager>(instance, username, info.placeid, info.linkcode);
			managers[username] = manager;
		}
	}

	auto result = m_Groups.insert({ info.groupname, std::make_shared<Group>(std::move(managers), info.relaunchinterval, info.launchdelay, info.dllpath, info.mode, info.method, info.color)});
	if (result.second)
	{
		result.first->second->Start();
	}
}

ImU32 InstanceControl::GetGroupColor(const std::string& groupname)
{
	if (m_Groups.find(groupname) == m_Groups.end())
	{
		return IM_COL32(77, 77, 77, 255);
	}

	return m_Groups[groupname]->GetColor();
}

std::shared_ptr<Manager> InstanceControl::GetManager(const std::string& username)
{
	if (m_LaunchedInstances.find(username) == m_LaunchedInstances.end())
	{
		return nullptr;
	}

	return m_LaunchedInstances[username];
}

const Roblox::Instance& InstanceControl::GetInstance(const std::string& username)
{
	return instances[username];
}

