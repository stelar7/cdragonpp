#include <iostream>
#include <fstream>
#include "wad\WADFile.hpp"
#include "util\DragonStream.hpp"

using namespace cdragon::wad;
using namespace cdragon::util;

int main()
{
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

