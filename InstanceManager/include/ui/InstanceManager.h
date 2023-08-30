#pragma once
#include "Base.hpp"

#include "utils/threadpool/ThreadPool.hpp"
#include "FileManagement.h"
#include "ui/AutoRelaunch.h"
#include "utils/threadpool/QueuedThreadPool.hpp"
#include "ui/AppLog.hpp"

class InstanceManager : public AppBase<InstanceManager>
{
public:
    InstanceManager();
    ~InstanceManager() = default;

	void StartUp();
	void Update();
private:
	ThreadPool m_ThreadManager;
	QueuedThreadManager m_QueuedThreadManager;
    FileManagement m_FileManagement;
    AutoRelaunch m_AutoRelaunch;
    AppLog m_AppLog;

	void RenderProcessControl();
	std::pair<int, int> MatchTemplate(const std::string& template_path, double threshold);
	void PerformMouseAction(int x_mid, int y_mid, std::optional<int> y_offset = std::nullopt);
	void HandleCodeValidation(DWORD pid, const std::string& username, const char* cookie);
	void RenderAutoLogin(int n);
	void RenderLaunch(std::string placeid, std::string linkcode, double launchdelay);
	void RenderSettings();
	void RenderTerminate();
	void DeleteInstances(const std::set<int>& indices);
	void RenderRemoveInstances();
	void RenderCreateInstance();
	void RenderUpdateTemplate();
	void RenderUpdateInstance();
	bool AnyInstanceSelected();
};