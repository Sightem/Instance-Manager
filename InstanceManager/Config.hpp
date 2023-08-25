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

    nlohmann::json& Get()
    {
        std::scoped_lock guard(mtx);
        hasUnsavedChanges = true;
        saveCondition.notify_one();
        return config;
    }

    bool Load(const std::string& configFile)
    {
        std::scoped_lock guard(mtx);
        configFilePath = configFile;
        std::ifstream inStream(configFile);
        if (!inStream) {
            return false;
        }
        inStream >> config;
        return true;
    }

    bool Save() {
        return Save(configFilePath);
    }

    bool Save(const std::string& configFile) {
        configFilePath = configFile;
        std::ofstream outStream(configFile);
        if (!outStream) {
            return false;
        }
        outStream << config;
        outStream.close();
        hasUnsavedChanges = false;
        return true;
    }

    bool GetStringForKey(const std::string& key, char* buffer, size_t bufferSize)
    {
        std::scoped_lock guard(mtx);
        if (config.contains(key) && config[key].is_string())
        {
            std::string value = config[key];
            if (value.size() < bufferSize)
            {
                std::strncpy(buffer, value.c_str(), bufferSize);
                return true;
            }
        }
        return false;
    }

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
        } // Extend with more types as needed
    }


private:

    std::optional<long long> tryStoll(const std::string& s) {
        try {
            return std::stoll(s);
        }
        catch (...) {
            return std::nullopt;
        }
    }

    std::optional<int> tryStoi(const std::string& s) {
        try {
            return std::stoi(s);
        }
        catch (...) {
            return std::nullopt;
        }
    }

    bool isValidString(const std::string& s) {
        return !s.empty();
    }


    Config() : hasUnsavedChanges(false), isRunning(true)
    {
        workerThread = std::thread(&Config::saveWorker, this);
    }

    ~Config() {
        isRunning = false;
        saveCondition.notify_one();
        workerThread.join();
    }


    void saveWorker()
    {
        while (isRunning) {
            std::unique_lock lock(mtx);
            saveCondition.wait(lock, [this]() { return hasUnsavedChanges || !isRunning; });
            if (hasUnsavedChanges) {
                Save();
            }
        }
    }

    nlohmann::json config;
    std::mutex mtx;
    std::thread workerThread;
    std::condition_variable saveCondition;
    bool hasUnsavedChanges;
    bool isRunning;
    std::string configFilePath;
};
