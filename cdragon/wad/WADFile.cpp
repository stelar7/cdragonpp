#include "WADFile.hpp"
#include "../util/DragonStream.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace cdragon::wad;
using namespace cdragon::util;

std::istream& cdragon::wad::operator>>(DragonInStream& is, WADFile& obj)
{
    try {
        is >> obj.header.magic;
        if (obj.header.magic[0] != 'R' || obj.header.magic[1] != 'W') {
            throw std::exception("Invalid magic number in header");
        }

        is >> obj.header.major;
        is >> obj.header.minor;

        std::int32_t fileCount = -1;

        if (obj.header.major == 1) {
            WADHeader::v1 ver;
            is >> ver.entryOffset;
            is >> ver.entryCellSize;
            is >> ver.entryCount;

            obj.header.version = ver;
            fileCount = ver.entryCount;
        }

        if (obj.header.major == 2) {
            WADHeader::v2 ver;
            is >> (ver.ECDSALength);

            // is there a better way to do this?
            for (std::int8_t i = 0; i < ver.ECDSALength; i++) {
                std::byte val;
                is >> val;
                ver.ECDSA.push_back(val);
            }

            // is there a better way to do this?
            for (std::int8_t i = 0; i < (83 - ver.ECDSALength); i++) {
                std::byte val;
                is >> val;
                ver.ECDSAPadding.push_back(val);
            }

            is >> ver.checksum;
            is >> ver.entryOffset;
            is >> ver.entryCellSize;
            is >> ver.entryCount;

            obj.header.version = ver;
            fileCount = ver.entryCount;
        }

        if (obj.header.major == 3) {
            WADHeader::v3 ver;

            // is there a better way to do this?
            for (std::int16_t i = 0; i < 256; i++) {
                std::byte val;
                is >> val;
                ver.ECDSA.push_back(val);
            }
            is >> ver.checksum;
            is >> ver.entryCount;

            obj.header.version = ver;
            fileCount = ver.entryCount;
        }

        for (std::int32_t i = 0; i < fileCount; i++) {

            WADContentHeader content;
            WADContentHeader::v1 var;

            is >> var.pathHash;
            is >> var.offset;
            is >> var.compressedSize;
            is >> var.uncompressedSize;

            // is there a better way to do this? seems like there isnt (https://stackoverflow.com/questions/5633784/input-from-stream-to-enum-type)
            is.read(var.compression, (obj.header.major > 1 ? sizeof(std::int8_t) : sizeof(std::int32_t)));
            var.compression = static_cast<WADCompressionType>(var.compression & 0xFF);

            content.version = var;
            if (obj.header.major > 1 && obj.header.major < 4) {
                WADContentHeader::v2 var2(var);
                is >> var2.duplicate;
                is >> var2.paddding;
                is >> var2.sha256;

                content.version = var2;
            }

            obj.content.push_back(content);
        }

        obj._valid = true;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        obj._valid = false;
    }

    if (!obj) {
        is.ifs.setstate(std::ios::failbit);
    }

    return is.ifs;
}

std::string cdragon::wad::WADContentHeader::v1::hashAsHex()
{
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << this->pathHash;
    return ss.str();
}
