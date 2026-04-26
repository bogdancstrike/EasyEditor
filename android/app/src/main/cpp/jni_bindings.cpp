#include <jni.h>

#include <exception>
#include <string>

#include "platform/vulkan/vulkan_backend.h"
#include "vx/public_api.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeVersion(JNIEnv* env, jobject /* thiz */) {
    return env->NewStringUTF(vx_version());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_videoeditor_app_engine_NativeBridge_nativeVulkanSmokeTest(JNIEnv* env, jobject /* thiz */) {
    try {
        return env->NewStringUTF(vx::runVulkanSmokeTest().c_str());
    } catch (const std::exception& e) {
        const std::string message = std::string{"Vulkan unavailable: "} + e.what();
        return env->NewStringUTF(message.c_str());
    } catch (...) {
        return env->NewStringUTF("Vulkan unavailable: unknown exception");
    }
}
