#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>

class Config {
public:
    // Create a static method to access class instance
    static Config& getInstance()
    {
        static Config instance;
        return instance;
    }

    Config(Config const&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config const&) = delete;
    Config& operator=(Config&&) = delete;

    nlohmann::json& get()
    {
        std::scoped_lock guard(mtx);
        return config;
    }

    bool load(const std::string& configFile)
    {
        std::scoped_lock guard(mtx);
        std::ifstream inStream(configFile);
        if (!inStream) {
            return false;
        }
        inStream >> config;
        return true;
    }

    bool save(const std::string& configFile) {
        std::scoped_lock guard(mtx);
        std::ofstream outStream(configFile);
        if (!outStream) {
            return false;
        }
        outStream << config;
        return true;
    }

private:
    Config() {}


    nlohmann::json config;
    std::mutex mtx;
};
