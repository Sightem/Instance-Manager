#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>

class Config {
public:
    static Config& getInstance()
    {
        static Config instance;
        return instance;
    }

    Config(Config const&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config const&) = delete;
    Config& operator=(Config&&) = delete;

    nlohmann::json& Get();

    bool Load(const std::string& configFile);

    bool Save();

    bool Save(const std::string& configFile);

    std::optional<std::string> GetStringForKey(const std::string& key);

    template <typename T>
    void UpdateConfig(const std::string& key, const std::string& value) {
     if constexpr (std::is_same_v<T, std::string>) {
            if (isValidString(value)) {
                auto& config = Get();
                config[key] = value;
            }
        }
    }

private:

    bool isValidString(const std::string& s);

    Config() : hasUnsavedChanges(false), isRunning(true)
    {
        workerThread = std::thread(&Config::saveWorker, this);
    }

    ~Config() {
        isRunning = false;
        saveCondition.notify_one();
        workerThread.join();
    }


    void saveWorker();

    nlohmann::json m_config;
    std::mutex mtx;
    std::thread workerThread;
    std::condition_variable saveCondition;
    bool hasUnsavedChanges;
    bool isRunning;
    std::string configFilePath;
};
