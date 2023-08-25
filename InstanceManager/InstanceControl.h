#pragma once
#include <string>
#include <Windows.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include "Manager.h"
#include "Functions.h"
#include "Group.h"

class InstanceControl
{
public:

	InstanceControl(const InstanceControl&) = delete;
	InstanceControl& operator=(const InstanceControl&) = delete;

	bool LaunchInstance(const std::string& username, const std::string& placeid, const std::string& linkcode);
	bool TerminateInstance(const std::string& username);
	bool IsInstanceRunning(const std::string& username);

	ImU32 IsGrouped(const std::string& username);

	std::vector<std::string> GetInstanceNames();

	std::shared_ptr<Manager> GetManager(const std::string& username);

	const Roblox::Instance& GetInstance(const std::string& username);

	bool CreateInstance(const std::string& name);

	void TerminateGroup(const std::string& groupname);

	void DeleteInstance(const std::string& name);

	void CreateGroup(const std::string& groupname, const std::vector<std::string>& username, const std::string& placeid, const std::string& linkcode, const std::string& dllpath, int launchdelay, int relaunchinterval, ImU32 color);

	ImU32 GetGroupColor(const std::string& groupname);


private:

	InstanceControl()
	{
		m_InstanceThread = new std::thread([this]() {
			while (true)
			{
				for (auto& instance : m_LaunchedInstances)
				{
					if (!Native::IsProcessRunning(instance.second->GetPID(), "Windows10Universal.exe"))
					{
						TerminateInstance(instance.first);
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(300));
			}
		});

	}

	friend InstanceControl& GetPrivateInstance();

	std::unordered_map<std::string, Roblox::Instance> instances = Roblox::ProcessRobloxPackages();
	std::unordered_map<std::string, std::shared_ptr<Manager>> m_LaunchedInstances;
	std::unordered_map<std::string, std::shared_ptr<Group>> m_Groups;


	std::thread* m_InstanceThread = nullptr;
};

extern InstanceControl& g_InstanceControl;