#ifndef VX_PLATFORM_VULKAN_VULKAN_BACKEND_H
#define VX_PLATFORM_VULKAN_VULKAN_BACKEND_H

#include <span>
#include <string>
#include <string_view>

#include <vulkan/vulkan.h>

#include "platform/i_gpu_backend.h"

namespace vx {

/// Android Vulkan implementation of IGpuBackend.
///
/// Phase 0 scope: initialize Vulkan, allocate image-backed textures, release
/// textures, and submit a no-op command buffer for smoke testing. Real compute
/// pipelines are added when render graph nodes define shader contracts.
class VulkanBackend final : public IGpuBackend {
public:
    VulkanBackend();
    ~VulkanBackend() override;

    VulkanBackend(const VulkanBackend&) = delete;
    VulkanBackend& operator=(const VulkanBackend&) = delete;
    VulkanBackend(VulkanBackend&&) = delete;
    VulkanBackend& operator=(VulkanBackend&&) = delete;

    [[nodiscard]] TextureHandle allocateTexture(Size2D size, PixelFormat fmt) override;
    [[nodiscard]] TextureHandle allocateTexture3D(int size, PixelFormat fmt, std::span<const uint8_t> data) override;
    void releaseTexture(TextureHandle texture) override;

    void dispatchCompute(std::string_view shader_name,
                         std::span<const TextureHandle> inputs,
                         TextureHandle output,
                         const ShaderConstants& constants) override;

    void copyTexture(TextureHandle src, TextureHandle dst) override;
    void updateTexture3D(TextureHandle texture, std::span<const uint8_t> data) override;
    void present(TextureHandle texture, void* window) override;
    void waitForGpu() override;

    [[nodiscard]] std::string deviceName() const { return device_name_; }

private:
    [[nodiscard]] uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags required) const;
    void submitNoop();

    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    uint32_t queue_family_index_ = 0;
    VkPhysicalDeviceMemoryProperties memory_properties_{};
    std::string device_name_;
};

/// Runs the Phase 0 Android Vulkan smoke path:
/// initialize backend, create/release one texture, submit no-op command buffer.
[[nodiscard]] std::string runVulkanSmokeTest();

}  // namespace vx

#endif  // VX_PLATFORM_VULKAN_VULKAN_BACKEND_H
