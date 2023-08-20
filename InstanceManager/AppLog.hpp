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
	static AppLog& getInstance()
	{
		static AppLog instance;
		return instance;
	}

	AppLog(AppLog const&) = delete;
	void operator=(AppLog const&) = delete;

	void clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}

	template <typename... Args>
	void add_log(const char* fmt, const Args&... args)
	{
		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		std::string timestamp = fmt::format("[{:%Y-%m-%d %H:%M:%S}.{:03d}]  ", *std::localtime(&now_time_t), now_ms.count());

		std::string message = timestamp + fmt::format(fmt::runtime(fmt), args...);

		{
			std::scoped_lock lock(mtx);
			logsQueue.push(message);
		}
		cv.notify_one();
	}

	void draw(const char* title, bool* p_open = NULL)
	{
		// Optiosns menu
		if (ImGui::BeginPopup("Options"))
		{
			ImGui::Checkbox("Auto-scroll", &AutoScroll);
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");

		ImGui::SameLine();
		bool clr = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("##LogFilter", -FLT_MIN);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clr)
			clear();
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		if (Filter.IsActive())
		{
			for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				if (Filter.PassFilter(line_start, line_end))
				{
					RenderLogLine(line_start, line_end);
				}
			}
		}
		else
		{
			ImGuiListClipper clipper;
			clipper.Begin(LineOffsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					RenderLogLine(line_start, line_end);
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
	}
private:

	AppLog()
		: workerThread(&AppLog::processLogs, this)
	{
		AutoScroll = true;
		clear();
	}

	~AppLog()
	{
		{
			std::scoped_lock lock(mtx);
			stopWorker = true;
		}
		cv.notify_all();
		if (workerThread.joinable()) workerThread.join();
	}


	void processLogs()
	{
		while (true)
		{
			std::string log;
			{
				std::unique_lock lock(mtx);
				cv.wait(lock, [this] { return !logsQueue.empty() || stopWorker; });
				if (stopWorker && logsQueue.empty()) return;
				log = logsQueue.front();
				logsQueue.pop();
			}
			// Process the log
			Buf.append(log.data(), log.data() + log.size());
			Buf.append("\n");  // Append newline here
			size_t old_size = Buf.size() - log.size();
			for (int new_size = Buf.size(); old_size < new_size; old_size++)
				if (Buf[old_size] == '\n')
					LineOffsets.push_back(old_size + 1);
		}
	}

	void RenderLogLine(const char* line_start, const char* line_end)
	{
		const char* timestamp_end = strstr(line_start, "]");  // Find the end of the timestamp
		if (timestamp_end)
		{
			// rgb(91 190 247) normalized
			ImGui::TextColored(ImVec4(0.3569f, 0.7451f, 0.9686f, 1.0f), "%.*s]", (int)(timestamp_end - line_start), line_start);  // Render the timestamp in yellow
			ImGui::SameLine();
			ImGui::TextUnformatted(timestamp_end + 2, line_end);  // Render the message
		}
		else
		{
			ImGui::TextUnformatted(line_start, line_end);  // Fallback if no timestamp is found
		}
	}

	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets;
	bool AutoScroll;

	std::mutex mtx;
	std::condition_variable cv;
	std::queue<std::string> logsQueue;
	bool stopWorker = false;
	std::thread workerThread;
};