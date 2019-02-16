#pragma once

#include <vector>
#include <istream>

namespace cdragon {

    namespace util {
        class DragonInStream;
    }

    namespace rman {

        class RMANFileHeader {
        public:
            char magic[4];

            union version {
                std::int16_t version;
                struct {
                    std::int8_t major;
                    std::int8_t minor;
                };
            };

            std::int8_t unknown;
            std::int8_t signatureType;
            std::int32_t offset;
            std::int32_t length;
            std::int64_t manifestId;
            std::int32_t decompressedLength;

            std::string idAsHex();
        };

        class RMANFileOffsetTable {
        public:
            std::int32_t offsetTableOffset;
            std::int32_t bundleListOffset;
            std::int32_t languageListOffset;
            std::int32_t fileListOffset;
            std::int32_t FolderListOffset;
            std::int32_t KeyHeaderOffset;
            std::int32_t unknownOffset;
        };

        class RMANFileBundleChunk {
        public:
            std::int32_t offsetTableOffset;
            std::int32_t compressedSize;
            std::int32_t uncompressedSize;
            std::int64_t chunkId;

            std::string idAsHex();
        };

        class RMANFileBundle {
        public:
            std::int32_t offset;
            std::int32_t offsetTableOffset;
            std::int32_t headerSize;
            std::int64_t bundleId;
            std::vector<std::byte> skipped;
            std::vector<RMANFileBundleChunk> chunks;

            std::string idAsHex();
        };

        class RMANFileLanguage {
        public:
            std::int32_t offset;
            std::int32_t offsetTableOffset;
            std::int32_t languageId;

            std::int32_t nameOffset;
            std::string name;

            std::string idAsHex();
        };

        class RMANFileFile {
        public:
            std::int32_t offset;
            std::int32_t offsetTableOffset;

            union nameOffset {
                union data {
                    std::int32_t offset : 24;
                    std::int32_t filetype : 8;
                };
                std::int32_t offset;
            };

            std::int32_t nameOffset;
            std::string name;

            std::int32_t structSize;

            std::int32_t symlinkOffset;
            std::string symlinkName;

            std::int64_t fileId;
            std::int64_t directoryId;
            std::int32_t fileSize;
            std::int32_t permissions;
            std::int32_t languageId;

            union unknowns {
                std::int64_t unknown;
                struct {
                    std::int32_t unknown1;
                    std::int32_t unknown2;
                };
            };

            bool singleChunk;
            std::vector<int64_t> chunks;

            std::string fileIdAsHex();
            std::string directoryIdAsHex();
            std::string languageIdAsHex();
        };

        class RMANFileFolder {
        public:
            std::int32_t offset;
            std::int32_t offsetTableOffset;
            std::int16_t folderIdOffset;
            std::int16_t parentIdOffset;

            std::int32_t nameOffset;
            std::string name;

            std::int64_t folderId;
            std::int64_t parentId;

            std::string folderIdAsHex();
            std::string parentIdAsHex();
        };

        class RMANFile {
        public:
            RMANFileHeader manifestHeader;
            RMANFileOffsetTable offsetTable;

            std::vector<RMANFileBundle> bundles;
            std::vector<RMANFileLanguage> languages;
            std::vector<RMANFileFile> files;
            std::vector<RMANFileFolder> folders;

            friend std::istream& operator>>(cdragon::util::DragonInStream &is, RMANFile &file);

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