#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include "imgui.h"
#include "config/Config.hpp"
#include "instance-control/InstanceControl.h"

#include "UI.h"

class AutoRelaunch
{
public:
    AutoRelaunch(std::vector<std::string>& instances)
		: m_InstanceNames(instances)
	{}

	void Draw(const char* title, bool* p_open = NULL);


private:
	std::vector<std::string>& m_InstanceNames;
    std::vector<bool> m_InstanceSelection;
	
	std::vector<std::string> m_Groups;
	std::vector<bool> m_GroupSelection;

    std::string m_InjectionMode;
    std::string m_InjectionMethod;

	char szFile[260] = { 0 };
};