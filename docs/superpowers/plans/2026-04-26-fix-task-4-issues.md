# Fix Task 1 Code Quality Issues Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Address code quality issues in the Android codec backend and improve interface standards.

**Architecture:** Use modern C++ resource management (`std::unique_ptr`) for NDK resources, improve error handling with `vx::Error`, and adhere to coding standards with `[[nodiscard]]` and Doxygen documentation.

**Tech Stack:** C++, Android NDK.

---

### Task 1: Update ICodecBackend Interface

**Files:**
- Modify: `core/include/vx/public_api.h` (to check for Error type if needed, but the prompt says use vx::Error, which might be in `util/error.h`)
- Modify: `core/src/platform/i_codec_backend.h`

- [ ] **Step 1: Add Doxygen comments and [[nodiscard]] to ICodecBackend**

```cpp
class ICodecBackend {
public:
    virtual ~ICodecBackend() = default;

    /**
     * @brief Opens a media file for extraction/decoding.
     * @param uri The URI or path to the media file.
     * @return true if successfully opened, false otherwise.
     */
    [[nodiscard]] virtual bool openMedia(const std::string& uri) = 0;

    /**
     * @brief Retrieves a frame at the specified time.
     * @param uri The URI of the media file.
     * @param time_us The timestamp in microseconds.
     * @return A handle to the decoded texture.
     */
    virtual TextureHandle getFrameAtTime(const std::string& uri, int64_t time_us) = 0;

    /**
     * @brief Opens a decoder for the specified media.
     * @param uri The URI of the media file.
     * @return A unique_ptr to the decoder.
     */
    [[nodiscard]] virtual std::unique_ptr<IDecoder> openDecoder(const std::string& uri) = 0;

    /**
     * @brief Opens an encoder to save media to a file.
     * @param output_path The path to save the encoded file.
     * @param settings The encoder settings.
     * @return A unique_ptr to the encoder.
     */
    [[nodiscard]] virtual std::unique_ptr<IEncoder> openEncoder(const std::string& output_path,
                                                                const IEncoder::Settings& settings) = 0;
};
```

### Task 2: Update AndroidCodecBackend Resource Management

**Files:**
- Modify: `core/src/platform/android/android_codec_backend.h`
- Modify: `core/src/platform/android/android_codec_backend.cpp`

- [ ] **Step 1: Update header to use std::unique_ptr for AMediaExtractor**

```cpp
#include <memory>
// ...
private:
    using ExtractorPtr = std::unique_ptr<AMediaExtractor, decltype(&AMediaExtractor_delete)>;
    std::map<std::string, ExtractorPtr> m_extractors;
    static constexpr size_t MAX_CACHED_EXTRACTORS = 10;
```

- [ ] **Step 2: Update implementation to use std::unique_ptr and improved error handling**

```cpp
#include "util/error.h"

// ...

bool AndroidCodecBackend::openMedia(const std::string& uri) {
    if (m_extractors.count(uri)) {
        return true;
    }

    // Enforce simple resource limit
    if (m_extractors.size() >= MAX_CACHED_EXTRACTORS) {
        VX_LOG_WARN("Reached max cached extractors (%zu), clearing cache", MAX_CACHED_EXTRACTORS);
        m_extractors.clear();
    }

    AMediaExtractor* raw_extractor = AMediaExtractor_new();
    if (!raw_extractor) {
        VX_LOG_ERROR("Failed to create AMediaExtractor for: %s", uri.c_str());
        // throw vx::Error(vx::ErrorCode::OutOfMemory, "Failed to create AMediaExtractor");
        return false;
    }

    ExtractorPtr extractor(raw_extractor, &AMediaExtractor_delete);
    
    media_status_t status = AMediaExtractor_setDataSource(extractor.get(), uri.c_str());
    if (status != AMEDIA_OK) {
        VX_LOG_ERROR("Failed to open media: %s, status: %d", uri.c_str(), status);
        return false;
    }

    m_extractors[uri] = std::move(extractor);
    VX_LOG_INFO("Successfully opened media: %s", uri.c_str());
    return true;
}
```

### Task 3: Verification

- [ ] **Step 1: Run existing tests to ensure no regressions**

Run: `ctest` in build directory.

- [ ] **Step 2: Verify build passes with new changes**

Run: `cmake --build build`
