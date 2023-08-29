#pragma once
#include "imgui.h"
#include <thread>
#include <condition_variable>
#include <queue>
#include <fmt/format.h>
#include <fmt/color.h>
#include <fmt/chrono.h>

// totally not stolen from the imgui demo
class AppLog
{
public:
	static AppLog& GetInstance()
	{
		static AppLog instance;
		return instance;
	}

	AppLog(AppLog const&) = delete;
	void operator=(AppLog const&) = delete;

	void Clear();

	void Draw(const char* title, bool* p_open = NULL);
private:

	AppLog();


	~AppLog()
	{
		Clear();
	}

	void ProcessLog(const std::string& log);

	void RenderLogLine(const char* line_start, const char* line_end);

	ImGuiTextBuffer m_Buf;
	ImGuiTextFilter m_Filter;
	ImVector<int> m_LineOffsets;
	bool m_AutoScroll;

	std::mutex mtx;
	std::condition_variable cv;
	std::queue<std::string> m_LogsQueue;
	bool m_StopWorker = false;
	std::thread m_WorkerThread;
};