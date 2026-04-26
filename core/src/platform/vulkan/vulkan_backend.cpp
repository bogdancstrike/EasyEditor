#include "vulkan_backend.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "util/error.h"

namespace vx {

namespace {

struct VulkanTexture {
    VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    Size2D size;
    PixelFormat format = PixelFormat::RGBA16F;
};

[[nodiscard]] const char* resultName(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        default:
            return "VK_ERROR_UNKNOWN";
    }
}

void checkVk(VkResult result, const char* operation) {
    if (result == VK_SUCCESS) {
        return;
    }
    throw Error(VX_ERR_GPU, std::string{operation} + " failed: " + resultName(result));
}

[[nodiscard]] VkFormat toVkFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGBA8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::RGBA16F:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case PixelFormat::BGRA8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
    }
    throw Error(VX_ERR_INVALID_ARG, "unsupported pixel format");
}

[[nodiscard]] VulkanTexture& requireTexture(TextureHandle texture) {
    if (!texture.valid()) {
        throw Error(VX_ERR_INVALID_ARG, "texture handle is invalid");
    }
    return *static_cast<VulkanTexture*>(texture.opaque);
}

void validateTextureMetadata(TextureHandle handle, const VulkanTexture& texture) {
    if (handle.size.width != texture.size.width || handle.size.height != texture.size.height ||
        handle.format != texture.format) {
        throw Error(VX_ERR_INVALID_ARG, "texture handle metadata does not match Vulkan texture");
    }
}

[[nodiscard]] VkPhysicalDevice pickPhysicalDevice(VkInstance instance) {
    uint32_t count = 0;
    checkVk(vkEnumeratePhysicalDevices(instance, &count, nullptr), "vkEnumeratePhysicalDevices");
    if (count == 0) {
        throw Error(VX_ERR_GPU, "no Vulkan physical devices available");
    }

    std::vector<VkPhysicalDevice> devices(count);
    checkVk(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "vkEnumeratePhysicalDevices");
    return devices.front();
}

[[nodiscard]] uint32_t findComputeQueueFamily(VkPhysicalDevice physical_device) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    if (count == 0) {
        throw Error(VX_ERR_GPU, "Vulkan device exposes no queue families");
    }

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
            return i;
        }
    }
    throw Error(VX_ERR_GPU, "Vulkan device exposes no compute queue");
}

}  // namespace

VulkanBackend::VulkanBackend() {
    const VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Video Editor",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "videoeditor",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_1,
    };

    const VkInstanceCreateInfo instance_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };

    checkVk(vkCreateInstance(&instance_info, nullptr, &instance_), "vkCreateInstance");

    physical_device_ = pickPhysicalDevice(instance_);
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device_, &properties);
    if (properties.apiVersion < VK_API_VERSION_1_1) {
        throw Error(VX_ERR_GPU, "Vulkan 1.1 is required");
    }
    device_name_ = properties.deviceName;

    queue_family_index_ = findComputeQueueFamily(physical_device_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);

    const float priority = 1.0F;
    const VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = queue_family_index_,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };

    const VkDeviceCreateInfo device_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
        .pEnabledFeatures = nullptr,
    };

    checkVk(vkCreateDevice(physical_device_, &device_info, nullptr, &device_), "vkCreateDevice");
    vkGetDeviceQueue(device_, queue_family_index_, 0, &queue_);

    const VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_index_,
    };
    checkVk(vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_), "vkCreateCommandPool");
}

VulkanBackend::~VulkanBackend() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
        }
        vkDestroyDevice(device_, nullptr);
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

TextureHandle VulkanBackend::allocateTexture(Size2D size, PixelFormat fmt) {
    if (size.width <= 0 || size.height <= 0) {
        throw Error(VX_ERR_INVALID_ARG, "texture dimensions must be positive");
    }

    auto texture = std::make_unique<VulkanTexture>();
    texture->device = device_;
    texture->size = size;
    texture->format = fmt;

    const VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = toVkFormat(fmt),
        .extent = {static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    checkVk(vkCreateImage(device_, &image_info, nullptr, &texture->image), "vkCreateImage");

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, texture->image, &requirements);

    const VkMemoryAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    checkVk(vkAllocateMemory(device_, &alloc_info, nullptr, &texture->memory), "vkAllocateMemory");
    checkVk(vkBindImageMemory(device_, texture->image, texture->memory, 0), "vkBindImageMemory");

    TextureHandle handle;
    handle.opaque = texture.release();
    handle.size = size;
    handle.format = fmt;
    return handle;
}

