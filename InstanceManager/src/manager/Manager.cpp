#include "manager/Manager.h"

#include "logging/CoreLogger.hpp"

#include "native/Native.h"

std::optional<DWORD> LaunchRobloxInternal(const std::string& AppID, const std::string& protocolString) {
    winrt::hstring protocolURI = winrt::to_hstring(protocolString);
    winrt::hstring hAppID = winrt::to_hstring(AppID);

    try {
        DWORD pid = Native::LaunchUWPAppWithProtocol(hAppID, protocolURI);
        if (pid <= 0) {
            CoreLogger::Log(LogLevel::ERR, "Failed to launch Roblox with AppID: {}", AppID);
            return std::nullopt;
        }
        return pid;
    } catch(const winrt::hresult_error& ex) {
        CoreLogger::Log(LogLevel::ERR, "Error occurred launching Roblox: {} - HRESULT: {}", ex.code(), winrt::to_string(ex.message()));
        return std::nullopt;
    }
}

std::optional<DWORD> LaunchRoblox(const std::string& AppID, const std::string& placeid) {
    std::string protocolString = "roblox://placeId=" + placeid;
    return LaunchRobloxInternal(AppID, protocolString);
}

std::optional<DWORD> LaunchRoblox(const std::string& AppID, const std::string& placeid, const std::string& linkCode) {
    std::string protocolString = fmt::format("roblox://placeId={}&linkCode={}", placeid, linkCode);
    return LaunchRobloxInternal(AppID, protocolString);
}

bool Manager::start()
{
    std::scoped_lock lock(this->mutex);

    std::optional<DWORD> procID;

    if (this->m_LinkCode.empty())
    {
        procID = LaunchRoblox(this->m_Instance.AppID, this->m_PlaceID);
    }
    else
    {
        procID = LaunchRoblox(this->m_Instance.AppID, this->m_PlaceID, this->m_LinkCode);
    }

    if (procID.has_value())
    {
        this->m_Pid = procID.value();
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
