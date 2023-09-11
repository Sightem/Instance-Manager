#pragma once

#include "logging/CoreLogger.hpp"

class FileLogger {
public:
	static FileLogger& GetInstance() {
		static FileLogger instance;
		return instance;
	}

	void Initialize(const std::filesystem::path& directoryPath) {
		std::scoped_lock lock(m_streamLock);

		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		std::ostringstream filename;

		std::tm local_tm{};
		localtime_s(&local_tm, &now_time_t);

		filename << "log_" << std::put_time(&local_tm, "%Y_%m_%d_%H_%M_%S") << ".log";

		std::filesystem::path logFilePath = directoryPath / filename.str();

		m_logFileStream.open(logFilePath, std::ofstream::out | std::ofstream::app);
		if (!m_logFileStream) {
			std::cerr << "Failed to open log file: " << logFilePath << std::endl;
		} else {
			CoreLogger::GetInstance().RegisterListener([this](const LogMessage& logMsg) {
				this->LogToFile(logMsg);
			});
		}
	}

	~FileLogger() {
		if (m_logFileStream.is_open()) {
			m_logFileStream.close();
		}
	}

private:
	std::ofstream m_logFileStream;
	std::mutex m_streamLock;
	size_t m_logCounter = 0;
	const size_t m_flushThreshold = 20;

	FileLogger() = default;

	void LogToFile(const LogMessage& logMsg) {
		std::string logLine = fmt::format("[{}] [{}] {}", logMsg.timestamp, logMsg.severity, logMsg.message);
		{
			std::scoped_lock lock(m_streamLock);
			m_logFileStream << logLine << std::endl;

			if (logMsg.severity == LogLevel::ERR) {
				m_logFileStream.flush();
			}

			m_logCounter++;
			if (m_logCounter >= m_flushThreshold) {
				m_logFileStream.flush();
				m_logCounter = 0;
			}
		}
	}

	FileLogger(const FileLogger&) = delete;
	FileLogger& operator=(const FileLogger&) = delete;
};