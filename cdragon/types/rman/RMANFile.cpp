#include <sstream>
#include <iomanip>
#include <iostream>
#include "../../util/ZSTDHandler.hpp"
#include "../../util/DragonStream.hpp"
#include "../../util/WebDownloader.hpp"
#include "../../../libs/tclap/ValueArg.h"
#include "../../../libs/tclap/SwitchArg.h"
#include "../../../libs/rapidjson/document.h"
#include "../../../libs/rapidjson/istreamwrapper.h"
#include "RMANFile.hpp"
#include "PatcherJsonFile.hpp"
#include <regex>
#include <set>

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
                    auto skip_size = bundle.headerSize - 12;
                    body.read(bundle.skipped, skip_size);
                }

                std::int32_t chunkCount;
                body >> chunkCount;
                for (auto j = 0; j < chunkCount; j++) {

                    std::int32_t chunkOffset;
                    body >> chunkOffset;

                    auto nextChunk = body.pos();
                    body.seek(chunkOffset + nextChunk - 4);

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

                int useCustomOffset;
                body >> useCustomOffset;
                body.seek(-4, std::ios_base::cur);

                if (useCustomOffset > 100) {
                    body >> file.nameOffset.offset;
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
                    for (auto j = 0; j < chunkCount; j++) {
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

        auto index = 0;
        for (auto & chunk : bundle.chunks) {
            RMANFileBundleChunkInfo chunkInfo(&bundle, &chunk, index, chunk.compressedSize);

            _chunkMap.insert_or_assign(chunk.chunkId, chunkInfo);
            index += chunk.compressedSize;
        }
    }
}

std::set<std::string> getBundles(std::vector<std::pair<RMANFile*, RMANFileFile*>>& extracts)
{
    std::set<std::string> bundleIds;

    for (const auto&[manifest, file] : extracts)
    {
        std::cout << file->name << std::endl;

        for (auto& chunk : file->chunks)
        {
            const auto info = manifest->_chunkMap.at(chunk);
            bundleIds.insert(info.bundle->idAsHex());
        }
    }

    return bundleIds;
}

void download_bundles(std::set<std::string>& bundleIds, const std::filesystem::path& bundle_path, TCLAP::SwitchArg& lazy)
{
    std::vector<std::pair<std::string, std::filesystem::path>> datas;
    for (auto& id : bundleIds)
    {
        auto bundle_string = id + ".bundle";
        auto url = "https://lol.dyn.riotcdn.net/channels/public/bundles/" + bundle_string;
        auto output_path = bundle_path / bundle_string;

        if (lazy.isSet())
        {
            if (exists(output_path))
            {
                std::cout << "Found already downloaded bundle: " << output_path << std::endl;
                continue;
            }
        }
        datas.emplace_back(std::make_pair(url, output_path));
    }

    const cdragon::web::Downloader downloader;
    downloader.downloadFiles(datas);
}


void RMANFile::parseCommandline(
    TCLAP::ValueArg<std::string>& server,
    TCLAP::ValueArg<std::string>& region,
    TCLAP::ValueArg<std::string>& platform,
    TCLAP::ValueArg<std::string>& type,
    TCLAP::ValueArg<std::string>& output,
    TCLAP::ValueArg<std::string>& pattern,
    TCLAP::SwitchArg& lazy_file,
    TCLAP::SwitchArg& lazy_bundle,
    TCLAP::SwitchArg& list
)
{
    std::cout << "Downloading patcher manifest" << std::endl;
    web::Downloader downloader;
    auto url = "https://lol.dyn.riotcdn.net/channels/public/" + server.getValue() + "-" + region.getValue() + "-" + platform.getValue() + ".json";
    auto patcher_manifest = downloader.downloadString(url);

    std::cout << "Parsing manifest" << std::endl;
    rapidjson::Document d;
    d.Parse(patcher_manifest.c_str());
    auto jsonval = PatcherJson(d);

    std::cout << "Fouind manifest version " << jsonval.version << std::endl;

    std::vector<RMANFile> files;
    if (type.getValue() == "game" || type.getValue() == "both")
    {
        std::cout << "Starting parsing of game manifest: " << jsonval.game_patch_url << std::endl;
        auto file = DragonInStream(jsonval.game_patch_url);

        RMANFile rman;
        file >> rman;

        files.emplace_back(std::move(rman));
    }

    if (type.getValue() == "lcu" || type.getValue() == "both")
    {
        std::cout << "Starting parsing of lcu manifest: " << jsonval.client_patch_url << std::endl;
        auto file = DragonInStream(jsonval.client_patch_url);

        RMANFile rman;
        file >> rman;

        files.emplace_back(std::move(rman));
    }

    // chunkmaps have pointers, and we need them to be valid when we lookup
    for (auto& file : files)
    {
        file.buildChunkMap();
    }

    std::cout << "Matching files to pattern" << std::endl;
    std::vector<std::pair<RMANFile*, RMANFileFile*>> extracts;
    std::regex rgx(pattern.getValue(), std::regex_constants::icase);
    for (auto& rman : files) {
        for (auto& file : rman.files)
        {
            if (pattern.isSet())
            {
                auto fullPath = file.getFilePath(rman);
                if (std::regex_search(fullPath, rgx))
                {
                    extracts.emplace_back(&rman, &file);
                }
            }
            else
            {
                extracts.emplace_back(&rman, &file);
            }
        }
    }

    std::cout << "Fetching bundles" << std::endl;
    auto needed_bundles = getBundles(extracts);
    auto bundle_path = std::filesystem::path(output.getValue()) / "bundles";
    download_bundles(needed_bundles, bundle_path, lazy_bundle);

    for (const auto&[manifest, file] : extracts) {

        auto output_filename = file->getFilePath(*manifest);

        if (list.isSet())
        {
            std::cout << output_filename << std::endl;
        }

        if (!output.isSet())
        {
            continue;
        }

        auto output_path = std::filesystem::path(output.getValue());
        output_path /= output_filename;

        if (lazy_file.isSet()) {
            if (exists(output_path))
            {
                std::cout << "File already exists! (skipping because --lazy is set)" << std::endl;
                continue;
            }
        }

        if (!exists(output_path.parent_path()))
        {
            create_directories(output_path.parent_path());
        }

        if (exists(output_path))
        {
            std::cout << "File already exists! (re-creating because --lazy isnt set)" << std::endl;
            std::filesystem::remove(output_path);
        }


        std::cout << "Writing file " << file->name << std::endl;

        std::ofstream output_writer;
        output_writer.open(output_path, std::ios::out | std::ios::binary | std::ios::app);

        auto current = manifest->_chunkMap[file->chunks[0]];
        auto inputPath = bundle_path / (current.bundle->idAsHex() + ".bundle");
        DragonInStream input_reader(inputPath);

        ZSTDHandler zstd;
        auto chunk_count = static_cast<std::int32_t>(file->chunks.size());
        for (auto i = 0; i < chunk_count; i++)
        {
            input_reader.seek(current.offset);

            std::vector<std::byte> compressed;
            input_reader.read(compressed, current.chunk->compressedSize);

            std::vector<std::byte> uncompressed;
            uncompressed.resize(current.chunk->uncompressedSize);

            zstd.decompress(compressed, uncompressed);
            output_writer.write(reinterpret_cast<char*>(uncompressed.data()), uncompressed.size() * sizeof(uncompressed.front()));

            if ((i + 1) >= chunk_count)
            {
                break;
            }

            auto next = manifest->_chunkMap[file->chunks[i + 1]];
            if (current.bundle->bundleId != next.bundle->bundleId)
            {
                auto new_path = bundle_path / (next.bundle->idAsHex() + ".bundle");
                input_reader.open(new_path);
            }

            current = next;
        }

        output_writer.close();
    }
}
