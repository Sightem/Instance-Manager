#pragma once

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <winrt/Windows.Foundation.h>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

enum class LogLevel {
	DEBUG,
	INFO,
	WARNING,
	ERR
};

struct LogMessage {
	std::string timestamp;
	LogLevel severity;
	std::string message;

	LogMessage(std::string ts, LogLevel sev, std::string msg)
	    : timestamp(std::move(ts)),
	      severity(sev),
	      message(std::move(msg)) {}
};

namespace fmt {
	template<>
	struct formatter<LogLevel> {
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) {
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(LogLevel level, FormatContext& ctx) {
			static constexpr std::array<std::string_view, 4> log_strings = {
			        "DEBUG", "INFO", "WARNING", "ERROR"};

			return fmt::format_to(ctx.out(), "{}", log_strings[static_cast<size_t>(level)]);
		}
	};
}// namespace fmt

namespace fmt {
	template<>
	struct formatter<winrt::hstring> {
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) {
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const winrt::hstring& hstr, FormatContext& ctx) {
			return fmt::format_to(ctx.out(), "{}", winrt::to_string(hstr));
		}
	};
}// namespace fmt

namespace fmt {
	template<>
	struct formatter<winrt::hresult> {
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) {
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const winrt::hresult& hr, FormatContext& ctx) {
			return fmt::format_to(ctx.out(), "{:#X}", static_cast<int32_t>(hr));
		}
	};
}// namespace fmt


class CoreLogger {
public:
	using Listener = std::function<void(const LogMessage&)>;

	static CoreLogger& GetInstance() {
		static CoreLogger instance;
		return instance;
	}

	void RegisterListener(const Listener& listener) {
		std::scoped_lock lock(m_listenersLock);
		m_listeners.push_back(listener);
	}

	template<typename... Args>
	static void Log(LogLevel severity, fmt::format_string<Args...> fmt, Args&&... args) {
		GetInstance().InternalLog(severity, fmt, std::forward<Args>(args)...);
	}

	static void Log(LogLevel severity, const std::string& message) {
		GetInstance().InternalLog(severity, "{}", message);
	}

private:
	template<typename... Args>
	void InternalLog(LogLevel severity, fmt::format_string<Args...> fmt, Args&&... args) {
		auto message = fmt::format(fmt, std::forward<Args>(args)...);
		EnqueueMessage(severity, message);
	}

	std::queue<LogMessage> m_queue;
	std::mutex m_queueLock;
	std::condition_variable m_condition;
	std::thread m_loggerThread;
	bool m_shouldExit = false;
	std::vector<Listener> m_listeners;
	std::mutex m_listenersLock;

	CoreLogger() {
		m_loggerThread = std::thread(&CoreLogger::LogThread, this);
	}

	~CoreLogger() {
		{
			std::lock_guard<std::mutex> lock(m_queueLock);
			m_shouldExit = true;
			m_condition.notify_one();
		}
		m_loggerThread.join();
	}

	void EnqueueMessage(LogLevel severity, const std::string& message) {
		using namespace std::chrono;

		auto now_tp = std::chrono::system_clock::now();

		auto now_t = std::chrono::system_clock::to_time_t(now_tp);
		std::tm* local_time = std::localtime(&now_t);

		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_tp.time_since_epoch()) % 1000;

		auto timestamp = fmt::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
		                             local_time->tm_year + 1900,
		                             local_time->tm_mon + 1,
		                             local_time->tm_mday,
		                             local_time->tm_hour,
		                             local_time->tm_min,
		                             local_time->tm_sec,
		                             now_ms.count());


		{
			std::scoped_lock lock(m_queueLock);
			m_queue.emplace(timestamp, severity, message);
			m_condition.notify_one();
		}
	}


	void LogThread() {
		while (true) {
			std::unique_lock lock(m_queueLock);
			m_condition.wait(lock, [this] { return !m_queue.empty() || m_shouldExit; });

			if (m_shouldExit && m_queue.empty()) {
				break;
			}

			auto logMsg = m_queue.front();
			m_queue.pop();
			lock.unlock();

			NotifyListeners(logMsg);
		}
	}

	void NotifyListeners(const LogMessage& logMsg) {
		std::scoped_lock lock(m_listenersLock);
		for (const auto& listener: m_listeners) {
			listener(logMsg);
		}
	}
};