#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <regex>
#include "WADFile.hpp"
#include "../../../libs/tclap/SwitchArg.h"
#include "../../../libs/tclap/ValueArg.h"
#include "../../util/DragonStream.hpp"
#include "../../util/HashHandler.hpp"
#include "../../util/ZSTDHandler.hpp"
#include "../../util/GZIPHandler.hpp"

using namespace cdragon::wad;
using namespace cdragon::util;

std::istream& cdragon::wad::operator>>(DragonInStream& is, WADFile& obj)
{
    try {

        {
            // clear out old data
            obj = WADFile();
        }

        is >> obj.header.magic;
        if (std::strcmp(obj.header.magic, "RW") != 0) {
            throw std::exception("Invalid magic number in header");
        }

        is >> obj.header.major;
        is >> obj.header.minor;

        auto fileCount = -1;

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
            is >> ver.ECDSALength;

            is.read(ver.ECDSA, ver.ECDSALength);
            is.read(ver.ECDSAPadding, 83 - ver.ECDSALength);

            is >> ver.checksum;
            is >> ver.entryOffset;
            is >> ver.entryCellSize;
            is >> ver.entryCount;

            obj.header.version = ver;
            fileCount = ver.entryCount;
        }

        if (obj.header.major == 3) {
            WADHeader::v3 ver;

            is.read(ver.ECDSA, 256);
            is >> ver.checksum;
            is >> ver.entryCount;

            obj.header.version = ver;
            fileCount = ver.entryCount;
        }

        for (auto i = 0; i < fileCount; i++) {

            WADContentHeader::v2 content;

            is >> content.pathHash;
            is >> content.offset;
            is >> content.compressedSize;
            is >> content.uncompressedSize;

            // is there a better way to do this? seems like there isnt (https://stackoverflow.com/questions/5633784/input-from-stream-to-enum-type)
            is.read(content.compression, (obj.header.major > 1 ? sizeof(std::int8_t) : sizeof(std::int32_t)));
            content.compression = static_cast<WADCompressionType>(content.compression & 0xFF);

            if (obj.header.major > 1 && obj.header.major < 4) {
                WADContentHeader::v2 var2(content);
                is >> var2.duplicate;
                is >> var2.paddding;
                is >> var2.sha256;
                obj.content.push_back(var2);
                continue;
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

std::string WADContentHeader::v1::hashAsHex() const
{
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << this->pathHash;
    return ss.str();
}

int32_t findFirstDivisorOverTenPercent(int32_t value) {
    std::vector<int32_t> v;
    for (int i = 1; i <= sqrt(value); i++)
    {
        if (value % i == 0)
        {
            if (value / i == i)
                v.push_back(i);
            else
            {
                v.push_back(i);
                v.push_back(value / i);
            }
        }
    }

    std::sort(v.begin(), v.end());

    for (auto& val : v) {
        if (val > (value / 10)) return val;
    }

    return 10;

}

void WADFile::parseCommandline(
    TCLAP::ValueArg<std::string>& input,
    TCLAP::ValueArg<std::string>& output,
    TCLAP::SwitchArg& output_disabled,
    TCLAP::ValueArg<std::string>& pattern,
    TCLAP::ValueArg<std::string>& unknown,
    TCLAP::SwitchArg& lazy,
    TCLAP::SwitchArg& list,
    std::vector<std::string>& hash_files,
    TCLAP::ValueArg<std::string>& wad_hash_output
)
{
    if (!output.isSet())
    {
        auto output_path = std::filesystem::absolute(std::filesystem::path(output.getValue()));
        std::cout << "No output folder is set, using default! (" << output_path << ")" << std::endl;
    }

    if (!input.isSet())
    {
        std::cout << "ERROR: " << "Missing input file(s)!" << std::endl;
        exit(0);
    }

    std::vector<std::filesystem::path> parse_files;
    if (!std::filesystem::is_directory(input.getValue()))
    {
        std::cout << "Input is file, parsing single!" << std::endl;
        parse_files.emplace_back(std::filesystem::path(input.getValue()));
    }
    else
    {
        std::cout << "Input is folder, parsing recursive!" << std::endl;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(input.getValue()))
        {
            const auto extension = entry.path().extension();
            if (".wad" == extension || ".client" == extension)
            {
                std::cout << "Found input file: " << entry.path() << std::endl;
                parse_files.push_back(entry.path());
            }
        }
    }

    std::unordered_map<std::int64_t, std::string> hashes;
    if (hash_files.empty())
    {
        std::cout << "Downloading hash lists..." << std::endl;
        std::vector<std::string> hashUrls = { "https://github.com/stelar7/lol-parser/raw/master/src/main/resources/hashes/wad/game.json", "https://github.com/stelar7/lol-parser/raw/master/src/main/resources/hashes/wad/lcu.json" };
        web::Downloader downloader;
        for (auto& url : hashUrls) {
            auto content = downloader.downloadString(url);
            rapidjson::Document d;
            d.Parse(content.c_str());
            auto temp = HashHandler::hash_json_document(d);
            hashes.merge(temp);
        }
    }
    else {
        std::cout << "Loading hashes" << std::endl;
        for (auto& file : hash_files) {

            std::cout << "Loading from " << file << std::endl;
            auto content = HashHandler::loadFile(file);
            hashes.merge(content);
        }
    }

    for (const auto& path : parse_files) {

        auto input_file = DragonInStream(path);
        std::cout << "Starting parsing of: " << path << std::endl;

        WADFile wad;
        input_file >> wad;
        std::cout << path << " parsed " << (wad ? "ok" : "bad") << "!" << std::endl;

        if (!wad)
        {
            std::cout << "Skipping invalid .wad file" << std::endl;
            continue;
        }

        crypto::ZSTDHandler zstd;
        crypto::GZIPHandler gzip;

        int32_t count = 0;
        auto symlink_error_printed = false;

        int32_t interval = findFirstDivisorOverTenPercent(static_cast<int32_t>(wad.content.size()));
        std::cout << "Found " << wad.content.size() << " files before filtering!" << std::endl;

        std::ofstream hash_writer;
        if (wad_hash_output.isSet()) {

            std::filesystem::path hash_path = wad_hash_output.getValue();
            if (!std::filesystem::exists(hash_path.parent_path()))
            {
                std::filesystem::create_directories(hash_path.parent_path());
            }

            hash_writer.open(wad_hash_output.getValue(), std::ios::out | std::ios::binary | std::ios::app);
        }

        for (auto& header : wad.content)
        {
            if (wad.content.size() > 1500 && ((++count) % interval) == 0)
            {
                auto percentage = static_cast<int32_t>(ceil((static_cast<float_t>(count) / static_cast<float_t>(wad.content.size())) * 100.0));
                std::cout << percentage << "% " << count << "/" << wad.content.size() << std::endl;
            }

            auto hash = header.pathHash;
            auto search = hashes.find(hash);

            if (unknown.getValue() == "only")
            {
                if (search != hashes.end())
                {
                    continue;
                }
            }

            if (unknown.getValue() == "no")
            {
                if (search == hashes.end())
                {
                    continue;
                }
            }

            auto output_filename = (search == hashes.end() ? "unknown/" + header.hashAsHex() : search->second);

            if (pattern.isSet())
            {
                std::regex rgx(pattern.getValue(), std::regex_constants::icase);
                if (!std::regex_search(output_filename, rgx))
                {
                    continue;
                }
            }

            if (wad_hash_output.isSet()) {
                auto hash_filename = (search == hashes.end() ? "?" : search->second);

                auto hash_line = header.hashAsHex() + " " + hash_filename + "\n";
                hash_writer << hash_line;
            }

            if (list.getValue())
            {
                std::cout << WADtoString(header.compression) << " " << output_filename << std::endl;
            }

            if (output_disabled.getValue()) {
                continue;
            }

            auto output_path = std::filesystem::path(output.getValue());
            output_path /= output_filename;
            output_path = std::filesystem::absolute(output_path.make_preferred());

            if (lazy.isSet()) {
                if (std::filesystem::exists(output_path))
                {
                    std::cout << "File already exists! (skipping because --lazy is set)" << std::endl;
                    continue;
                }
            }

            if (!std::filesystem::exists(output_path.parent_path()))
            {
                std::filesystem::create_directories(output_path.parent_path());
            }


            std::filesystem::file_status status;
            if (std::filesystem::is_symlink(output_path)) {
                status = std::filesystem::symlink_status(output_path);
            }
            else {
                status = std::filesystem::status(output_path);
            }

            if (std::filesystem::exists(status))
            {
                std::cout << "File already exists! (re-creating because --lazy isnt set)" << std::endl;
                std::filesystem::remove(output_path);
            }

            input_file.seek(header.offset);
            std::vector<std::byte> data;
            switch (header.compression) {
                case NONE:
                {
                    input_file.read(data, header.uncompressedSize);
                    break;
                };

                case GZIP: {
                    std::vector<std::byte> compressed;
                    input_file.read(data, header.compressedSize);

                    data.resize(header.uncompressedSize);
                    gzip.decompress(reinterpret_cast<char*>(compressed.data()), static_cast<int>(compressed.size()), reinterpret_cast<char*>(data.data()), header.uncompressedSize);
                    break;
                };

                case REFERENCE: {
                    std::int32_t data_size;
                    input_file.read(data_size);

                    std::string data_string;
                    input_file.read(data_string, data_size);

                    std::error_code err;
                    std::filesystem::create_symlink(data_string, output_path, err);

                    if (err) {
                        if (!symlink_error_printed) {
                            std::cout << "Failed to create symlink for files.\n";
                            std::cout << "Please run this code as admin to generate them.\n";
                            std::cout << "Otherwise feel free to ignore this message." << std::endl;
                            symlink_error_printed = true;
                        }
                    }

                    break;
                };

                case ZSTD: {
                    std::vector<std::byte> compressed;
                    input_file.read(compressed, header.compressedSize);

                    data.resize(header.uncompressedSize);
                    zstd.decompress(compressed, data);
                    break;
                };

                default:
                {
                    std::cout << "Invalid file compression!" << std::endl;
                };
            }

            if (header.compression != REFERENCE) {
                std::ofstream output_writer;
                output_writer.open(output_path, std::ios::out | std::ios::binary, std::ios::trunc);
                output_writer.write(reinterpret_cast<char*>(data.data()), data.size() * sizeof(data.front()));
                output_writer.close();
            }
        }

        if (wad_hash_output.isSet()) {
            hash_writer.close();
        }
    }
}
