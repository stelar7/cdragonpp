#include <filesystem>
#include <iostream>
#include "../libs/tclap/SwitchArg.h"
#include "../libs/tclap/ValueArg.h"
#include "../libs/tclap/CmdLine.h"
#include "types/rman/RMANFile.hpp"
#include "util/WebDownloader.hpp"
#include "types/wad/WADFile.hpp"

using namespace cdragon::rman;
using namespace cdragon::util;
using namespace cdragon::web;

#define TEST_RMAN 0
#define TEST_WAD 0
#define TEST_GET 0


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
            ValueArg<std::string> wad_pattern("", "wad-pattern", "extract only files matching regex pattern", false, "", "string", cmd);
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
                "--wad-unknown", "yes",
                "--wad-output", R"(C:\Users\Steffen\Downloads\test)",
                "--wad-lazy"
            };
            cmd.parse(test);

            if (wad.isSet())
            {
                auto hash_files = wad_hashes.getValue();
                cdragon::wad::WADFile::parseCommandline(wad_input, wad_output, wad_pattern, wad_unknown, wad_lazy, hash_files);
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

