#include <fstream>
#include <filesystem>
#include "../libs/rapidjson/istreamwrapper.h"
#include "../libs/rapidjson/stringbuffer.h"
#include "../libs/rapidjson/document.h"
#include "../libs/rapidjson/writer.h"
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
#define TEST_WAD 0
#define TEST_GET 0


int main()
{

#if TEST_RMAN
    {

        const std::string manifestPath = "C:/Users/Steffen/Downloads/cdragon/patcher/manifests/72.json";
        std::ifstream ifs(manifestPath, std::ios::binary);

        using namespace  rapidjson;
        rapidjson::IStreamWrapper isw(ifs);
        Document d;
        d.ParseStream(isw);
        auto jsonval = PatcherJson(d);

        std::cout << "Starting parsing of: " << jsonval.client_patch_url << std::endl;
        auto file = DragonInStream(jsonval.client_patch_url);
        RMANFile rman;
        file >> rman;

        
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        rman.Serialize(writer);

        std::ofstream myfile;
        myfile.open(R"(C:\Users\Steffen\Desktop\rman.txt)");
        myfile << buffer.GetString();
        myfile.close();
        

       // std::cin.get();
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

        std::cout << "Starting parsing of: " << path.data() << std::endl;

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

        std::cout << "Starting parsing of: " << path.data() << std::endl;

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

