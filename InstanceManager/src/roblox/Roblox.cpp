#include "roblox/Roblox.h"

#include <iostream>
#include <regex>
#include <variant>

#include "cpr/cpr.h"
#include "instance-control/InstanceControl.h"
#include "logging/CoreLogger.hpp"
#include "native/Native.h"
#include "nlohmann/json.hpp"
#include "tinyxml2.h"
#include "utils/Utils.hpp"
#include "utils/filesystem/FS.h"

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36"

namespace Roblox {
	void NukeInstance(const std::string& packagefullname, const std::string& path) {
		Native::RemoveUWPApp(winrt::to_hstring(packagefullname));

		std::thread([path]() {
			FS::RemovePath(path);
		}).detach();
	}

	Roblox::Instance createRobloxInstance(const winrt::Windows::ApplicationModel::Package& package) {
		Roblox::Instance instance;

		instance.PackageID = winrt::to_string(package.Id().FullName().c_str());
		instance.PackageFullName = winrt::to_string(package.Id().FullName().c_str());
		instance.PackageFamilyName = winrt::to_string(package.Id().FamilyName().c_str());
		instance.Version = fmt::format("{}.{}.{}.{}",
		                               package.Id().Version().Major,
		                               package.Id().Version().Minor,
		                               package.Id().Version().Revision,
		                               package.Id().Version().Build);

		try {
			if (package.InstalledLocation()) {
				instance.InstallLocation = winrt::to_string(package.InstalledLocation().Path().c_str());
			} else {
				instance.InstallLocation = "Not Available";
			}
		} catch (const winrt::hresult_error& ex) {
			instance.InstallLocation = "Not Available";
			std::wcerr << L"Error while fetching InstalledLocation for package: " << winrt::to_hstring(package.Id().FullName()).c_str() << L" - " << winrt::to_message().c_str() << std::endl;

			std::wcerr << L"Error code: " << ex.code() << std::endl;
		} catch (...) {
			instance.InstallLocation = "Not Available";
			std::wcerr << L"Unknown error occurred while fetching InstalledLocation for package: " << winrt::to_hstring(package.Id().FullName()).c_str() << std::endl;
		}

		return instance;
	}
	void fetchAppListEntryInfo(Roblox::Instance& instance, const winrt::Windows::ApplicationModel::Package& package) {
		auto asyncOp = package.GetAppListEntriesAsync();
		if (asyncOp.Status() == winrt::Windows::Foundation::AsyncStatus::Error) {
			std::wcerr << L"Error before calling get on GetAppListEntriesAsync. Error code: " << asyncOp.ErrorCode() << std::endl;
		} else if (asyncOp.Status() == winrt::Windows::Foundation::AsyncStatus::Canceled) {
			std::wcerr << L"Operation GetAppListEntriesAsync was canceled." << std::endl;
		} else {
			auto appListEntries = asyncOp.get();

			for (const auto& appListEntry: appListEntries) {
				std::string fullName = winrt::to_string(package.Id().FullName().c_str());
				size_t underscorePosition = fullName.find('_');
				if (underscorePosition != std::string::npos) {
					instance.Name = fullName.substr(0, underscorePosition);
				} else {
					instance.Name = fullName;
				}

				instance.AppID = winrt::to_string(appListEntry.AppUserModelId().c_str());
				winrt::Windows::ApplicationModel::AppDisplayInfo displayInfo = appListEntry.DisplayInfo();
				instance.DisplayName = winrt::to_string(displayInfo.DisplayName().c_str());
			}
		}
	}

	std::optional<std::string> extractKeyFromPackage(const winrt::Windows::ApplicationModel::Package& package) {
		std::string fullName = winrt::to_string(package.Id().Name().c_str());
		std::regex r(R"(([^.]+)$)");
		std::smatch match;
		if (std::regex_search(fullName, match, r) && match.size() > 1) {
			return match.str(1);
		}
		return std::nullopt;
	}

	std::unordered_map<std::string, std::tuple<Roblox::Instance, ImU32>> ProcessRobloxPackages() {
		std::unordered_map<std::string, std::tuple<Roblox::Instance, ImU32>> instancesMap;
		winrt::Windows::Management::Deployment::PackageManager packageManager;

		for (const auto& package: packageManager.FindPackages()) {
			if (std::wstring publisherID = package.Id().PublisherId().c_str(); publisherID == L"55nm5eh3cm0pr") {
				try {
					Roblox::Instance instance = createRobloxInstance(package);
					fetchAppListEntryInfo(instance, package);

					ImU32 color = instance.InstallLocation == "Not Available" ? IM_COL32(255, 0, 0, 255) : IM_COL32(77, 77, 77, 255);

					if (auto keyOpt = extractKeyFromPackage(package); keyOpt) {
						instancesMap[*keyOpt] = {instance, color};
					}
				} catch (const winrt::hresult_error& ex) {
					std::wcerr << L"WinRT error for package: " << winrt::to_hstring(package.Id().FullName()).c_str() << L" - " << winrt::to_message().c_str() << std::endl;
					std::wcerr << L"Error code: " << ex.code() << std::endl;
				}
			}
		}

		std::erase_if(instancesMap, [](const auto& pair) {
			const auto& [key, _] = pair;
			return key == "ROBLOX";
		});

		return instancesMap;
	}

