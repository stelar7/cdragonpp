#pragma once

#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include "RMANFile.hpp"
#include "../../util/ZSTDHandler.hpp"
#include "../../util/DragonStream.hpp"

using namespace cdragon::crypto;
using namespace cdragon::util;
using namespace cdragon::rman;

std::istream& cdragon::rman::operator>>(DragonInStream& is, RMANFile& obj)
{
    try {

        {
            // clear out old data
            obj = RMANFile();
        }

        {
            // parse header
            is >> obj.manifestHeader.magic;
            if (std::strcmp(obj.manifestHeader.magic, "RMAN") != 0) {
                throw std::exception("Invalid magic number in header");
            }

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

        ZSTDHandler handler;
        handler.decompress(compressedBytes, decompressedBytes);

        DragonByteStream body(decompressedBytes);
        std::int32_t headerOffset;
        body >> headerOffset;
        body.seek(headerOffset);

        {
            // parse offset table
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
        }

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
                body.read(lang.name, nameSize);

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
                body >> file.offset;
                std::int32_t nextFile = body.pos();
                body.seek(file.offset - 4, std::ios_base::cur);

                body >> file.offsetTableOffset;

                body >> file.nameOffset.offset;
                if (!(file.nameOffset.data.offset > 0 && file.nameOffset.data.offset != 0x10200)) {
                    body >> file.nameOffset;
                }
                body.seek(file.nameOffset.offset - 4, std::ios_base::cur);
                std::int32_t nameSize;
                body >> nameSize;
                body.read(file.name, nameSize);
                body.seek(-file.nameOffset.offset - nameSize, std::ios_base::cur);

                body >> file.structSize;

                body >> file.symlinkOffset;
                body.seek(file.symlinkOffset - 4, std::ios_base::cur);
                body >> nameSize;
                body.read(file.symlinkName, nameSize);
                body.seek(-file.symlinkOffset, std::ios_base::cur);

                body >> file.fileId;

                if (file.structSize > 28) {
                    body >> file.folderId;
                }

                body >> file.fileSize;
                body >> file.permissions;

                if (file.structSize > 36) {
                    body >> file.languageId;
                    body >> file.unknowns.unknowns.unknown1;
                }

                body >> file.singleChunk;
                if (file.singleChunk) {
                    std::int64_t chunk;
                    body >> chunk;
                    body >> file.unknowns.unknowns.unknown2;
                    file.chunks.push_back(chunk);
                }
                else {
                    std::int32_t chunkCount;
                    body >> chunkCount;
                    for (std::int32_t j = 0; j < fileCount; j++) {
                        std::int64_t chunk;
                        body >> chunk;
                        file.chunks.push_back(chunk);
                    }
                }

                body.seek(nextFile);
                obj.files.push_back(file);
            }
        }

        {
            // parse folder
            body.seek(obj.offsetTable.folderListOffset);
            std::int32_t folderCount;
            body >> folderCount;
            for (std::int32_t i = 0; i < folderCount; i++) {
                RMANFileFolder folder;
                body >> folder.offset;
                std::int32_t nextFolder = body.pos();
                body.seek(folder.offset - 4, std::ios_base::cur);

                body >> folder.offsetTableOffset;
                std::int32_t resume = body.pos();
                body.seek(-folder.offsetTableOffset, std::ios_base::cur);
                body >> folder.folderIdOffset;
                body >> folder.parentIdOffset;
                body.seek(resume);
                body >> folder.nameOffset;
                body.seek(folder.nameOffset - 4, std::ios_base::cur);
                std::int32_t nameSize;
                body >> nameSize;
                body.read(folder.name, nameSize);
                body.seek(-folder.nameOffset - nameSize, std::ios_base::cur);

                if (folder.folderIdOffset > 0) {
                    body >> folder.folderId;
                }
                else {
                    folder.folderId = 0;
                }

                if (folder.parentIdOffset > 0) {
                    body >> folder.parentId;
                }
                else {
                    folder.parentId = 0;
                }

                body.seek(nextFolder);
                obj.folders.push_back(folder);
            }
        }

        obj.buildChunkMap();
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

std::string cdragon::rman::RMANFileFile::folderIdAsHex()
{
    return toHex(this->folderId);
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

std::string cdragon::rman::RMANFileFile::getFilePath(RMANFile& manifest)
{
    std::string output = this->name;
    RMANFileFolder parent;
    for (RMANFileFolder& folder : manifest.folders) {
        if (this->folderId == folder.folderId) {
            parent = folder;
            break;
        }
    }

    while (parent.folderId != 0) {
        output.insert(0, parent.name + "/");

        for (RMANFileFolder& folder : manifest.folders) {
            if (parent.parentId == folder.folderId) {
                parent = folder;
                break;
            }
        }
    }

    return output;
}

void cdragon::rman::RMANFile::buildChunkMap()
{
    for (RMANFileBundle& bundle : cdragon::rman::RMANFile::bundles) {
        _bundleMap.insert(std::make_pair(bundle.bundleId, bundle));

        std::int64_t index = 0;
        for (RMANFileBundleChunk& chunk : bundle.chunks) {
            RMANFileBundleChunkInfo chunkInfo(bundle.bundleId, chunk.chunkId, index, chunk.compressedSize);

            _chunkMap.insert(std::make_pair(chunk.chunkId, chunkInfo));
            index += chunk.compressedSize;
        }
    }
}
