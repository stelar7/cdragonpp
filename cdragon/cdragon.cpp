#include <filesystem>
#include <iostream>

#include "../libs/tclap/SwitchArg.h"
#include "../libs/tclap/ValueArg.h"
#include "../libs/tclap/CmdLine.h"


#include "types/rman/RMANFile.hpp"
#include "types/wad/WADFile.hpp"

using namespace cdragon::util;
using namespace cdragon::rman;
using namespace cdragon::wad;

int main(const int argc, char** argv)
{
    {
        using namespace TCLAP;
        try {
            // basic parsing
            CmdLine cmd("cdragon main parser", ' ', "0.9", true);
            SwitchArg verbose("", "verbose", "Enable verbose logging", false, cmd);
            ValueArg<std::string> storage("", "storage", "Directory for downloaded intermediary files", false, "./cdragon", "string", cmd);
            std::vector<Arg*> mains;

            // wad parsing
            SwitchArg wad("w", "wad", "enable wad parsing", false);
            ValueArg<std::string> wad_input("", "wad-input", "WAD file to extract", false, "", "string", cmd);
            ValueArg<std::string> wad_output("", "wad-output", "WAD content output directory", false, storage.getValue() + "/wad", "string", cmd);
            ValueArg<std::string> wad_pattern("", "wad-pattern", "Extract only files matching regex pattern", false, "", "string", cmd);
            std::vector<std::string> unknown_options = { "yes", "no", "only" };
            ValuesConstraint<std::string>unknown_constraint(unknown_options);
            ValueArg<std::string> wad_unknown("", "wad-unknown", "Control extraction of unknown files", false, "yes", &unknown_constraint, cmd);
            SwitchArg wad_lazy("", "wad-lazy", "Don't overwrite files, assume they are already correctly extracted", false, cmd);
            MultiArg<std::string> wad_hashes("", "wad-hashes", "Files to load hashes from", false, "string", cmd);
            mains.push_back(&wad);

            /*
        std::vector<std::string> test = {
            "cdragon", "-w",
            "--wad-input", R"(C:\Users\Steffen\Downloads\extractedFiles2\Plugins\rcp-be-lol-game-data\default-assets.wad)",
            "--wad-hashes", R"(C:\Dropbox\Private\workspace\cdragon\src\main\resources\hashes\wad\game.json)",
            "--wad-hashes", R"(C:\Dropbox\Private\workspace\cdragon\src\main\resources\hashes\wad\lcu.json)",
            "--wad-unknown", "yes",
            "--wad-pattern", ".",
            "--wad-output", R"(C:\Users\Steffen\Downloads\test)",
            "--wad-lazy"
        };
        */

        // rman parsing
            SwitchArg rman("r", "rman", "Enable rman parsing", false);
            ValueArg<std::string> rman_output("", "rman-output", "RMAN content output directory", false, storage.getValue() + "/rman", "string", cmd);
            MultiArg<std::string> rman_language("", "rman-language", "List of languages to parse", false, "string", cmd);
            ValueArg<std::string> rman_pattern("", "rman-pattern", "Extract only files matching regex pattern", false, ".", "string", cmd);
            std::vector<std::string> server_options = { "pbe", "live" };
            ValuesConstraint<std::string>server_constraint(server_options);
            ValueArg<std::string> rman_server("", "rman-server", "Server to download from", false, "pbe", &server_constraint, cmd);
            std::vector<std::string> region_options = { "pbe", "euw", "na" };
            ValuesConstraint<std::string>region_constraints(region_options);
            ValueArg<std::string> rman_region("", "rman-region", "Region to download from", false, "pbe", &region_constraints, cmd);
            std::vector<std::string> platform_options = { "win" };
            ValuesConstraint<std::string>platform_constraints(platform_options);
            ValueArg<std::string> rman_platform("", "rman-platform", "Platform to download to", false, "win", &platform_constraints, cmd);
            std::vector<std::string> type_options = { "game", "lcu", "both" };
            ValuesConstraint<std::string>type_constraints(type_options);
            ValueArg<std::string> rman_type("", "rman-type", "Type to download", false, "both", &type_constraints, cmd);
            SwitchArg rman_lazy("", "rman-lazy", "Don't overwrite files, assume they are already correctly extracted", false, cmd);
            SwitchArg rman_list("", "rman-list", "Output the list of files", false, cmd);
            mains.push_back(&rman);
            cmd.xorAdd(mains);

            std::vector<std::string> test = {
                "cdragon", "-r",
                "--rman-type", "game",
                "--rman-pattern", "Vayne.cs_CZ",
                "--rman-list",
            };

            cmd.parse(test);
            //cmd.parse(argc, argv);

            if (rman.isSet())
            {
                RMANFile::parseCommandline(rman_server, rman_region, rman_platform, rman_type, rman_output, rman_pattern, rman_lazy, rman_list);
            }

            if (wad.isSet())
            {
                auto hash_files = wad_hashes.getValue();
                WADFile::parseCommandline(wad_input, wad_output, wad_pattern, wad_unknown, wad_lazy, hash_files);
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

}

