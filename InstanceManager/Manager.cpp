#include "Manager.h"

#include "AppLog.h"

#include "Native.h"

template<typename... Args>
DWORD LaunchRoblox(const std::string& AppID, const std::string& placeid, Args... args) {
	auto protocolStringGenerator = [&]() {
		if constexpr (sizeof...(Args) == 0)
			return "roblox://placeId=" + placeid;
		else if constexpr (sizeof...(Args) == 1)
			return fmt::format("roblox://placeId={}&linkCode={}", placeid, args...);
	};

	std::string protocolString = protocolStringGenerator();
	winrt::hstring protocolURI = winrt::to_hstring(protocolString);

	winrt::hstring hAppID = winrt::to_hstring(AppID);

	DWORD pid = -1;

	pid = Native::LaunchUWPAppWithProtocol(hAppID, protocolURI);
	if (pid <= 0) {
		AppLog::GetInstance().AddLog("Failed to launch Roblox with AppID: {}", AppID);
	}

	return pid;
}

bool Manager::start()
{
	std::scoped_lock lock(this->mutex);
	DWORD procID;
	
	procID = LaunchRoblox(this->m_Instance.AppID, this->m_PlaceID, this->m_LinkCode);

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

bool Manager::Inject(std::string path)
{
	std::string programName = "x86-injector.exe";  // Replace with the actual path

	std::string commandLine = programName + " " + std::to_string(this->m_Pid) + " \"" + path + "\"";

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process.
	if (!CreateProcessA(
		nullptr,
		&commandLine[0],
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		nullptr,
		&si,
		&pi
	)) {
		AppLog::GetInstance().AddLog("CreateProcess failed ({})", GetLastError());
		return false;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}

int64_t Manager::GetLifeTime() const
{
	auto now = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - this->m_CreationTime);
	return elapsed.count();
}

bool Manager::IsRunning() const
{
	return this->m_Pid != -1 && Native::IsProcessRunning(this->m_Pid, "Windows10Universal.exe");
}
