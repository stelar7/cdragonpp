#include <iostream>
#include <fstream>
#include <filesystem>
#include "../libs/json/json.hpp"
#include "util/WebDownloader.hpp"
#include "types/wad/WADFile.hpp"
#include "types/rman/PatcherJsonFile.hpp"
#include "types/rman/RMANFile.hpp"
#include "util/DragonStream.hpp"

using namespace cdragon::wad;
using namespace cdragon::rman;
using namespace cdragon::util;
using namespace cdragon::web;

#define TEST_RMAN 1
#define TEST_WAD 0
#define TEST_GET 0


int main()
{

#if TEST_RMAN
    {
        using json = nlohmann::json;
        std::string manifestPath = "C:/Users/Steffen/Downloads/cdragon/patcher/manifests/72.json";
        std::ifstream ifs(manifestPath, std::ios::binary);
        json value = json::parse(ifs);
        PatcherJson jsonval = PatcherJson(value);

        RMANFile rman;
        DragonInStream file = DragonInStream(jsonval.client_patch_url);
        file >> rman;
        std::cin.get();
    }
#endif

#if TEST_GET
    {
        // downloads a string and stores it in memory
        std::string url = "http://stelar7.no/cdragon/version.txt";
        std::string output = "C:\\Users\\Steffen\\Desktop\\test\\version.txt";
        std::filesystem::path outPath = std::filesystem::path(output);

        std::string data = downloadString(url);
        std::cout << data.data() << std::endl;

        // downloads a string and stores it in a file
        downloadFile(url, outPath);
        std::cin.get();
    }
#endif

#if TEST_WAD
    {
        // tries to parse a valid WAD file
        WADFile wad;
        std::string path = "C:\\Users\\Steffen\\Downloads\\cdragon\\FiddleSticks.wad.client";
        DragonInStream file = DragonInStream(std::filesystem::path(path));

        file >> wad;
        if (wad) {
            std::cout << path.data() << " parsed ok!" << std::endl;
        }
        else {
            std::cout << path.data() << " parsed bad!" << std::endl;
        }

        // tries to parse an invalid WAD file
        path = "C:\\Users\\Steffen\\Downloads\\cdragon\\9fbb7f50baf65f23.crid";
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

