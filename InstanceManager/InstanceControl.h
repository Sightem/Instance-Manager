#pragma once
#include <string>
#include <Windows.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include "Manager.h"
#include "Functions.h"

class InstanceControl
{
public:

	InstanceControl(const InstanceControl&) = delete;
	InstanceControl& operator=(const InstanceControl&) = delete;

	bool LaunchInstance(std::string username, const std::string& placeid, std::string linkcode);
	bool TerminateInstance(std::string username);
	bool IsInstanceRunning(std::string_view username);

	std::vector<std::string> GetInstanceNames();

	std::shared_ptr<Manager> GetManager(std::string username);

	const Roblox::Instance& GetInstance(std::string username);

	bool CreateInstance(std::string name);

	void DeleteInstance(std::string name);


private:

	InstanceControl() = default;

	friend InstanceControl& GetPrivateInstance();

	std::unordered_map<std::string, Roblox::Instance> instances = Roblox::ProcessRobloxPackages();
	std::unordered_map<std::string, std::shared_ptr<Manager>> launched_instances;

};

extern InstanceControl& g_InstanceControl;