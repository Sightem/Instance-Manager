#pragma once
#include <utility>
#include <vector>
#include "manager/Manager.h"
#include <atomic>
#include <optional>
#include <thread>
#include "imgui.h"

class Group
{
public:
	Group(std::unordered_map<std::string, std::unique_ptr<Manager>>&& managers, int restarttime, int launchdelay, int injectdelay, std::string dllpath, std::string mode, std::string method) :
		m_Managers(std::move(managers)), m_IsActive(true), m_RestartTime(restarttime), m_LaunchDelay(launchdelay), m_InjectDelay(injectdelay), m_DllPath(std::move(dllpath)), m_Mode(std::move(mode)), m_Method(std::move(method)), m_Thread(nullptr) {}

	~Group()
	{
		Stop();
	}

	bool IsManaged(const std::string& username) const;

	void Start();

    void StartManager(Manager& manager);

	void Stop();

	void RemoveAccount(const std::string& username);

    std::vector<std::string> GetAccounts() const;

private:
	std::unordered_map<std::string, std::unique_ptr<Manager>> m_Managers;

	std::thread* m_Thread;

	std::atomic_bool m_IsActive;
	std::atomic_int m_RestartTime;
	std::atomic_int m_LaunchDelay;
    std::atomic_int m_InjectDelay;

	std::string m_DllPath;
    std::string m_Mode;
    std::string m_Method;
};