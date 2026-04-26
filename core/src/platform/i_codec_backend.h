#ifndef VX_PLATFORM_I_CODEC_BACKEND_H
#define VX_PLATFORM_I_CODEC_BACKEND_H

#include <cstdint>
#include <memory>
#include <string>

#include "i_gpu_backend.h"

namespace vx {

class IDecoder {
public:
    virtual ~IDecoder() = default;

    /// Seek to the given source-time and decode forward until the first frame
    /// with pts >= time_us. Returns the texture handle of that frame.
    /// Caller does NOT free the texture; the decoder pools internally.
    [[nodiscard]] virtual TextureHandle decodeFrameAt(int64_t time_us) = 0;
};

class IEncoder {
public:
    virtual ~IEncoder() = default;

    struct Settings {
        Size2D resolution{1920, 1080};
        int fps = 30;
        int bitrate_bps = 6'000'000;
        std::string codec = "h264";       // "h264", "hevc", "av1"
        std::string container = "mp4";    // "mp4", "mov"
    };

    virtual void appendFrame(TextureHandle, int64_t pts_us) = 0;
    virtual void finish() = 0;
};

class ICodecBackend {
public:
    virtual ~ICodecBackend() = default;

    [[nodiscard]] virtual std::unique_ptr<IDecoder> openDecoder(const std::string& uri) = 0;
    [[nodiscard]] virtual std::unique_ptr<IEncoder> openEncoder(const std::string& output_path,
                                                                const IEncoder::Settings&) = 0;
};

}  // namespace vx

#endif  // VX_PLATFORM_I_CODEC_BACKEND_H
