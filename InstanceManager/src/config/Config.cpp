#include "config/Config.hpp"

nlohmann::json& Config::Get()
{
    std::scoped_lock guard(mtx);
    hasUnsavedChanges = true;
    saveCondition.notify_one();
    return m_config;
}

bool Config::Load(const std::string& configFile)
{
    std::scoped_lock guard(mtx);
    configFilePath = configFile;
    std::ifstream inStream(configFile);
    if (!inStream) {
        return false;
    }
    inStream >> m_config;
    return true;
}

bool Config::Save() {
    return Save(configFilePath);
}

bool Config::Save(const std::string& configFile) {
    configFilePath = configFile;
    std::ofstream outStream(configFile);
    if (!outStream) {
        return false;
    }
    outStream << m_config;
    outStream.close();
    hasUnsavedChanges = false;
    return true;
}

std::optional<std::string> Config::GetStringForKey(const std::string& key)
{
    std::scoped_lock guard(mtx);
    if (m_config.contains(key) && m_config[key].is_string())
    {
        return m_config[key].get<std::string>();
    }
    return std::nullopt;
}


bool Config::isValidString(const std::string& s) {
    return !s.empty();
}
void Config::saveWorker()
{
    while (isRunning) {
        std::unique_lock lock(mtx);
        saveCondition.wait(lock, [this]() { return hasUnsavedChanges || !isRunning; });
        if (hasUnsavedChanges) {
            Save();
        }
    }
}
