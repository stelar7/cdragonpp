#include <fstream>
#include <filesystem>
#include "../libs/json/json.hpp"
#include "util/WebDownloader.hpp"
#include "types/wad/WADFile.hpp"
#include "types/rman/PatcherJsonFile.hpp"
#include "types/rman/RMANFile.hpp"
#include "util/DragonStream.hpp"
#include <iostream>

using namespace cdragon::wad;
using namespace cdragon::rman;
using namespace cdragon::util;
using namespace cdragon::web;

#define TEST_RMAN 1
#define TEST_WAD 1
#define TEST_GET 1


int main()
{

#if TEST_RMAN
    {
        using json = nlohmann::json;
        std::string manifestPath = "C:/Users/Steffen/Downloads/cdragon/patcher/manifests/72.json";
        std::ifstream ifs(manifestPath, std::ios::binary);
        auto value = json::parse(ifs);
        auto jsonval = PatcherJson(value);

        auto file = DragonInStream(jsonval.client_patch_url);
        RMANFile rman;
        file >> rman;

        std::cin.get();
    }
#endif

#if TEST_GET
    {
        // downloads a string and stores it in memory
        std::string url = "http://stelar7.no/cdragon/version.txt";
        std::string output = R"(C:\Users\Steffen\Desktop\test\version.txt)";
        auto outPath = std::filesystem::path(output);

        Downloader downloader;
        auto data = downloader.downloadString(url);
        std::cout << data.data() << std::endl;

        // downloads a string and stores it in a file
        downloader.downloadFile(url, outPath);

        std::cin.get();
    }
#endif

#if TEST_WAD
    {
        // tries to parse a valid WAD file
        std::string path = R"(C:\Users\Steffen\Downloads\cdragon\FiddleSticks.wad.client)";
        auto file = DragonInStream(std::filesystem::path(path));

        WADFile wad;
        file >> wad;
        if (wad) {
            std::cout << path.data() << " parsed ok!" << std::endl;
        }
        else {
            std::cout << path.data() << " parsed bad!" << std::endl;
        }

        // tries to parse an invalid WAD file
        path = R"(C:\Users\Steffen\Downloads\cdragon\9fbb7f50baf65f23.crid)";
        file.ifs.open(std::filesystem::path(path));

        file >> wad;
        if (wad) {
            std::cout << path.data() << " parsed ok!" << std::endl;
        }
        else {
            std::cout << path.data() << " parsed bad!" << std::endl;
        }

        std::cin.get();
    }
#endif
}

