#include "RMANFile.hpp"
#include "../../util/DragonStream.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace cdragon::util;
using namespace cdragon::rman;

std::istream& operator>>(DragonInStream& is, RMANFile& file)
{
    return is;
}

std::string toHex(std::int64_t val) {
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << this->manifestId;
    return ss.str();
}

std::string cdragon::rman::RMANFileHeader::idAsHex()
{
    return toHex(this->manifestId);
}

std::string cdragon::rman::RMANFileBundleChunk::idAsHex()
{
    return toHex(this->chunkId);
}

std::string cdragon::rman::RMANFileBundle::idAsHex()
{
    return toHex(this->bundleId);
}

std::string cdragon::rman::RMANFileLanguage::idAsHex()
{
    return toHex(this->languageId);
}

std::string cdragon::rman::RMANFileFile::fileIdAsHex()
{
    return toHex(this->fileId);
}

std::string cdragon::rman::RMANFileFile::directoryIdAsHex()
{
    return toHex(this->directoryId);
}

std::string cdragon::rman::RMANFileFile::languageIdAsHex()
{
    return toHex(this->languageId);
}

std::string cdragon::rman::RMANFileFolder::folderIdAsHex()
{
    return toHex(this->folderId);
}

std::string cdragon::rman::RMANFileFolder::parentIdAsHex()
{
    return toHex(this->parentId);
}

