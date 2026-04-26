#ifndef VX_PLATFORM_MOCK_CODEC_BACKEND_H
#define VX_PLATFORM_MOCK_CODEC_BACKEND_H

#include "platform/i_codec_backend.h"

namespace vx {

class MockCodecBackend : public ICodecBackend {
public:
    [[nodiscard]] bool openMedia(const std::string& uri) override {
        static_cast<void>(uri);
        return true;
    }

    [[nodiscard]] TextureHandle getFrameAtTime(const std::string& uri, int64_t time_us) override {
        static_cast<void>(uri);
        static_cast<void>(time_us);
        return TextureHandle{.opaque = nullptr, .size = {0, 0}};
    }

    [[nodiscard]] std::unique_ptr<IDecoder> openDecoder(const std::string& uri) override {
        static_cast<void>(uri);
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<IEncoder> openEncoder(const std::string& output_path,
                                          const IEncoder::Settings& settings) override {
        static_cast<void>(output_path);
        static_cast<void>(settings);
        return nullptr;
    }
};

} // namespace vx

#endif // VX_PLATFORM_MOCK_CODEC_BACKEND_H
