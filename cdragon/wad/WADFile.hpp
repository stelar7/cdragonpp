#pragma once
#include <list>
#include <vector>
#include <cstddef>

namespace wad {

	enum WADCompressionType {
		NONE = 0,
		GZIP = 1,
		REFERENCE = 2,
		ZSTD = 3
	};

	class WADHeader {
		std::vector<std::byte> magic;
		std::byte major;
		std::byte minor;

		union version {
			struct v1 {
				short entryOffset;
				short entryCellSize;
				int entryCount;
			};

			struct v2 {
				std::byte ECDSALength;
				std::vector<std::byte> ECDSA;
				std::vector<std::byte> ECDSAPadding;
				long long checksum;
				short entryOffset;
				short entryCellSize;
				int entryCount;
			};

			struct v3 {
				std::vector<std::byte> ECDSA;
				long long checksum;
				int entryCount;
			};
		};

	};

	class WADContentHeader {
		union version {
			struct v1 {
				std::string pathHash;
				int offset;
				int compressedSize;
				int uncompressedSize;
				WADCompressionType compression;
			};

			struct v2 :v1 {
				bool duplicate;
				short paddding;
				long long sha256;
			};
		};
	};


	class WADFile {
		WADHeader header;
		std::list<WADContentHeader> content;
	};
}