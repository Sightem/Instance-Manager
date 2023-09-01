#pragma once
#include <string>
#include <vector>
#include <thread>

namespace Utils
{
    std::vector<unsigned char> ParsePattern(const std::string& pattern);
    std::string ModifyAppxManifest(std::string inputXML, std::string name);
    uintptr_t BoyerMooreHorspool(const unsigned char* signature, size_t signatureSize, const unsigned char* data, size_t dataSize);
    void DownloadAndSave(const std::string& url, const std::string& localFileName);
    void DecompressZip(const std::string& zipFile, const std::string& destination);
    void CopyFileToDestination(const std::string& source, const std::string& destination);
    void WriteAppxManifest(const std::string& url, const std::string& localPath, const std::string& name = "");
    void UpdatePackage(const std::string& baseFolder, const std::string& instanceName = "");
    bool SaveScreenshotAsPng(const char* filename);
    std::pair<int, int> MatchTemplate(const std::string& template_path, double threshold);


    template<typename T>
    void SleepFor(T duration) {
        std::this_thread::sleep_for(duration);
    }
}