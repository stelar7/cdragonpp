#include <iostream>
#include <fstream>
#include "wad\WADFile.hpp"

using namespace cdragon::wad;

int main()
{
	std::string path = "C:\\Users\\Steffen\\Downloads\\cdragon\\Ahri.wad.client";
	std::ifstream file;
	file.open(path);
	cdragon::wad::WADFile wad;
	file >> wad;
}

