#ifndef VX_PLATFORM_ANDROID_CODEC_BACKEND_H
#define VX_PLATFORM_ANDROID_CODEC_BACKEND_H

#include "platform/i_codec_backend.h"
#include <map>
#include <string>
#include <memory>
#include <media/NdkMediaExtractor.h>

struct AMediaExtractor;

namespace vx {

class AndroidCodecBackend : public ICodecBackend {
public:
    AndroidCodecBackend() = default;
    ~AndroidCodecBackend() override;

    [[nodiscard]] bool openMedia(const std::string& uri) override;
    TextureHandle getFrameAtTime(const std::string& uri, int64_t time_us) override;

    [[nodiscard]] std::unique_ptr<IDecoder> openDecoder(const std::string& uri) override;
    [[nodiscard]] std::unique_ptr<IEncoder> openEncoder(const std::string& output_path,
                                          const IEncoder::Settings& settings) override;

private:
    struct ExtractorDeleter {
        void operator()(AMediaExtractor* extractor) const {
            if (extractor) AMediaExtractor_delete(extractor);
        }
    };
    using ExtractorPtr = std::unique_ptr<AMediaExtractor, ExtractorDeleter>;
    std::map<std::string, ExtractorPtr> m_extractors;

    static constexpr size_t MAX_CACHED_EXTRACTORS = 10;
};

} // namespace vx

#endif // VX_PLATFORM_ANDROID_CODEC_BACKEND_H
