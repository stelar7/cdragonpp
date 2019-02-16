#pragma once

#include <fstream>
#include <filesystem>
#include "WebUtil.hpp"

namespace cdragon {
    namespace util {

        class DragonInStream {
        public:
            std::ifstream ifs;
            DragonInStream(std::filesystem::path path) : ifs(std::ifstream(path, std::ios::binary)) {};

            DragonInStream(std::string url) {
                _filename = "temp/" + (++_counter);
                std::filesystem::path filepath(_filename);

                cdragon::web::downloadFile(url, filepath);
                ifs = std::ifstream(filepath, std::ios::binary);
            };

            ~DragonInStream() {
                if (!_filename.empty()) {
                    std::filesystem::remove_all(_filename);
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
            std::int64_t _counter = 0;
            std::string _filename = "";
        };
    }
}