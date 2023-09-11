#pragma once
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>

#include <condition_variable>
#include <queue>
#include <thread>

#include "imgui.h"

// totally not stolen from the imgui demo
class AppLog {
public:
	void Clear();

	AppLog();

	~AppLog();

	void Draw();

private:
	void ProcessLog(const std::string& log);

	static void RenderLogLine(const char* line_start, const char* line_end);

	ImGuiTextBuffer m_Buf;
	ImGuiTextFilter m_Filter;
	ImVector<int> m_LineOffsets;
	bool m_AutoScroll{true};

	std::mutex mtx;
	std::condition_variable cv;
	std::thread m_WorkerThread;
};