#include "group/Group.h"
#include "utils/Utils.h"
#include <algorithm>

using Minutes = std::chrono::minutes;
using Seconds = std::chrono::seconds;

constexpr int ROBLOXWAITTIME = 7;

void Group::StartManager(const std::shared_ptr<Manager>& manager)
{
    manager->start();

    if (!m_DllPath.empty())
    {
        Utils::SleepFor(std::chrono::seconds(m_InjectDelay.load(std::memory_order_relaxed)));
        manager->Inject(m_DllPath, m_Mode, m_Method);
    }

    Utils::SleepFor(Seconds(m_LaunchDelay.load(std::memory_order_relaxed)));
}

void Group::Start()
{
    m_Thread = new std::thread([this]() {

        while (this->m_IsActive)
        {
            for (const auto& [username, manager] : m_Managers)
            {
                StartManager(manager);
            }

            auto start = std::chrono::system_clock::now();

            while (m_IsActive.load(std::memory_order_relaxed) &&
                   std::chrono::duration_cast<Minutes>(std::chrono::system_clock::now() - start).count() < m_RestartTime.load(std::memory_order_relaxed))
            {
                Utils::SleepFor(Seconds(1));

                for (const auto& [username, manager] : m_Managers)
                {
                    if (!manager->IsRunning())
                    {
                        Utils::SleepFor(Seconds(ROBLOXWAITTIME));
                        StartManager(manager);
                    }
                }
            }

            for (const auto& [username, manager] : m_Managers)
            {
                manager->terminate();
            }

            if (m_IsActive.load(std::memory_order_relaxed))
                Utils::SleepFor(Seconds(ROBLOXWAITTIME));
        }
    });
}


void Group::Stop()
{
	m_IsActive.store(false, std::memory_order_relaxed);
	m_Thread->join();
	delete m_Thread;
}


void Group::RemoveAccount(const std::string& username)
{
	m_Managers.erase(username);
}

bool Group::IsManaged(const std::string &username) const {
    return m_Managers.find(username) != m_Managers.end();
}

std::vector<std::string> Group::GetAccounts() const {
    std::vector<std::string> accounts;
    for (const auto& [username, manager] : m_Managers)
    {
        accounts.push_back(username);
    }
    return accounts;
}
