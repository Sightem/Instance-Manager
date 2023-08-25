#pragma once
#include <string>
#include <Windows.h>
#include "Functions.h"

class Manager
{
public:
	Manager(Roblox::Instance instance, std::string username, std::string palceid, std::string linkcode)
		: m_Instance(instance), m_Username(username), m_PlaceID(palceid), m_LinkCode(linkcode), m_CreationTime(std::chrono::system_clock::now()) {}

	~Manager() {
		this->terminate();
	}

	bool start();

	bool terminate();

	bool Inject(std::string path);

	int64_t GetLifeTime() const;

	DWORD GetPID() const { return this->m_Pid; }

	std::string GetUsername() const { return this->m_Username; }

private:
	std::string m_PlaceID;
	std::string m_LinkCode;
	std::string m_Username;
	std::mutex mutex;
	DWORD m_Pid = 0;
	Roblox::Instance m_Instance;
	std::chrono::system_clock::time_point m_CreationTime;
};