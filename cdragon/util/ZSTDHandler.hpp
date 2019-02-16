#pragma once

#include "../../../libs/zstd/include/zstd.h"
#include <vector>

namespace cdragon {
    namespace crypto {
        class ZSTDHandler {
        public:
            ZSTDHandler() {
                _dctx = ZSTD_createDCtx();
            }

            ~ZSTDHandler() {
                ZSTD_freeDCtx(_dctx);
            }

            void decompress(std::vector<std::byte>& input, std::vector<std::byte>& output)
            {
                ZSTD_inBuffer in = { input.data(), input.size(), 0 };
                ZSTD_outBuffer out = { output.data(), output.size(), 0 };

                std::size_t err = ZSTD_isError(ZSTD_decompressStream(_dctx, &out, &in));
                if (err) {
                    std::cout << ZSTD_getErrorName(err) << std::endl;
                }
            }

        private:
            ZSTD_DCtx* _dctx;
        };
    }
}