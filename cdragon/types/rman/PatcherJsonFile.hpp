#pragma once
#include "../../../libs/json/json.hpp"

#define VARIABLE_TO_STRING(Variable) (#Variable)

class PatcherJson {
public:
    std::string client_patch_url;
    std::string game_patch_url;
    std::int64_t version;

    PatcherJson(nlohmann::json json) :
        client_patch_url(json[VARIABLE_TO_STRING(client_patch_url)]),
        game_patch_url(json[VARIABLE_TO_STRING(game_patch_url)]),
        version(json[VARIABLE_TO_STRING(version)]) {

    }
};