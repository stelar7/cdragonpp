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
                    std::cout << "FAILED TO INIT CURL" << std::endl;
                }
            }

            ~Downloader() {
                curl_easy_cleanup(curl);
            }

            std::string downloadString(std::string url);
            bool downloadFile(std::string url, std::filesystem::path output);

        private:
            CURL* curl;
            std::size_t writeString(void *ptr, std::size_t size, std::size_t nmemb, std::string* data);
            std::size_t writeData(void *ptr, std::size_t size, std::size_t nmemb, FILE *stream);
        };
    }
}