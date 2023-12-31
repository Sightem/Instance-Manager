#pragma once
#include <filesystem>
#include <opencv2/core/matx.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

#include "logging/CoreLogger.hpp"


namespace Utils {
	std::vector<unsigned char> ParsePattern(const std::string& pattern);
	void ModifyAppxManifest(const std::filesystem::path& filePath, const std::string& name);
	uintptr_t BoyerMooreHorspool(const unsigned char* signature, size_t signatureSize, const unsigned char* data, size_t dataSize);
	void DownloadAndSave(const std::string& url, const std::string& localFileName);
	void DecompressZip(const std::string& zipFile, const std::string& destination);
	void CopyFileToDestination(const std::string& source, const std::string& destination);
	void WriteAppxManifest(const std::string& url, const std::string& localPath, const std::string& name = "");
	void UpdatePackage(const std::string& baseFolder, const std::string& instanceName = "");
	bool SaveScreenshotAsPng(const char* filename);
	std::pair<int, int> MatchTemplate(const std::string& template_path, double threshold);

	template<typename Func, typename SelectionType>
	    requires requires(SelectionType selection) {
		    { selection.size() } -> std::convertible_to<std::size_t>;
		    { selection[std::size_t{}] } -> std::convertible_to<bool>;
	    }
	void ForEachSelectedInstance(const SelectionType& selection, Func func) {
		for (const auto [index, isSelected] : selection | std::views::enumerate) {
			if (isSelected) {
				func(index);
			}
		}
	}

	template<typename T>
	concept IsDuration = std::is_convertible_v<T, std::chrono::duration<typename T::rep, typename T::period>>;
	template<IsDuration T>
	void SleepFor(T duration) {
		std::this_thread::sleep_for(duration);
	}
}// namespace Utils