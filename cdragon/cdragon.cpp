#include <iostream>
#include <fstream>
#include "wad\WADFile.hpp"

using namespace cdragon::wad;

int main()
{
	std::string path = "C:\\Users\\Steffen\\Downloads\\cdragon\\9fbb7f50baf65f23.crid";
	std::ifstream file(path, std::ios::binary);
	cdragon::wad::WADFile wad;
	file >> wad;

	if (wad) {
		std::cout << path.data() << " parsed ok!" << std::endl;
	}
	else {
		std::cout << path.data() << " parsed bad!" << std::endl;
	}

	path = "C:\\Users\\Steffen\\Downloads\\cdragon\\FiddleSticks.wad.client";
	file = std::ifstream(path, std::ios::binary);
	file >> wad;

	if (wad) {
		std::cout << path.data() << " parsed ok!" << std::endl;
	}
	else {
		std::cout << path.data() << " parsed bad!" << std::endl;
	}

}

