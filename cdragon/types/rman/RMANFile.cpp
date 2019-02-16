#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include "RMANFile.hpp"
#include "../../util/DragonStream.hpp"
#include "../../../libs/zstd/include/zstd.h"

using namespace cdragon::util;
using namespace cdragon::rman;

std::istream& cdragon::rman::operator>>(DragonInStream& is, RMANFile& obj)
{
    try {

        {
            // parse header
            is >> obj.manifestHeader.magic;
            is >> obj.manifestHeader.version;
            is >> obj.manifestHeader.unknown;
            is >> obj.manifestHeader.signatureType;
            is >> obj.manifestHeader.offset;
            is >> obj.manifestHeader.length;
            is >> obj.manifestHeader.manifestId;
            is >> obj.manifestHeader.decompressedLength;
        }

        is.seek(obj.manifestHeader.offset);

        // read remaining body into arrays
        std::vector<std::byte> compressedBytes;
        for (std::int32_t i = 0; i < obj.manifestHeader.length; i++) {
            std::byte val;
            is >> val;
            compressedBytes.push_back(val);
        }

        if (obj.manifestHeader.signatureType != 0) {
            for (std::int16_t i = 0; i < 256; i++) {
                std::byte val;
                is >> val;
                obj.signature.push_back(val);
            }
        }

        // decompress and keep parsing
        std::vector<std::byte> decompressedBytes;
        decompressedBytes.resize(obj.manifestHeader.decompressedLength);

        ZSTD_DCtx* dctx = ZSTD_createDCtx();
        ZSTD_inBuffer in = { compressedBytes.data(), compressedBytes.size(), 0 };
        ZSTD_outBuffer out = { decompressedBytes.data(), decompressedBytes.size(), 0 };
        std::size_t err = ZSTD_isError(ZSTD_decompressStream(dctx, &out, &in));
        if (err) {
            std::cout << ZSTD_getErrorName(err) << std::endl;
        }


        DragonByteStream body(decompressedBytes);
        std::int32_t headerOffset;
        body >> headerOffset;
        body.seek(headerOffset);

        body >> obj.offsetTable.offsetTableOffset;

        std::int32_t offsetHolder;

        body >> offsetHolder;
        body.seek(offsetHolder, std::ios_base::cur);
        obj.offsetTable.bundleListOffset = body.pos() - 4;
        body.seek(-offsetHolder, std::ios_base::cur);

        body >> offsetHolder;
        body.seek(offsetHolder, std::ios_base::cur);
        obj.offsetTable.languageListOffset = body.pos() - 4;
        body.seek(-offsetHolder, std::ios_base::cur);

        body >> offsetHolder;
        body.seek(offsetHolder, std::ios_base::cur);
        obj.offsetTable.fileListOffset = body.pos() - 4;
        body.seek(-offsetHolder, std::ios_base::cur);

        body >> offsetHolder;
        body.seek(offsetHolder, std::ios_base::cur);
        obj.offsetTable.folderListOffset = body.pos() - 4;
        body.seek(-offsetHolder, std::ios_base::cur);

        body >> offsetHolder;
        body.seek(offsetHolder, std::ios_base::cur);
        obj.offsetTable.keyHeaderOffset = body.pos() - 4;
        body.seek(-offsetHolder, std::ios_base::cur);

        body >> offsetHolder;
        body.seek(offsetHolder, std::ios_base::cur);
        obj.offsetTable.unknownOffset = body.pos() - 4;
        body.seek(-offsetHolder, std::ios_base::cur);


        {
            // parse bundles
            body.seek(obj.offsetTable.bundleListOffset);
            std::int32_t bundleCount;
            body >> bundleCount;
            for (std::int32_t i = 0; i < bundleCount; i++) {
                RMANFileBundle bundle;
                body >> bundle.offset;
                std::int32_t nextBundle = body.pos();

                body.seek(bundle.offset - 4, std::ios_base::cur);

                body >> bundle.offsetTableOffset;
                body >> bundle.headerSize;
                body >> bundle.bundleId;

                if (bundle.headerSize > 12) {
                    for (std::int16_t i = 0; i < (bundle.headerSize - 12); i++) {
                        std::byte val;
                        body >> val;
                        bundle.skipped.push_back(val);
                    }
                }

                std::int32_t chunkCount;
                body >> chunkCount;
                for (std::int32_t j = 0; j < chunkCount; j++) {
                    std::int32_t chunkOffset;
                    body >> chunkOffset;
                    std::int32_t nextChunk = body.pos();
                    body.seek(chunkOffset - 4);

                    RMANFileBundleChunk chunk;
                    body >> chunk.offsetTableOffset;
                    body >> chunk.compressedSize;
                    body >> chunk.uncompressedSize;
                    body >> chunk.chunkId;
                    bundle.chunks.push_back(chunk);

                    body.seek(nextChunk);
                }
                body.seek(nextBundle);
                obj.bundles.push_back(bundle);
            }
        }

        {
            // parse languages
            body.seek(obj.offsetTable.languageListOffset);
            std::int32_t languageCount;
            body >> languageCount;
            for (std::int32_t i = 0; i < languageCount; i++) {
                RMANFileLanguage lang;
                body >> lang.offset;
                std::int32_t nextLang = body.pos();

                body.seek(lang.offset - 4, std::ios_base::cur);

                body >> lang.offsetTableOffset;
                body >> lang.languageId;

                body >> lang.nameOffset;
                body.seek(lang.nameOffset - 4, std::ios_base::cur);
                std::int32_t nameSize;
                body >> nameSize;
                //body.read(lang.name, nameSize); this doesnt work...
                char buff[256];
                body.read(buff, nameSize);
                lang.name = std::string(buff);
                lang.name.resize(nameSize);

                body.seek(nextLang);
                obj.languages.push_back(lang);
            }
        }

        {
            // parse files
            body.seek(obj.offsetTable.fileListOffset);
            std::int32_t fileCount;
            body >> fileCount;
            for (std::int32_t i = 0; i < fileCount; i++) {
                RMANFileFile file;
                // todo
            }
        }

        {
            // parse folder
            body.seek(obj.offsetTable.folderListOffset);
            std::int32_t folderCount;
            body >> folderCount;
            for (std::int32_t i = 0; i < folderCount; i++) {
                RMANFileFolder folder;
                // todo
            }
        }


        std::cin.get();
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

std::string toHex(std::int64_t val) {
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << val;
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

