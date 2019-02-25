#pragma once

#include "../../../libs/rapidjson/prettywriter.h"
#include "../../../libs/tclap/SwitchArg.h"
#include "../../../libs/tclap/ValueArg.h"
#include <istream>
#include <vector>
#include <map>

std::string toHex(std::int64_t const val);

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

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("manifest_id");
                writer.String(idAsHex().c_str());
                writer.String("version");
                writer.StartObject();
                writer.String("major");
                writer.Uint(version.major);
                writer.String("minor");
                writer.Uint(version.minor);
                writer.EndObject();
            }
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

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("chunk_id");
                writer.String(idAsHex().c_str());
                writer.EndObject();
            };
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

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("bundle_id");
                writer.String(idAsHex().c_str());
                writer.String("chunks");
                writer.StartArray();
                for (auto& chunk : chunks)
                {
                    writer.String(chunk.idAsHex().c_str());
                }
                writer.EndArray();
                writer.EndObject();
            };
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

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("language_id");
                writer.String(idAsHex().c_str());
                writer.String("name");
                writer.String(name.c_str());
                writer.EndObject();
            };
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
            std::vector<std::string> chunksAsHex() const;

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("name");
                writer.String(name.c_str());
                writer.String("symlink_name");
                writer.String(symlinkName.c_str());
                writer.String("language_id");
                writer.String(languageIdAsHex().c_str());
                writer.String("folder_id");
                writer.String(folderIdAsHex().c_str());
                writer.String("filesize");
                writer.Uint(fileSize);
                writer.String("chunks");
                writer.StartArray();
                for (auto& chunk : chunks)
                {
                    writer.String(toHex(chunk).c_str());
                }
                writer.EndArray();
                writer.EndObject();
            };
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

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("name");
                writer.String(name.c_str());
                writer.String("folder_id");
                writer.String(folderIdAsHex().c_str());
                writer.String("parent_id");
                writer.String(parentIdAsHex().c_str());
                writer.EndObject();
            };
        };

        class RMANFileBundleChunkInfo {
        public:
            RMANFileBundle* bundle;
            RMANFileBundleChunk* chunk;
            std::int32_t offset;
            std::int32_t compressedSize;

            RMANFileBundleChunkInfo() = default;

            RMANFileBundleChunkInfo(RMANFileBundle* bundle, RMANFileBundleChunk* chunk, const std::int32_t off, const std::int32_t compressed) :
                bundle(bundle),
                chunk(chunk),
                offset(off),
                compressedSize(compressed) {};

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

            std::map<std::int64_t, RMANFileBundleChunkInfo> _chunkMap;

            friend std::istream& operator>>(cdragon::util::DragonInStream &is, RMANFile &obj);

            bool operator!() const
            {
                return !_valid;
            }

            explicit operator bool() const
            {
                return _valid;
            }

            static void parseCommandline(
                TCLAP::ValueArg<std::string>& server,
                TCLAP::ValueArg<std::string>& region,
                TCLAP::ValueArg<std::string>& platform,
                TCLAP::ValueArg<std::string>& type,
                TCLAP::ValueArg<std::string>& output,
                TCLAP::ValueArg<std::string>& pattern,
                TCLAP::SwitchArg& lazy,
                TCLAP::SwitchArg& list
            );

            template <typename Writer>
            void Serialize(Writer& writer) const {
                writer.StartObject();
                writer.String("header");
                manifestHeader.Serialize(writer);

                writer.String("bundles");
                writer.StartArray();
                for (auto& bundle : bundles)
                {
                    bundle.Serialize(writer);
                }
                writer.EndArray();

                writer.String("languages");
                writer.StartArray();
                for (auto& language : languages)
                {
                    language.Serialize(writer);
                }
                writer.EndArray();

                writer.String("files");
                writer.StartArray();
                for (auto& file : files)
                {
                    file.Serialize(writer);
                }writer.EndArray();

                writer.String("folders");
                writer.StartArray();
                for (auto& folder : folders)
                {
                    folder.Serialize(writer);
                }
                writer.EndArray();

                writer.EndObject();
            };
        private:
            bool _valid = false;
            void buildChunkMap();
        };
    }
}
