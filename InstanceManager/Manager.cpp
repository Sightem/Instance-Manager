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
		nullptr,                // No module name (use command line)
		&commandLine[0],        // Command line
		nullptr,                // Process handle not inheritable
		nullptr,                // Thread handle not inheritable
		FALSE,                  // Set handle inheritance to FALSE
		0,                      // No creation flags
		nullptr,                // Use parent's environment block
		nullptr,                // Use parent's starting directory
		&si,                    // Pointer to STARTUPINFO structure
		&pi                     // Pointer to PROCESS_INFORMATION structure
	)) {
		std::cerr << "CreateProcess failed (" << GetLastError() << ")\n";
		return -1;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int64_t Manager::GetLifeTime() const
{
	auto now = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - this->m_CreationTime);
	return elapsed.count();
}