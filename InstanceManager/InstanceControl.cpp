#include "InstanceControl.h"
#include <fmt/format.h>

InstanceControl& GetPrivateInstance()
{
	static InstanceControl instance;
	return instance;
}

InstanceControl& g_InstanceControl = GetPrivateInstance();

bool InstanceControl::LaunchInstance(std::string username, const std::string& placeid, std::string linkcode)
{
    auto instance = instances[username];
	auto manager = std::make_shared<Manager>(instance, username, placeid, linkcode);
	if (!manager->start())
	{
		return false;
	}

	launched_instances[username] = manager;
	return true;
}

bool InstanceControl::TerminateInstance(std::string username)
{
	if (launched_instances.find(username) == launched_instances.end())
	{
		return false;
	}

	auto manager = launched_instances[username];
	manager->terminate();

	launched_instances.erase(username);
	return true;
}

bool InstanceControl::IsInstanceRunning(std::string_view username)
{
	if (launched_instances.find(username.data()) == launched_instances.end() || OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, launched_instances[username.data()]->GetPID()) == NULL)
	{
		return false;
	}

	return true;
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

bool InstanceControl::CreateInstance(std::string username)
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
	std::string cmd = "Add-AppxPackage -path '" + abs_path + "' -register";
	Native::RunPowershellCommand(cmd);

	instances = Roblox::ProcessRobloxPackages();

	return true;
}

void InstanceControl::DeleteInstance(std::string name)
{
	Roblox::NukeInstane(instances[name].Name, instances[name].InstallLocation);
}

std::shared_ptr<Manager> InstanceControl::GetManager(std::string username)
{
	if (launched_instances.find(username) == launched_instances.end())
	{
		return nullptr;
	}

	return launched_instances[username];
}

const Roblox::Instance& InstanceControl::GetInstance(std::string username)
{
	return instances[username];
}
