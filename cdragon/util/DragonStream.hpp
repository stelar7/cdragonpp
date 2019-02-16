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

            void read(std::string& v, std::int32_t size)
            {
                char buff[256];
                read(buff, size);
                v = std::string(buff);
                v.resize(size);
            }

            void read(std::vector<std::byte>&v, std::int32_t size)
            {
                for (std::int32_t i = 0; i < size; i++) {
                    std::byte val;
                    read(val, sizeof(std::int8_t));
                    v.push_back(val);
                }
            }

            template<typename T>
            std::ifstream& operator>>(T& type) {
                ifs.read(reinterpret_cast<char*>(&type), sizeof(type));
                return ifs;
            }

            void seek(std::int32_t pos) {
                ifs.seekg(pos);
            }

            void seek(std::int32_t pos, std::ios_base::seekdir direction) {
                ifs.seekg(pos, direction);
            }

            std::int64_t pos() {
                return ifs.tellg();
            }

        private:
            cdragon::web::Downloader getter;
            std::vector<std::filesystem::path> paths;
            std::int64_t _counter = 0;
        };

        class membuf : public std::basic_streambuf<char> {
        public:
            membuf(std::vector<std::byte> &data) {
                this->setg((char*)data.data(), (char*)data.data(), (char*)data.data() + data.size());
            }

            pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override {
                if (dir == std::ios_base::cur) {
                    gbump(static_cast<std::int32_t>(off));
                }
                else if (dir == std::ios_base::end) {
                    setg(eback(), egptr() + off, egptr());
                }
                else if (dir == std::ios_base::beg) {
                    setg(eback(), eback() + off, egptr());
                }

                return gptr() - eback();
            }

            pos_type seekpos(pos_type sp, std::ios_base::openmode which) override {
                return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
            }
        };

        class DragonByteStream : public std::istream {
        public:
            DragonByteStream(std::vector<std::byte> &data) : std::istream(&_buffer), _buffer(data) {
                rdbuf(&_buffer);
            };

            template<typename T>
            void read(T& v, std::int32_t size)
            {
                std::istream::read(reinterpret_cast<char*>(&v), size);
                if (std::istream::fail()) {
                    std::cout << "Failed to read stream!" << std::endl;
                }
            }

            void read(std::string& v, std::int32_t size)
            {
                char buff[256];
                read(buff, size);
                v = std::string(buff);
                v.resize(size);
            }

            void read(std::vector<std::byte>&v, std::int32_t size)
            {
                for (std::int32_t i = 0; i < size; i++) {
                    std::byte val;
                    read(val, sizeof(std::int8_t));
                    v.push_back(val);
                }
            }

            template<typename T>
            void read(T& v)
            {
                read(v, sizeof(v));
            }

            template<typename T>
            DragonByteStream* operator>>(T& type) {
                read(type, sizeof(type));
                return this;
            }

            void seek(std::int32_t pos) {
                std::istream::seekg(pos, std::ios_base::beg);
            }

            void seek(std::int32_t pos, std::ios_base::seekdir direction) {
                std::istream::seekg(pos, direction);
            }

            std::int32_t pos() {
                return static_cast<std::int32_t>(std::istream::tellg());
            }

        private:
            membuf _buffer;
        };
    }
}