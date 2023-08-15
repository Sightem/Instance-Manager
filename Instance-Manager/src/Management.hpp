#pragma once
#include "imgui.h"
#include "Walnut/UI/UI.h"
#include "Walnut/Application.h"

#include "functions.h"

namespace fs = std::filesystem;

class Management : public Walnut::Layer
{
	public:
	Management(std::vector<RobloxInstance>& instances, std::vector<bool>& selection)
		: instances(instances), selection(selection)
	{}

	~Management() = default;

	virtual void OnUIRender() override
	{
		ImGui::Begin("File Management");

        if (ImGui::Button("Refresh"))
        {
            cache.clear();
        }

		for (int i = 0; i < instances.size(); ++i)
		{
			if (selection[i])
			{

				if (ImGui::TreeNode(instances[i].Package.Name.c_str()))
				{
                    std::string path = FS::FindFiles(fmt::format("C:\\Users\\{}\\AppData\\Local\\Packages", Native::GetCurrentUsername()), instances[i].Package.Name + "_")[0];
					DisplayFilesAndDirectories(instances[i].Package.PackageFamilyName, path, true);
					ImGui::TreePop();

				}
			}
		}

		ImGui::End();
	}

private:
	std::vector<RobloxInstance>& instances;
	std::vector<bool>& selection;

    struct DirectoryEntryInfo {
        fs::directory_entry entry;
        std::string filename;
        std::string uniqueId;
    };
    std::unordered_map<std::string, std::vector<DirectoryEntryInfo>> cache;

    void DisplayFilesAndDirectories(std::string packageFamilyName, const std::filesystem::path& directory, bool forceRefresh = false)
    {

        if (forceRefresh || cache.find(directory.string()) == cache.end())
        {
            cache[directory.string()] = std::vector<DirectoryEntryInfo>();

            for (const auto& entry : fs::directory_iterator(directory))
            {
                DirectoryEntryInfo info;
                info.entry = entry;
                info.filename = entry.path().filename().string();
                info.uniqueId = directory.string() + "/" + info.filename;

                cache[directory.string()].push_back(info);
            }
        }

        for (const auto& info : cache[directory.string()])
        {
            if (fs::is_directory(info.entry.status()))
            {
                bool isOpen = ImGui::TreeNode(info.filename.c_str());

                ImGui::PushID(info.uniqueId.c_str());
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Open in explorer")) 
                    {
                        if (!Native::OpenInExplorer(info.entry.path().string()))
                        {
                            //MessageBox(NULL, "Failed to open directory.", "Error", MB_ICONERROR);
                            AppLog::getInstance().add_log("Failed to open directory {}", info.entry.path().string());
                        }
                    }

                    if (ImGui::MenuItem("Clone"))
                    {
                        clone_dir(packageFamilyName, info.entry.path().string());
                    }

                    ImGui::EndPopup();
                }
                ImGui::PopID();

                if (isOpen)
                {
                    DisplayFilesAndDirectories(packageFamilyName, info.entry.path());
                    ImGui::TreePop();
                }
            }
            else if (fs::is_regular_file(info.entry.status()))
            {
                ImGui::Selectable(info.filename.c_str());

                // Context menu for file
                ImGui::PushID(info.uniqueId.c_str());
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Open in explorer"))
                    {
                        Native::OpenInExplorer(info.entry.path().string(), true);
                    }
                    // ... add more file menu items
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
        }
    }

    void clone_dir(std::string packageFamilyName, const std::filesystem::path& full_src_path)
    {
        static std::string base_src = "C:\\Users\\" + Native::GetCurrentUsername() + "\\AppData\\Local\\Packages";
        std::filesystem::path relative_path = full_src_path.lexically_relative(base_src + "\\" + packageFamilyName);

        // Iterate through the selected instances and clone the source to each of them
        for (int i = 0; i < instances.size(); i++)
        {
            if (selection[i])
            {
                std::filesystem::path dst_path = base_src + "\\" + instances[i].Package.PackageFamilyName + "\\" + relative_path.string();

                if (full_src_path == dst_path)
                {
                    AppLog::getInstance().add_log("Source and destination are the same. Skipping copy for {}", full_src_path.string());
                    continue;  // Skip the copy operation for this iteration
                }

                if (!FS::CopyDirectory(full_src_path, dst_path)) {
                    std::cerr << "Failed to copy directory " << full_src_path << " to " << dst_path << '\n';
                    AppLog::getInstance().add_log("Failed to copy directory {} to {}", full_src_path.string(), dst_path.string());
                }
            }
        }
    }
};