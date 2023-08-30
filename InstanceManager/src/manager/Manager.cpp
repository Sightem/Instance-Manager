#include "manager/Manager.h"

#include "logging/CoreLogger.hpp"

#include "native/Native.h"

template<typename... Args>
DWORD LaunchRoblox(std::string AppID, const std::string& placeid, Args... args) {
	auto protocolStringGenerator = [&]() {
		if constexpr (sizeof...(Args) == 0)
			return "roblox://placeId=" + placeid;
		else if constexpr (sizeof...(Args) == 1)
			return fmt::format("roblox://placeId={}&linkCode={}", placeid, args...);
	};

	std::string protocolString = protocolStringGenerator();
	winrt::hstring protocolURI = winrt::to_hstring(protocolString);

	winrt::hstring hAppID = winrt::to_hstring(AppID);

	DWORD pid;

	pid = Native::LaunchUWPAppWithProtocol(hAppID, protocolURI);
	if (pid <= 0) {
		CoreLogger::Log(LogLevel::ERR, "Failed to launch Roblox with AppID: {}", AppID);
	}

	return pid;
}

bool Manager::start()
{
	std::scoped_lock lock(this->mutex);
	DWORD procID;
	
	if (this->m_LinkCode == "")
	{
		procID = LaunchRoblox(this->m_Instance.AppID, this->m_PlaceID);
	}
	else
	{
		procID = LaunchRoblox(this->m_Instance.AppID, this->m_PlaceID, this->m_LinkCode);
	}

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
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, this->m_Pid);
    return TerminateProcess(hProcess, 9);
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
		CoreLogger::Log(LogLevel::ERR, "CreateProcess failed ({})", GetLastError());
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
	return Native::IsProcessRunning(this->m_Pid, "Windows10Universal.exe");
}
