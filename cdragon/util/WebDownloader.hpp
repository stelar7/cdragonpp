#pragma once

#include "../../libs/curl/include/curl.h"
#include <filesystem>
#include <iostream>
#include <string.h>
#include <errno.h>

namespace cdragon {
    namespace web {

        class Downloader {
        public:
            Downloader() {
                curl = curl_easy_init();

                if (!curl) {
                    throw std::exception("FAILED TO INIT CURL");
                }
            }

            ~Downloader() {
                curl_easy_cleanup(curl);
            }

            std::string downloadString(std::string url);
            bool downloadFile(std::string url, std::filesystem::path output);
        private:
            CURL* curl;
        };
    }
}