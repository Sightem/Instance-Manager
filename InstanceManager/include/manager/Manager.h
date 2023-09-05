#pragma once
#include <string>
#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <utility>

#include "roblox/Roblox.h"

class Manager {
public:
	Manager(Roblox::Instance instance, std::string username, std::string palceid, std::string linkcode)
	    : m_Instance(std::move(instance)),
	      m_Username(std::move(username)),
	      m_PlaceID(std::move(palceid)),
	      m_LinkCode(std::move(linkcode)),
	      m_CreationTime(std::chrono::system_clock::now()) {}

	~Manager() {
		this->terminate();
	}

	bool start();

	bool terminate() const;

	bool Inject(const std::string& path, const std::string& mode, const std::string& method) const;

	int64_t GetLifeTime() const;

	DWORD GetPID() const { return this->m_Pid; }

	std::string GetUsername() const { return this->m_Username; }

	bool IsRunning() const;

private:
	std::string m_PlaceID;
	std::string m_LinkCode;
	std::string m_Username;
	DWORD m_Pid = 0;
	Roblox::Instance m_Instance;
	std::chrono::system_clock::time_point m_CreationTime;
};