#pragma once

#include <istream>
#include <vector>
#include <map>

namespace cdragon {

    namespace util {
        class DragonInStream;
    }

    namespace rman {

        class RMANFile;

        class RMANFileHeader {
        public:
            char magic[4];

            union version {
                std::int16_t combined;
                struct {
                    std::int8_t major;
                    std::int8_t minor;
                };
            } version;

            std::int8_t unknown;
            std::int8_t signatureType;
            std::int32_t offset;
            std::int32_t length;
            std::int64_t manifestId;
            std::int32_t decompressedLength;

            std::string idAsHex() const;
        };

        class RMANFileOffsetTable {
        public:
            std::int32_t offsetTableOffset;
            std::int32_t bundleListOffset;
            std::int32_t languageListOffset;
            std::int32_t fileListOffset;
            std::int32_t folderListOffset;
            std::int32_t keyHeaderOffset;
            std::int32_t unknownOffset;
        };

        class RMANFileBundleChunkInfo {
        public:
            std::int64_t bundleId;
            std::int64_t chunkId;
            std::int64_t offset;
            std::int64_t compressedSize;
            RMANFileBundleChunkInfo(const std::int64_t bundle, const std::int64_t chunk, const std::int64_t off, const std::int64_t compressed) :
                bundleId(bundle),
                chunkId(chunk),
                offset(off),
                compressedSize(compressed) {};

        };

        class RMANFileBundleChunk {
        public:
            RMANFileBundleChunk() : offsetTableOffset(0),
                compressedSize(0),
                uncompressedSize(0),
                chunkId(0) {};

            std::int32_t offsetTableOffset;
            std::int32_t compressedSize;
            std::int32_t uncompressedSize;
            std::int64_t chunkId;

            std::string idAsHex() const;
        };

        class RMANFileBundle {
        public:
            RMANFileBundle() : offset(0),
                offsetTableOffset(0),
                headerSize(0),
                bundleId(0) {};

            std::int32_t offset;
            std::int32_t offsetTableOffset;
            std::int32_t headerSize;
            std::int64_t bundleId;
            std::vector<std::byte> skipped;
            std::vector<RMANFileBundleChunk> chunks;

            std::string idAsHex() const;
        };

        class RMANFileLanguage {
        public:
            RMANFileLanguage() : offset(0),
                offsetTableOffset(0),
                languageId(0),
                nameOffset(0) {};

            std::int32_t offset;
            std::int32_t offsetTableOffset;
            std::int32_t languageId;

            std::int32_t nameOffset;
            std::string name;

            std::string idAsHex() const;
        };

        class RMANFileFile {
        public:
            RMANFileFile() : offset(0),
                offsetTableOffset(0),
                nameOffset({}),
                structSize(0),
                symlinkOffset(0),
                fileId(0),
                folderId(0),
                fileSize(0),
                permissions(0),
                languageId(0),
                unknowns({}),
                singleChunk(0) {};

            std::int32_t offset;
            std::int32_t offsetTableOffset;

            union nameOffset {
                union data {
                    std::int32_t offset : 24;
                    std::int32_t filetype : 8;
                } data;
                std::int32_t offset;
            } nameOffset;
            std::string name;

            std::int32_t structSize;

            std::int32_t symlinkOffset;
            std::string symlinkName;

            std::int64_t fileId;
            std::int64_t folderId;
            std::int32_t fileSize;
            std::int32_t permissions;
            std::int32_t languageId;

            union unknowns {
                std::int64_t large;
                struct {
                    std::int32_t unknown1;
                    std::int32_t unknown2;
                } combined;
            } unknowns;

            std::int32_t singleChunk;
            std::vector<int64_t> chunks;

            std::string fileIdAsHex() const;
            std::string folderIdAsHex() const;
            std::string languageIdAsHex() const;
            std::string getFilePath(RMANFile& manifest) const;
        };

        class RMANFileFolder {
        public:
            RMANFileFolder() : offset(0), offsetTableOffset(0), folderIdOffset(0), parentIdOffset(0), nameOffset(0), folderId(0), parentId(0) {};

            std::int32_t offset;
            std::int32_t offsetTableOffset;
            std::int16_t folderIdOffset;
            std::int16_t parentIdOffset;

            std::int32_t nameOffset;
            std::string name;

            std::int64_t folderId;
            std::int64_t parentId;

            std::string folderIdAsHex() const;
            std::string parentIdAsHex() const;
        };

        class RMANFile {
        public:
            RMANFile() : manifestHeader(RMANFileHeader()), offsetTable(RMANFileOffsetTable()) {};

            RMANFileHeader manifestHeader;
            RMANFileOffsetTable offsetTable;
            std::vector<std::byte> signature;

            std::vector<RMANFileBundle> bundles;
            std::vector<RMANFileLanguage> languages;
            std::vector<RMANFileFile> files;
            std::vector<RMANFileFolder> folders;

            friend std::istream& operator>>(cdragon::util::DragonInStream &is, RMANFile &obj);

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
            std::map<std::int64_t, RMANFileBundle> _bundleMap;
            std::map<std::int64_t, RMANFileBundleChunkInfo> _chunkMap;

            void buildChunkMap();
        };
    }
}