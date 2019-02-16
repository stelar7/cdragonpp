#pragma once

#include <fstream>
#include <filesystem>
#include "WebDownloader.hpp"

namespace cdragon {
    namespace util {

        /*
        you must access this.ifs.open(file) to change it...
        */
        class DragonInStream {
        public:
            std::ifstream ifs;
            DragonInStream(std::filesystem::path path) : ifs(std::ifstream(path, std::ios::binary)) {};

            DragonInStream(std::string url) {

                // make sure we have a unique filepath
                std::filesystem::path filepath;
                do {
                    std::string _filename = "./temp/" + std::to_string(++_counter);
                    filepath = std::filesystem::path(_filename);
                } while (std::filesystem::exists(filepath));
                paths.push_back(filepath);

                getter.downloadFile(url, filepath);
                ifs = std::ifstream(filepath, std::ios::binary);
            };

            ~DragonInStream() {
                ifs.close();

                std::error_code err;
                for (const std::filesystem::path& path : paths) {
                    std::filesystem::remove_all(path, err);
                    std::cout << err.message() << std::endl;
                }
            }

            template<typename T>
            void read(T& v)
            {
                ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
            }

            template<typename T>
            void read(T& v, std::int32_t size)
            {
                ifs.read(reinterpret_cast<char*>(&v), size);
            }

            template<typename T>
            std::ifstream& operator>>(T& type) {
                ifs.read(reinterpret_cast<char*>(&type), sizeof(type));
                return ifs;
            }

        private:
            cdragon::web::Downloader getter;
            std::vector<std::filesystem::path> paths;
            std::int64_t _counter = 0;
        };
    }
}