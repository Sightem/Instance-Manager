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

	void clear();

	template <typename... Args>
	void AddLog(const char* fmt, const Args&... args)
	{
		auto now = std::chrono::system_clock::now();
		auto nowTimeT = std::chrono::system_clock::to_time_t(now);
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		std::string timestamp = fmt::format("[{:%Y-%m-%d %H:%M:%S}.{:03d}]  ", *std::localtime(&nowTimeT), nowMs.count());

		std::string message = timestamp + fmt::format(fmt::runtime(fmt), args...);

		{
			std::scoped_lock lock(mtx);
			m_LogsQueue.push(message);
		}
		cv.notify_one();
	}

	void Draw(const char* title, bool* p_open = NULL);
private:

	AppLog()
		: m_WorkerThread(&AppLog::m_ProcessLogs, this)
	{
		m_AutoScroll = true;
		clear();
	}

	~AppLog()
	{
		{
			std::scoped_lock lock(mtx);
			m_StopWorker = true;
		}
		cv.notify_all();
		if (m_WorkerThread.joinable()) m_WorkerThread.join();
	}


	void m_ProcessLogs();

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