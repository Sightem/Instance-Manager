#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "manager/Manager.h"
#include "group/Group.h"
#include "roblox/Roblox.h"
#include "native/Native.h"
#include "utils/Utils.h"

class InstanceControl
{
public:

	InstanceControl(const InstanceControl&) = delete;
	InstanceControl& operator=(const InstanceControl&) = delete;

	bool LaunchInstance(const std::string& username, const std::string& placeid, const std::string& linkcode);
	bool TerminateInstance(const std::string& username);
	bool IsInstanceRunning(const std::string& username);

	bool IsGrouped(const std::string& username);

	std::vector<std::string> GetInstanceNames();

	std::shared_ptr<Manager> GetManager(const std::string& username);

	const Roblox::Instance& GetInstance(const std::string& username);

	bool CreateInstance(const std::string& name);

	void TerminateGroup(const std::string& groupname);

	void DeleteInstance(const std::string& name);

    struct GroupCreationInfo {
        std::string groupname;
        std::vector<std::string> usernames;
        std::string placeid;
        std::string linkcode;
        std::string dllpath;
        std::string mode;
        std::string method;
        int launchdelay;
        int relaunchinterval;
        int injectdelay;
        ImU32 color;
    };

    void CreateGroup(const GroupCreationInfo& info);

    ImU32 GetColor(const std::string& username) { return std::get<ImU32>(this->m_Instances[username]); };

private:

	InstanceControl() = default;

	friend InstanceControl& GetPrivateInstance();

	std::unordered_map<std::string, std::tuple<Roblox::Instance, ImU32>> m_Instances = Roblox::ProcessRobloxPackages();
	std::unordered_map<std::string, std::shared_ptr<Manager>> m_LaunchedInstances;
	std::unordered_map<std::string, std::shared_ptr<Group>> m_Groups;

    void AnimateNewInstances(const std::vector<std::string> &newInstances);
};

extern InstanceControl& g_InstanceControl;