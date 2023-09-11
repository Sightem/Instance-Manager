#include "ui/AppLog.h"

#include "imgui.h"
#include "logging/CoreLogger.hpp"

void AppLog::Clear() {
	m_Buf.clear();
	m_LineOffsets.clear();
	m_LineOffsets.push_back(0);
}

AppLog::AppLog() {
	m_AutoScroll = true;
	Clear();

	// Register a listener with the central logger
	CoreLogger::GetInstance().RegisterListener([this](const LogMessage& logMsg) {
		std::string log = fmt::format("[{}]  {}", logMsg.timestamp, logMsg.message);
		ProcessLog(log);
	});
}

void AppLog::Draw() {
	// Optiosns menu
	if (ImGui::BeginPopup("Options")) {
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
	if (m_Filter.IsActive()) {
		for (int line_no = 0; line_no < m_LineOffsets.Size; line_no++) {
			const char* line_start = buf + m_LineOffsets[line_no];
			const char* line_end = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] - 1) : buf_end;
			if (m_Filter.PassFilter(line_start, line_end)) {
				RenderLogLine(line_start, line_end);
			}
		}
	} else {
		ImGuiListClipper clipper;
		clipper.Begin(m_LineOffsets.Size);
		while (clipper.Step()) {
			for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
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

void AppLog::ProcessLog(const std::string& log) {
	m_Buf.append(log.data(), log.data() + log.size());
	m_Buf.append("\n");

	std::size_t initialOffset = m_Buf.size() - log.size();

	std::for_each(m_Buf.begin() + initialOffset, m_Buf.end(),
	              [this, &initialOffset](char c) {
		              if (c == '\n') {
			              m_LineOffsets.push_back(static_cast<int>(++initialOffset));
		              } else {
			              ++initialOffset;
		              }
	              });
}

void AppLog::RenderLogLine(const char* line_start, const char* line_end) {
	const char* timestampEnd = strstr(line_start, "]");
	if (timestampEnd) {
		ImGui::TextColored(ImVec4(0.3569f, 0.7451f, 0.9686f, 1.0f), "%.*s]", static_cast<int>(timestampEnd - line_start), line_start);
		ImGui::SameLine();
		ImGui::TextUnformatted(timestampEnd + 2, line_end);
	} else {
		ImGui::TextUnformatted(line_start, line_end);
	}
}

AppLog::~AppLog() {
	Clear();
}