#include <iostream>
#include <fstream>
#include "wad\WADFile.hpp"

using namespace cdragon::wad;

int main()
{
	std::string path = "C:\\Users\\Steffen\\Downloads\\cdragon\\Ahri.wad.client";
	std::ifstream file(path, std::ios::binary);
	cdragon::wad::WADFile wad;
	file >> wad;
}

