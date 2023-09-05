#include "utils/filesystem/FS.h"

#define NOMINMAX
#include <windows.h>

#include <fstream>

#include "libzippp/libzippp.h"
#include "logging/CoreLogger.hpp"

namespace FS {
	bool CopyDirectory(const std::filesystem::path& src, const std::filesystem::path& dst) {
		auto abs_src = std::filesystem::absolute(src);
		auto abs_dst = std::filesystem::absolute(dst);

		if (!std::filesystem::exists(abs_src) || !std::filesystem::is_directory(abs_src)) {
			CoreLogger::Log(LogLevel::WARNING, "Source directory {} does not exist or is not a directory.", abs_src.string());
			return false;
		}

		if (!std::filesystem::exists(abs_dst)) {
			std::filesystem::create_directories(abs_dst);
		}

		auto dir_iter = std::filesystem::recursive_directory_iterator(abs_src);
		return std::ranges::all_of(dir_iter, [&](const auto& entry) {
			const auto& src_path = entry.path();
			auto dst_path = abs_dst / src_path.lexically_relative(abs_src);

			if (entry.is_directory()) {
				std::filesystem::create_directories(dst_path);
			} else if (entry.is_regular_file()) {
				try {
					std::filesystem::copy_file(src_path, dst_path, std::filesystem::copy_options::overwrite_existing);
				} catch (const std::filesystem::filesystem_error& e) {
					CoreLogger::Log(LogLevel::ERR, "Error copying file {} to {}: {}", src_path.string(), dst_path.string(), e.what());
					return false;
				}
			} else {
				CoreLogger::Log(LogLevel::WARNING, "Skipping non-regular file {}", src_path.string());
				return true;// Consider skipping as successful to allow the rest of the files to be copied.
			}
			return true;
		});
	}


	bool RemovePath(const std::filesystem::path& path_to_delete) {
		std::error_code ec;

		if (std::filesystem::exists(path_to_delete, ec)) {
			try {
				if (std::filesystem::is_regular_file(path_to_delete)) {
					return std::filesystem::remove(path_to_delete, ec);
				} else if (std::filesystem::is_directory(path_to_delete)) {
					return std::filesystem::remove_all(path_to_delete, ec) > 0;
				} else {
					CoreLogger::Log(LogLevel::ERR, "Unsupported file type.");
					return false;
				}
			} catch (const std::filesystem::filesystem_error& e) {
				CoreLogger::Log(LogLevel::ERR, "Error: {}", e.what());
				return false;
			}
		} else {
			// Path does not exist
			CoreLogger::Log(LogLevel::ERR, "Error: Path does not exist.");
			return false;
		}
	}

	bool DecompressZip(const std::string& zipPath, const std::string& destination) {
		using namespace libzippp;

		ZipArchive zip(zipPath);
		if (!zip.open(ZipArchive::ReadOnly)) {
			CoreLogger::Log(LogLevel::ERR, "Failed to open zip archive: {}", zipPath);
			return false;
		}


		auto nbEntries = zip.getNbEntries();
		for (int i = 0; i < nbEntries; ++i) {
			ZipEntry entry = zip.getEntry(i);
			if (!entry.isNull()) {
				std::string outputPath = destination + "\\" + entry.getName();

				if (entry.isDirectory()) {
					CreateDirectoryA(outputPath.c_str(), NULL);
				} else {
					std::string content = entry.readAsText();
					std::ofstream outputFile(outputPath, std::ios::binary);
					if (outputFile) {
						outputFile.write(content.c_str(), content.size());
						outputFile.close();
					} else {
						CoreLogger::Log(LogLevel::ERR, "Failed to write file: {}", outputPath);
					}
				}
			}
		}

		zip.close();

		return true;
	}

	bool DecompressZipToFile(const std::string& zipPath, const std::string& destination) {
		using namespace libzippp;

		ZipArchive zip(zipPath);
		if (!zip.open(ZipArchive::ReadOnly)) {
			CoreLogger::Log(LogLevel::ERR, "Failed to open zip archive: {}", zipPath);
			return false;
		}

		auto nbEntries = zip.getNbEntries();

		if (nbEntries != 1) {
			CoreLogger::Log(LogLevel::ERR, "Zip archive must contain only one file: {}", zipPath);
			zip.close();
			return false;
		}

		ZipEntry entry = zip.getEntry(0);
		if (!entry.isNull() && !entry.isDirectory()) {
			std::string content = entry.readAsText();
			std::ofstream outputFile(destination, std::ios::binary);
			if (outputFile) {
				outputFile.write(content.c_str(), content.size());
				outputFile.close();
			} else {
				CoreLogger::Log(LogLevel::ERR, "Failed to write file: {}", destination);
				zip.close();
				return false;
			}
		} else {
			CoreLogger::Log(LogLevel::ERR, "Invalid zip entry: {}", zipPath);
			zip.close();
			return false;
		}

		zip.close();

		return true;
	}

	std::vector<std::string> FindFiles(const std::string& path, const std::string& substring) {
		std::vector<std::string> result;
		try {
			for (const auto& entry: std::filesystem::directory_iterator(path)) {
				if ((entry.is_regular_file() || entry.is_directory()) && entry.path().string().find(substring) != std::string::npos) {
					result.push_back(entry.path().string());
				}
			}
		} catch (const std::filesystem::filesystem_error& e) {
			CoreLogger::Log(LogLevel::ERR, "Error: {}", e.what());
		}

		return result;
	}

}// namespace FS