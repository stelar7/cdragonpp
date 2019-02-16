#pragma once

#include <fstream>

namespace cdragon {
    namespace util {

        class DragonInStream {
        public:
            std::ifstream ifs;
            DragonInStream(std::string path) : ifs(std::ifstream(path, std::ios::binary)) {};

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

        };
    }
}