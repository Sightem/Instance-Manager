#pragma once
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include "UI.h"
#include "config/Config.hpp"
#include "imgui.h"
#include "instance-control/InstanceControl.h"

class AutoRelaunch {
public:
	AutoRelaunch(std::vector<std::string>& instances)
	    : m_InstanceNames(instances),
	      m_InjectionMode("LoadLibrary"),
	      m_InjectionMethod("NtCreateThreadEx") {}

	void Draw(const char* title, bool* p_open = NULL);


private:
	std::vector<std::string>& m_InstanceNames;
	std::vector<bool> m_InstanceSelection;

	std::vector<std::string> m_Groups;
	std::vector<bool> m_GroupSelection;

	std::string m_InjectionMode;
	std::string m_InjectionMethod;

	char szFile[260] = {0};
};