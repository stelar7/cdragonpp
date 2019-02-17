#pragma once

#include <fstream>
#include <iostream>
#include <filesystem>
#include "WebDownloader.hpp"

namespace cdragon {
    namespace util {

        /*
        you must access this.ifs.open(file) to change it...
        */
        class DragonInStream {
        public:
            DragonInStream(DragonInStream &other) = delete;
            DragonInStream(DragonInStream &&other) = delete;
            DragonInStream& operator=(DragonInStream& other) = delete;
            DragonInStream& operator=(DragonInStream&& other) = delete;


            std::ifstream ifs;
            explicit DragonInStream(const std::filesystem::path& path) : ifs(std::ifstream(path, std::ios::binary)) {};

            explicit DragonInStream(std::string& url) {

                char temp_name[256];
                tmpnam_s(temp_name, 256);
                auto filepath = std::filesystem::temp_directory_path();
                filepath /= temp_name;

                getter.downloadFile(url, filepath);
                ifs = std::ifstream(filepath, std::ios::binary);
            };

            ~DragonInStream() {
                ifs.close();

                std::error_code err;
                for (auto& path : paths) {
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
            void read(T& v, const std::int32_t size)
            {
                ifs.read(reinterpret_cast<char*>(&v), size);
            }

            void read(std::string& v, const std::int32_t size)
            {
                v.resize(size);
                ifs.read(static_cast<char*>(v.data()), size);
            }

            void read(std::vector<std::byte>&v, const std::int32_t size)
            {
                v.resize(size);
                ifs.read(reinterpret_cast<char*>(v.data()), size);
            }

            template<typename T>
            DragonInStream& operator>>(T& type) {
                ifs.read(reinterpret_cast<char*>(&type), sizeof(type));
                return *this;
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

        class membuf final : public std::basic_streambuf<char> {
        public:
            explicit membuf(std::vector<std::byte> &data) {
                this->setg(reinterpret_cast<char*>(data.data()), reinterpret_cast<char*>(data.data()), reinterpret_cast<char*>(data.data()) + data.size());
            }

            pos_type seekoff(const off_type off, const std::ios_base::seekdir dir, std::ios_base::openmode which) override {
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

        class DragonByteStream final : public std::istream {
        public:
            explicit DragonByteStream(std::vector<std::byte> &data) : std::istream(&_buffer), _buffer(data) {
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

            void read(std::string& v, const std::int32_t size)
            {
                v.resize(size);
                std::istream::read(reinterpret_cast<char*>(v.data()), size);
            }

            void read(std::vector<std::byte>&v, const std::int32_t size)
            {
                v.resize(size);
                std::istream::read(reinterpret_cast<char*>(v.data()), size);
            }

            template<typename T>
            void read(T& v)
            {
                read(v, sizeof(v));
            }

            template<typename T>
            DragonByteStream& operator>>(T& type) {
                read(type, sizeof(type));
                return *this;
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