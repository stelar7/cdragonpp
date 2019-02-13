#pragma once
#include <vector>
#include <cstddef>
#include <variant>

namespace cdragon {
	namespace wad {

		enum WADCompressionType {
			NONE = 0,
			GZIP = 1,
			REFERENCE = 2,
			ZSTD = 3
		};

		class WADHeader {
		public:
			struct v1 {
				std::int16_t entryOffset;
				std::int16_t entryCellSize;
				std::int32_t entryCount;
			};

			struct v2 {
				std::int8_t ECDSALength;
				std::vector<std::byte> ECDSA;
				std::vector<std::byte> ECDSAPadding;
				std::int64_t checksum;
				std::int16_t entryOffset;
				std::int16_t entryCellSize;
				std::int32_t entryCount;
			};

			struct v3 {
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
				std::string pathHash;
				std::int32_t offset;
				std::int32_t compressedSize;
				std::int32_t uncompressedSize;
				WADCompressionType compression;
			};

			class v2 : v1 {
			public:
				v2(v1 old) : v1(old) {};

				bool duplicate;
				std::int16_t paddding;
				std::int64_t sha256;
			};

			std::variant<v1, v2> version;
		};


		class WADFile {
		public:
			WADHeader header;
			std::vector<WADContentHeader> content;

			friend std::istream& operator>>(std::istream &is, WADFile &file);
			explicit operator bool();
			friend bool operator!(WADFile &file);

		private:
			bool _valid = false;
		};
	}
}