#include <sstream>
#include <iomanip>
#include <iostream>
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
        is.read(compressedBytes, obj.manifestHeader.length);

        if (obj.manifestHeader.signatureType != 0) {
            is.read(obj.signature, 256);
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
            for (auto i = 0; i < bundleCount; i++) {
                RMANFileBundle bundle;
                body >> bundle.offset;
                auto nextBundle = body.pos();

                body.seek(bundle.offset - 4, std::ios_base::cur);

                body >> bundle.offsetTableOffset;
                body >> bundle.headerSize;
                body >> bundle.bundleId;

                if (bundle.headerSize > 12) {
                    is.read(bundle.skipped, bundle.headerSize - 12);
                }

                std::int32_t chunkCount;
                body >> chunkCount;
                for (auto j = 0; j < chunkCount; j++) {
                    std::int32_t chunkOffset;
                    body >> chunkOffset;
                    auto nextChunk = body.pos();
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
            for (auto i = 0; i < languageCount; i++) {
                RMANFileLanguage lang;
                body >> lang.offset;
                auto nextLang = body.pos();

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
            for (auto i = 0; i < fileCount; i++) {
                RMANFileFile file;
                body >> file.offset;
                auto nextFile = body.pos();
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
                    body >> file.unknowns.combined.unknown1;
                }

                body >> file.singleChunk;
                if (file.singleChunk) {
                    std::int64_t chunk;
                    body >> chunk;
                    body >> file.unknowns.combined.unknown2;
                    file.chunks.push_back(chunk);
                }
                else {
                    std::int32_t chunkCount;
                    body >> chunkCount;
                    for (auto j = 0; j < fileCount; j++) {
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
            for (auto i = 0; i < folderCount; i++) {
                RMANFileFolder folder;
                body >> folder.offset;
                auto nextFolder = body.pos();
                body.seek(folder.offset - 4, std::ios_base::cur);

                body >> folder.offsetTableOffset;
                auto resume = body.pos();
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

std::string toHex(std::int64_t const val) {
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << val;
    return ss.str();
}

std::string RMANFileHeader::idAsHex() const
{
    return toHex(this->manifestId);
}

std::string RMANFileBundleChunk::idAsHex() const
{
    return toHex(this->chunkId);
}

std::string RMANFileBundle::idAsHex() const
{
    return toHex(this->bundleId);
}

std::string RMANFileLanguage::idAsHex() const
{
    return toHex(this->languageId);
}

std::string RMANFileFile::fileIdAsHex() const
{
    return toHex(this->fileId);
}

std::string RMANFileFile::folderIdAsHex() const
{
    return toHex(this->folderId);
}

std::string RMANFileFile::languageIdAsHex() const
{
    return toHex(this->languageId);
}

std::string RMANFileFolder::folderIdAsHex() const
{
    return toHex(this->folderId);
}

std::string RMANFileFolder::parentIdAsHex() const
{
    return toHex(this->parentId);
}

std::vector<std::string> RMANFileFile::chunksAsHex() const
{
    std::vector<std::string> chunks;
    for (auto& chunk : this->chunks)
    {
        chunks.push_back(toHex(chunk));
    }

    return chunks;
}

std::string RMANFileFile::getFilePath(RMANFile& manifest) const
{
    auto output = this->name;
    RMANFileFolder parent;
    for (auto& folder : manifest.folders) {
        if (this->folderId == folder.folderId) {
            parent = folder;
            break;
        }
    }

    while (parent.folderId != 0) {
        output.insert(0, parent.name + "/");

        for (auto& folder : manifest.folders) {
            if (parent.parentId == folder.folderId) {
                parent = folder;
                break;
            }
        }
    }

    return output;
}


void RMANFile::buildChunkMap()
{
    for (auto& bundle : bundles) {
        _bundleMap.insert(std::make_pair(bundle.bundleId, bundle));

        std::int64_t index = 0;
        for (auto & chunk : bundle.chunks) {
            RMANFileBundleChunkInfo chunkInfo(bundle.bundleId, chunk.chunkId, index, chunk.compressedSize);

            _chunkMap.insert(std::make_pair(chunk.chunkId, chunkInfo));
            index += chunk.compressedSize;
        }
    }
}
