#include <iostream>
#include <fstream>
#include <filesystem>
#include "..\libs\json\json.hpp"
#include "util\DragonStream.hpp"
#include "util\WebUtil.hpp"
#include "types\wad\WADFile.hpp"
#include "types\rman\PatcherJsonFile.hpp"
#include "types\rman\RMANFile.hpp"

using namespace cdragon::wad;
using namespace cdragon::rman;
using namespace cdragon::util;
using namespace cdragon::web;


int main()
{


    {
        using json = nlohmann::json;

        json value = R"(
{
    "enabled_client_installs_percent": 0.0, 
    "toggles": {
        "new_client_patcher": 0.0, 
        "batch_16MB": 66.6, 
        "new_game_patcher": 50.0, 
        "batch_4MB": 66.6, 
        "batch_8MB": 66.6
    }, 
    "timestamp": "2019-02-15T23:57:26.914000", 
    "client_patch_url": "https://lol.secure.dyn.riotcdn.net/channels/public/releases/F8487EDD0EE47547.manifest", 
    "version": 72, 
    "game_patch_url": "https://lol.secure.dyn.riotcdn.net/channels/public/releases/BE533EC6530EC3D8.manifest", 
    "enabled_game_installs_percent": 50.0
}
)"_json;

        PatcherJson jsonval = PatcherJson(value);

        RMANFile rman;
        DragonInStream file = DragonInStream(jsonval.client_patch_url);
        file >> rman;

        std::cin.get();

    }

    

    std::string url = "http://stelar7.no/cdragon/version.txt";
    std::string output = "C:\\Users\\Steffen\\Desktop\\test\\version.txt";
    std::filesystem::path outPath = std::filesystem::path(output);

    std::string data = downloadString(url);
    std::cout << data.data() << std::endl;

    downloadFile(url, outPath);
    std::cin.get();


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

    path = "C:\\Users\\Steffen\\Downloads\\cdragon\\9fbb7f50baf65f23.crid";
    file = DragonInStream(std::filesystem::path(path));

    file >> wad;
    if (wad) {
        std::cout << path.data() << " parsed ok!" << std::endl;
    }
    else {
        std::cout << path.data() << " parsed bad!" << std::endl;
    }

    

    std::cin.get();

}

