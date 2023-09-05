#include "utils/Utils.h"

#include <filesystem>
#include <fstream>
#include <thread>
#define NOMINMAX
#include <windows.h>

#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "cpr/cpr.h"
#include "logging/CoreLogger.hpp"
#include "tinyxml2.h"
#include "utils/filesystem/FS.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace Utils {
	std::vector<unsigned char> ParsePattern(const std::string& patternString) {
		std::vector<unsigned char> pattern;
		size_t start = 0;
		size_t end = patternString.find(' ');

		while (end != std::string::npos) {
			std::string byteString = patternString.substr(start, end - start);
			if (byteString == "??") {
				pattern.push_back('?');
			} else {
				unsigned long byteValue = std::strtol(byteString.c_str(), nullptr, 16);
				pattern.push_back(static_cast<unsigned char>(byteValue));
			}
			start = end + 1;
			end = patternString.find(' ', start);
		}

		// Handle the last token in the string (after the last space)
		std::string byteString = patternString.substr(start, end);
		if (byteString == "??") {
			pattern.push_back('?');
		} else {
			unsigned long byteValue = std::strtol(byteString.c_str(), nullptr, 16);
			pattern.push_back(static_cast<unsigned char>(byteValue));
		}

		return pattern;
	}

	uintptr_t BoyerMooreHorspool(const unsigned char* signature, size_t signatureSize, const unsigned char* data, size_t dataSize) {
		size_t maxShift = signatureSize;
		size_t maxIndex = signatureSize - 1;
		size_t wildCardIndex = 0;
		for (size_t i = 0; i < maxIndex; i++) {
			if (signature[i] == '?') {
				maxShift = maxIndex - i;
				wildCardIndex = i;
			}
		}

		size_t shiftTable[256];
		for (size_t i = 0; i <= 255; i++) {
			shiftTable[i] = maxShift;
		}

		for (size_t i = wildCardIndex + 1; i < maxIndex - 1; i++) {
			shiftTable[signature[i]] = maxIndex - i;
		}

		for (size_t currentIndex = 0; currentIndex < dataSize - signatureSize;) {
			for (size_t sigIndex = maxIndex; sigIndex >= 0; sigIndex--) {
				if (data[currentIndex + sigIndex] != signature[sigIndex] && signature[sigIndex] != '?') {
					currentIndex += shiftTable[data[currentIndex + maxIndex]];
					break;
				} else if (sigIndex == 0) {
					return currentIndex;
				}
			}
		}

		return 0;
	}

	void DownloadAndSave(const std::string& url, const std::string& localFileName) {
		std::ofstream out(localFileName, std::ios::binary);
		cpr::Response r = cpr::Download(out, cpr::Url{url});
		out.close();
	}

	void DecompressZip(const std::string& zipFile, const std::string& destination) {
		if (std::filesystem::exists(destination)) {
			std::filesystem::remove(destination);
		}
		FS::DecompressZipToFile(zipFile, destination);
	}

	void CopyFileToDestination(const std::string& source, const std::string& destination) {
		if (std::filesystem::exists(destination)) {
			std::filesystem::remove(destination);
		}
		std::filesystem::copy_file(source, destination);
	}

	void ModifyAppxManifest(const std::filesystem::path& filePath, const std::string& name) {
		tinyxml2::XMLDocument doc;
		tinyxml2::XMLError loadResult = doc.LoadFile(filePath.string().c_str());

		if (loadResult != tinyxml2::XML_SUCCESS) {
			CoreLogger::Log(LogLevel::ERR, "Failed to load AppxManifest.xml");
			return;
		}

		// <Identity> element's Name attribute
		tinyxml2::XMLElement* identityElement = doc.FirstChildElement("Package")->FirstChildElement("Identity");
		if (identityElement) {
			const char* nameValue = identityElement->Attribute("Name");
			if (nameValue) {
				std::string newNameValue = std::string(nameValue) + "." + name;
				identityElement->SetAttribute("Name", newNameValue.c_str());
			}
		}

		// <uap:VisualElements> tag
		tinyxml2::XMLElement* visualElement = doc.FirstChildElement("Package")->FirstChildElement("Applications")->FirstChildElement("Application")->FirstChildElement("uap:VisualElements");
		if (visualElement) {
			const char* displayNameValue = visualElement->Attribute("DisplayName");
			if (displayNameValue) {
				std::string newDisplayNameValue = std::string(displayNameValue) + " " + name;
				visualElement->SetAttribute("DisplayName", newDisplayNameValue.c_str());
			}
		}

		// <uap:DefaultTile> tag
		tinyxml2::XMLElement* defaultTile = visualElement->FirstChildElement("uap:DefaultTile");
		if (defaultTile) {
			const char* shortNameValue = defaultTile->Attribute("ShortName");
			if (shortNameValue) {
				std::string newShortNameValue = std::string(shortNameValue) + " " + name;
				defaultTile->SetAttribute("ShortName", newShortNameValue.c_str());
			}
		}

		// <DisplayName> tag under <Properties>
		tinyxml2::XMLElement* propertiesDisplayName = doc.FirstChildElement("Package")->FirstChildElement("Properties")->FirstChildElement("DisplayName");
		if (propertiesDisplayName) {
			const char* displayNameText = propertiesDisplayName->GetText();
			if (displayNameText) {
				std::string newDisplayNameText = std::string(displayNameText) + " " + name;
				propertiesDisplayName->SetText(newDisplayNameText.c_str());
			}
		}

		doc.SaveFile(filePath.string().c_str());
	}

	bool write_data_to_file(const std::string& data, intptr_t userdata) {
		auto* outfile = reinterpret_cast<std::ofstream*>(userdata);
		outfile->write(data.c_str(), data.size());
		return true;
	}

	void WriteAppxManifest(const std::string& url, const std::string& localPath, const std::string& name) {
		cpr::Session session;
		session.SetUrl(cpr::Url{url});

		std::ofstream outfile(localPath, std::ofstream::out | std::ofstream::trunc);

		cpr::Response result = session.Download(cpr::WriteCallback{write_data_to_file, reinterpret_cast<intptr_t>(&outfile)});

		outfile.flush();
		outfile.close();

		if (!name.empty()) {
			Utils::ModifyAppxManifest(localPath, name);// Modify the file in-place
		}
	}

	void UpdatePackage(const std::string& baseFolder, const std::string& instanceName) {
		// For Windows10Universal.zip
		std::thread win10t([baseFolder]() {
			DownloadAndSave("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Windows10Universal.zip", "Windows10Universal.zip");
			DecompressZip("Windows10Universal.zip", baseFolder + "\\Windows10Universal.exe");
		});

		// For CrashHandler.exe
		std::thread crasht([baseFolder]() {
			DownloadAndSave("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/Assets/CrashHandler.exe", "CrashHandler.exe");
			CopyFileToDestination("CrashHandler.exe", baseFolder + "\\Assets\\CrashHandler.exe");
		});

		// For AppxManifest.xml
		std::thread appxt([baseFolder, instanceName]() {
			WriteAppxManifest("https://raw.githubusercontent.com/Sightem/Instance-Manager/master/Template/AppxManifest.xml", baseFolder + "\\AppxManifest.xml");
		});

		win10t.join();

		CoreLogger::Log(LogLevel::INFO, "Updated Windows10Universal");

		crasht.join();

		CoreLogger::Log(LogLevel::INFO, "Updated CrashHandler");

		appxt.join();

		CoreLogger::Log(LogLevel::INFO, "Updated AppxManifest");
	}

	bool SaveScreenshotAsPng(const char* filename) {
		HDC screenDC = GetDC(NULL);
		HDC memoryDC = CreateCompatibleDC(screenDC);

		// Get the dimensions of the entire virtual screen (all monitors)
		int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
		int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

		HBITMAP bitmap = CreateCompatibleBitmap(screenDC, screenWidth, screenHeight);
		HBITMAP oldBitmap = (HBITMAP) SelectObject(memoryDC, bitmap);

		// Copy the entire virtual screen
		BitBlt(memoryDC, 0, 0, screenWidth, screenHeight, screenDC, screenX, screenY, SRCCOPY);

		BITMAPINFOHEADER bi;
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = screenWidth;
		bi.biHeight = -screenHeight;// Top-down orientation
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		DWORD dwBmpSize = ((screenWidth * bi.biBitCount + 31) / 32) * 4 * screenHeight;
		std::vector<BYTE> lpBitmapBits(dwBmpSize);
		GetDIBits(screenDC, bitmap, 0, screenHeight, lpBitmapBits.data(), (BITMAPINFO*) &bi, DIB_RGB_COLORS);

		// Convert BGR to RGB
		for (size_t i = 0; i < dwBmpSize; i += 3) {
			std::swap(lpBitmapBits[i], lpBitmapBits[i + 2]);
		}

		SelectObject(memoryDC, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(memoryDC);
		ReleaseDC(NULL, screenDC);

		int result = stbi_write_png(filename, screenWidth, screenHeight, 3, lpBitmapBits.data(), screenWidth * 3);
		return result != 0;
	}

	std::pair<int, int> MatchTemplate(const std::string& template_path, double threshold) {
		Utils::SaveScreenshotAsPng("screenshot.png");

		cv::Mat screen = cv::imread("screenshot.png");
		cv::Mat templateIMG = cv::imread(template_path);

		cv::Mat result;
		cv::matchTemplate(screen, templateIMG, result, cv::TM_CCOEFF_NORMED);

		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

		if (maxVal > threshold) {
			int xMid = maxLoc.x + (templateIMG.cols / 2);
			int yMid = maxLoc.y + (templateIMG.rows / 2);
			CoreLogger::Log(LogLevel::INFO, "Button found from template {} at location: ({}, {})", template_path.c_str(), xMid, yMid);
			return {xMid, yMid};
		} else {
			return {-1, -1};
		}
	}
}// namespace Utils
