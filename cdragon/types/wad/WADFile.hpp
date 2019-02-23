#pragma once

#include <vector>
#include <variant>
#include <istream>
#include "../../../libs/tclap/ValueArg.h"

namespace TCLAP {
    class SwitchArg;
}

namespace cdragon {
    namespace util {
        class DragonInStream;
    }

    namespace wad {

        enum WADCompressionType {
            NONE = 0,
            GZIP = 1,
            REFERENCE = 2,
            ZSTD = 3
        };

        class WADHeader {
        public:
            WADHeader() :magic{ 0,0 }, major(0), minor(0) {}

            struct v1 {
                v1() : entryOffset(0), entryCellSize(0), entryCount(0) {};
                std::int16_t entryOffset;
                std::int16_t entryCellSize;
                std::int32_t entryCount;
            };

            struct v2 {
                v2() :ECDSALength(0), checksum(0), entryOffset(0), entryCellSize(0), entryCount(0) {};
                std::int8_t ECDSALength;
                std::vector<std::byte> ECDSA;
                std::vector<std::byte> ECDSAPadding;
                std::int64_t checksum;
                std::int16_t entryOffset;
                std::int16_t entryCellSize;
                std::int32_t entryCount;
            };

            struct v3 {
                v3() : checksum(0), entryCount(0) {};
                std::vector<std::byte> ECDSA;
                std::int64_t checksum;
                std::int32_t entryCount;
            };

            char magic[2];
            std::int8_t major;
            std::int8_t minor;
            std::variant<v1, v2, v3> version;
        };

        class WADContentHeader {
        public:
            class v1 {
            public:
                v1() : pathHash(0), offset(0), compressedSize(0), uncompressedSize(0), compression(WADCompressionType::NONE) {};

                std::int64_t pathHash;
                std::int32_t offset;
                std::int32_t compressedSize;
                std::int32_t uncompressedSize;
                WADCompressionType compression;

                std::string hashAsHex() const;
            };

            class v2 : public v1 {
            public:
                explicit v2(const v1 old) : v1(old), duplicate(0), paddding(0), sha256(0) {};

                std::uint8_t duplicate;
                std::int16_t paddding;
                std::int64_t sha256;
            };

            std::variant<v1, v2> version;
        };


        class WADFile {
        public:
            WADHeader header;
            std::vector<WADContentHeader> content;

            friend std::istream& operator>>(cdragon::util::DragonInStream &is, WADFile &obj);
            static void parseCommandline(
                TCLAP::ValueArg<std::string>& input,
                TCLAP::ValueArg<std::string>& output,
                TCLAP::ValueArg<std::string>& pattern,
                TCLAP::ValueArg<std::string>& unknown,
                TCLAP::SwitchArg& lazy,
                std::vector<std::string>& hash_files
            );

            bool operator!() const
            {
                return !_valid;
            }

            explicit operator bool() const
            {
                return _valid;
            }

        private:
            bool _valid = false;
        };
    }
}
