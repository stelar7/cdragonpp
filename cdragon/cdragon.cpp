#include <iostream>
#include <fstream>
#include "wad\WADFile.hpp"
#include "util\DragonStream.hpp"
#include "util\WebUtil.hpp"

using namespace cdragon::wad;
using namespace cdragon::util;
using namespace cdragon::web;


int main()
{
    std::string url = "http://stelar7.no/cdragon/version.txt";
    std::string output = "C:\\Users\\Steffen\\Desktop\\test\\version.txt";
    std::filesystem::path outPath = std::filesystem::path(output);

    std::string data = downloadString(url);
    std::cout << data.data() << std::endl;

    downloadFile(url, outPath);

    std::cin.get();


    WADFile wad;
    std::string path = "C:\\Users\\Steffen\\Downloads\\cdragon\\FiddleSticks.wad.client";
    DragonInStream file = DragonInStream(path);

    file >> wad;
    if (wad) {
        std::cout << path.data() << " parsed ok!" << std::endl;
    }
    else {
        std::cout << path.data() << " parsed bad!" << std::endl;
    }

    path = "C:\\Users\\Steffen\\Downloads\\cdragon\\9fbb7f50baf65f23.crid";
    file = DragonInStream(path);

    file >> wad;
    if (wad) {
        std::cout << path.data() << " parsed ok!" << std::endl;
    }
    else {
        std::cout << path.data() << " parsed bad!" << std::endl;
    }

    std::cin.get();

}

