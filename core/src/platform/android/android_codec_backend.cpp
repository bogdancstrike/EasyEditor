#include "android_codec_backend.h"
#include "util/log.h"
#include "util/error.h"
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaError.h>

namespace vx {

AndroidCodecBackend::~AndroidCodecBackend() = default;

bool AndroidCodecBackend::openMedia(const std::string& uri) {
    if (m_extractors.count(uri)) {
        return true;
    }

    // Simple resource limit management
    if (m_extractors.size() >= MAX_CACHED_EXTRACTORS) {
        VX_LOG_WARN("AndroidCodecBackend", "Reached max cached extractors, clearing cache");
        m_extractors.clear();
    }

    AMediaExtractor* raw_extractor = AMediaExtractor_new();
    if (!raw_extractor) {
        VX_LOG_ERROR("AndroidCodecBackend", "Failed to create AMediaExtractor for: " + uri);
        return false;
    }

    ExtractorPtr extractor(raw_extractor);

    media_status_t status = AMediaExtractor_setDataSource(extractor.get(), uri.c_str());
    
    if (status != AMEDIA_OK) {
        VX_LOG_ERROR("AndroidCodecBackend", "Failed to open media: " + uri + ", status: " + std::to_string(status));
        return false;
    }

    m_extractors[uri] = std::move(extractor);
    VX_LOG_INFO("AndroidCodecBackend", "Successfully opened media: " + uri);
    return true;
}

TextureHandle AndroidCodecBackend::getFrameAtTime(const std::string& uri, int64_t time_us) {
    // Phase 1 placeholder: Return a dummy handle.
    // In a real implementation, we would seek the extractor and use AMediaCodec to decode.
    VX_LOG_INFO("AndroidCodecBackend", "getFrameAtTime requested for " + uri + " at " + std::to_string(time_us) + " us (stub)");
    return TextureHandle{.opaque = nullptr, .size = {0, 0}};
}

std::unique_ptr<IDecoder> AndroidCodecBackend::openDecoder(const std::string& uri) {
    static_cast<void>(uri);
    return nullptr;
}

std::unique_ptr<IEncoder> AndroidCodecBackend::openEncoder(const std::string& output_path,
                                                          const IEncoder::Settings& settings) {
    static_cast<void>(output_path);
    static_cast<void>(settings);
    return nullptr;
}

} // namespace vx
