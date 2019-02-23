#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "../libs/rapidjson/istreamwrapper.h"
#include "../libs/rapidjson/stringbuffer.h"
#include "../libs/rapidjson/document.h"
#include "../libs/rapidjson/writer.h"
#include "../libs/tclap/SwitchArg.h"
#include "../libs/tclap/ValueArg.h"
#include "../libs/tclap/CmdLine.h"
#include "types/rman/PatcherJsonFile.hpp"
#include "types/rman/RMANFile.hpp"
#include "types/wad/WADFile.hpp"
#include "util/PlatformHandler.hpp"
#include "util/WebDownloader.hpp"
#include "util/DragonStream.hpp"
#include "util/HashHandler.hpp"

using namespace cdragon::wad;
using namespace cdragon::rman;
using namespace cdragon::util;
using namespace cdragon::web;

#define TEST_RMAN 0
#define TEST_WAD 0
#define TEST_GET 0


void parseWADFile(
    TCLAP::ValueArg<std::string>& input,
    TCLAP::ValueArg<std::string>& output,
    TCLAP::ValueArg<std::string>& pattern,
    TCLAP::ValueArg<std::string>& unknown,
    TCLAP::SwitchArg& lazy,
    std::vector<std::string>& hash_files
)
{
    if (!input.isSet())
    {
        std::cout << "ERROR: " << "Missing input file!" << std::endl;
        exit(0);
    }

    auto path = std::filesystem::path(input.getValue());
    auto input_file = DragonInStream(path);
    std::cout << "Starting parsing of: " << input.getValue() << std::endl;

    WADFile wad;
    input_file >> wad;
    std::cout << input.getValue() << " parsed " << (wad ? "ok" : "bad") << "!" << std::endl;

    std::cout << "Loading hashes" << std::endl;
    std::unordered_map<std::int64_t, std::string> hashes;
    for (auto& file : hash_files) {

        std::cout << "Loading from " << file << std::endl;
        auto content = HashHandler::loadFile(file);
        hashes.merge(content);
    }

    std::cout << "Found " << wad.content.size() << " files!" << std::endl;
    for (auto& contentHeader : wad.content)
    {
        auto header = std::get<WADContentHeader::v2>(contentHeader.version);
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

        auto value = (search == hashes.end() ? header.hashAsHex() : search->second);
        std::cout << value << std::endl;
    }

}

void parseRMANFile(
    std::vector<std::string>& languages
)
{

}

int main(const int argc, char** argv)
{
    {
        using namespace TCLAP;
        try {
            // basic parsing
            CmdLine cmd("cdragon main parser", ' ', "0.9", true);
            SwitchArg verbose("", "verbose", "Enable verbose logging", false, cmd);
            ValueArg<std::string> storage("", "storage", "Directory for downloaded intermediary files", false, "./cdragon/temp", "string", cmd);
            std::vector<Arg*> mains;

            // wad parsing
            SwitchArg wad("w", "wad", "enable wad parsing", false);
            ValueArg<std::string> wad_input("", "wad-input", "WAD file to extract", false, "", "string", cmd);
            ValueArg<std::string> wad_output("", "wad-output", "WAD content output directory", false, "./cdragon/wad", "string", cmd);
            ValueArg<std::string> wad_pattern("", "wad-pattern", "extract only files matching pattern with shell-like wildcards", false, "", "string", cmd);
            std::vector<std::string> unknown_options = { "yes", "no", "only" };
            ValuesConstraint<std::string>unknown_constraint(unknown_options);
            ValueArg<std::string> wad_unknown("", "wad-unknown", "control extraction of unknown files", false, "yes", &unknown_constraint, cmd);
            SwitchArg wad_lazy("", "wad-lazy", "don't overwrite files, assume they are already correctly extracted", false, cmd);
            MultiArg<std::string> wad_hashes("", "wad-hashes", "Files to load hashes from", false, "string", cmd);
            mains.push_back(&wad);

            /*
            // rman parsing
            SwitchArg rman("r", "rman", "Enable rman parsing", false);
            MultiArg<std::string> languages("", "rman-language", "List of languages to parse", false, "string", cmd);
            mains.push_back(&rman);
            */

            cmd.xorAdd(mains);
            //cmd.parse(argc, argv);

            std::vector<std::string> test = {
                "cdragon", "-w",
                "--wad-input", R"(C:\Users\Steffen\Downloads\extractedFiles2\Plugins\rcp-be-lol-game-data\default-assets.wad)",
                "--wad-hashes", R"(C:\Dropbox\Private\workspace\cdragon\src\main\resources\hashes\wad\game.json)",
                "--wad-hashes", R"(C:\Dropbox\Private\workspace\cdragon\src\main\resources\hashes\wad\lcu.json)",
                //"--wad-unknown", "only"
                // TODO: wad-lazy, wad-pattern, wad-output

            };
            cmd.parse(test);

            if (wad.isSet())
            {
                auto hashFiles = wad_hashes.getValue();
                parseWADFile(wad_input, wad_output, wad_pattern, wad_unknown, wad_lazy, hashFiles);
            }

            std::cin.get();

            /*

            if (rman.isSet())
            {
                std::vector<std::string> langs = { "all" };
                if (languages.isSet())
                {
                    langs = languages.getValue();
                }

                parseRMANFile(langs);
            }

            */
        }
        catch (ArgException& e)
        {
            std::cout << "ERROR: " << e.error() << " " << e.argId() << std::endl;
        }
    }

#if TEST_RMAN
    {

        const std::string manifestPath = "C:/Users/Steffen/Downloads/cdragon/patcher/manifests/72.json";
        std::ifstream ifs(manifestPath, std::ios::binary);

        using namespace  rapidjson;
        rapidjson::IStreamWrapper isw(ifs);
        Document d;
        d.ParseStream(isw);
        auto jsonval = PatcherJson(d);

        std::cout << "Starting parsing of: " << jsonval.client_patch_url << std::endl;
        auto file = DragonInStream(jsonval.client_patch_url);
        RMANFile rman;
        file >> rman;


        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        rman.Serialize(writer);

        std::ofstream myfile;
        myfile.open(R"(C:\Users\Steffen\Desktop\rman.txt)");
        myfile << buffer.GetString();
        myfile.close();

        // std::cin.get();
    }
#endif

#if TEST_GET
    {
        // downloads a string and stores it in memory
        std::string url = "http://stelar7.no/cdragon/version.txt";
        std::string output = R"(C:\Users\Steffen\Desktop\test\version.txt)";
        auto outPath = std::filesystem::path(output);

        Downloader downloader;
        auto data = downloader.downloadString(url);
        std::cout << data.data() << std::endl;

        // downloads a string and stores it in a file
        downloader.downloadFile(url, outPath);

        std::cin.get();
    }
#endif
}

