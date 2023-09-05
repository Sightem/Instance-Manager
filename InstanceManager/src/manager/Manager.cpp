#include "manager/Manager.h"

#include "logging/CoreLogger.hpp"
#include "native/Native.h"

std::mutex mutex;

std::optional<DWORD> LaunchRoblox(const std::string& AppID, const std::string& placeid, const std::string& linkCode = "") {
	std::string protocolString;
	if (linkCode.empty()) {
		protocolString = "roblox://placeId=" + placeid;
	} else {
		protocolString = fmt::format("roblox://placeId={}&linkCode={}", placeid, linkCode);
	}
	return Native::LaunchAppWithProtocol("Roblox", AppID, protocolString);
}


bool Manager::start() {
	std::scoped_lock lock(mutex);

	std::optional<DWORD> procID = LaunchRoblox(this->m_Instance.AppID, this->m_PlaceID, this->m_LinkCode);

	if (procID.has_value()) {
		this->m_Pid = procID.value();
		return true;
	} else {
		return false;
	}
}

bool Manager::terminate() const {
	std::scoped_lock lock(mutex);
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, this->m_Pid);
	return TerminateProcess(hProcess, 9);
}

bool Manager::Inject(const std::string& path, const std::string& mode, const std::string& method) const {
	std::string programName = "x86-injector.exe";

	std::string commandLine = fmt::format("{} -p {} -d \"{}\" -m {} -l {}",
	                                      programName,
	                                      this->m_Pid,
	                                      path,
	                                      mode,
	                                      method);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

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
	            &pi)) {
		CoreLogger::Log(LogLevel::ERR, "CreateProcess failed ({})", GetLastError());
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}

int64_t Manager::GetLifeTime() const {
	auto now = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - this->m_CreationTime);
	return elapsed.count();
}

bool Manager::IsRunning() const {
	return Native::IsProcessRunning(this->m_Pid, "Windows10Universal.exe");
}
