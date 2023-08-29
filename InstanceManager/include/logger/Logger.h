#pragma once

#include <fmt/format.h>
#include <fmt/core.h>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <filesystem>

enum class LogLevel {
    DEBUG, INFO, WARNING, ERR
};

struct LogMessage {
    std::string timestamp;
    LogLevel severity;
    std::string message;

    LogMessage(const std::string& ts, LogLevel sev, const std::string& msg)
        : timestamp(ts), severity(sev), message(msg) {}
};

namespace fmt {
    template <>
    struct formatter<LogLevel> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(LogLevel level, FormatContext& ctx) {
            const char* log_string = "";
            switch (level) {
            case LogLevel::DEBUG:   log_string = "DEBUG";   break;
            case LogLevel::INFO:    log_string = "INFO";    break;
            case LogLevel::WARNING: log_string = "WARNING"; break;
            case LogLevel::ERR:     log_string = "ERROR";   break;
            }
            return fmt::format_to(ctx.out(), "{}", log_string);
        }
    };

}


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
    void Log(LogLevel severity, fmt::format_string<Args...> fmt, Args&&... args) {
        auto message = fmt::format(fmt, std::forward<Args>(args)...);
        EnqueueMessage(severity, message);
    }

private:
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
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream timestamp;
        std::tm local_tm;
        localtime_s(&local_tm, &now_time_t);

        timestamp << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << now_ms.count();


        {
            std::scoped_lock lock(m_queueLock);
            m_queue.emplace(timestamp.str(), severity, message);
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
        for (const auto& listener : m_listeners) {
            listener(logMsg);
        }
    }
};