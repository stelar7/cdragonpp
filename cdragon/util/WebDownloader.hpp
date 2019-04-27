#pragma once

#include "../../libs/curl/include/curl.h"
#include <filesystem>
#include <map>

namespace cdragon {
    namespace web {

        constexpr std::int32_t MAX_CONCURRENT_DOWNLOAD = 5;

        class Downloader {
        public:
            Downloader(Downloader &other) = delete;
            Downloader(Downloader &&other) = delete;
            Downloader& operator=(Downloader& other) = delete;
            Downloader& operator=(Downloader&& other) = delete;

            Downloader() {
                handle = curl_easy_init();

                if (!handle) {
                    throw std::exception("FAILED TO INIT CURL");
                }

                curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);

                multi_handle = curl_multi_init();

            }

            ~Downloader() {
                curl_multi_remove_handle(multi_handle, handle);
                curl_easy_cleanup(handle);
                curl_multi_cleanup(multi_handle);
            }

            std::string downloadString(std::string& url) const;
            bool downloadFile(std::string& url, std::filesystem::path& output) const;
            bool downloadFiles(std::vector<std::pair<std::string, std::filesystem::path>>& urls) const;
        private:
            CURL* handle;
            CURLM* multi_handle;
        };
    }
}
