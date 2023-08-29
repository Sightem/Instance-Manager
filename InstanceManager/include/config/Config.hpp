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

    bool GetStringForKey(const std::string& key, char* buffer, size_t bufferSize);

    // Generic update method
    template <typename T>
    void UpdateConfig(const std::string& key, const std::string& value) {
        if constexpr (std::is_same_v<T, long long>) {
            auto optValue = tryStoll(value);
            if (optValue) {
                auto& config = Get();
                config[key] = *optValue;
            }
        }
        else if constexpr (std::is_same_v<T, int>) {
            auto optValue = tryStoi(value);
            if (optValue) {
                auto& config = Get();
                config[key] = *optValue;
            }
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            if (isValidString(value)) {
                auto& config = Get();
                config[key] = value;
            }
        }
    }


private:

    std::optional<long long> tryStoll(const std::string& s);

    std::optional<int> tryStoi(const std::string& s);

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

    nlohmann::json config;
    std::mutex mtx;
    std::thread workerThread;
    std::condition_variable saveCondition;
    bool hasUnsavedChanges;
    bool isRunning;
    std::string configFilePath;
};
