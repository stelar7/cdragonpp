#include "WADFile.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace cdragon::wad;
bool cdragon::wad::operator!(WADFile & file)
{
	return bool();
}

cdragon::wad::WADFile::operator bool()
{
	return _valid;
}

// is needs to be opened in binary mode
std::istream& cdragon::wad::operator>>(std::istream& is, WADFile& obj)
{
	try {
		is.read(reinterpret_cast<char*>(&obj.header.magic), sizeof(std::int16_t));
		is.read(reinterpret_cast<char*>(&obj.header.major), sizeof(std::int8_t));
		is.read(reinterpret_cast<char*>(&obj.header.minor), sizeof(std::int8_t));

		std::int32_t fileCount = -1;

		if (obj.header.major == 1) {
			WADHeader::v1 ver;
			is.read(reinterpret_cast<char*>(&ver.entryOffset), sizeof(std::int16_t));
			is.read(reinterpret_cast<char*>(&ver.entryCellSize), sizeof(std::int16_t));
			is.read(reinterpret_cast<char*>(&ver.entryCount), sizeof(std::int32_t));
			obj.header.version = ver;

			fileCount = ver.entryCount;
		}

		if (obj.header.major == 2) {
			WADHeader::v2 ver;
			is.read(reinterpret_cast<char*>(&ver.ECDSALength), sizeof(std::int8_t));

			// is there a better way to do this?
			for (std::int8_t i = 0; i < ver.ECDSALength; i++) {
				std::byte val;
				is.read(reinterpret_cast<char*>(&val), sizeof(val));
				ver.ECDSA.push_back(val);
			}

			// is there a better way to do this?
			for (std::int8_t i = 0; i < (83 - ver.ECDSALength); i++) {
				std::byte val;
				is.read(reinterpret_cast<char*>(&val), sizeof(val));
				ver.ECDSAPadding.push_back(val);
			}

			is.read(reinterpret_cast<char*>(&ver.checksum), sizeof(std::int64_t));
			is.read(reinterpret_cast<char*>(&ver.entryOffset), sizeof(std::int16_t));
			is.read(reinterpret_cast<char*>(&ver.entryCellSize), sizeof(std::int16_t));
			is.read(reinterpret_cast<char*>(&ver.entryCount), sizeof(std::int32_t));
			obj.header.version = ver;

			fileCount = ver.entryCount;
		}

		if (obj.header.major == 3) {
			WADHeader::v3 ver;

			// is there a better way to do this?
			for (std::int16_t i = 0; i < 256; i++) {
				std::byte val;
				is.read(reinterpret_cast<char*>(&val), sizeof(val));
				ver.ECDSA.push_back(val);
			}

			is.read(reinterpret_cast<char*>(&ver.checksum), sizeof(std::int64_t));
			is.read(reinterpret_cast<char*>(&ver.entryCount), sizeof(std::int32_t));
			obj.header.version = ver;

			fileCount = ver.entryCount;
		}

		for (std::int32_t i = 0; i < fileCount; i++) {

			WADContentHeader content;
			WADContentHeader::v1 var;

			// is there a better way to do this?
			std::int64_t temp;
			std::stringstream ss;
			is.read(reinterpret_cast<char*>(&temp), sizeof(std::int64_t));
			ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << temp;
			var.pathHash = ss.str();

			is.read(reinterpret_cast<char*>(&var.offset), sizeof(std::int32_t));
			is.read(reinterpret_cast<char*>(&var.compressedSize), sizeof(std::int32_t));
			is.read(reinterpret_cast<char*>(&var.uncompressedSize), sizeof(std::int32_t));

			// is there a better way to do this?
			is.read(reinterpret_cast<char*>(&var.compression), (obj.header.major > 1 ? sizeof(std::int8_t) : sizeof(std::int32_t)));
			var.compression = static_cast<WADCompressionType>(var.compression & 0xFF);

			content.version = var;
			if (obj.header.major > 1 && obj.header.major < 4) {
				WADContentHeader::v2 var2(var);
				is.read(reinterpret_cast<char*>(&var2.duplicate), sizeof(std::int8_t));
				is.read(reinterpret_cast<char*>(&var2.paddding), sizeof(std::int16_t));
				is.read(reinterpret_cast<char*>(&var2.sha256), sizeof(std::int64_t));
				content.version = var2;
			}

			obj.content.push_back(content);
		}

		obj._valid = true;
	}
	catch (const std::exception& e) {
		std::cout << e.what() << '\n';
	}


	if (!obj) {
		is.setstate(std::ios::failbit);
	}

	return is;
}