TextureHandle VulkanBackend::allocateTexture3D(int size, PixelFormat fmt, std::span<const uint8_t> data) {
    static_cast<void>(size);
    static_cast<void>(fmt);
    static_cast<void>(data);
    throw Error(VX_ERR_UNSUPPORTED, "Vulkan 3D texture allocation is not implemented in Phase 0");
}

void VulkanBackend::releaseTexture(TextureHandle texture) {
    if (texture.opaque == nullptr) {
        return;
    }
    auto* vulkan_texture = static_cast<VulkanTexture*>(texture.opaque);
    validateTextureMetadata(texture, *vulkan_texture);
    if (vulkan_texture->device != VK_NULL_HANDLE) {
        if (vulkan_texture->image != VK_NULL_HANDLE) {
            vkDestroyImage(vulkan_texture->device, vulkan_texture->image, nullptr);
        }
        if (vulkan_texture->memory != VK_NULL_HANDLE) {
            vkFreeMemory(vulkan_texture->device, vulkan_texture->memory, nullptr);
        }
    }
    delete vulkan_texture;
}

void VulkanBackend::dispatchCompute(std::string_view shader_name,
                                    std::span<const TextureHandle> /*inputs*/,
                                    TextureHandle /*output*/,
                                    const ShaderConstants& /*constants*/) {
    if (shader_name != "noop") {
        throw Error(VX_ERR_UNSUPPORTED,
                    "VulkanBackend has no compute shader implementation: " + std::string{shader_name});
    }
    submitNoop();
}

void VulkanBackend::copyTexture(TextureHandle src, TextureHandle dst) {
    static_cast<void>(requireTexture(src));
    static_cast<void>(requireTexture(dst));
    throw Error(VX_ERR_UNSUPPORTED, "Vulkan texture copy is not implemented in Phase 0");
}

void VulkanBackend::updateTexture3D(TextureHandle texture, std::span<const uint8_t> data) {
    static_cast<void>(texture);
    static_cast<void>(data);
    throw Error(VX_ERR_UNSUPPORTED, "Vulkan 3D texture updates are not implemented yet");
}

void VulkanBackend::waitForGpu() {
    checkVk(vkDeviceWaitIdle(device_), "vkDeviceWaitIdle");
}

uint32_t VulkanBackend::findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags required) const {
    for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; ++i) {
        const bool type_supported = (type_bits & (1U << i)) != 0U;
        const bool flags_match = (memory_properties_.memoryTypes[i].propertyFlags & required) == required;
        if (type_supported && flags_match) {
            return i;
        }
    }

    for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; ++i) {
        if ((type_bits & (1U << i)) != 0U) {
            return i;
        }
    }

    throw Error(VX_ERR_GPU, "no compatible Vulkan memory type");
}

void VulkanBackend::submitNoop() {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    const VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    checkVk(vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer), "vkAllocateCommandBuffers");

    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    checkVk(vkBeginCommandBuffer(command_buffer, &begin_info), "vkBeginCommandBuffer");
    checkVk(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer");

    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    checkVk(vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE), "vkQueueSubmit");
    checkVk(vkQueueWaitIdle(queue_), "vkQueueWaitIdle");
    vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
}

std::string runVulkanSmokeTest() {
    VulkanBackend backend;
    TextureHandle texture = backend.allocateTexture(Size2D{16, 16}, PixelFormat::RGBA16F);
    backend.dispatchCompute("noop", {}, texture, ShaderConstants{});
    backend.waitForGpu();
    backend.releaseTexture(texture);
    return "Vulkan OK: " + backend.deviceName();
}

}  // namespace vx
