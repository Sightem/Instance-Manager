#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace FS {
	bool CopyDirectory(const std::filesystem::path& src, const std::filesystem::path& dst);
	bool RemovePath(const std::filesystem::path& path_to_delete);
	bool DecompressZip(const std::string& zipPath, const std::string& destination);
	bool DecompressZipToFile(const std::string& zipPath, const std::string& destination);
	std::vector<std::string> FindFiles(const std::string& path, const std::string& substring);
}// namespace FS