	ModifyXMLError ModifySettings(const std::string& filePath, const std::vector<Setting>& settings) {
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLError eResult = doc.LoadFile(filePath.c_str());
		if (eResult != tinyxml2::XML_SUCCESS) {
			throw std::runtime_error("Failed to load XML file.");
		}

		// Accessing the properties element as before
		tinyxml2::XMLElement* propertiesElement = doc.FirstChildElement("roblox")
		                                                  ->FirstChildElement("Item")
		                                                  ->FirstChildElement("Properties");
		if (!propertiesElement) {
			throw std::runtime_error("Properties element not found.");
		}

		for (const auto& setting : settings) {
			tinyxml2::XMLElement* element = propertiesElement->FirstChildElement(setting.type.c_str());
			while (element && std::string(element->Attribute("name")) != setting.name) {
				element = element->NextSiblingElement(setting.type.c_str());
			}

			if (element) {
				std::visit([&element](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float>) {
						element->SetText(arg);
					} else {
						element->SetText(arg.c_str());
					}
				}, setting.value);
			}
		}

		eResult = doc.SaveFile(filePath.c_str());
		if (eResult != tinyxml2::XML_SUCCESS) {
			throw std::runtime_error("Failed to save XML file.");
		}

		return ModifyXMLError::Success;
	}



	std::string EnterCode(const std::string& code, std::string cookie) {
		nlohmann::json j;
		j["code"] = code;

		cpr::Response r = cpr::Post(
		        cpr::Url{"https://apis.roblox.com/auth-token-service/v1/login/enterCode"},
		        cpr::Header{{"Content-Type", "application/json"}, {"Referer", "https://www.roblox.com/"}, {"User-Agent", USER_AGENT}, {"x-csrf-token", GetCSRF(cookie)}, {"Accept", "application/json"}, {"Origin", "https://www.roblox.com"}},
		        cpr::Cookies{{".ROBLOSECURITY", cookie}},
		        cpr::Body{j.dump()});

		return r.text;
	}

	std::string ValidateCode(const std::string& code, std::string cookie) {
		nlohmann::json j;
		j["code"] = code;

		cpr::Response r = cpr::Post(
		        cpr::Url{"https://apis.roblox.com/auth-token-service/v1/login/validateCode"},
		        cpr::Header{{"Content-Type", "application/json"}, {"Referer", "https://www.roblox.com/"}, {"User-Agent", USER_AGENT}, {"x-csrf-token", GetCSRF(cookie)}, {"Accept", "application/json"}, {"Origin", "https://www.roblox.com"}},
		        cpr::Cookies{{".ROBLOSECURITY", cookie}},
		        cpr::Body{j.dump()});

		return r.text;
	}

	std::string GetCSRF(std::string cookie) {
		cpr::Response r = cpr::Post(
		        cpr::Url{"https://auth.roblox.com/v1/authentication-ticket"},
		        cpr::Header{{"Content-Type", "application/json"}, {"Referer", "https://www.roblox.com/"}},
		        cpr::Cookies{{".ROBLOSECURITY", cookie}});

		return r.header["x-csrf-token"];
	}

	std::string ExtractCode(const unsigned char* dataStart, size_t dataSize) {
		std::string code;
		const unsigned char* dataEnd = dataStart + dataSize;
		while (dataStart != dataEnd && *dataStart != 0x22) {
			code += static_cast<char>(*dataStart);
			dataStart++;
		}
		return code;
	}

	std::string FindCodeValue(HANDLE pHandle) {
		// First pattern: key=???????-????-????-????-????????????&code=
		auto pattern1 = Utils::ParsePattern("6B 65 79 3D ?? ?? ?? ?? ?? ?? ?? ?? 2D ?? ?? ?? ?? 2D ?? ?? ?? ?? 2D ?? ?? ?? ?? 2D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 26 63 6F 64 65 3D");
		std::string codeValue = Native::SearchEntireProcessMemory(pHandle, pattern1.data(), pattern1.size(), Roblox::ExtractCode);

		if (!codeValue.empty()) return codeValue;

		// Second pattern: "\"code\":\""
		unsigned char pattern2[] = {0x22, 0x63, 0x6F, 0x64, 0x65, 0x22, 0x3A, 0x22};
		size_t patternSize2 = sizeof(pattern2) / sizeof(unsigned char);
		codeValue = Native::SearchEntireProcessMemory(pHandle, pattern2, patternSize2, Roblox::ExtractCode);

		return codeValue;
	}

	std::vector<std::string> GetNewInstances(const std::vector<std::string>& old_instances) {
		std::vector<std::string> new_instances = g_InstanceControl.GetInstanceNames();
		std::vector<std::string> result;

		// Check if an instance in new_instances is not present in old_instances
		for (const auto& new_instance: new_instances) {
			bool found = false;
			for (const auto& old_instance: old_instances) {
				if (new_instance == old_instance) {
					found = true;
					break;
				}
			}
			if (!found) {
				result.push_back(new_instance);
			}
		}

		return result;
	}

	void HandleCodeValidation(DWORD pid, const std::string& cookie) {
		HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

		std::string codeValue = Roblox::FindCodeValue(pHandle);

		if (codeValue.empty()) {
			CoreLogger::Log(LogLevel::INFO, "Code value not found");
		} else {
			CoreLogger::Log(LogLevel::INFO, "Code value found: {}", codeValue);
			Roblox::EnterCode(codeValue, cookie);
			Roblox::ValidateCode(codeValue, cookie);
		}
	}
}// namespace Roblox