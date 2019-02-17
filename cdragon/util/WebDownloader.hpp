#pragma once

#include "../../libs/curl/include/curl.h"
#include <filesystem>

namespace cdragon {
    namespace web {

        class Downloader {
        public:
            Downloader(Downloader &other) = delete;
            Downloader(Downloader &&other) = delete;
            Downloader& operator=(Downloader& other) = delete;
            Downloader& operator=(Downloader&& other) = delete;

            Downloader() {
                curl = curl_easy_init();

                if (!curl) {
                    throw std::exception("FAILED TO INIT CURL");
                }

                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            }

            ~Downloader() {
                curl_easy_cleanup(curl);
            }

            std::string downloadString(std::string& url) const;
            bool downloadFile(std::string& url, std::filesystem::path& output) const;
        private:
            CURL* curl;
        };
    }
}