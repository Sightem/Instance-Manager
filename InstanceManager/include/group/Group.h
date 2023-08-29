#pragma once
#include <vector>
#include "manager/Manager.h"
#include <atomic>
#include <optional>
#include <thread>
#include "imgui.h"

class Group
{
public:
	Group(std::unordered_map<std::string, std::shared_ptr<Manager>>&& managers, int restarttime, int launchdelay, std::string dllpath, ImU32 color) : 
		m_Managers(std::move(managers)), m_IsActive(true), m_RestartTime(restarttime), m_LaunchDelay(launchdelay), m_DllPath(dllpath), m_Color(color) {}

	~Group()
	{
		Stop();
	}

	std::optional<ImU32> GetColorForManagedAccount(const std::string& username) const;

	void Start();

	void Stop();

	ImU32 GetColor();

	void RemoveAccount(const std::string& username);
private:
	std::unordered_map<std::string, std::shared_ptr<Manager>> m_Managers;

	std::thread* m_Thread;

	std::atomic_bool m_IsActive;
	std::atomic_int m_RestartTime;
	std::atomic_int m_LaunchDelay;

	std::string m_DllPath;

	ImU32 m_Color;
};