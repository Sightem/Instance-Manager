#include "Manager.h"

bool Manager::start()
{
	std::scoped_lock lock(this->mutex);
	DWORD procID;
	
	if (m_LinkCode.empty())
		procID = Roblox::LaunchRoblox(this->m_Instance.AppID, this->m_Username, this->m_PlaceID);
	else
		procID = Roblox::LaunchRoblox(this->m_Instance.AppID, this->m_Username, this->m_PlaceID, this->m_LinkCode);

	if (procID != -1)
	{
		this->m_Pid = procID;
		return true;
	}
	else
	{
		return false;
	}
}

bool Manager::terminate()
{
	std::scoped_lock lock(this->mutex);
	if (this->m_Pid != -1)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, this->m_Pid);

		return TerminateProcess(hProcess, 9);
	}
	else
	{
		return false;
	}
}

int64_t Manager::GetLifeTime() const
{
	auto now = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - this->m_CreationTime);
	return elapsed.count();
}