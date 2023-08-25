#include "Group.h"
#include <chrono>

using Clock = std::chrono::steady_clock;
using Minutes = std::chrono::minutes;
using Seconds = std::chrono::seconds;

std::optional<ImU32> Group::GetColorForManagedAccount(const std::string& username) const
{
	if (m_Managers.find(username) != m_Managers.end())
	{
		return m_Color;
	}
	return std::nullopt;
}

void Group::Start()
{
	m_Thread = new std::thread([this]() {
		
		while (this->m_IsActive)
		{
			for (auto& manager : this->m_Managers)
			{
				manager.second->start();

				std::this_thread::sleep_for(Seconds(2));

				if (!this->m_DllPath.empty())
				{
					manager.second->Inject(this->m_DllPath);
				}

				std::this_thread::sleep_for(Seconds(this->m_LaunchDelay.load(std::memory_order_relaxed)));
			}


			auto start = Clock::now();
			while (m_IsActive.load(std::memory_order_relaxed) && std::chrono::duration_cast<Minutes>(Clock::now() - start).count() < m_RestartTime.load(std::memory_order_relaxed))
			{
				std::this_thread::sleep_for(Seconds(1));
			}

			for (auto& manager : this->m_Managers)
			{
				manager.second->terminate();
			}

			if (m_IsActive.load(std::memory_order_relaxed))
				std::this_thread::sleep_for(Seconds(10));
		}
	});
}


void Group::Stop()
{
	m_IsActive.store(false, std::memory_order_relaxed);
	m_Thread->join();
	delete m_Thread;
}

ImU32 Group::GetColor()
{
	return m_Color;
}
