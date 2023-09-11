#pragma once
#include "Base.hpp"
#include "FileManagement.h"
#include "ui/AppLog.h"
#include "ui/AutoRelaunch.h"
#include "utils/threadpool/ThreadPool.hpp"

class InstanceManager : public AppBase<InstanceManager> {
public:
	InstanceManager();
	~InstanceManager() override = default;

	static void StartUp();
	void Update() override;

private:
	ThreadPool m_ThreadPool;
	ThreadPool m_QueuedThreadPool;
	FileManagement m_FileManagement;
	AutoRelaunch m_AutoRelaunch;
	AppLog m_AppLog;

	void RenderProcessControl();
	void RenderAutoLogin(int n);
	void RenderLaunchButton(const std::string& placeid, const std::string& linkcode, double launchdelay);
	void RenderSettings();
	void RenderTerminate();
	void DeleteInstances(const std::set<int>& indices);
	void RenderRemoveInstances();
	void RenderCreateInstance();
	void RenderUpdateTemplate();
	void RenderContextMenu(int n);
	void RenderUpdateInstance();
	bool AnyInstanceSelected();

	void SubmitDeleteTask(int idx);
	void RenderLaunch();
};