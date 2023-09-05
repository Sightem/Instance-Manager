#include "ui/FileManagement.h"

#include <ranges>

#include "logging/CoreLogger.hpp"

void FileManagement::Draw(const char* title, bool* p_open) {
	if (!ImGui::Begin(title, p_open)) {
		ImGui::End();
		return;
	}

	if (ImGui::Button("Refresh")) {
		cache.clear();
	}

	auto SelectedIndices = std::views::iota(0, static_cast<int>(instances.size())) | std::views::filter([&](int i) { return selection[i]; });

	for (int i: SelectedIndices) {
		if (ImGui::TreeNode(g_InstanceControl.GetInstance(instances[i]).Name.c_str())) {
			std::string path = FS::FindFiles(fmt::format(R"({}\AppData\Local\Packages)", Native::GetUserProfilePath()), g_InstanceControl.GetInstance(instances[i]).Name + "_")[0];
			DisplayFilesAndDirectories(g_InstanceControl.GetInstance(instances[i]).PackageFamilyName, path, true);
			ImGui::TreePop();
		}
	}

	ImGui::End();
}


void FileManagement::DisplayFilesAndDirectories(const std::string& packageFamilyName, const std::filesystem::path& directory, bool forceRefresh) {

	if (forceRefresh || cache.find(directory.string()) == cache.end()) {
		cache[directory.string()] = std::vector<DirectoryEntryInfo>();

		for (const auto& entry: fs::directory_iterator(directory)) {
			DirectoryEntryInfo info;
			info.entry = entry;
			info.filename = entry.path().filename().string();
			info.uniqueId = directory.string() + "/" + info.filename;

			cache[directory.string()].push_back(info);
		}
	}

	for (const auto& info: cache[directory.string()]) {
		if (fs::is_directory(info.entry.status())) {
			bool isOpen = ImGui::TreeNode(info.filename.c_str());

			ImGui::PushID(info.uniqueId.c_str());
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Open in explorer")) {
					if (!Native::OpenInExplorer(info.entry.path().string())) {
						CoreLogger::Log(LogLevel::ERR, "Failed to open directory {}", info.entry.path().string());
					}
				}

				if (ImGui::MenuItem("Clone")) {
					CloneDir(packageFamilyName, info.entry.path().string());
				}

				ImGui::EndPopup();
			}
			ImGui::PopID();

			if (isOpen) {
				DisplayFilesAndDirectories(packageFamilyName, info.entry.path());
				ImGui::TreePop();
			}
		} else if (fs::is_regular_file(info.entry.status())) {
			ImGui::Selectable(info.filename.c_str());

			// Context menu for file
			ImGui::PushID(info.uniqueId.c_str());
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Open in explorer")) {
					Native::OpenInExplorer(info.entry.path().string(), true);
				}
				// ... add more file menu items
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
	}
}

void FileManagement::CloneDir(const std::string& packageFamilyName, const std::filesystem::path& full_src_path) {
	static std::string base_src = fmt::format(R"({}\AppData\Local\Packages)", Native::GetUserProfilePath());
	std::filesystem::path relative_path = full_src_path.lexically_relative(base_src + "\\" + packageFamilyName);

	// Iterate through the selected instances and clone the source to each of them
	for (int i = 0; i < instances.size(); i++) {
		if (selection[i]) {
			std::filesystem::path dst_path = base_src + "\\" + g_InstanceControl.GetInstance(instances[i]).PackageFamilyName + "\\" + relative_path.string();

			if (full_src_path == dst_path) {
				CoreLogger::Log(LogLevel::INFO, "Source and destination are the same. Skipping copy for {}", full_src_path.string());
				continue;// Skip the copy operation for this iteration
			}

			if (!FS::CopyDirectory(full_src_path, dst_path)) {
				CoreLogger::Log(LogLevel::ERR, "Failed to copy directory {} to {}", full_src_path.string(), dst_path.string());
			}
		}
	}
}