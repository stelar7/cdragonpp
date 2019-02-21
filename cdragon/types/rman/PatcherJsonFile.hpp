#pragma once
#include "../../../libs/rapidjson/document.h"

#define VARIABLE_TO_STRING(Variable) (#Variable)

class PatcherJson {
public:
    std::string client_patch_url;
    std::string game_patch_url;
    std::int64_t version;

    explicit PatcherJson(rapidjson::Document& json) :
        client_patch_url(json[VARIABLE_TO_STRING(client_patch_url)].GetString()),
        game_patch_url(json[VARIABLE_TO_STRING(game_patch_url)].GetString()),
        version(json[VARIABLE_TO_STRING(version)].GetInt64()) {

    }
};