#include "AppLog.hpp"

#include "imgui.h"

void AppLog::Clear()
{
	m_Buf.clear();
	m_LineOffsets.clear();
	m_LineOffsets.push_back(0);
}

void AppLog::Draw(const char* title, bool* p_open)
{
	// Optiosns menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
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
	m_Filter.Draw("##LogFilter", -FLT_MIN);

	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (clr)
		Clear();
	if (copy)
		ImGui::LogToClipboard();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	const char* buf = m_Buf.begin();
	const char* buf_end = m_Buf.end();
	if (m_Filter.IsActive())
	{
		for (int line_no = 0; line_no < m_LineOffsets.Size; line_no++)
		{
			const char* line_start = buf + m_LineOffsets[line_no];
			const char* line_end = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] - 1) : buf_end;
			if (m_Filter.PassFilter(line_start, line_end))
			{
				RenderLogLine(line_start, line_end);
			}
		}
	}
	else
	{
		ImGuiListClipper clipper;
		clipper.Begin(m_LineOffsets.Size);
		while (clipper.Step())
		{
			for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
			{
				const char* lineStart = buf + m_LineOffsets[line_no];
				const char* lineEnd = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] - 1) : buf_end;
				RenderLogLine(lineStart, lineEnd);
			}
		}
		clipper.End();
	}
	ImGui::PopStyleVar();

	if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
}


void AppLog::m_ProcessLogs()
{
	while (true)
	{
		std::string log;
		{
			std::unique_lock lock(mtx);
			cv.wait(lock, [this] { return !m_LogsQueue.empty() || m_StopWorker; });
			if (m_StopWorker && m_LogsQueue.empty()) return;
			log = m_LogsQueue.front();
			m_LogsQueue.pop();
		}
		// Process the log
		m_Buf.append(log.data(), log.data() + log.size());
		m_Buf.append("\n");  // Append newline here
		size_t oldSize = m_Buf.size() - log.size();
		for (int newSize = m_Buf.size(); oldSize < newSize; oldSize++)
			if (m_Buf[static_cast<std::string::size_type>(oldSize)] == '\n')
				m_LineOffsets.push_back(static_cast<int>(oldSize + 1));
	}
}

void AppLog::RenderLogLine(const char* line_start, const char* line_end)
{
	const char* timestampEnd = strstr(line_start, "]");  // Find the end of the timestamp
	if (timestampEnd)
	{
		// rgb(91 190 247) normalized
		ImGui::TextColored(ImVec4(0.3569f, 0.7451f, 0.9686f, 1.0f), "%.*s]", (int)(timestampEnd - line_start), line_start);  // Render the timestamp in yellow
		ImGui::SameLine();
		ImGui::TextUnformatted(timestampEnd + 2, line_end);  // Render the message
	}
	else
	{
		ImGui::TextUnformatted(line_start, line_end);  // Fallback if no timestamp is found
	}
}
