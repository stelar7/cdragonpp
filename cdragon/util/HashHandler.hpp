#pragma once
#include <map>
#include "../util/PlatformHandler.hpp"
#include "../../libs/rapidjson/error/error.h"
#include "../../libs/rapidjson/document.h"
#include "../../libs/rapidjson/error/en.h"

class HashHandler
{
public:
    static std::unordered_map<std::int64_t, std::string> loadFile(std::string file)
    {
        using namespace  rapidjson;

        std::string data = map_file_to_string(file);

        Document d;
        d.Parse(data.c_str());
        if (d.HasParseError())
        {
            auto error = ParseErrorCode(d.GetParseError());
            fprintf(stderr, "JSON parse error: %s", GetParseError_En(error));
        }

        std::unordered_map<std::int64_t, std::string> content;
        for (auto iter = d.MemberBegin(); iter != d.MemberEnd(); ++iter)
        {
            auto key = iter->name.GetString();
            auto key_int = static_cast<std::int64_t>(std::stoull(key, nullptr, 16));
            auto value = iter->value.GetString();

            content.insert(std::make_pair(key_int, value));
        }

        return content;
    }
};