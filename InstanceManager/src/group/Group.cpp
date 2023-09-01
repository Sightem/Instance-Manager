#include "group/Group.h"
#include "utils/Utils.h"
#include <chrono>
#include <algorithm>

using Minutes = std::chrono::minutes;
using Seconds = std::chrono::seconds;

constexpr int ROBLOXWAITTIME = 7;

std::optional<ImU32> Group::GetColorForManagedAccount(const std::string& username) const
{
	if (m_Managers.find(username) != m_Managers.end())
	{
		return m_Color;
	}
	return std::nullopt;
}

void Group::StartManager(const std::shared_ptr<Manager>& manager)
{
    manager->start();

    if (!m_DllPath.empty())
    {
        Utils::SleepFor(std::chrono::seconds(2));
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

ImU32 Group::GetColor() const
{
	return m_Color;
}

void Group::RemoveAccount(const std::string& username)
{
	m_Managers.erase(username);
}